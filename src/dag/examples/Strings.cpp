#include "../Graph.hpp"

#include <cctype>
#include <string>
#include <type_traits>

template <typename T>
concept HasPushBack = requires(
    std::remove_cvref_t<T> t,
    typename std::remove_cvref_t<T>::value_type val) { t.push_back(val); };

template <typename Then = goose::dag::Successor>
struct Filter : Then {
  template <HasPushBack T>
  constexpr auto process(this auto&& self,
                         T&& acc,
                         auto&& predicate,
                         auto&&... args) {
    using Container = std::remove_cvref_t<T>;
    Container result;
    for (auto c : acc) {
      if (predicate(c)) {
        result.push_back(c);
      }
    }
    return self.Then::process(std::move(result),
                              std::forward<decltype(args)>(args)...);
  }
};

template <typename Then = goose::dag::Successor>
struct Transform : Then {
  template <HasPushBack T>
  constexpr auto process(this auto&& self,
                         T&& acc,
                         auto&& transform,
                         auto&&... args) {
    using Container = std::remove_cvref_t<T>;
    Container result;
    for (auto c : acc) {
      result.push_back(transform(c));
    }
    return self.Then::process(std::move(result),
                              std::forward<decltype(args)>(args)...);
  }
};

using FilterTransform =
    goose::dag::Graph<Filter<>, Transform<>, goose::dag::Collect>;
static_assert(sizeof(FilterTransform) == 1, "EBO");

constexpr auto test = []() {
  FilterTransform graph;
  constexpr auto isLower = [](auto c) -> bool {
    return !(c >= 'A' && c <= 'Z');
  };

  constexpr auto toUpper = [](auto c) -> char {
    if (c >= 'a' && c <= 'z') {
      return 'A' + (c - 'a');
    }
    return c;
  };

  auto [result] =
      graph.process(std::string("hello,FILTERED world"), isLower, toUpper);

  return result == "HELLO, WORLD";
};

static_assert(test(), "should return 'HELLO, WORLD'");

int main() {
  return 0;
}
