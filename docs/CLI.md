# LTF Command-Line Interface (CLI)

The `ltf` executable is the primary tool for creating, managing, and running your test projects. This guide details all available commands and their options.

## Main Commands

The `ltf` executable uses a command/sub-command structure.

**Usage:**
```bash
ltf <command> [sub-command] [arguments...] [options...]
```

| Command | Description |
| :--- | :--- |
| [`init`](#ltf-init) | Initialize a new LTF project. |
| [`test`](#ltf-test) | Run tests for a project. |
| [`target`](#ltf-target) | Manage targets for a multi-target project. |
| [`logs`](#ltf-logs) | Parse and display information from LTF log files. |
| `version` | Display the installed LTF version. |
| `help` | Display the main help message. |

---

### `ltf init`

Creates a new LTF project directory with the necessary files and structure.

**Usage:**
```bash
ltf init <project-name> [options...]
```

#### Arguments
*   `project-name` (required): The name of the new project. A directory with this name will be created.

#### Options
| Option | Alias | Description |
| :--- | :--- | :--- |
| `--multitarget` | `-m` | Initializes the project with a multi-target structure. |
| `--internal-log`| `-i` | Dumps an internal LTF log file for advanced debugging. |
| `--help` | `-h` | Displays the help message for the `init` command. |

#### Examples
```bash
# Create a standard single-target project
ltf init my_api_tests

# Create a project designed for multiple hardware targets
ltf init my_embedded_project --multitarget
```

---

### `ltf test`

Executes the tests within the current project.

**Usage:**
```bash
# For Single-Target projects
ltf test [options...]

# For Multi-Target projects
ltf test <target_name> [options...]
```

#### Arguments
*   `target_name` (optional): The name of the target to run tests against. This is **required** for multi-target projects.

#### Options
| Option | Alias | Description |
| :--- | :--- | :--- |
| `--log-level <level>` | `-l` | Sets the minimum log level to display in the TUI. Valid levels are `critical`, `error`, `warning`, `info`, `debug`, `trace`. See the [Logging](./Logging.md) documentation for details. |
| `--tags <tags>` | `-t` | Runs only the tests that have at least one of the specified comma-separated tags. See the [Tag System](./Tag-system.md) documentation for details. |
| `--no-logs` | `-n` | Disables the creation of log files for this test run. |
| `--internal-log`| `-i` | Dumps an internal LTF log file for advanced debugging. |
| `--help` | `-h` | Displays the help message for the `test` command. |

#### Examples
```bash
# Run all tests in a single-target project
ltf test

# Run only smoke tests
ltf test --tags smoke

# Run tests for a specific target in a multi-target project with a verbose log level
ltf test my_board_v2 -l debug
```

---

### `ltf target`

Manages the targets in a multi-target project. This command requires a sub-command.

#### `ltf target add`

Adds a new target to the project and creates its corresponding test directory.

**Usage:**
```bash
ltf target add <target_name>
```
*   **Arguments:** `target_name` (required) - The name of the new target to add.

#### `ltf target remove`

Removes a target from the project's configuration.

**Usage:**
```bash
ltf target remove <target_name>
```
*   **Arguments:** `target_name` (required) - The name of the target to remove.

> **Note:** The `remove` command does not delete the target's test directory, allowing you to manage the files manually.

---

### `ltf logs`

Provides utilities for interacting with LTF log files. This command requires a sub-command.

#### `ltf logs info`

Parses a raw JSON log file and displays a summary of the test run.

**Usage:**
```bash
ltf logs info <path_to_log | latest>
```

#### Arguments
*   `path_to_log | latest` (required): Either the literal string `latest` to parse the most recent log, or the file path to a specific `test_run_[...]_raw.json` file.

#### Example
```bash
# Get a summary of the last test run
ltf logs info latest
```
