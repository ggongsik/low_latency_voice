#pragma once

#include <string>
#include <utility>

namespace llvc::common {

class Result {
public:
  static Result ok() { return Result(true, {}); }
  static Result error(std::string message) { return Result(false, std::move(message)); }

  bool succeeded() const noexcept { return succeeded_; }
  explicit operator bool() const noexcept { return succeeded_; }
  const std::string& message() const noexcept { return message_; }

private:
  Result(bool succeeded, std::string message)
      : succeeded_(succeeded), message_(std::move(message)) {}

  bool succeeded_ = false;
  std::string message_;
};

} // namespace llvc::common
