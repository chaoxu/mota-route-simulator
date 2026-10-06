#pragma once
#include <cstdint>
namespace engine::probe {
inline thread_local bool enabled = false;
inline thread_local std::uint64_t allocations = 0, bytes = 0;
inline void begin() {
  allocations = bytes = 0;
  enabled = true;
}
inline void end() {
  enabled = false;
}
} // namespace engine::probe
