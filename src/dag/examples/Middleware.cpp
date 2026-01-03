#include "../Graph.hpp"

template <typename Then = goose::dag::Successor>
struct WithX : Then {
  constexpr WithX(int x) : x(x) {}

  int x;

  constexpr auto process(this auto&& self, auto&&... args) {
    return self.Then::process(std::forward<decltype(args)>(args)...);
  };
};

template <typename Then = goose::dag::Successor>
struct Adder : Then {
  constexpr auto process(this auto&& self, auto amt, auto&&... args) {
    self.x += amt;
    return self.Then::process(std::forward<decltype(args)>(args)...);
  };
};

template <typename Then = goose::dag::Successor>
struct Shifter : Then {
  constexpr auto process(this auto&& self, auto amt) {
    self.x <<= amt;
    return self.Then::process(self.x);
  };
};

using AddShift = goose::dag::Graph<Adder<>, Shifter<>, goose::dag::Collect>;
static_assert(sizeof(AddShift) == 1, "EBO");

using WithMember = WithX<AddShift>;
static_assert(sizeof(WithMember) == sizeof(int),
              "`int` member of 4 bytes is introduced");

constexpr auto test = []() {
  WithMember graph(20);
  auto [result] = graph.process(1, 1);
  return (result == 42) && (result == graph.x);
};

static_assert(test(), "should update context 'x'");

int main() {
  return 0;
}
