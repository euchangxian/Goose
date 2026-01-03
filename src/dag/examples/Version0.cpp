#include <cstdint>
#include <iostream>

namespace runtime {

struct Then {
  virtual void process(std::int64_t) = 0;

  virtual std::int64_t result() const = 0;
};

struct Store : Then {
  std::int64_t result_;

  void process(std::int64_t x) { result_ = x; }

  std::int64_t result() const { return result_; }
};

struct Doubler : Then {
  Then* then_;

  Doubler(Then* then) : then_(then) {}

  void process(std::int64_t x) { return then_->process(x * 2); }

  std::int64_t result() const { return then_->result(); }
};

struct Adder : Then {
  Then* then_;

  Adder(Then* then) : then_(then) {}

  void process(std::int64_t x) { return then_->process(x + 1); }

  std::int64_t result() const { return then_->result(); }
};

struct Pipeline {
  Store store;
  Doubler doubler{&store};
  Adder adder{&doubler};
} storage;

auto* pipeline = static_cast<Then*>(&storage.adder);
auto process(std::int64_t x) {
  pipeline->process(x);
  return pipeline->result();
}

constexpr auto VTABLE_OVERHEAD_BYTES = 8;

static_assert(sizeof(Then) == VTABLE_OVERHEAD_BYTES, "contains only vptr");

static_assert(sizeof(Store) == sizeof(Then) + sizeof(std::int64_t),
              "contains vptr and std::int64_t member = 16 bytes");

static_assert(sizeof(Doubler) == VTABLE_OVERHEAD_BYTES + sizeof(Then),
              "contains vptr and Then* pointer = 16 bytes");

static_assert(sizeof(Adder) == VTABLE_OVERHEAD_BYTES + sizeof(Then),
              "contains vptr and Then* pointer = 16 bytes");

static_assert(sizeof(Pipeline) ==
                  (3 * sizeof(Then)) + (3 * VTABLE_OVERHEAD_BYTES),
              "Adder + Doubler + Store = 48 bytes");

}  // namespace runtime

namespace compiletime {

struct Store {
  std::int64_t result_;

  void process(std::int64_t x) { result_ = x; }

  std::int64_t result() const { return result_; }
};

template <typename Then>
struct Doubler : Then {
  void process(std::int64_t x) { Then::process(x * 2); }
};

template <typename Then>
struct Adder : Then {
  void process(std::int64_t x) { Then::process(x + 1); }
};

using Pipeline = Adder<Doubler<Store>>;

auto process(std::int64_t x) {
  Pipeline pipeline;
  pipeline.process(x);
  return pipeline.result();
}

static_assert(sizeof(Store) == 8, "size of std::int64_t member field");

static_assert(sizeof(Doubler<Store>) == 8, "empty derived class");

static_assert(sizeof(Pipeline) == 8, "empty derived class");

}  // namespace compiletime

int main() {
  std::int64_t x = 20;
  std::cout << runtime::process(x) << '\n';
  std::cout << compiletime::process(x) << '\n';
}
