/**
 * Copyright 2022 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

resource "aws_launch_template" "instance_launch_template" {
  name          = "${var.service}-${var.environment}-instance-lt"
  image_id      = var.instance_ami_id
  instance_type = var.instance_type

  iam_instance_profile {
    arn = var.instance_profile_arn
  }

  vpc_security_group_ids = [
    var.instance_security_group_id
  ]

  enclave_options {
    enabled = true
  }

  user_data = base64encode(templatefile(
    "${path.module}/instance_init_script.tftpl",
    {
      enclave_memory_mib = var.enclave_memory_mib,
      enclave_cpu_count  = var.enclave_cpu_count,
      server_port        = var.server_port
  }))

  # Enforce IMDSv2.
  metadata_options {
    http_endpoint          = "enabled"
    http_tokens            = "required"
    instance_metadata_tags = "enabled"
  }

  tag_specifications {
    resource_type = "instance"
    tags = {
      Name        = "${var.service}-${var.environment}-instance"
      service     = var.service
      environment = var.environment
    }
  }
}

# Create auto scaling group for EC2 instances
resource "aws_autoscaling_group" "instance_asg" {
  name                      = "${var.service}-${var.environment}-instance-asg"
  max_size                  = var.autoscaling_max_size
  min_size                  = var.autoscaling_min_size
  desired_capacity          = var.autoscaling_desired_capacity
  health_check_grace_period = 600
  health_check_type         = "ELB"
  vpc_zone_identifier       = var.autoscaling_subnet_ids
  target_group_arns         = var.target_group_arns

  launch_template {
    id      = aws_launch_template.instance_launch_template.id
    version = aws_launch_template.instance_launch_template.latest_version
  }

  instance_refresh {
    strategy = "Rolling"
  }
}

resource "aws_autoscaling_lifecycle_hook" "launch_hook" {
  name                   = "${var.service}-${var.environment}-launch-hook"
  autoscaling_group_name = aws_autoscaling_group.instance_asg.name
  default_result         = "ABANDON"
  // TODO(b/235502114): Shorten timeout
  heartbeat_timeout      = 3600
  lifecycle_transition   = "autoscaling:EC2_INSTANCE_LAUNCHING"
}