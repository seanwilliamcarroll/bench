# bench

A sampling profiler for Linux/AArch64. Uses `ptrace` to periodically interrupt a target process, capture the call stack via frame pointer unwinding, and resolve symbols from ELF binaries.

## Usage

```sh
# Record a profile
bench record [-o output] [-r interval_ms] <command>

# Report results
bench report [-i input]
```

**Examples:**
```sh
bench record ./myapp
bench record -o prof.out -r 5 ./myapp arg1 arg2
bench report -i prof.out
```

## How it works

1. Forks the target process with `PTRACE_TRACEME`
2. Periodically sends `SIGSTOP`, walks the frame pointer chain to collect a call stack
3. Resolves symbol names from `SHT_SYMTAB`/`SHT_DYNSYM` sections in mapped ELF files
4. Writes samples to a text file; `report` aggregates top-of-stack frequency

## Building

```sh
cmake -B build && cmake --build build
```

Requires: Linux, AArch64, CMake 3.20+, a C++20 compiler.

## Possible improvements

- **PLT symbol synthesis** — read `.rela.plt` to label PLT stubs (e.g. `sqrt@plt`) instead of leaving them unresolved
- **Inclusive counting** — count a function whenever it appears anywhere in the stack, not just at the top
- **Eager symbol loading** — currently symbols are loaded from ELF on first miss per region; could preload all regions upfront at attach time
- **Flame graph output** — emit folded stack format for use with tools like speedscope or flamegraph.pl
- **`perf_event_open`** — replace `SIGSTOP` polling with hardware/software perf counters for lower overhead and more precise sampling
- **C++ demangling** — run symbol names through `__cxa_demangle` for readable C++ output
- **Multi-thread support** — currently only traces the main thread
