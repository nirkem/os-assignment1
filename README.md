# xv6: new process system calls

[![test](https://github.com/nirkem/xv6-process-syscalls/actions/workflows/test.yml/badge.svg)](https://github.com/nirkem/xv6-process-syscalls/actions/workflows/test.yml)

New system calls for [xv6](https://github.com/mit-pdos/xv6-riscv), MIT's small Unix-like teaching kernel for RISC-V. Written for the Operating Systems course at Ben-Gurion University in 2025 (assignment 1).

The work is in the kernel's process code, `kernel/proc.c` and `kernel/sysproc.c`, plus the syscall plumbing that connects user programs to it. Every other file matches the xv6 the course started from, so `git diff cb0971f` shows exactly what I added.

## What I added

**`memsize()`** returns how many bytes of memory the calling process is using (its `sz`).

**Exit messages.** `exit(status, msg)` now takes a message, kept in the process table entry. `wait(&status, msg)` copies the child's message back into the parent's memory. The xv6 shell prints it, so a program can say something on its way out. Messages hold up to 31 characters (`MAXEXITMSG` is 32 bytes with the null). A longer one is cut to fit, and a null or unreadable pointer becomes an empty message, because `exit` must never fail. Every user program in the tree was updated to the new signatures.

**`forkn(n, pids)`** creates `n` children (1 to `NFORKN`, which is 16) in one call:
- The parent gets `0` back, plus every child's PID in `pids`.
- Child `i` gets `i` back (1 to `n`), so each child knows which one it is without any extra coordination.
- It's all or nothing. Every child is prepared first and only marked runnable once all of them exist. If one fails, or `pids` can't be written, the ones already created are freed and the call returns `-1`.

**`waitall(&n, statuses)`** blocks until every child has exited, then returns how many there were and all their exit statuses at once.

## The programs

| Program | What it shows |
| --- | --- |
| `helloworld` | The basics: a user program that prints and exits |
| `goodbye` | An exit message: the shell prints `Goodbye World xv6` when it ends |
| `memsize_test` | `memsize` before and after a 20 KB `malloc`, and after `free` |
| `bigarray` | `forkn` + `waitall`: sums 65,536 integers across 6 children |
| `syscalltests` | Tests for all four calls, including their error paths |

`bigarray` fills an array with 0 to 65,535 and calls `forkn(6, pids)`. Each child sums its own slice and returns the partial sum as its exit status. The parent collects all six with `waitall` and adds them up. The total should be 2,147,450,880, the sum of 0 to 65,535, which just fits in a 32-bit `int`.

## Tests

`syscalltests` runs each test in its own child. A failing test exits with status 1 and uses its exit message to say why, so the tests rely on the feature they check. They cover:

- `memsize` growing and shrinking exactly with `sbrk`
- Exit messages that are normal, too long, null, or a bad pointer, and `wait` with a bad message pointer
- `forkn` with `n` out of range or a bad `pids` pointer, leaving no children behind
- `forkn(16)`: distinct PIDs, and each index from 1 to 16 used once
- All or nothing: with only 10 free process slots, `forkn(16)` fails and `forkn(10)` still succeeds, so the partial children were freed
- 20 rounds of `forkn(16)` in a row, which would run out of slots if any leaked
- `waitall` with no children, and with children made by plain `fork`

GitHub Actions builds the kernel and runs `syscalltests` and xv6's own `usertests -q` in QEMU on every push, with [`.github/run-tests.sh`](.github/run-tests.sh).

## Things worth knowing

- **Why `memsize` doesn't drop after `free`.** xv6's `malloc` grows the process with `sbrk`, but `free` only hands memory back to the user-space allocator, never to the kernel. The process stays the size it grew to.
- **Why the message is copied twice.** The child's message lives in its own address space, which is gone by the time the parent asks. So `exit` copies it into the kernel's process table, and `wait` copies it out into the parent with `copyout`.
- **Why `forkn` waits before making children runnable.** If each child ran the moment it was created, a later failure would leave some children already running with no clean way to undo them. Preparing them all first keeps the call all or nothing. The children also get their parent only at the end, under `wait_lock`, so no `wait` can see a child that might still be thrown away.

## Build and run

The repo includes a devcontainer with the RISC-V toolchain and QEMU. Open it in VS Code and choose "Reopen in Container", or install `gcc-riscv64-linux-gnu` and `qemu-system-misc` yourself (on Windows, WSL works). Then:

```bash
make qemu
```

At the xv6 prompt, run `syscalltests`, `helloworld`, `goodbye`, `memsize_test` or `bigarray`. Quit QEMU with `Ctrl-A`, then `X`.

To run the full test suite the way CI does:

```bash
bash .github/run-tests.sh
```

## Files I changed

| File | Change |
| --- | --- |
| `kernel/proc.c` | `forkn`, `waitall`, and exit messages in `exit` / `wait` |
| `kernel/proc.h` | `exit_msg` field in the process struct |
| `kernel/param.h` | `MAXEXITMSG` and `NFORKN` |
| `kernel/sysproc.c` | `sys_memsize`, `sys_forkn`, `sys_waitall`, new `sys_exit` / `sys_wait` arguments |
| `kernel/syscall.c`, `kernel/syscall.h` | Syscall numbers 22 to 24 and their dispatch entries |
| `kernel/defs.h` | Kernel prototypes for the new and changed functions |
| `user/user.h`, `user/usys.pl` | User-space declarations and stubs |
| `user/sh.c` | Prints a finished command's exit message |
| `user/*.c` | The five programs above, and every existing program moved to the new `exit` / `wait` |

The original xv6 README and its credits are in [`README`](README), and the MIT license is in [`LICENSE`](LICENSE).
