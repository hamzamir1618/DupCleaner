#pragma once

#include <stdexcept>
#include <string>

namespace dupcleaner {

class DupCleanerIOException : public std::runtime_error {
public:
    explicit DupCleanerIOException(const std::string& message)
        : std::runtime_error(message) {}
};

} // namespace dupcleaner
