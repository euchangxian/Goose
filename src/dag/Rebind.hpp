#pragma once

namespace goose::dag {

namespace detail {

template <typename Pattern, typename Replacement, typename Target>
struct RebindImpl {
  using Type = Target;
};

template <typename Pattern, typename Replacement>
struct RebindImpl<Pattern, Replacement, Pattern> {
  using Type = Replacement;
};

template <typename Pattern,
          typename Replacement,
          template <typename...> class Target,
          typename... Args>
struct RebindImpl<Pattern, Replacement, Target<Args...>> {
  using Type = Target<typename RebindImpl<Pattern, Replacement, Args>::Type...>;
};

template <typename Pattern,
          typename Replacement,
          template <auto, typename...> class Target,
          auto V,
          typename... Args>
struct RebindImpl<Pattern, Replacement, Target<V, Args...>> {
  using Type =
      Target<V, typename RebindImpl<Pattern, Replacement, Args>::Type...>;
};

}  // namespace detail

template <typename Pattern, typename Replacement, typename Target>
using Rebind = typename detail::RebindImpl<Pattern, Replacement, Target>::Type;

}  // namespace goose::dag
