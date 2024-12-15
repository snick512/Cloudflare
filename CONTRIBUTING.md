# Contributing to Cloudflare DNS Management Tool

Thank you for your interest in contributing to the Cloudflare DNS Management Tool! This guide outlines the process for contributing and best practices to ensure a smooth collaboration.

---

## Getting Started

### Prerequisites

1. **Development Environment**:
   - Install a C compiler (e.g., GCC or Clang).
   - Install `libcurl` and `cJSON` libraries.
   - Ensure `make` or a similar build system is available.

2. **Configuration File**:
   - Create a `config.txt` file with the following format:
     ```
     API_KEY=<your_api_key>
     EMAIL=<your_email>
     ```

3. **Testing Environment**:
   - Set up a Cloudflare account with appropriate access.
   - Have access to a valid Zone ID for testing DNS record operations.

---

## How to Contribute

### Reporting Bugs

1. Search the issue tracker to ensure the bug hasn’t already been reported.
2. If not found, create a new issue with:
   - A clear and descriptive title.
   - Steps to reproduce the issue.
   - The expected outcome vs. actual behavior.
   - Environment details (OS, compiler, library versions).

### Suggesting Features

1. Check the issue tracker to ensure the feature hasn’t already been requested.
2. Create a new feature request with:
   - A clear and descriptive title.
   - A summary of the feature and its purpose.
   - Any potential implementation ideas.

### Submitting Changes

#### 1. Fork the Repository

Fork the repository to your own GitHub account and clone it to your local machine:
```bash
$ git clone https://github.com/your-username/cloudflare-dns-management-tool.git
$ cd cloudflare-dns-management-tool
```

#### 2. Create a Branch

Create a branch for your feature or bugfix:
```bash
$ git checkout -b feature/my-new-feature
```

#### 3. Make Changes

- Follow the [Style Guide](style_guide.md) for consistency.
- Add or update comments to describe your code.
- Ensure memory management is handled correctly.
- Write meaningful commit messages.

#### 4. Test Your Changes

- Test thoroughly to ensure functionality and backward compatibility.
- Use real or mock API calls to verify correctness.

#### 5. Submit a Pull Request

Push your changes to your forked repository:
```bash
$ git push origin feature/my-new-feature
```

Create a pull request from your branch to the `main` branch of the upstream repository.

---

## Code of Conduct

By contributing, you agree to uphold our [Code of Conduct](CODE_OF_CONDUCT.md).

---

## Contribution Standards

### Code Standards

- Follow the conventions outlined in the [Style Guide](STYLE_GUIDE.md).
- Ensure all new code is modular and thoroughly documented.
- Avoid hardcoding sensitive information. Use configuration files for API keys and emails.

### Commit Messages

- Use clear and descriptive commit messages.
- Example:
  ```
  Fix: Correct memory leak in make_request()
  Feature: Add CLI support for purging cache
  ```

### Testing Standards

- Test thoroughly for edge cases and errors.
- Validate input and ensure robust error handling.
- Ensure no memory leaks or dangling pointers.

---

## Development Workflow

### Typical Workflow for Features

1. Identify a task or issue.
2. Create a branch and implement the change.
3. Test locally.
4. Submit a pull request.
5. Address feedback from maintainers.

### Typical Workflow for Bug Fixes

1. Reproduce the bug.
2. Identify the root cause.
3. Implement a fix following best practices.
4. Add a test case (if applicable).
5. Submit a pull request.

---

## Testing Locally

### Build the Tool

Use `make` or compile manually:
```bash
$ gcc -o cloudflare base.c -lcurl -lcjson
```

### Run Commands

Examples:
```bash
$ ./cloudflare list_zones
$ ./cloudflare add_update_record <zone_id> A example.com 192.0.2.1 3600 1
$ ./cloudflare delete_record <zone_id> <record_id>
$ ./cloudflare purge_cache <zone_id>
```

### Memory Checks

Use tools like `valgrind` to ensure no memory leaks:
```bash
$ valgrind ./cloudflare list_zones
```

---

## Contact

If you have any questions or need further clarification, please reach out by creating an issue or contacting the maintainers directly.

Thank you for contributing!

