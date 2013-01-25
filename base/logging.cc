// Copyright 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#include <iomanip>

#include "base/safe_strerror_posix.h"

#if defined(OS_MACOSX)
#include <mach/mach.h>
#elif defined(OS_LINUX)
#include <sys/syscall.h>
#include <sys/types.h>
#endif

namespace logging {

namespace {

const char* const log_severity_names[] = {
  "INFO",
  "WARNING",
  "ERROR",
  "ERROR_REPORT",
  "FATAL"
};

}  // namespace

LogMessage::LogMessage(const char* function,
                       const char* file_path,
                       int line,
                       LogSeverity severity)
    : severity_(severity) {
  Init(function, file_path, line);
}

LogMessage::LogMessage(const char* function,
                       const char* file_path,
                       int line,
                       std::string* result)
    : severity_(LOG_FATAL) {
  Init(function, file_path, line);
  stream_ << "Check failed: " << *result << ". ";
  delete result;
}

LogMessage::~LogMessage() {
  stream_ << std::endl;
  std::string str_newline(stream_.str());
  fprintf(stderr, "%s", str_newline.c_str());
  fflush(stderr);
  if (severity_ == LOG_FATAL) {
#ifndef NDEBUG
    abort();
#else
    __asm__("int3");
#endif
  }
}

void LogMessage::Init(const char* function,
                      const std::string& file_path,
                      int line) {
  std::string file_name;
  size_t last_slash = file_path.find_last_of('/');
  if (last_slash != std::string::npos) {
    file_name.assign(file_path.substr(last_slash + 1));
  } else {
    file_name.assign(file_path);
  }

  pid_t pid = getpid();
#if defined(OS_MACOSX)
  mach_port_t thread = pthread_mach_thread_np(pthread_self());
#elif defined(OS_LINUX)
  pid_t thread = syscall(__NR_gettid);
#endif

  struct timeval tv;
  gettimeofday(&tv, NULL);
  struct tm local_time;
  localtime_r(&tv.tv_sec, &local_time);

  stream_ << '['
          << pid
          << ':'
          << thread
          << ':'
          << std::setfill('0')
          << std::setw(4) << local_time.tm_year + 1900
          << std::setw(2) << local_time.tm_mon + 1
          << std::setw(2) << local_time.tm_mday
          << ','
          << std::setw(2) << local_time.tm_hour
          << std::setw(2) << local_time.tm_min
          << std::setw(2) << local_time.tm_sec
          << '.'
          << std::setw(6) << tv.tv_usec
          << ':';

  if (severity_ >= 0) {
    stream_ << log_severity_names[severity_];
  } else {
    stream_ << "VERBOSE" << -severity_;
  }

  stream_ << ' '
          << file_name
          << ':'
          << line
          << "] ";
}

ErrnoLogMessage::ErrnoLogMessage(const char* function,
                                 const char* file_path,
                                 int line,
                                 LogSeverity severity,
                                 int err)
    : LogMessage(function, file_path, line, severity),
      err_(err) {
}

ErrnoLogMessage::~ErrnoLogMessage() {
  stream() << ": "
           << safe_strerror(err_)
           << " ("
           << err_
           << ")";
}

}  // namespace logging
