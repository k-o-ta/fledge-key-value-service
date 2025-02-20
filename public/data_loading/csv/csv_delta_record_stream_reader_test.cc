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

#include "public/data_loading/csv/csv_delta_record_stream_reader.h"

#include <sstream>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "public/data_loading/csv/csv_delta_record_stream_writer.h"
#include "public/data_loading/records_utils.h"

namespace kv_server {
namespace {

using testing::UnorderedElementsAre;

KeyValueMutationRecordStruct GetKVMutationRecord(
    KeyValueMutationRecordValueT value = "value") {
  KeyValueMutationRecordStruct record;
  record.key = "key";
  record.value = value;
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

TEST(CsvDeltaRecordStreamReaderTest,
     ValidateReadingAndWriting_KVMutation_StringValues_Success) {
  std::stringstream string_stream;
  CsvDeltaRecordStreamWriter record_writer(string_stream);
  DataRecordStruct expected = GetDataRecord(GetKVMutationRecord());
  EXPECT_TRUE(record_writer.WriteRecord(expected).ok());
  EXPECT_TRUE(record_writer.Flush().ok());
  CsvDeltaRecordStreamReader record_reader(string_stream);
  EXPECT_TRUE(record_reader
                  .ReadRecords([&expected](DataRecordStruct record) {
                    EXPECT_EQ(record, expected);
                    return absl::OkStatus();
                  })
                  .ok());
}

TEST(CsvDeltaRecordStreamReaderTest,
     ValidateReadingCsvRecords_KVMutation_InvalidTimestamps_Failure) {
  const char invalid_data[] =
      R"csv(key,value,value_type,mutation_type,logical_commit_time
  key,value,string,Update,invalid_time)csv";
  std::stringstream csv_stream;
  csv_stream.str(invalid_data);
  CsvDeltaRecordStreamReader record_reader(csv_stream);
  absl::Status status = record_reader.ReadRecords(
      [](const DataRecordStruct&) { return absl::OkStatus(); });
  EXPECT_FALSE(status.ok()) << status;
  EXPECT_STREQ(std::string(status.message()).c_str(),
               "Cannot convert timestamp:invalid_time to a number.")
      << status;
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument) << status;
}

TEST(CsvDeltaRecordStreamReaderTest,
     ValidateReadingCsvRecords_KVMutation_InvalidMutation_Failure) {
  const char invalid_data[] =
      R"csv(key,value,value_type,mutation_type,logical_commit_time
  key,value,string,invalid_mutation,1000000)csv";
  std::stringstream csv_stream;
  csv_stream.str(invalid_data);
  CsvDeltaRecordStreamReader record_reader(csv_stream);
  absl::Status status = record_reader.ReadRecords(
      [](DataRecordStruct) { return absl::OkStatus(); });
  EXPECT_FALSE(status.ok()) << status;
  EXPECT_STREQ(std::string(status.message()).c_str(),
               "Unknown mutation type:invalid_mutation")
      << status;
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument) << status;
}

TEST(CsvDeltaRecordStreamReaderTest,
     ValidateReadingAndWriting_KVMutation_SetValues_Success) {
  const std::vector<std::string_view> values{
      "elem1",
      "elem2",
      "elem3",
  };
  std::stringstream string_stream;
  CsvDeltaRecordStreamWriter record_writer(string_stream);

  DataRecordStruct expected = GetDataRecord(GetKVMutationRecord(values));
  auto status = record_writer.WriteRecord(expected);
  EXPECT_TRUE(status.ok()) << status;
  status = record_writer.Flush();
  EXPECT_TRUE(status.ok()) << status;
  CsvDeltaRecordStreamReader record_reader(string_stream);
  status = record_reader.ReadRecords([&expected](DataRecordStruct record) {
    EXPECT_EQ(record, expected);
    return absl::OkStatus();
  });
  EXPECT_TRUE(status.ok()) << status;
}

TEST(CsvDeltaRecordStreamReaderTest,
     ReadingCsvRecords_KvMutation_UdfConfigHeader_Failure) {
  std::stringstream string_stream;
  CsvDeltaRecordStreamWriter record_writer(string_stream);
  DataRecordStruct expected = GetDataRecord(GetKVMutationRecord());
  EXPECT_TRUE(record_writer.WriteRecord(expected).ok());
  EXPECT_TRUE(record_writer.Flush().ok());
  CsvDeltaRecordStreamReader record_reader(
      string_stream,
      CsvDeltaRecordStreamReader<std::stringstream>::Options{
          .record_type = DataRecordType::kUserDefinedFunctionsConfig});
  const auto status = record_reader.ReadRecords(
      [](DataRecordStruct) { return absl::OkStatus(); });
  EXPECT_FALSE(status.ok()) << status;
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument) << status;
}

TEST(CsvDeltaRecordStreamReaderTest,
     ValidateReadingAndWriting_UdfConfig_Success) {
  std::stringstream string_stream;
  CsvDeltaRecordStreamWriter record_writer(
      string_stream,
      CsvDeltaRecordStreamWriter<std::stringstream>::Options{
          .record_type = DataRecordType::kUserDefinedFunctionsConfig});
  DataRecordStruct expected = GetDataRecord(GetUserDefinedFunctionsConfig());
  EXPECT_TRUE(record_writer.WriteRecord(expected).ok());
  EXPECT_TRUE(record_writer.Flush().ok());
  CsvDeltaRecordStreamReader record_reader(
      string_stream,
      CsvDeltaRecordStreamReader<std::stringstream>::Options{
          .record_type = DataRecordType::kUserDefinedFunctionsConfig});
  EXPECT_TRUE(record_reader
                  .ReadRecords([&expected](DataRecordStruct record) {
                    EXPECT_EQ(record, expected);
                    return absl::OkStatus();
                  })
                  .ok());
}

TEST(CsvDeltaRecordStreamReaderTest,
     ReadingAndWriting_UdfConfig_KvMutationHeader_Failure) {
  std::stringstream string_stream;
  CsvDeltaRecordStreamWriter record_writer(
      string_stream,
      CsvDeltaRecordStreamWriter<std::stringstream>::Options{
          .record_type = DataRecordType::kUserDefinedFunctionsConfig});
  DataRecordStruct expected = GetDataRecord(GetUserDefinedFunctionsConfig());
  EXPECT_TRUE(record_writer.WriteRecord(expected).ok());
  EXPECT_TRUE(record_writer.Flush().ok());
  CsvDeltaRecordStreamReader record_reader(
      string_stream,
      CsvDeltaRecordStreamReader<std::stringstream>::Options{
          .record_type = DataRecordType::kKeyValueMutationRecord});
  const auto status = record_reader.ReadRecords(
      [](DataRecordStruct) { return absl::OkStatus(); });
  EXPECT_FALSE(status.ok()) << status;
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument) << status;
}

TEST(CsvDeltaRecordStreamReaderTest,
     ValidateReadingCsvRecords_UdfConfig_InvalidTimestamps_Failure) {
  const char invalid_data[] =
      R"csv(code_snippet,handler_name,logical_commit_time,language
  function hello(){},hello,invalid_time,javascript)csv";
  std::stringstream csv_stream;
  csv_stream.str(invalid_data);
  CsvDeltaRecordStreamReader record_reader(
      csv_stream,
      CsvDeltaRecordStreamReader<std::stringstream>::Options{
          .record_type = DataRecordType::kUserDefinedFunctionsConfig});
  absl::Status status = record_reader.ReadRecords(
      [](const DataRecordStruct&) { return absl::OkStatus(); });
  EXPECT_FALSE(status.ok()) << status;
  EXPECT_STREQ(std::string(status.message()).c_str(),
               "Cannot convert timestamp:invalid_time to a number.")
      << status;
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument) << status;
}

TEST(CsvDeltaRecordStreamReaderTest,
     ValidateReadingCsvRecords_UdfConfig_InvalidLanguage_Failure) {
  const char invalid_data[] =
      R"csv(code_snippet,handler_name,logical_commit_time,language
  function hello(){},hello,1000000,invalid_language)csv";
  std::stringstream csv_stream;
  csv_stream.str(invalid_data);
  CsvDeltaRecordStreamReader record_reader(
      csv_stream,
      CsvDeltaRecordStreamReader<std::stringstream>::Options{
          .record_type = DataRecordType::kUserDefinedFunctionsConfig});
  absl::Status status = record_reader.ReadRecords(
      [](const DataRecordStruct&) { return absl::OkStatus(); });
  EXPECT_FALSE(status.ok()) << status;
  EXPECT_STREQ(std::string(status.message()).c_str(),
               "Language: invalid_language is not supported.")
      << status;
  EXPECT_EQ(status.code(), absl::StatusCode::kInvalidArgument) << status;
}

}  // namespace
}  // namespace kv_server
