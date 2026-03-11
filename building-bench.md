# Building a Sampling Profiler with Claude

I recently built [`bench`](https://github.com/seanwilliamcarroll/bench), a sampling profiler for Linux/AArch64. It uses `ptrace` to periodically interrupt a target process, walks the frame pointer chain to collect call stacks, and resolves symbol names from ELF binaries. The result is something like a lightweight `perf record` that works without hardware performance counters.

This post is about how I built it and where Claude fit into the process — not as a code generator, but as a teacher and a tool for offloading the parts I didn't want to think about.

---

## Why build this?

I run Linux inside UTM/QEMU on Apple Silicon. Hardware performance monitoring counters (PMCs) aren't exposed to VM guests, so `perf` with hardware events simply doesn't work. `bench` sidesteps that entirely: it uses ptrace-based sampling — pure software, no PMC access required.

---

## What I built

The core of the profiler is `ptrace`. At a high level:

1. Fork the target process, attach via `PTRACE_SEIZE`
2. On a timer, send `PTRACE_INTERRUPT` to each thread
3. Read the thread's registers via `PTRACE_GETREGSET`
4. Walk the frame pointer chain to collect the call stack
5. Resolve each address to a symbol name via ELF section parsing

I wrote all of that myself. The ptrace mechanics, the register reads, the frame pointer unwinding, the ELF symbol parsing — those were the parts I actually wanted to learn. When I was stuck, I'd ask Claude to explain a concept, but the code came from me working through it.

The main areas of solo work, roughly in order:

- **ptrace and register reads** — attaching to the target with `PTRACE_SEIZE`, reading registers via `PTRACE_GETREGSET` + `user_pt_regs` on AArch64, walking the frame pointer chain. One commit is literally called "Work towards reading the regs myself."
- **ELF symbol resolution** — parsing `/proc/pid/maps`, mmap'ing ELF files, walking `SHT_SYMTAB`/`SHT_DYNSYM`, computing load bias for position-independent executables. I also wrote a `RangeMap<K,V>` (sorted vector + binary search) for caching address-to-region lookups.
- **Multi-thread support** — restructuring the profile data model to track samples per-TID, handling `PTRACE_EVENT_CLONE` to discover and seize new threads dynamically.
- **Reporting** — inclusive/exclusive frequency counting, folded stacks output for flame graphs, C++ demangling via `__cxa_demangle`.

---

## Where Claude came in

### As a teacher

Before I implemented multi-thread support, I asked Claude to walk me through `PTRACE_SEIZE` vs `PTRACE_ATTACH` and what `PTRACE_EVENT_CLONE` actually fires on. Before I wrote the ELF parser, I asked about `SHT_SYMTAB` vs `SHT_DYNSYM`, when `sh_link` points to the string table, and how load bias works for position-independent executables. I could have found all of this in man pages and the ELF spec, but having it explained in context — with the tradeoffs surfaced — was faster and stuck better.

I also asked questions I would have felt embarrassed Googling. "What does `__cxa_demangle` return for a non-C++ binary?" (It returns null, so you just fall back to the original name — demangling is always safe to attempt.) "Is `if (auto it = map.find(k); it != map.end())` idiomatic?" (Yes, C++17, but I decided I didn't like it for readability.)

### For scaffolding and boilerplate

The parts I didn't want to spend time on:

- Initial CMake setup and project structure
- `getopt`-based CLI flag parsing (`-o`, `-r`, `-i`, `-f`)
- Config structs (`RecordConfig`, `ReportConfig`)
- Test programs — two C programs and a C++20 producer-consumer pipeline with `BoundedQueue`, matrix multiply, and an IO writer thread
- README drafts
- clang-format config and pre-commit hook

These aren't unimportant — you need them — but they're not the reason I built this project. Asking Claude to handle them let me stay focused on the parts that were actually interesting.

### For the tedious finishing work

Once the core was working, there was a category of work that was correct but mechanical: column-aligned output formatting, switching an if-else chain to a `switch` with `-Wswitch` coverage, updating the README after each feature landed, getting `.gitignore` right. Claude handled most of that, and I reviewed and committed it.

---

## The workflow in practice

The rough pattern was:

1. I'd hit a concept I didn't fully understand → ask Claude to explain it
2. I'd write the code myself
3. I'd show Claude what I wrote → ask for a review
4. Claude handled the surrounding work (CLI, tests, docs) so I wasn't context-switching

Step 3 was more useful than I expected. After I implemented folded output, Claude caught an unnecessary `sort()`, a `size_t` vs `int` type mismatch, and unused parameters — the kind of things you miss when you've been staring at the same code for an hour.

I didn't treat it as "generate this feature for me." The features I cared about, I wrote. Claude wrote the scaffolding that made the project usable and the tests that made it verifiable.

---

## What the project does now

```sh
# Profile a process
bench record -r 10 ./myapp

# Flat report: exclusive + inclusive frequency per thread
bench report

# Folded stacks for flame graphs
bench report -f folded | inferno-flamegraph > flamegraph.svg
```

The output shows per-thread call frequency with exclusive and inclusive counts. The folded format works with speedscope, flamegraph.pl, and inferno.

Still on the list: PLT stub synthesis, attaching to already-running processes, DWARF unwinding for binaries without frame pointers, and thread name resolution.

---

## What I took away

The thing that surprised me most was how well this workflow preserved the learning. I understand `ptrace` and ELF internals now — not because Claude explained them to me, but because I wrote the code after Claude explained them to me. The explanation gave me a running start; the implementation made it stick.

If I'd asked Claude to write the whole thing, I'd have a working profiler and no idea how it works. If I'd done it entirely alone, I'd have spent hours in man pages for concepts I could have absorbed in minutes with a good explanation. The middle ground — learn from it, build it yourself, let it handle the rest — turned out to be the right balance.
