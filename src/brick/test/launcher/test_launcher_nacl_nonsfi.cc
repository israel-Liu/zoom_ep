// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <inttypes.h>
#include <stdio.h>

#include <string>

#include "brick/command_line.h"
#include "brick/files/file_path.h"
#include "brick/files/file_util.h"
#include "brick/message_loop/message_loop.h"
#include "brick/path_service.h"
#include "brick/process/launch.h"
#include "brick/strings/string_piece.h"
#include "brick/strings/string_tokenizer.h"
#include "brick/strings/string_util.h"
#include "brick/sys_info.h"
#include "brick/test/launcher/test_launcher.h"
#include "brick/test/launcher/unit_test_launcher.h"
#include "brick/test/test_switches.h"
#include "brick/test/test_timeouts.h"
#include "build/build_config.h"

#if defined(OS_POSIX)
#include "brick/files/file_descriptor_watcher_posix.h"
#endif

namespace base {

namespace {

const char kHelpFlag[] = "help";

void PrintUsage() {
  fprintf(stdout,
          "Runs tests using the gtest framework, each batch of tests being\n"
          "run in their own process. Supported command-line flags:\n"
          "\n"
          " Common flags:\n"
          "  --gtest_filter=...\n"
          "    Runs a subset of tests (see --gtest_help for more info).\n"
          "\n"
          "  --help\n"
          "    Shows this message.\n"
          "\n"
          " Other flags:\n"
          "  --test-launcher-retry-limit=N\n"
          "    Sets the limit of test retries on failures to N.\n"
          "\n"
          "  --test-launcher-summary-output=PATH\n"
          "    Saves a JSON machine-readable summary of the run.\n"
          "\n"
          "  --test-launcher-print-test-stdio=auto|always|never\n"
          "    Controls when full test output is printed.\n"
          "    auto means to print it when the test failed.\n"
          "\n"
          "  --test-launcher-total-shards=N\n"
          "    Sets the total number of shards to N.\n"
          "\n"
          "  --test-launcher-shard-index=N\n"
          "    Sets the shard index to run to N (from 0 to TOTAL - 1).\n");
  fflush(stdout);
}

class NonSfiUnitTestPlatformDelegate : public base::UnitTestPlatformDelegate {
 public:
  NonSfiUnitTestPlatformDelegate() = default;

  bool Init(const std::string& test_binary) {
    base::FilePath dir_exe;
    if (!PathService::Get(base::DIR_EXE, &dir_exe)) {
      LOG(ERROR) << "Failed to get directory of the current executable.";
      return false;
    }

    test_path_ = dir_exe.AppendASCII(test_binary);
    return true;
  }

 private:
  bool CreateResultsFile(base::FilePath* path) override {
    if (!base::CreateNewTempDirectory(base::FilePath::StringType(), path))
      return false;
    *path = path->AppendASCII("test_results.xml");
    return true;
  }

  bool CreateTemporaryFile(base::FilePath* path) override { return false; }

  bool GetTests(std::vector<base::TestIdentifier>* output) override {
    base::FilePath output_file;
    if (!base::CreateTemporaryFile(&output_file)) {
      LOG(ERROR) << "Failed to create a temp file.";
      return false;
    }

    base::CommandLine cmd_line(test_path_);
    cmd_line.AppendSwitchPath(switches::kTestLauncherListTests, output_file);

    base::LaunchOptions launch_options;
    launch_options.wait = true;

    if (!base::LaunchProcess(cmd_line, launch_options).IsValid())
      return false;

    return base::ReadTestNamesFromFile(output_file, output);
  }

  std::string GetWrapperForChildGTestProcess() override {
    return std::string();
  }

  base::CommandLine GetCommandLineForChildGTestProcess(
      const std::vector<std::string>& test_names,
      const base::FilePath& output_file,
      const base::FilePath& flag_file) override {
    base::CommandLine cmd_line(test_path_);
    cmd_line.AppendSwitchPath(
        switches::kTestLauncherOutput, output_file);
    cmd_line.AppendSwitchASCII(
        base::kGTestFilterFlag, base::JoinString(test_names, ":"));
    return cmd_line;
  }

  void RelaunchTests(base::TestLauncher* test_launcher,
                     const std::vector<std::string>& test_names,
                     int launch_flags) override {
    RunUnitTestsBatch(test_launcher, this, test_names, launch_flags);
  }

  base::FilePath test_path_;
};

}  // namespace

int TestLauncherNonSfiMain(const std::string& test_binary) {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(kHelpFlag)) {
    PrintUsage();
    return 0;
  }

  base::TimeTicks start_time(base::TimeTicks::Now());

  TestTimeouts::Initialize();

  base::MessageLoopForIO message_loop;
#if defined(OS_POSIX)
  FileDescriptorWatcher file_descriptor_watcher(&message_loop);
#endif

  NonSfiUnitTestPlatformDelegate platform_delegate;
  if (!platform_delegate.Init(test_binary)) {
    fprintf(stderr, "Failed to initialize test launcher.\n");
    fflush(stderr);
    return 1;
  }

  base::UnitTestLauncherDelegate delegate(&platform_delegate, 10, true);
  base::TestLauncher launcher(&delegate, base::SysInfo::NumberOfProcessors());
  bool success = launcher.Run();

  fprintf(stdout, "Tests took %" PRId64 " seconds.\n",
          (base::TimeTicks::Now() - start_time).InSeconds());
  fflush(stdout);
  return success ? 0 : 1;
}

}  // namespace base
