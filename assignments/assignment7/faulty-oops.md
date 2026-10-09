# Assignment 7 Faulty Driver Kernel Oops

## Trigger

The Buildroot AArch64 image automatically loaded the `faulty` module and
created `/dev/faulty`. Writing to that character device triggered the fault:

```sh
echo "hello_world" > /dev/faulty
```

## Important oops output

```text
Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000
ESR = 0x0000000096000045
EC = 0x25: DABT (current EL), IL = 32 bits
FSC = 0x05: level 1 translation fault
WnR = 1
Internal error: Oops: 0000000096000045 [#1] SMP
Modules linked in: hello(O) faulty(O) scull(O)
CPU: 0 PID: 155 Comm: sh Tainted: G           O       6.1.44 #1
pc : faulty_write+0x10/0x20 [faulty]
lr : vfs_write+0xc8/0x390
x1 : 0000000000000000
Call trace:
 faulty_write+0x10/0x20 [faulty]
 ksys_write+0x74/0x110
 __arm64_sys_write+0x1c/0x30
 invoke_syscall+0x54/0x130
 el0_svc_common.constprop.0+0x44/0xf0
 do_el0_svc+0x2c/0xc0
 el0_svc+0x2c/0x90
 el0t_64_sync_handler+0xf4/0x120
 el0t_64_sync+0x18c/0x190
```

## Analysis

The shell's `write()` system call entered the kernel through the ARM64
exception path. The virtual filesystem dispatched the request through
`vfs_write()` and `ksys_write()` to the character driver's registered write
handler, `faulty_write()`.

The program counter identifies the failing function and offset as
`faulty_write+0x10/0x20`. The exception class (`DABT`) identifies a data abort,
the fault address is zero, and `WnR = 1` says the failed memory access was a
write. These details agree with the driver's deliberate null-pointer store:

```c
*(int *)0 = 0;
```

That statement is at line 53 of `misc-modules/faulty.c` in the Assignment 7
source. A debug-enabled kernel/module build could map the symbol-plus-offset
directly to a source line with the kernel's `scripts/faddr2line` tool. This
build did not enable `CONFIG_DEBUG_INFO`, so the symbolized program counter,
call trace, and source inspection provide the mapping instead.

The `O` taint flag records that out-of-tree modules were loaded. It does not
identify the root cause, but it is important context when reporting a crash.
The oops terminated the writing shell and returned to a login prompt; the
kernel continued running. Continuing after an oops is unsafe in production,
because kernel state may be inconsistent or corrupted even when the system
appears responsive.
