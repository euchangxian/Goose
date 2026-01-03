#pragma once

#include <tuple>
#include <type_traits>
#include <utility>

namespace goose::dag {

struct Successor {};

namespace meta {

template <typename T>
concept Internal = std::is_class_v<T> && std::is_base_of_v<Successor, T>;

template <typename T>
concept Terminal = std::is_class_v<T> && !std::is_base_of_v<Successor, T>;

template <typename T>
concept NodeLike = Internal<T> || Terminal<T>;

}  // namespace meta

template <typename Then = Successor>
  requires meta::NodeLike<Then>
struct PassThrough : Then {
  template <typename... Args>
  constexpr decltype(auto) process(this auto&& self, Args&&... args) {
    return self.Then::process(std::forward<Args>(args)...);
  }
};

template <typename Then = Successor>
  requires meta::NodeLike<Then>
struct Break : Then {
  template <typename... Args>
  constexpr void process(auto&&...) {
    return;
  }
};

struct Sink {
  constexpr void process(this auto&&, auto&&...) { return; }
};

struct Collect {
  template <typename... Args>
  constexpr decltype(auto) process(this auto&&, Args&&... args) {
    using ReturnType =
        std::tuple<std::conditional_t<std::is_lvalue_reference_v<Args>, Args,
                                      std::decay_t<Args>>...>;
    return ReturnType{std::forward<Args>(args)...};
  }
};

static_assert(meta::NodeLike<Successor>);
static_assert(meta::Internal<Break<>>);
static_assert(meta::Internal<PassThrough<>>);
static_assert(meta::Terminal<Sink>);
static_assert(meta::Terminal<Collect>);

}  // namespace goose::dag
