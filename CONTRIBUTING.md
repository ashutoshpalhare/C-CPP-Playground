# Contributing to C-CPP-Playground

Thanks for helping improve this learning collection. Contributions should be small, documented, and verifiable.

## Before you start

1. Read the root [README](./README.md) and the relevant language guide: [C](./C/README.md) or [C++](./CPP/README.md).
2. Check the target project's README and existing issues before making changes.
3. For substantial work, open an issue first so the scope is agreed on.

## Project expectations

- Keep each project inside its existing `C/<Project>` or `CPP/<Project>` folder.
- Prefer standard C (C11) or C++ (C++17) unless a project's README explains otherwise.
- Keep source code readable, focused, and portable where practical.
- Validate user input and handle file, memory, and system-call errors.
- Document platform-specific dependencies and linker flags.
- Do not commit executables, object files, build directories, local logs, or IDE settings.
- Update the website catalogue only when the corresponding repository content supports the displayed status.
- Do not mark a project **Completed** based only on source-file presence.

## Build and verification

Use the compiler command and requirements in the project's README. As a general starting point:

```bash
# C — run from the project directory
gcc -std=c11 -Wall -Wextra -Wpedantic main.c -o app

# C++ — run from the project directory
g++ -std=c++17 -Wall -Wextra -Wpedantic main.cpp -o app
```

These are examples, not universal commands: projects may need extra source files, libraries, or platform-specific flags. Follow project-specific instructions. Do not run network, file-modifying, or system-level examples unless you understand their effects.

Before submitting, check:
- [ ] The project compiles with warnings enabled, or the exact blocker is documented.
- [ ] Typical, invalid, and boundary inputs have been considered.
- [ ] Relevant resource and error paths have been checked.
- [ ] README build, usage, platform, and limitations sections are accurate.
- [ ] Generated binaries and local-only files are not included.
- [ ] Catalogue status matches the evidence.

## Pull requests

- Use a descriptive title and explain why the change is needed.
- List affected projects and files.
- Include compiler/platform details and exact checks performed.
- Clearly label anything not tested; never claim checks that were not run.
- Keep unrelated formatting or refactors out of the change.

By contributing, you agree that your contributions are provided under the repository's MIT License.
