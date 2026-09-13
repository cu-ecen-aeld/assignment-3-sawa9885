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
