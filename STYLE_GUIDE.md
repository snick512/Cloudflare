# Style Guide for Cloudflare DNS Management Tool

This style guide outlines the conventions and best practices to follow when working on the Cloudflare DNS management tool codebase.

---

## General Guidelines

- Use **descriptive and consistent naming** for variables, functions, and constants.
  - Example: `CONFIG_FILE`, `API_KEY`, `load_config()`.
- Ensure **error handling** is implemented for all operations that may fail (e.g., file handling, HTTP requests, memory allocation).
- Keep functions **small and modular**, each performing a single responsibility.
- Add **comments** to describe the purpose of complex logic, particularly in functions.
- Follow consistent **indentation** and spacing (4 spaces for indentation).
- Use **constants or macros** for repeated values like API URLs or file paths.

---

## Naming Conventions

### Variables
- Use `snake_case` for local variables and global variables.
  - Example: `fetch_response`, `zone_id`
- Global variables should be prefixed with a descriptive name (e.g., `API_KEY`).

### Constants
- Use `UPPER_CASE` with underscores for constants and macros.
  - Example: `#define CONFIG_FILE "config.txt"`

### Functions
- Use `snake_case` for function names.
  - Example: `load_config()`, `make_request()`
- Function names should be **verbs** or verb phrases.
  - Example: `list_zones()`, `delete_record()`

---

## Coding Standards

### File Structure

1. **Include Statements**:
   - Standard libraries: Place them at the top of the file.
   - Third-party libraries: Follow standard libraries.

2. **Global Variables and Constants**:
   - Declare global variables and constants after the include statements.

3. **Helper Functions**:
   - Implement helper functions before the `main()` function.

4. **Main Function**:
   - The `main()` function should be concise and primarily handle argument parsing and command delegation.

### Function Implementation

- **Input Validation**:
  - Validate all input parameters before using them.
- **Error Handling**:
  - Handle all potential errors (e.g., failed memory allocations, invalid API responses).
  - Print meaningful error messages to `stderr`.
- **Memory Management**:
  - Free all dynamically allocated memory before returning or exiting.
- **Modularity**:
  - Keep functions focused and avoid deeply nested logic.

### Example Function Template

```c
return_type function_name(parameters) {
    // Step 1: Validate input

    // Step 2: Perform the main operation

    // Step 3: Handle errors and cleanup

    return result;
}
```

---

## API Request Standards

- Use `make_request()` for all HTTP requests.
- Always include required headers such as:
  - `Content-Type: application/json`
  - `Authorization: Bearer <API_KEY>`
- Handle HTTP responses appropriately:
  - Log response errors.
  - Parse JSON responses using `cJSON`.

### Example API Call

```c
char *response = make_request("https://api.cloudflare.com/client/v4/zones", "GET", NULL);
if (response) {
    printf("API Response: %s\n", response);
    free(response);
} else {
    fprintf(stderr, "Error: API call failed.\n");
}
```

---

## JSON Parsing

- Use `cJSON` for JSON parsing.
- Check for `NULL` or invalid types before accessing JSON objects.
- Always clean up JSON objects with `cJSON_Delete()` after use.

### Example JSON Parsing

```c
cJSON *json = cJSON_Parse(response);
if (!json) {
    fprintf(stderr, "Error: Failed to parse JSON response.\n");
    free(response);
    return;
}

cJSON *result = cJSON_GetObjectItem(json, "result");
if (!result || !cJSON_IsArray(result)) {
    fprintf(stderr, "Error: Invalid response structure.\n");
    cJSON_Delete(json);
    free(response);
    return;
}

// Process result...
cJSON_Delete(json);
free(response);
```

---

## Memory Management

- **Reallocate Carefully**:
  - Use `realloc()` to expand memory safely.
  - Always check for `NULL` after memory allocation.
- **Free Dynamically Allocated Memory**:
  - Free memory allocated with `malloc()` or `realloc()`.
- **Avoid Memory Leaks**:
  - Ensure all dynamically allocated memory is freed before returning from functions.

### Example Memory Management

```c
struct memory chunk = {NULL, 0};
char *ptr = realloc(chunk.response, chunk.size + new_size + 1);
if (!ptr) {
    fprintf(stderr, "Error: Out of memory.\n");
    free(chunk.response);
    return;
}
chunk.response = ptr;
```

---

## Command-Line Interface (CLI)

- Provide clear and concise usage instructions.
- Validate the number and format of arguments.
- Map commands to specific functions based on user input.

### Example CLI Argument Validation

```c
if (argc < 2) {
    printf("Usage: ./cloudflare <command> [args]\n");
    return 1;
}

const char *command = argv[1];
if (strcmp(command, "list_zones") == 0) {
    list_zones();
} else {
    printf("Unknown command: %s\n", command);
}
```

---

## Logging and Debugging

- Use `printf()` for general logging and `fprintf(stderr, ...)` for error messages.
- Include enough information in log messages to debug issues (e.g., HTTP responses, function parameters).

---

## Additional Practices

- Avoid hardcoding sensitive information (e.g., API keys).
  - Use a configuration file (`config.txt`).
- Document all new functions with comments explaining their purpose, parameters, and return values.
- Test thoroughly after changes to ensure no regressions.

