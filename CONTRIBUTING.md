# Contributing to dupcleaner

Thank you for contributing to dupcleaner! To maintain a stable and readable codebase, we ask all contributors to follow these project conventions.

## 1. Testing is Required
Every new feature or bug fix must include corresponding tests in the `tests/` directory. Your Pull Request will not be accepted without adequate test coverage verifying the core logic.

## 2. Documentation Updates
If your Pull Request introduces a new feature, changes public behavior, or alters the internal structure of the application:
- You must update the `README.md` to reflect any new capabilities or usage instructions.
- You must update `docs/architecture.md` if the dependency graph or core application architecture changes.

## 3. Continuous Integration
All Pull Requests must pass the Continuous Integration (CI) pipeline on GitHub Actions. Ensure your code compiles cleanly and passes all tests (`ctest`) on Ubuntu, macOS, and Windows before merging. The CI status must be **green**.

By following these simple rules, you help us keep dupcleaner fast, safe, and reliable. Happy coding!
