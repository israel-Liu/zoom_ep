// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// A fuzzer that checks correctness of json parser/writer.
// The fuzzer input is passed through parsing twice,
// so that presumably valid json is parsed/written again.

#include <stddef.h>
#include <stdint.h>

#include <string>

#include "brick/json/json_reader.h"
#include "brick/json/json_writer.h"
#include "brick/json/string_escape.h"
#include "brick/logging.h"
#include "brick/values.h"

// Entry point for libFuzzer.
// We will use the last byte of data as parsing options.
// The rest will be used as text input to the parser.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size < 2)
    return 0;

  int error_code, error_line, error_column;
  std::string error_message;

  // Create a copy of input buffer, as otherwise we don't catch
  // overflow that touches the last byte (which is used in options).
  std::unique_ptr<char[]> input(new char[size - 1]);
  memcpy(input.get(), data, size - 1);

  base::StringPiece input_string(input.get(), size - 1);

  const int options = data[size - 1];
  auto parsed_value = base::JSONReader::ReadAndReturnError(
      input_string, options, &error_code, &error_message, &error_line,
      &error_column);
  if (!parsed_value)
    return 0;

  std::string parsed_output;
  bool b = base::JSONWriter::Write(*parsed_value, &parsed_output);
  LOG_ASSERT(b);

  auto double_parsed_value = base::JSONReader::ReadAndReturnError(
      parsed_output, options, &error_code, &error_message, &error_line,
      &error_column);
  LOG_ASSERT(double_parsed_value);
  std::string double_parsed_output;
  bool b2 =
      base::JSONWriter::Write(*double_parsed_value, &double_parsed_output);
  LOG_ASSERT(b2);

  LOG_ASSERT(parsed_output == double_parsed_output)
      << "Parser/Writer mismatch."
      << "\nInput=" << base::GetQuotedJSONString(parsed_output)
      << "\nOutput=" << base::GetQuotedJSONString(double_parsed_output);

  return 0;
}
