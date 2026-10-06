#pragma once
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

namespace engine {
struct StateIdentity {
  std::uint64_t hash;
  std::string bytes;
};
// Identity belongs to a COW state page, not a query cache. Only mutations
// invalidate it. Independent readers can publish the same immutable result.
class IdentityCache {
  mutable std::shared_ptr<const StateIdentity> value_;

public:
  IdentityCache() = default;
  IdentityCache(const IdentityCache &other) : value_(std::atomic_load(&other.value_)) {}
  IdentityCache &operator=(const IdentityCache &other) {
    if (this != &other)
      std::atomic_store(&value_, std::atomic_load(&other.value_));
    return *this;
  }
  // Called only after the enclosing page has detached for mutation.
  void reset() { value_.reset(); }
  template <class F> std::shared_ptr<const StateIdentity> get(F compute) const {
    auto known = std::atomic_load(&value_);
    if (known)
      return known;
    std::shared_ptr<const StateIdentity> fresh = std::make_shared<StateIdentity>(compute());
    if (std::atomic_compare_exchange_strong(&value_, &known, fresh))
      return fresh;
    return known;
  }
};
} // namespace engine
