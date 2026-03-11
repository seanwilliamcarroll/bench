# bench

A sampling profiler for Linux/AArch64. Uses `ptrace` to periodically interrupt a target process, capture the call stack via frame pointer unwinding, and resolve symbols from ELF binaries.

## Motivation

Built as a learning project while running Linux on AArch64 inside a UTM/QEMU VM on an Apple Silicon Mac. Hardware performance monitoring counters (PMCs) are not exposed to VM guests, so tools like `perf` that rely on `perf_event_open` with hardware events don't work in this environment. `bench` sidesteps this by using `ptrace` + `SIGSTOP` polling — pure software, no PMC access required.

## Usage

```sh
# Record a profile
bench record [-o output] [-r interval_ms] <command>

# Report results
bench report [-i input] [-f flat|folded]
```

**Examples:**
```sh
bench record ./myapp
bench record -o prof.out -r 5 ./myapp arg1 arg2
bench report -i prof.out
```

## How it works

1. Forks and attaches to the target process with `PTRACE_SEIZE`
2. Periodically interrupts all threads via `PTRACE_INTERRUPT`, walks the frame pointer chain to collect call stacks
3. Resolves and demangles symbol names from `SHT_SYMTAB`/`SHT_DYNSYM` sections in mapped ELF files
4. Writes samples to a text file; `report` aggregates exclusive and inclusive frequency per thread

## Building

```sh
cmake -B build && cmake --build build
```

Requires: Linux, AArch64, CMake 3.20+, a C++20 compiler.

## Possible improvements

- **PLT symbol synthesis** — read `.rela.plt` to label PLT stubs (e.g. `sqrt@plt`) instead of leaving them unresolved
- **Flame graph output** — emit folded stack format for use with tools like speedscope or flamegraph.pl
