/*
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

#include "public/data_loading/writers/snapshot_stream_writer.h"

#include <filesystem>

#include "absl/strings/str_format.h"
#include "gmock/gmock.h"
#include "google/protobuf/util/message_differencer.h"
#include "gtest/gtest.h"
#include "public/data_loading/readers/delta_record_stream_reader.h"
#include "public/data_loading/records_utils.h"
#include "public/data_loading/writers/delta_record_stream_writer.h"

namespace kv_server {
namespace {

using SnapshotWriterOptions = SnapshotStreamWriter<std::stringstream>::Options;

constexpr std::string_view kBaseSnapshotFilename = "SNAPSHOT_0000000000000001";
constexpr std::string_view kEndingDeltaFilename = "DELTA_0000000000000010";

KVFileMetadata GetMetadata() {
  KVFileMetadata metadata;
  return metadata;
}
KVFileMetadata GetSnapshotMetadata() {
  KVFileMetadata metadata;
  SnapshotMetadata* snapshot = metadata.mutable_snapshot();
  *snapshot->mutable_starting_file() = kBaseSnapshotFilename;
  *snapshot->mutable_ending_delta_file() = kEndingDeltaFilename;
  return metadata;
}

KeyValueMutationRecordStruct GetKVMutationRecord(std::string_view key = "key") {
  KeyValueMutationRecordStruct record;
  record.key = key;
  record.value = "value";
  record.logical_commit_time = 1234567890;
  record.mutation_type = KeyValueMutationType::Update;
  return record;
}

UserDefinedFunctionsConfigStruct GetUserDefinedFunctionsConfig() {
  UserDefinedFunctionsConfigStruct udf_config_record;
  udf_config_record.language = UserDefinedFunctionsLanguage::Javascript;
  udf_config_record.code_snippet = "function hello(){}";
  udf_config_record.handler_name = "hello";
  udf_config_record.logical_commit_time = 1234567890;
  return udf_config_record;
}

DataRecordStruct GetDataRecord(const RecordT& record) {
  DataRecordStruct data_record;
  data_record.record = record;
  return data_record;
}

std::filesystem::path GetRecordAggregatorDbFile() {
  std::filesystem::path full_path(std::filesystem::temp_directory_path());
  full_path /= absl::StrFormat("RecordAggregator.%d.db", std::rand());
  if (std::filesystem::exists(full_path)) {
    std::filesystem::remove(full_path);
  }
  return full_path;
}

class SnapshotStreamWriterTest
    : public ::testing::TestWithParam<SnapshotWriterOptions> {
 protected:
  absl::StatusOr<std::unique_ptr<SnapshotStreamWriter<std::stringstream>>>
  CreateSnapshotWriter(std::stringstream& dest_stream) {
    return SnapshotStreamWriter<std::stringstream>::Create(GetParam(),
                                                           dest_stream);
  }
};

INSTANTIATE_TEST_SUITE_P(
    Options, SnapshotStreamWriterTest,
    testing::Values(
        SnapshotWriterOptions{.metadata = GetSnapshotMetadata(),
                              .temp_data_file = "",
                              .compress_snapshot = false},
        SnapshotWriterOptions{.metadata = GetSnapshotMetadata(),
                              .temp_data_file = "",
                              .compress_snapshot = true},
        SnapshotWriterOptions{.metadata = GetSnapshotMetadata(),
                              .temp_data_file = GetRecordAggregatorDbFile(),
                              .compress_snapshot = false},
        SnapshotWriterOptions{.metadata = GetSnapshotMetadata(),
                              .temp_data_file = GetRecordAggregatorDbFile(),
                              .compress_snapshot = true}));

TEST_P(SnapshotStreamWriterTest, ValidateThatRecordsAreDedupedInSnapshot) {
  std::stringstream dest_stream;
  auto snapshot_writer =
      SnapshotStreamWriterTest::CreateSnapshotWriter(dest_stream);
  EXPECT_TRUE(snapshot_writer.ok()) << snapshot_writer.status();
  auto data_record = GetDataRecord(GetKVMutationRecord());
  // Write the same record to snapshot 3 times.
  std::vector data_records{data_record, data_record, data_record};
  for (const auto& recd : data_records) {
    auto status = (*snapshot_writer)->WriteRecord(recd);
    EXPECT_TRUE(status.ok()) << status;
  }
  auto status = (*snapshot_writer)->Finalize();
  EXPECT_TRUE(status.ok()) << status;

  DeltaRecordStreamReader record_reader(dest_stream);
  testing::MockFunction<absl::Status(DataRecordStruct)> record_callback;
  // We expect one call to record_callback because records will be deduped in
  // the snapshot stream.
  EXPECT_CALL(record_callback, Call(data_record))
      .Times(1)
      .WillOnce([](DataRecordStruct) { return absl::OkStatus(); });
  status = record_reader.ReadRecords(record_callback.AsStdFunction());
  EXPECT_TRUE(status.ok()) << status;
}

TEST_P(SnapshotStreamWriterTest, ValidateThatDeletedRecordsAreNotInSnapshot) {
  std::stringstream dest_stream;
  auto snapshot_writer =
      SnapshotStreamWriterTest::CreateSnapshotWriter(dest_stream);
  EXPECT_TRUE(snapshot_writer.ok()) << snapshot_writer.status();
  auto kv_record = GetKVMutationRecord();
  auto data_record = GetDataRecord(kv_record);
  auto status = (*snapshot_writer)->WriteRecord(data_record);
  EXPECT_TRUE(status.ok()) << status;
  // Delete the record written above.
  kv_record.mutation_type = KeyValueMutationType::Delete;
  kv_record.logical_commit_time++;
  auto data_record_with_deletion = GetDataRecord(kv_record);
  status = (*snapshot_writer)->WriteRecord(data_record_with_deletion);
  EXPECT_TRUE(status.ok()) << status;
  status = (*snapshot_writer)->Finalize();
  EXPECT_TRUE(status.ok()) << status;

  DeltaRecordStreamReader record_reader(dest_stream);
  testing::MockFunction<absl::Status(DataRecordStruct)> record_callback;
  EXPECT_CALL(record_callback, Call(data_record)).Times(0);
  status = record_reader.ReadRecords(record_callback.AsStdFunction());
  EXPECT_TRUE(status.ok()) << status;
}

TEST_P(SnapshotStreamWriterTest,
       ValidateThatLastestRecordUpdatesAreInSnapshot) {
  std::stringstream dest_stream;
  auto snapshot_writer =
      SnapshotStreamWriterTest::CreateSnapshotWriter(dest_stream);
  EXPECT_TRUE(snapshot_writer.ok()) << snapshot_writer.status();
  auto kv_record = GetKVMutationRecord();
  auto data_record = GetDataRecord(kv_record);
  auto status = (*snapshot_writer)->WriteRecord(data_record);
  EXPECT_TRUE(status.ok()) << status;
  // Update the record written above.
  std::string value =
      absl::StrCat(std::get<std::string_view>(kv_record.value), "-updated");
  kv_record.value = value;
  kv_record.logical_commit_time++;
  status = (*snapshot_writer)->WriteRecord(data_record);
  EXPECT_TRUE(status.ok()) << status;
  status = (*snapshot_writer)->Finalize();
  EXPECT_TRUE(status.ok()) << status;

  DeltaRecordStreamReader record_reader(dest_stream);
  testing::MockFunction<absl::Status(DataRecordStruct)> record_callback;
  EXPECT_CALL(record_callback, Call(data_record))
      .Times(1)
      .WillOnce([](DataRecordStruct) { return absl::OkStatus(); });
  status = record_reader.ReadRecords(record_callback.AsStdFunction());
  EXPECT_TRUE(status.ok()) << status;
}

TEST_P(SnapshotStreamWriterTest,
       ValidateThatOlderRecordUpdatesAreNotInSnapshot) {
  std::stringstream dest_stream;
  auto snapshot_writer =
      SnapshotStreamWriterTest::CreateSnapshotWriter(dest_stream);
  EXPECT_TRUE(snapshot_writer.ok()) << snapshot_writer.status();
  auto kv_record = GetKVMutationRecord();
  auto data_record = GetDataRecord(kv_record);
  auto status = (*snapshot_writer)->WriteRecord(data_record);
  EXPECT_TRUE(status.ok()) << status;
  // Update the record written above, but also with an older
  // logical_commit_time.
  std::string value =
      absl::StrCat(std::get<std::string_view>(kv_record.value), "-updated");
  kv_record.value = value;
  kv_record.logical_commit_time--;
  auto old_data_record = GetDataRecord(kv_record);
  status = (*snapshot_writer)->WriteRecord(data_record);
  EXPECT_TRUE(status.ok()) << status;
  status = (*snapshot_writer)->Finalize();
  EXPECT_TRUE(status.ok()) << status;

  DeltaRecordStreamReader record_reader(dest_stream);
  testing::MockFunction<absl::Status(DataRecordStruct)> record_callback;
  EXPECT_CALL(record_callback, Call(data_record))
      .Times(1)
      .WillOnce([](DataRecordStruct) { return absl::OkStatus(); });
  status = record_reader.ReadRecords(record_callback.AsStdFunction());
  EXPECT_TRUE(status.ok()) << status;
}

TEST_P(SnapshotStreamWriterTest,
       ValidateWritingMultipleRecordsUsingASrcStream) {
  testing::MockFunction<absl::Status(DataRecordStruct)> record_callback;
  std::stringstream src_stream;
  auto record_writer = DeltaRecordStreamWriter<>::Create(
      src_stream, DeltaRecordWriter::Options{.metadata = GetMetadata()});
  EXPECT_TRUE(record_writer.ok());
  for (std::string_view key : std::vector{"key1", "key2", "key3", "key4"}) {
    auto data_record = GetDataRecord(GetKVMutationRecord(key));
    EXPECT_TRUE((*record_writer)->WriteRecord(data_record).ok());
    EXPECT_CALL(record_callback, Call(data_record))
        .WillOnce([](DataRecordStruct) { return absl::OkStatus(); });
  }
  (*record_writer)->Close();
  std::stringstream dest_stream;
  auto snapshot_writer =
      SnapshotStreamWriterTest::CreateSnapshotWriter(dest_stream);
  EXPECT_TRUE(snapshot_writer.ok()) << snapshot_writer.status();
  auto status = (*snapshot_writer)->WriteRecordStream(src_stream);
  EXPECT_TRUE(status.ok()) << status;
  status = (*snapshot_writer)->Finalize();
  EXPECT_TRUE(status.ok()) << status;
  DeltaRecordStreamReader record_reader(dest_stream);
  status = record_reader.ReadRecords(record_callback.AsStdFunction());
  EXPECT_TRUE(status.ok()) << status;
}

TEST_P(SnapshotStreamWriterTest,
       ValidateThatSnapshotMetadataIsSavedWithSnapshot) {
  std::stringstream dest_stream;
  auto snapshot_writer =
      SnapshotStreamWriterTest::CreateSnapshotWriter(dest_stream);
  EXPECT_TRUE(snapshot_writer.ok()) << snapshot_writer.status();
  auto status = (*snapshot_writer)->Finalize();
  EXPECT_TRUE(status.ok()) << status;
  DeltaRecordStreamReader record_reader(dest_stream);
  auto metadata = record_reader.ReadMetadata();
  EXPECT_TRUE(metadata.ok()) << metadata.status();
  EXPECT_TRUE(google::protobuf::util::MessageDifferencer::Equals(
      GetSnapshotMetadata(), *metadata));
}

TEST_P(SnapshotStreamWriterTest, UdfConfig_DedupedInSnapshot) {
  std::stringstream dest_stream;
  auto snapshot_writer =
      SnapshotStreamWriterTest::CreateSnapshotWriter(dest_stream);
  EXPECT_TRUE(snapshot_writer.ok()) << snapshot_writer.status();
  auto data_record = GetDataRecord(GetUserDefinedFunctionsConfig());
  // Write the same record to snapshot 3 times.
  std::vector data_records{data_record, data_record, data_record};
  for (const auto& recd : data_records) {
    auto status = (*snapshot_writer)->WriteRecord(recd);
    EXPECT_TRUE(status.ok()) << status;
  }
  auto status = (*snapshot_writer)->Finalize();
  EXPECT_TRUE(status.ok()) << status;

  DeltaRecordStreamReader record_reader(dest_stream);
  testing::MockFunction<absl::Status(DataRecordStruct)> record_callback;
  // We expect one call to record_callback because records will be deduped in
  // the snapshot stream.
  EXPECT_CALL(record_callback, Call(data_record))
      .Times(1)
      .WillOnce([](DataRecordStruct) { return absl::OkStatus(); });
  status = record_reader.ReadRecords(record_callback.AsStdFunction());
  EXPECT_TRUE(status.ok()) << status;
}

TEST_P(SnapshotStreamWriterTest,
       UdfConfig_UpdatesWithLargestCommitTimestampInSnapshot) {
  std::stringstream dest_stream;
  auto snapshot_writer =
      SnapshotStreamWriterTest::CreateSnapshotWriter(dest_stream);
  EXPECT_TRUE(snapshot_writer.ok()) << snapshot_writer.status();
  auto udf_config = GetUserDefinedFunctionsConfig();
  auto data_record = GetDataRecord(udf_config);
  auto status = (*snapshot_writer)->WriteRecord(data_record);
  EXPECT_TRUE(status.ok()) << status;
  // Update the udf config written above.
  std::string handler_name = absl::StrCat(udf_config.handler_name, "-updated");
  udf_config.handler_name = handler_name;
  udf_config.logical_commit_time++;
  status = (*snapshot_writer)->WriteRecord(data_record);
  EXPECT_TRUE(status.ok()) << status;
  status = (*snapshot_writer)->Finalize();
  EXPECT_TRUE(status.ok()) << status;

  DeltaRecordStreamReader record_reader(dest_stream);
  testing::MockFunction<absl::Status(DataRecordStruct)> record_callback;
  EXPECT_CALL(record_callback, Call(data_record))
      .Times(1)
      .WillOnce([](DataRecordStruct) { return absl::OkStatus(); });
  status = record_reader.ReadRecords(record_callback.AsStdFunction());
  EXPECT_TRUE(status.ok()) << status;
}

TEST_P(SnapshotStreamWriterTest,
       UdfConfig_IgnoresSameCommitTimestampInSnapshot) {
  std::stringstream dest_stream;
  auto snapshot_writer =
      SnapshotStreamWriterTest::CreateSnapshotWriter(dest_stream);
  EXPECT_TRUE(snapshot_writer.ok()) << snapshot_writer.status();
  auto udf_config = GetUserDefinedFunctionsConfig();
  auto data_record = GetDataRecord(udf_config);
  auto status = (*snapshot_writer)->WriteRecord(data_record);
  EXPECT_TRUE(status.ok()) << status;
  // Update the udf config written above.
  auto old_data_record = GetDataRecord(udf_config);
  std::string handler_name = absl::StrCat(udf_config.handler_name, "-updated");
  udf_config.handler_name = handler_name;
  udf_config.logical_commit_time;
  status = (*snapshot_writer)->WriteRecord(old_data_record);
  EXPECT_TRUE(status.ok()) << status;
  status = (*snapshot_writer)->Finalize();
  EXPECT_TRUE(status.ok()) << status;

  DeltaRecordStreamReader record_reader(dest_stream);
  testing::MockFunction<absl::Status(DataRecordStruct)> record_callback;
  EXPECT_CALL(record_callback, Call(data_record))
      .Times(1)
      .WillOnce([](DataRecordStruct) { return absl::OkStatus(); });
  status = record_reader.ReadRecords(record_callback.AsStdFunction());
  EXPECT_TRUE(status.ok()) << status;
}

TEST(SnapshotStreamWriterTest,
     ValidateCreatingSnapshotWriterWithValidMetadata) {
  std::stringstream dest_stream;
  auto snapshot_writer = SnapshotStreamWriter<>::Create(
      {.metadata = GetSnapshotMetadata()}, dest_stream);
  EXPECT_TRUE(snapshot_writer.ok()) << snapshot_writer.status();
}

TEST(SnapshotStreamWriterTest,
     ValidateCreatingSnapshotWriterWithEmptySnapshotMetadata) {
  std::stringstream dest_stream;
  auto metadata = GetSnapshotMetadata();
  metadata.mutable_snapshot()->Clear();
  auto snapshot_writer =
      SnapshotStreamWriter<>::Create({.metadata = metadata}, dest_stream);
  EXPECT_FALSE(snapshot_writer.ok()) << snapshot_writer.status();
  EXPECT_EQ(snapshot_writer.status().code(),
            absl::StatusCode::kInvalidArgument);
}

TEST(SnapshotStreamWriterTest,
     ValidateCreatingSnapshotWriterWithInvalidStartingFilename) {
  std::stringstream dest_stream;
  auto metadata = GetSnapshotMetadata();
  *metadata.mutable_snapshot()->mutable_starting_file() = "invalid_filename";
  auto snapshot_writer =
      SnapshotStreamWriter<>::Create({.metadata = metadata}, dest_stream);
  EXPECT_FALSE(snapshot_writer.ok()) << snapshot_writer.status();
  EXPECT_EQ(snapshot_writer.status().code(),
            absl::StatusCode::kInvalidArgument);
}

TEST(SnapshotStreamWriterTest,
     ValidateCreatingSnapshotWriterWithInvalidEndingFilename) {
  std::stringstream dest_stream;
  auto metadata = GetSnapshotMetadata();
  *metadata.mutable_snapshot()->mutable_ending_delta_file() =
      "invalid_filename";
  auto snapshot_writer =
      SnapshotStreamWriter<>::Create({.metadata = metadata}, dest_stream);
  EXPECT_FALSE(snapshot_writer.ok()) << snapshot_writer.status();
  EXPECT_EQ(snapshot_writer.status().code(),
            absl::StatusCode::kInvalidArgument);
}
}  // namespace
}  // namespace kv_server
