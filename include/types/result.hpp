#pragma once

#include <string>
#include <expected>

namespace types {

using Err = std::string;
template <class T> using Result = std::expected<T, Err>;

} // namespace types
