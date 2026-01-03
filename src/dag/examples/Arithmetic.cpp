#include "../Graph.hpp"

#include <utility>

template <typename Then = goose::dag::Successor>
struct Shifter : Then {
  constexpr auto process(auto acc, auto value, auto&&... args) {
    return Then::process(acc << value, std::forward<decltype(args)>(args)...);
  }
};

template <typename Then = goose::dag::Successor>
struct Adder : Then {
  constexpr auto process(auto acc, auto value, auto&&... args) {
    return Then::process(acc + value, std::forward<decltype(args)>(args)...);
  }
};

using ShiftAdd = goose::dag::Graph<Shifter<>, Adder<>, goose::dag::Collect>;
static_assert(sizeof(ShiftAdd) == 1, "EBO");

constexpr auto test = []() {
  ShiftAdd shiftAdder{};
  auto [result] = shiftAdder.process(10, 2, 2);
  return result == 42;
};

static_assert(test(), "Should return 42");

int main() {
  return 0;
}
