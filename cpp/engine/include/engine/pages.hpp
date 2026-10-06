#pragma once
#include <array>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

namespace engine {
// A branch copies only the pages it writes. Unused source switch slots cost
// one null page pointer rather than sixty-four string-capable values.
template <class T> class ValuePages {
  using Page = std::array<std::optional<T>, 64>;
  std::vector<std::shared_ptr<Page>> pages_;
  std::size_t size_ = 0;

public:
  std::size_t size() const { return size_; }
  void resize(std::size_t n) {
    if (n < size_)
      throw std::logic_error("Flag slots cannot shrink");
    pages_.resize((n + 63) / 64);
    size_ = n;
  }
  const std::optional<T> &operator[](std::size_t i) const {
    static const std::optional<T> missing;
    return pages_[i / 64] ? (*pages_[i / 64])[i % 64] : missing;
  }
  template <class F> void for_each(F visit) const {
    for (std::size_t p = 0; p < pages_.size(); ++p)
      if (pages_[p])
        for (std::size_t j = 0; j < 64 && p * 64 + j < size_; ++j)
          if ((*pages_[p])[j])
            visit(p * 64 + j, *(*pages_[p])[j]);
  }
  void set(std::size_t i, std::optional<T> value) {
    if (i >= size_)
      throw std::out_of_range("Flag slot");
    auto &page = pages_[i / 64];
    if (!page)
      page = std::make_shared<Page>();
    else if (page.use_count() != 1)
      page = std::make_shared<Page>(*page);
    (*page)[i % 64] = std::move(value);
  }
  void detach() {
    for (auto &page : pages_)
      if (page && page.use_count() != 1)
        page = std::make_shared<Page>(*page);
  }
};
} // namespace engine
