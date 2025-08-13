#pragma once

#include <string>

namespace ssl {

// Returns the semantic version of this library.
std::string version();

// Example initialization API. Returns true on success.
bool initialize();

}  // namespace ssl


