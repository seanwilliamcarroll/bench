# Building a Sampling Profiler with Claude

I recently built [`bench`](https://github.com/seanwilliamcarroll/bench), a sampling profiler for Linux/AArch64. It uses `ptrace` to periodically interrupt a target process, walks the frame pointer chain to collect call stacks, and resolves symbol names from ELF binaries. The result is something like a lightweight `perf stat` that works without hardware performance counters.

This post is about how I built it and where Claude fit into the process — not as a code generator, but as a teacher and a tool for offloading the parts I didn't want to think about.

---

## Why build this?

I run Linux inside UTM/QEMU on Apple Silicon. Hardware performance monitoring counters (PMCs) aren't exposed to VM guests, so `perf` with hardware events simply doesn't work. `bench` sidesteps that entirely: it uses `ptrace` + `SIGSTOP` polling — pure software, no PMC access required.

---

## What I built

The core of the profiler is `ptrace`. At a high level:

1. Fork the target process, attach via `PTRACE_SEIZE`
2. On a timer, send `PTRACE_INTERRUPT` to each thread
3. Read the thread's registers via `PTRACE_GETREGSET`
4. Walk the frame pointer chain to collect the call stack
5. Resolve each address to a symbol name via ELF section parsing

I wrote all of that myself. The ptrace mechanics, the register reads, the frame pointer unwinding, the ELF symbol parsing — those were the parts I actually wanted to learn. When I was stuck, I'd ask Claude to explain a concept, but the code came from me working through it.

Some specific commits that were solo work:

- **`Start down the record side first`** — first `ptrace` attach and wait loop
- **`Work towards reading the regs myself`** — `PTRACE_GETREGSET` + `user_pt_regs` on AArch64
- **`Start looking up symbols from binaries`** — opening ELF files, walking `SHT_SYMTAB`/`SHT_DYNSYM`, computing load bias for `ET_DYN` vs `ET_EXEC`
- **`Add data structure for efficient lookup of address in mapped regions`** — `RangeMap<K,V>` using a sorted vector + binary search for mutually exclusive address ranges
- **`Rough refactor to begin multi-thread support`** — restructuring `Profile` to track samples per-TID
- **`Actually attach to new threads during the recording process`** — handling `PTRACE_EVENT_CLONE`, discovering and seizing new threads dynamically
- **`Add inclusive and exclusive calculation to report`** — per-thread exclusive (top-of-stack) and inclusive (any frame, deduplicated per sample) frequency counting
- **`Implement folded reporting for flamegraph outputs`** — generating folded stacks format (`tid=N;root;...;leaf count`) for consumption by speedscope or flamegraph.pl
- **`Add C++ demangling`** — `abi::__cxa_demangle` at symbol load time, falling back to the raw mangled name on failure

---

## Where Claude came in

### As a teacher

Before I implemented multi-thread support, I asked Claude to walk me through `PTRACE_SEIZE` vs `PTRACE_ATTACH` and what `PTRACE_EVENT_CLONE` actually fires on. Before I wrote the ELF parser, I asked about `SHT_SYMTAB` vs `SHT_DYNSYM`, when `sh_link` points to the string table, and how load bias works for position-independent executables. I could have found all of this in man pages and the ELF spec, but having it explained in context — with the tradeoffs surfaced — was faster and stuck better.

I also asked questions I would have felt embarrassed Googling. "What does `__cxa_demangle` return for a non-C++ binary?" (The mangled name is returned as-is, so demangling is always safe to attempt.) "Is `if (auto it = map.find(k); it != map.end())` idiomatic?" (Yes, C++17, but I decided I didn't like it for readability.)

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

### Code review

After I implemented something, I'd show Claude the diff and ask for a review. This caught real issues: an unnecessary `sort()` in `folded_report()`, a `size_t` vs `int` type mismatch in a loop, unused parameters. Having an outside read on freshly-written code is useful even when the reviewer isn't human.

---

## The workflow in practice

The rough pattern was:

1. I'd hit a concept I didn't fully understand → ask Claude to explain it
2. I'd write the code myself
3. I'd show Claude what I wrote → ask for a review
4. Claude handled the surrounding work (CLI, tests, docs) so I wasn't context-switching

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

Still on the list: PLT stub synthesis (labeling `sqrt@plt` instead of showing an unresolved address), attaching to an already-running process with `-p pid`, DWARF unwinding for binaries compiled without `-fno-omit-frame-pointer`, and thread name resolution from `/proc/pid/task/tid/comm`.
