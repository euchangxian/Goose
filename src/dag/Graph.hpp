#pragma once

#include "Nodes.hpp"
#include "Rebind.hpp"

namespace goose::dag {

namespace detail {

template <typename... Nodes>
struct GraphImpl;

template <typename Leaf>
struct GraphImpl<Leaf> {
  static_assert(meta::Terminal<Leaf>,
                "terminal node must be a class and should not derive from "
                "goose::dag::Successor.");

  using Type = Leaf;
};

template <typename Head, typename... Tail>
struct GraphImpl<Head, Tail...> {
  static_assert(
      meta::Internal<Head>,
      "intermediate nodes must be class templates and should derive from "
      "goose::dag::Successor.");
  using Type = Rebind<Successor, typename GraphImpl<Tail...>::Type, Head>;
};

}  // namespace detail

template <typename... Nodes>
  requires(meta::NodeLike<Nodes> && ...)
struct Graph : detail::GraphImpl<Nodes...>::Type {
  using Subgraph = typename detail::GraphImpl<Nodes...>::Type;
  using Subgraph::Subgraph;

  template <typename... Args>
  constexpr decltype(auto) process(this auto&& self, Args&&... args) {
    return self.Subgraph::process(std::forward<Args>(args)...);
  }
};

}  // namespace goose::dag
