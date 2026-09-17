# AI Assistance Attribution

## Assignment 2 - File Operations and Cross Compiler

OpenAI Codex/ChatGPT was used as a collaborative programming assistant for
Assignment 2.

### Full chat history

https://chatgpt.com/s/cx_6a973d28490c81918a2b1329cff8fb72

### AI-assisted files

- `finder-app/writer.c`
- `finder-app/Makefile`
- `finder-app/finder-test.sh`
- `AI_ATTRIBUTION.md`
- `AI_HANDOFF.md`
- `AGENTS.md`

### Assistance provided

AI assistance was used to:

- Draft the implementation of `finder-app/writer.c`.
- Implement argument validation, C file I/O, syslog setup, and file-operation
  error handling in `writer.c`.
- Draft `finder-app/Makefile` with native compilation, `CROSS_COMPILE`
  support, compiler warnings, and a `clean` target.
- Identify the required changes to the course-provided
  `finder-app/finder-test.sh`.
- Add native clean/build commands to `finder-test.sh`.
- Replace the invocation of `writer.sh` with the compiled `writer`
  application.
- Explain the commands used to capture compiler version, configuration, and
  sysroot information in `assignments/assignment2/cross-compile.txt`.
- Explain native and AArch64 build verification and the creation of
  `assignments/assignment2/fileresult.txt`.
- Assist with Git remotes, SSH pushing, and moving a self-hosted GitHub Actions
  runner between assignment repositories.
- Interpret the course AI-attribution policy and draft repository guidance for
  future AI-assisted assignments.

### Course starter code

`finder-app/finder-test.sh` was supplied by the course. AI assistance changed
only the native clean/build commands and the command invoking `./writer`.
The remainder of that script is course starter code and is not represented as
AI-generated.

The repository structure, automated tests, GitHub Actions workflow, and other
starter files originated from the course assignment repository.

### Student review and verification

The student:

- Entered and integrated the suggested code.
- Compiled `writer.c` with `-Wall -Wextra -Werror`.
- Tested successful file writing.
- Tested missing arguments.
- Tested failure when the destination directory did not exist.
- Confirmed successful operations and errors were recorded in
  `/var/log/syslog`.
- Built and identified both native x86-64 and ARM AArch64 executables.
- Ran `finder-test.sh` and confirmed it returned `success`.
- Ran `full-test.sh` and confirmed it passed.
- Pushed the repository and confirmed the latest GitHub Actions workflow
  passed.

### External code and sources

No external implementation code and no other student's assignment were used.

Sources consulted for requirements or operational guidance included:

- ECEN 5713 Assignment 1 and Assignment 2 instructions.
- ECEN 5713 Class 1 Fall 2026 lecture slides, particularly slides 7-16
  covering academic integrity and LLM attribution.
- Course-provided AESD starter repository and test scripts.
- Official GitHub documentation for SSH remotes and self-hosted runner
  registration/removal.

### Other student assignments

No other student's assignment was used, supplied to the AI, or referenced when
developing this submission.

## Assignment 4 Part 1 - Threading

OpenAI Codex/ChatGPT was used as a collaborative programming assistant for
Assignment 4 Part 1 on September 15-16, 2026.

### Full chat history

https://chatgpt.com/s/cx_6aab3fc81a988191921d350d1adeeef5

### AI-assisted files

- `examples/threading/threading.c`
- `examples/threading/threading.h`
- `AI_ATTRIBUTION.md`

### Assistance provided

AI assistance was used to:

- Merge the course-provided `assignment4` starter branch without committing
  before the required interactive review.
- Extend `struct thread_data` with the mutex pointer and wait durations needed
  by each worker.
- Implement `start_thread_obtaining_mutex()` with argument validation, dynamic
  allocation, per-thread state initialization, `pthread_create()`, and cleanup
  when thread creation fails.
- Implement the worker's wait, mutex lock, second wait, mutex unlock, completion
  status, and returned result pointer.
- Add millisecond sleeping with retry after signal interruption and ensure an
  acquired mutex is released on later failures.
- Run the course unit tests and a strict warning-enabled compilation check.
- Conduct an interactive review covering thread control flow, allocation
  ownership, mutex ownership, error cleanup, concurrency, mutex lifetime, and
  deadlock prevention.

### Course starter code

The threading skeleton, Assignment 4 CMake and workflow changes, configuration,
test infrastructure, and Unity threading tests originated from the course
starter repository. AI assistance completed the threading TODO sections and
made the assignment-scoped validation and error-handling changes described
above. Untouched starter code is not represented as AI-generated.

### Student review and verification

The student explained in their own words:

- Why freeing the per-thread data after a successful `pthread_create()` would
  be unsafe and why the joiner frees it only after the worker finishes.
- Why a mutex must be released after a later worker failure so other threads
  are not blocked permanently.
- How a mutex serializes two workers and makes the second wait while the first
  owns it.
- Why the caller must release its initial mutex lock before joining a worker,
  otherwise the caller and worker deadlock waiting for each other.

The review also corrected the distinction between the worker and joining
thread, the undefined behavior of unlocking a mutex the worker did not acquire,
and the requirement not to re-lock the shared mutex between worker joins.

During the AI-assisted session, `./unit-test.sh` passed all four Assignment 4
threading tests with zero failures. `threading.c` also compiled successfully
with C11, POSIX interfaces, pthread support, and `-Wall -Wextra -Werror`.

### External code and sources

No external implementation code and no other student's assignment were used.
Requirements and implementation structure came from the Assignment 4 Part 1
instructions and the course-provided threading header and Unity tests.

### Other student assignments

No other student's assignment was used, supplied to the AI, or referenced when
developing this submission.

## Assignment 3 Part 2 - Manual Kernel and Root Filesystem Build

OpenAI Codex/ChatGPT was used as a collaborative programming assistant for
Assignment 3 Part 2 on September 13, 2026.

### Full chat history

https://chatgpt.com/s/cx_6aa729d0dc088191940bb8183a26f240

### AI-assisted files

- `finder-app/manual-linux.sh`
- `finder-app/finder-test.sh`
- `finder-app/finder.sh`
- `AI_ATTRIBUTION.md`

### Assistance provided

AI assistance was used to:

- Merge the course-provided `assignment3-part-2` starter branch without
  committing before the required interactive review.
- Complete `manual-linux.sh` so it creates an absolute output directory,
  downloads and cross-compiles Linux v5.15.163 and BusyBox 1.33.1, stages an
  ARM64 root filesystem, installs the cross-toolchain runtime libraries and
  device nodes, cross-compiles `writer`, copies the assignment files, and
  creates `initramfs.cpio.gz`.
- Add quoting, fail-fast behavior, output-directory creation checks, and an
  explicit refusal to use `/` as the output directory before recursive
  root-filesystem operations.
- Change the target-side `finder.sh` interpreter from `/bin/bash` to the
  BusyBox-provided `/bin/sh`.
- Change `finder-test.sh` to read `conf/assignment.txt` from its target-side
  layout.
- Diagnose and correct the BusyBox executable path used for dependency
  inspection.
- Run syntax, formatting, unit, artifact, initramfs-content, and QEMU boot
  validation.
- Conduct an interactive review covering host versus target execution,
  cross-compilation, kernel and root-filesystem roles, initramfs paths and
  interpreters, device nodes, privileges and path safety, fail-fast error
  handling, staged-file ownership, debugging boundaries, and the local versus
  GitHub Actions validation workflow.

### Course starter code

The outline of `manual-linux.sh`, QEMU scripts, automated tests, and GitHub
Actions full-test job originated from the course starter repository. AI
assistance completed the TODO sections and made the assignment-scoped safety
and compatibility changes described above. Untouched starter code is not
represented as AI-generated.

### Student review and verification

The student explained in their own words:

- Why the kernel build is cached while the user-specific root filesystem is
  reconstructed, including the stale-kernel risk.
- Why host-built x86-64 code cannot execute on the ARM64 target and why shell
  scripts can work on both when a compatible interpreter exists.
- How an incorrect shebang can produce a misleading “not found” failure.
- Why unsafe or empty output paths can redirect privileged recursive commands
  to unintended host locations.
- How fail-fast behavior prevents stale artifacts and cascading errors from
  obscuring the original failure.
- How to localize failures using kernel, initramfs, writer, finder, and QEMU
  log boundaries.
- The complete flow from host-side cross-compilation through ARM64 QEMU boot,
  creation of ten files, and finder result validation.
- Why local QEMU success does not independently prove a clean GitHub Actions
  checkout will pass.

During the session, `./unit-test.sh` passed all three Assignment 3 tests.
The course `24-assignment3` container built Linux, BusyBox, the root filesystem,
and initramfs; QEMU booted the generated ARM64 artifacts and printed both
`success` and `Completed with success!!`. The generated writer was verified as
an ARM64 ELF executable, and the required files and device nodes were verified
inside the initramfs. The student will manually confirm the GitHub Actions
unit-test and full-test jobs after the reviewed commit and tag are pushed.

### External code and sources

No external implementation code and no other student's assignment were used.
Requirements and implementation structure came from the Assignment 3 Part 2
instructions and course-provided starter repository supplied for this work.

### Other student assignments

No other student's assignment was used, supplied to the AI, or referenced when
developing this submission.

## Assignment 3 Part 1 - System Calls

OpenAI Codex/ChatGPT was used as a collaborative programming assistant for
Assignment 3 Part 1 between September 1 and September 13, 2026.

### Full chat history

https://chatgpt.com/s/cx_6aa6e1bd41c0819199d6b38807b3e187

### AI-assisted files

- `examples/systemcalls/systemcalls.c`
- `finder-app/finder-test.sh`
- `AI_HANDOFF.md`
- `AGENTS.md`
- `AI_ATTRIBUTION.md`

### Assistance provided

AI assistance was used to:

- Merge the course-provided `assignment3-part-1` starter branch and initialize
  its test submodules.
- Implement `do_system()` by checking the status returned by `system()`.
- Implement `do_exec()` using `fork()`, `execv()`, and `waitpid()`, including
  child exit-status and interrupted-wait handling.
- Implement `do_exec_redirect()` using `open()` and `dup2()` to redirect child
  standard output, with file-descriptor, fork, exec, and wait error handling.
- Remove the Assignment 2 native build commands from `finder-test.sh`.
- Diagnose and replace a stale generated CMake build directory, then run the
  Assignment 3 unit tests and focused local verification.
- Conduct an interactive review covering process control, wait status,
  descriptor ownership and redirection, argument arrays, and shell-injection
  behavior.
- Add a reusable interactive-review gate to the repository guidance so future
  materially AI-assisted assignment changes are reviewed before commit, tag,
  and push.

### Course starter code

The `examples/systemcalls` skeleton, test infrastructure, and other files
introduced by the `assignment3-part-1` merge originated from the course starter
repository. AI assistance filled the TODO sections in `systemcalls.c` and
removed the previously AI-assisted Assignment 2 build step from
`finder-test.sh`; untouched starter code is not represented as AI-generated.

### Student review and verification

The student reviewed the implementation interactively and demonstrated their
understanding of `fork()`/`execv()` process behavior, encoded child exit status,
interrupted `waitpid()` calls, variadic argument-array termination, `dup2()`
output redirection, file-descriptor ownership after `fork()`, and the difference
between direct `execv()` arguments and shell interpretation.

During the AI-assisted session, `./unit-test.sh` passed all three Assignment 3
tests with zero failures, `finder-app/finder-test.sh` returned `success`, and
`systemcalls.c` compiled with `-std=c99 -Wall -Wextra -Werror`.

### External code and sources

No external implementation code and no other student's assignment were used.
Requirements were taken from the Assignment 3 Part 1 instructions supplied by
the student and the course-provided starter code and tests.

### Other student assignments

No other student's assignment was used, supplied to the AI, or referenced when
developing this submission.
