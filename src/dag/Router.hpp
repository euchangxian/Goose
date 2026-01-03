#pragma once

#include "Nodes.hpp"

#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace goose::dag {

namespace detail {

template <typename IC, typename... Routes>
struct isHandledImpl {
  static constexpr auto value =
      ((std::is_same_v<std::remove_cvref_t<IC>, typename Routes::Tag> || ...));
};

template <typename IC, typename... Routes>
struct isHandledImpl<IC, std::tuple<Routes...>> : isHandledImpl<IC, Routes...> {
};

template <typename T>
struct isVariantImpl : std::false_type {};

template <typename... Ts>
struct isVariantImpl<std::variant<Ts...>> : std::true_type {};

}  // namespace detail

namespace meta {

template <typename IC, typename... Routes>
concept Handleable = detail::isHandledImpl<IC, Routes...>::value;

template <typename T>
concept Variant = detail::isVariantImpl<std::remove_cvref_t<T>>::value;

// NOTE: Unable to check for operator(Tag, Args...), because of variadic args.
template <typename T>
concept RouteLike = meta::NodeLike<T> && requires { typename T::Tag; };

};  // namespace meta

template <auto EventV>
using Tag = std::integral_constant<decltype(EventV), EventV>;

template <auto EventV, typename Node>
  requires(meta::NodeLike<Node>)
struct Route : Node {
  using Tag = Tag<EventV>;

  template <typename... Args>
  constexpr auto operator()(this auto&& self, Tag, Args&&... args) {
    return self.Node::process(std::forward<decltype(args)>(args)...);
  }
};

template <typename... Routes>
  requires(meta::RouteLike<Routes> && ...)
struct Router : Routes... {
  using Routes::operator()...;

  using Event = std::variant<typename Routes::Tag...>;
  using RoutesTuple = std::tuple<Routes...>;

  template <auto EventV>
  static constexpr auto createTag() {
    static_assert(meta::Handleable<Tag<EventV>, Routes...>,
                  "no route defined for this Tag. A Route<Tag, Handler> must "
                  "be added to the Router");
    return Tag<EventV>{};
  }

  template <typename IC, typename... Args>
    requires(!meta::Variant<IC> && meta::Handleable<IC, Routes...>)
  constexpr decltype(auto) process(this auto&& self, IC&& ic, Args&&... args) {
    return self.operator()(std::forward<IC>(ic), std::forward<Args>(args)...);
  }

  template <typename Variant, typename... Args>
    requires(meta::Variant<Variant>)
  constexpr decltype(auto) process(this auto&& self,
                                   Variant&& var,
                                   Args&&... args) {
    return std::visit(
        [&](auto tag) -> decltype(auto) {
          return self.process(tag, std::forward<Args>(args)...);
        },
        std::forward<Variant>(var));
  }
};

};  // namespace goose::dag
