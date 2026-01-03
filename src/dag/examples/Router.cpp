#include "../Graph.hpp"
#include "../Router.hpp"

#include <array>
#include <string>
#include <vector>

enum class Events {
  A,
  B,
  C,
  D,
};

template <typename Then = goose::dag::Successor>
struct HandleA : Then {
  template <typename String, typename... Args>
  constexpr auto process(String&& acc, Args&&... args) {
    acc += 'A';
    return Then::process(std::forward<String>(acc),
                         std::forward<Args>(args)...);
  }
};

template <typename Then = goose::dag::Successor>
struct HandleB : Then {
  template <typename String, typename... Args>
  constexpr auto process(String&& acc, Args&&... args) {
    acc += 'B';
    return Then::process(std::forward<String>(acc),
                         std::forward<Args>(args)...);
  }
};

template <typename Then = goose::dag::Successor>
struct HandleC : Then {
  template <typename String, typename... Args>
  constexpr auto process(String&& acc, Args&&... args) {
    acc += 'C';
    return Then::process(std::forward<String>(acc),
                         std::forward<Args>(args)...);
  }
};

using EventRouter = goose::dag::Router<goose::dag::Route<Events::A, HandleA<>>,
                                       goose::dag::Route<Events::B, HandleB<>>,
                                       goose::dag::Route<Events::C, HandleC<>>>;
using EventHandler = goose::dag::Graph<EventRouter, goose::dag::Collect>;

// NOTE: Unable to cleanly optimise this to 1 byte without a more-complex
// dispatch function which would sacrifice readability.
static_assert(
    sizeof(EventHandler) == sizeof(EventRouter) && sizeof(EventRouter) == 3,
    "each Route must occupy a unique address in the Router Base class.");

// NOTE: Events::D does not have a corresponding handler.
// Users can do static checks using the provided concepts if necessary.
static_assert(!goose::dag::meta::Handleable<goose::dag::Tag<Events::D>,
                                            EventRouter::RoutesTuple>);

constexpr auto test() {
  std::string acc{""};
  EventHandler handle{};
  auto [a] = handle.process(EventHandler::createTag<Events::A>(), acc);
  auto [ab] = handle.process(EventHandler::createTag<Events::B>(), acc);
  auto [abc] = handle.process(EventHandler::createTag<Events::C>(), acc);

  return a == ab && ab == abc && abc == acc && acc == "ABC";
};

constexpr auto testVisitOverload() {
  std::vector<EventRouter::Event> stream{EventRouter::createTag<Events::A>(),
                                         EventRouter::createTag<Events::B>(),
                                         EventRouter::createTag<Events::C>()};
  std::array<std::string_view, 3> expected{"A", "AB", "ABC"};

  EventHandler handle{};
  std::string acc{""};
  for (std::size_t i = 0; i < stream.size(); ++i) {
    auto [x] = handle.process(stream[i], acc);
    if (x != expected[i]) {
      return false;
    }
  }
  return acc == "ABC";
};

static_assert(test(),
              "acc should be 'ABC', and references should be returned at each "
              "process invocation");

static_assert(
    testVisitOverload(),
    "acc should be 'ABC', each iteration should append the expected character");

int main() {
  return 0;
}
