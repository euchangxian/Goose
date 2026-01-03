# Directed Acyclic Graph (DAG)

<!--toc:start-->

- [Directed Acyclic Graph (DAG)](#directed-acyclic-graph-dag)
  - [Problem Statement](#problem-statement)
    - [Scalability "Wall"](#scalability-wall)
  - [Version 0: Run-Time (Virtual) -> Compile-Time (Templates)](#version-0-run-time-virtual-compile-time-templates)
    - [Key Concepts](#key-concepts)
    - [Run-Time](#run-time)
      - [Trade-Offs](#trade-offs)
        - [Memory Tax](#memory-tax)
        - [CPU Tax](#cpu-tax)
    - [Compile-Time](#compile-time)
      - [Benefits (over Run-Time)](#benefits-over-run-time)
      - [Trade-Offs](#trade-offs)
    - [Compiler Explorer Comparison](#compiler-explorer-comparison)
    - [Simplified Pipeline (Pure Arithmetic):](#simplified-pipeline-pure-arithmetic)
      - [Compile-Time (Templates)](#compile-time-templates)
      - [Run-Time (`virtual`)](#run-time-virtual)
      - [Conclusion](#conclusion)
  - [Credits](#credits)

<!--toc:end-->

Data Processing Pipeline Framework.

## Problem Statement

When processing data, we very often want to (or need-to) chain a series of discrete
operations: Transform, Filter, Collect, Fan-Out, etc.

In the simplest case, these are just sequential function calls:

```C++
int increment(int x) {
  return x + 1;
}

int double(int x) {
  return x * 2;
}

int process(int x) {
  x = increment(x);
  x = double(x);
  return x;
}
```

### Scalability "Wall"

As requirements grow, simple function chaining becomes difficult for a whole bunch
of reasons:

- State Management: What if an operation requires maintaining an internal counter, a buffer
  (e.g. collecting metrics)?
  Passing state through function parameters become unwieldy quickly (Parameter Drilling).
  Global State introduces thread-safety risk, makes the code difficult to reason about, and test.

- Reusability: If we want to use the `double` logic in different `process` functions,
  we often run into conflicting requirements.
  Some use-cases require logging, others need access to a specific context.
  We inevitably end up with multiple versions of `double` (e.g., `doubleWithLog`, `doubleWithContext`)
  with slightly different signatures to accommodate every edge cases.

- Dynamic Topology: In more complex systems, we may want to "Fan-Out" data from a Stage to a
  "Pass-Through" stage based on configuration without rewriting the core logic.
  For example, in Staging, we may want to pass-through data because we only have one testing
  destination server.
  In Production, we may want to Fan-Out this data to multiple destinations.

- Testing: This comes without saying. Free-functions cannot be unit-tested easily.

## Version 0: Run-Time (Virtual) -> Compile-Time (Templates)

In Version 0, we solve the problems by using Run-time Polymorphism, which most of us
would be familiar with (`virtual` in C++, `interface` in Go, Inheritance in Java).

While this makes the code much more flexible and maintainable than free functions,
it introduces a runtime "tax" that we want to avoid in high-performance systems.

Hence, we evolve this to Compile-time polymorphism later. Just because we can in C++.

Refer to [Version0.cpp](examples/Version0.cpp) for the implementation details.

### Key Concepts

Version 0 treats the pipeline as a bunch of Stages.
Because every stage derives (or inherits) from a common interface `Then`, the
stages do not need to know the specific type of the "Then" stage, only that it exists.

- Decoupled Topology: Stages can be swapped out.
  - In the run-time version, the `Adder` can be swapped for a `Multiplier` by changing the Constructor arguments
  - In the compile-time version, change the Template arguments.

- Encapsulated State: If `Adder` needs to count how many messages it has processed,
  it can store a `std::uint64_t count_` member internally, and is invisible to the
  rest of the pipeline.
  Note that this does not yet address the passing of State (Parameter Drilling).

- Testability: We can mock Stages, allowing us to unit-test them in isolation.

### Run-Time

In the Run-Time version, we define an "interface" that allows us to define a
pipeline that processes `std::int64_t`s:

```C++
struct Then {
  virtual void process(std::int64_t) = 0;
  virtual std::int64_t result() const = 0;
};
```

```C++
struct Pipeline {
  Store store;
  Doubler doubler{&store}; // Doubler wraps Store
  Adder adder{&doubler};   // Adder wraps Doubler
} storage;
```

```C++
Then* pipeline = &storage.adder; // gets the entry-point into the Pipeline.
pipeline->process(x); // adder.process -> doubler.process -> store.process
return pipeline->result(); // returns final value of x.
```

This is simple-enough. But we are using C++.

#### Trade-Offs

The flexibility afforded by run-time polymorphism (`virtual`) comes at the cost
of **indirection**.

As the name **run-time** implies, the CPU has to do extra work at run-time to
support this "plug-and-play" feature.

##### Memory Tax

In the code, `sizeof(Then)` is **8 bytes**. `sizeof(Doubler)` is **16 bytes**

1. **Virtual Function Table Pointer (vptr)**: Since `Doubler` has virtual functions,
   the compiler adds a hidden pointer that points to it's class's "Virtual Function Table" (**vtable**).
2. Member **Pointer**: The address of the Then stage: `Then* then_` must be stored.

This `virtual` overhead can be the difference between an object fitting nicely
in a single cache line, or spanning two cache line, which is a **biiiig** problem.

##### CPU Tax

When `pipeline->process(x)` is invoked, the CPU cannot simply jump to the next
instruction (ideally, the equivalent of `addi`, `multi` instructions in MIPS).

The CPU must:

1. Dereference the **vptr** to find the **vtable**.
2. Index into the table to find the address of the `process` function.
3. Call through the address (`call` instruction)

This is a form of **Pointer-Chasing**, which is difficult for the CPU's branch
predictor to optimise, and prevents the Compiler from inlining the code.
Each vtable access is also a potential cache miss.

### Compile-Time

In this version, we use C++ templates to achieve (**more!!**) flexibility without runtime overhead.

The key difference: instead of storing a pointer to the `Then` stage, the `Then` stage is encoded directly in the template parameter.

```C++
struct Store {
  std::int64_t result_;
  void process(std::int64_t x) { result_ = x; }
  std::int64_t result() const { return result_; }
};

template <typename Then>
struct Doubler : Then {
  void process(std::int64_t x) { Then::process(x*2); }
};

template <typename Then>
struct Adder : Then {
  void process(std::int64_t x) { Then::process(x+1); }
};

using Pipeline = Adder<Doubler<Store>>;
```

```C++
Pipeline pipeline;
pipeline.process(x);
return pipeline.result();
```

Given that `Pipeline` is now a type, not an Object with pointers, the compiler
knows the exact type at compile-time.

#### Benefits (over Run-Time)

- **Zero** Indirection: No vtable lookups. When `pipeline.process(x)` is called,
  the compiler knows exactly which function to call, and can inline the entire chain.
  The entire pipeline collapse into a few instructions.
  Refer to [Compile-time vs Run-time instructions generated](#compiler-explorer-comparison)

- **Reduced Memory Overhead**: Since `Doubler` and `Adder` have no member variables
  and hold no state, `sizeof(Pipeline)` is just **8 bytes** for the `Store::result_` field,
  and can even be optimised further to **1 byte** (return the result instead of storing),
  a vast difference compared to the run-time version of **48 bytes**, sitting comfortably
  within a single cache line.
  NOTE: It is inaccurate to call this optimisation
  EBCO (**E**mpty **B**ase **C**lass **O**ptimisation).

#### Trade-Offs

- Static Topology: The pipeline structure is fixed at compile-time. `Adder` cannot
  be swapped for `Multiplier` based on a configuration file or runtime condition.
  Thankfully, we can fix this (later)!

- Code-Bloat: For each Pipeline configuration, the compiler generates code for each
  instantiation.
  This increases binary size and can lead to instruction cache misses.
  However, we can minimize **i-cache** misses using techniques like cache warming and
  optimising for the Hot-Path where performance is critical.
  Most code paths (including the hot-path) will still see an improvement due to
  reduced **data-cache** misses.

- Longer Compile Times: I guess, when run-time performance is vital, this is an
  acceptable trade-off.

### Compiler Explorer Comparison

Compiled using `CXX_COMPILER=x86-64 gcc 15.2`, `CXX_FLAGS=-g -std=c++23 -O3`.
Irrelevant instructions (like `main`) are removed for brevity.

See source code on [Compiler Explorer](https://godbolt.org/z/3GfMGKWP7).

### Simplified Abstract Pipeline

```
x -> x+1 -> (x+1) * 2
```

#### Compile-Time (Templates)

The entire pipeline is inlined, and optimised to a single `lea` instruction

```assembly
compiletime::process(long):
        lea     rax, [rdi+2+rdi]
        ret
```

The `lea` (Load Effective Address) instruction computes `rdi + rdi + 2`,
which is equivalent to `2*x + 2` (equivalent to `(x+1)*2`).

The compiler recognised the entire pipeline's computation and collapsed it
into pure arithmetic - a single instruction!

#### Run-Time (`virtual`)

Even with `-O3` optimisations, virtual function calls cannot be inlined because
the compiler does not know which concrete functions will be called at runtime.

```assembly
# Entry Point: Demonstrates vtable lookup overhead
runtime::process(long):
        # pipeline->process()
        sub     rsp, 8                                 # Allocate stack space
        mov     rsi, rdi                               # rsi = x (input parameter)
        mov     rdi, QWORD PTR runtime::pipeline[rip]  # load pipeline pointer
        mov     rax, QWORD PTR [rdi]                   # load vptr from object
        call    [QWORD PTR [rax]]                      # call through vtable (process function)

        # pipeline->result()
        mov     rdi, QWORD PTR runtime::pipeline[rip]  # reload pipeline pointer
        mov     rax, QWORD PTR [rdi]                   # reload vptr
        mov     rax, QWORD PTR [rax+8]                 # load vtable[8] (result function)
        add     rsp, 8                                 # clean up stack
        jmp     rax                                    # tail call to result()

# Each Stage requires its own function, with virtual function call overhead: pointer chasing 
runtime::Store::process(long):
        mov     QWORD PTR [rdi+8], rsi                 # result_ = x (store)
        ret

runtime::Store::result() const:
        mov     rax, QWORD PTR [rdi+8]                 # Load result_ from memory
        ret

runtime::Doubler::process(long):
        mov     rdi, QWORD PTR [rdi+8]                 # Load then_ pointer
        add     rsi, rsi                               # x = x * 2 (actual work!)
        mov     rax, QWORD PTR [rdi]                   # load vptr from then_ object
        jmp     [QWORD PTR [rax]]                      # jump through vtable to then_->process()

runtime::Doubler::result() const:
        mov     rdi, QWORD PTR [rdi+8]                 # Load then_ pointer
        mov     rax, QWORD PTR [rdi]                   # Load vptr
        jmp     [QWORD PTR [rax+8]]                    # Jump through vtable to then_->result()

runtime::Adder::process(long):
        mov     rdi, QWORD PTR [rdi+8]                 # Load then_ pointer
        add     rsi, 1                                 # x = x + 1 (actual work done!)
        mov     rax, QWORD PTR [rdi]                   # load vptr from then_ object
        jmp     [QWORD PTR [rax]]                      # jump through vtable to then_->process

runtime::Adder::result() const:
        mov     rdi, QWORD PTR [rdi+8]                 # Load then_ pointer
        mov     rax, QWORD PTR [rdi]                   # Load vptr
        jmp     [QWORD PTR [rax+8]]                    # Jump through vtable to then_->result()

# main omitted

# Global Constructor - runs before main() to initialise vtables and objects.
_GLOBAL__sub_I_runtime::storage:
        movq    xmm0, QWORD PTR .LC0[rip]
        mov     QWORD PTR runtime::storage[rip], OFFSET FLAT:vtable for runtime::Store+16
        movhps  xmm0, QWORD PTR .LC1[rip]
        movaps  XMMWORD PTR runtime::storage[rip+16], xmm0
        movq    xmm0, QWORD PTR .LC2[rip]
        movhps  xmm0, QWORD PTR .LC3[rip]
        movaps  XMMWORD PTR runtime::storage[rip+32], xmm0
        ret

# Runtime Type Information (RTTI) - enables dynamic_cast and typeid
typeinfo name for runtime::Then:
        .string "N7runtime4ThenE"
typeinfo for runtime::Then:
        .quad   vtable for __cxxabiv1::__class_type_info+16
        .quad   typeinfo name for runtime::Then
typeinfo name for runtime::Store:
        .string "N7runtime5StoreE"
typeinfo for runtime::Store:
        .quad   vtable for __cxxabiv1::__si_class_type_info+16
        .quad   typeinfo name for runtime::Store
        .quad   typeinfo for runtime::Then
typeinfo name for runtime::Doubler:
        .string "N7runtime7DoublerE"
typeinfo for runtime::Doubler:
        .quad   vtable for __cxxabiv1::__si_class_type_info+16
        .quad   typeinfo name for runtime::Doubler
        .quad   typeinfo for runtime::Then
typeinfo name for runtime::Adder:
        .string "N7runtime5AdderE"
typeinfo for runtime::Adder:
        .quad   vtable for __cxxabiv1::__si_class_type_info+16
        .quad   typeinfo name for runtime::Adder
        .quad   typeinfo for runtime::Then

# Virtual Function Tables - Array of Function Pointers for each class
vtable for runtime::Store:
        .quad   0                                      # Offset to top
        .quad   typeinfo for runtime::Store            # RTTI pointer
        .quad   runtime::Store::process(long)          # vtable[0]: process()
        .quad   runtime::Store::result() const         # vtable[8]: result()
vtable for runtime::Doubler:
        .quad   0                                      # Offset to top
        .quad   typeinfo for runtime::Doubler          # RTTI pointer
        .quad   runtime::Doubler::process(long)        # vtable[0]: process()
        .quad   runtime::Doubler::result() const       # vtable[8]: result()
vtable for runtime::Adder:
        .quad   0                                      # Offset to top
        .quad   typeinfo for runtime::Adder            # RTTI pointer
        .quad   runtime::Adder::process(long)          # vtable[0]: process()
        .quad   runtime::Adder::result() const         # vtable[8]: result()

# Global Data
runtime::pipeline:
        .quad   runtime::storage+32                 # Points to Adder (entry point)

runtime::storage:
        .zero   48                                  # 48 bytes: Store(16)+Doubler(16)+Adder(16)

# Constants for Initialisation
.LC0:
        .quad   vtable for runtime::Doubler+16
.LC1:
        .quad   runtime::storage
.LC2:
        .quad   vtable for runtime::Adder+16
.LC3:
        .quad   runtime::storage+16
```

#### Conclusion

Should be self-explanatory. `virtual` generates so much more instructions. So much overhead.

## Credits

Inspired by my internship at Squarepoint on the Trading Controls (Core Trading Services) team.
