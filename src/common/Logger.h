#pragma once

#include <iostream>
#include <string>

namespace llvc::common {

enum class LogLevel { info, warning, error };

inline void log(LogLevel level, const std::string& message) {
  const char* prefix = "[info]";
  if (level == LogLevel::warning) {
    prefix = "[warning]";
  } else if (level == LogLevel::error) {
    prefix = "[error]";
  }

  std::cerr << prefix << ' ' << message << '\n';
}

} // namespace llvc::common
