# xv6: new system calls

New system calls for [xv6](https://github.com/mit-pdos/xv6-riscv), MIT's small Unix-like teaching kernel for RISC-V. Written for the Operating Systems course at Ben-Gurion University in 2025 (assignment 1).

The work is in the kernel's process code, `kernel/proc.c` and `kernel/sysproc.c`, plus the syscall plumbing that connects user programs to it.

## What I added

**`memsize()`** returns how many bytes of memory the calling process is using (its `sz`).

**Exit messages.** `exit(status, msg)` now takes a message of up to 32 bytes, stored in the process table entry. `wait(&status, msg)` copies the child's message back into the parent's memory. The xv6 shell prints it, so a program can say something on its way out. Every user program in the tree was updated to the new signatures.

**`forkn(n, pids)`** creates `n` children (1 to 16) in one call:
- The parent gets `0` back, plus every child's PID in `pids`.
- Child `i` gets `i` back (1 to `n`), so each child knows which one it is without any extra coordination.
- It's all or nothing. Every child is prepared first and only marked runnable once all of them exist. If one fails, the ones already created are freed and the call returns `-1`.

**`waitall(&n, statuses)`** blocks until every child has exited, then returns how many there were and all their exit statuses at once.

## The programs

| Program | What it shows |
| --- | --- |
| `helloworld` | The basics: a user program that prints and exits |
| `goodbye` | An exit message: the shell prints `Goodbye World xv6` when it ends |
| `memsize_test` | `memsize` before and after a 20 KB `malloc`, and after `free` |
| `bigarray` | `forkn` + `waitall`: sums 65,536 integers across 6 children |

`bigarray` fills an array with 0 to 65,535 and calls `forkn(6, pids)`. Each child sums its own slice and returns the partial sum as its exit status. The parent collects all six with `waitall` and adds them up. The total should be 2,147,450,880, the sum of 0 to 65,535, which just fits in a 32-bit `int`.

## Things worth knowing

- **Why `memsize` doesn't drop after `free`.** xv6's `malloc` grows the process with `sbrk`, but `free` only hands memory back to the user-space allocator, never to the kernel. The process stays the size it grew to.
- **Why the message is copied twice.** The child's message lives in its own address space, which is gone by the time the parent asks. So `exit` copies it into the kernel's process table, and `wait` copies it out into the parent with `copyout`.
- **Why `forkn` waits before making children runnable.** If each child ran the moment it was created, a later failure would leave some children already running with no clean way to undo them. Preparing them all first keeps the call all or nothing.

## Build and run

The repo includes a devcontainer with the RISC-V toolchain and QEMU. Open it in VS Code and choose "Reopen in Container", or install `gcc-riscv64-linux-gnu` and `qemu-system-misc` yourself. Then:

```bash
make qemu
```

At the xv6 prompt, run `helloworld`, `goodbye`, `memsize_test` or `bigarray`. Quit QEMU with `Ctrl-A`, then `X`.

## Files I changed

| File | Change |
| --- | --- |
| `kernel/proc.c` | `forkn`, `waitall`, and exit messages in `exit` / `wait` |
| `kernel/proc.h` | `exit_msg` field in the process struct |
| `kernel/sysproc.c` | `sys_memsize`, `sys_forkn`, `sys_waitall`, new `sys_exit` / `sys_wait` arguments |
| `kernel/syscall.c`, `kernel/syscall.h` | Syscall numbers 22 to 24 and their dispatch entries |
| `user/user.h`, `user/usys.pl` | User-space declarations and stubs |
| `user/*.c` | The four programs above, and every existing program moved to the new `exit` / `wait` |

The original xv6 README and its credits are in [`README`](README), and the MIT license is in [`LICENSE`](LICENSE).
