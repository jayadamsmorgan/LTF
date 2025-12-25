# LTF Command-Line Interface (CLI)

The `ltf` executable is the primary tool for creating, managing, and running your test projects. This guide details all available commands and their options.

## Main Commands

The `ltf` executable uses a command/sub-command structure.

**Usage:**
```bash
ltf <command> [sub-command] [arguments...] [options...]
````

| Command                    | Description                                       |
| :------------------------- | :------------------------------------------------ |
| [`init`](#ltf-init)        | Initialize a new LTF project.                     |
| [`test`](#ltf-test)        | Run tests for a project.                          |
| [`target`](#ltf-target)    | Manage targets for a multi-target project.        |
| [`logs`](#ltf-logs)        | Parse and display information from LTF log files. |
| [`eval`](#ltf-eval)        | Run Lua scripts with support for LTF libraries.   |
| `version`|`--version`|`-v` | Display the installed LTF version.                |
| `help`|`--help`|`-h`       | Display the main help message.                    |

---

## `ltf init`

Creates a new LTF project directory with the necessary files and structure.

**Usage:**

```bash
ltf init <project-name> [options...]
```

### Arguments

* `project-name` (required): The name of the new project. A directory with this name will be created.

### Options

| Option           | Alias | Description                                            |
| :--------------- | :---- | :----------------------------------------------------- |
| `--multitarget`  | `-m`  | Initializes the project with a multi-target structure. |
| `--internal-log` | `-i`  | Dumps an internal LTF log file for advanced debugging. |
| `--help`         | `-h`  | Displays the help message for the `init` command.      |

### Examples

```bash
# Create a standard single-target project
ltf init my_api_tests

# Create a project designed for multiple hardware targets
ltf init my_embedded_project --multitarget
```

---

## `ltf test`

Executes the tests within the current project.

**Usage:**

```bash
# For Single-Target projects
ltf test [options...]

# For Multi-Target projects
ltf test <target_name> [options...]
```

### Arguments

* `target_name` (optional): The name of the target to run tests against. This is **required** for multi-target projects.

### Options

| Option                  | Alias | Description                                                                                                                                               |
| :---------------------- | :---- | :-------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `--log-level <level>`   | `-l`  | Sets the minimum log level to display in the TUI. Valid levels are `critical`, `error`, `warning`, `info`, `debug`, `trace`. See [Logging](./LOGGING.md). |
| `--no-logs`             | `-n`  | Disables the creation of log files for this test run.                                                                                                     |
| `--ltf-lib-path <path>` | `-p`  | Provide custom path to LTF Lua libraries location.                                                                                                        |
| `--tags <tags>`         | `-t`  | Runs only tests that have at least one of the specified comma-separated tags. See [Tag System](./TAG-SYSTEM.md).                                          |
| `--vars <vars>`         | `-v`  | Set one or more test variables as a comma-separated list of `name:value` pairs. See [Test Variables](./TEST_VARIABLES.md).                                |
| `--scenario <file>`     | `-s`  | Run using a scenario JSON file (tags/vars/log settings/ordering). CLI flags still override scenario values. See [Test Scenarios](./TEST_SCENARIOS.md).    |
| `--internal-log`        | `-i`  | Dumps an internal LTF log file for advanced debugging.                                                                                                    |
| `--headless`            | `-e`  | Runs LTF in "headless" mode (no TUI). Performs faster but without fancy TUI.                                                                              |
| `--help`                | `-h`  | Displays the help message for the `test` command.                                                                                                         |

### Examples

```bash
# Run all tests in a single-target project
ltf test

# Run only smoke tests
ltf test --tags smoke

# Run tests for a specific target in a multi-target project with a verbose log level
ltf test my_board_v2 -l debug
```

---

## Test variables (`--vars` / `-v`)

Variables let you parameterize your test runs and override values from the command line.

**Format:** comma-separated `name:value` pairs

```bash
ltf test --vars serial_port:/dev/ttyUSB0,env:staging,log_level:debug
```

Shorthand:

```bash
ltf test -v serial_port:/dev/ttyUSB0,env:staging
```

Notes:

* Values are treated as strings on the CLI side.
* Variable validation happens before any tests run (required vars must be provided; enums must match allowed values, etc.).

See: [Test Variables](./TEST_VARIABLES.md)

---

## Test scenarios (`--scenario` / `-s`)

Scenarios let you describe a test run in a JSON file: variables, tags, log settings, and (optionally) a preferred test execution order.

Run a scenario:

```bash
ltf test -s scenarios/scenario.json
```

### CLI overrides scenario values

Anything under `cmd` in the scenario can still be overridden with CLI flags. **CLI always wins**.

```bash
ltf test -s scenarios/scenario.json -v var1:new_value
```

See: [Test Scenarios](./TEST_SCENARIOS.md)

---

## `ltf target`

Manages the targets in a multi-target project. This command requires a sub-command.

### `ltf target add`

Adds a new target to the project and creates its corresponding test directory.

**Usage:**

```bash
ltf target add <target_name>
```

* **Arguments:** `target_name` (required) - The name of the new target to add.

### `ltf target remove`

Removes a target from the project's configuration.

**Usage:**

```bash
ltf target remove <target_name>
```

* **Arguments:** `target_name` (required) - The name of the target to remove.

> **Note:** The `remove` command does not delete the target's test directory, allowing you to manage the files manually.

---

## `ltf logs`

Provides utilities for interacting with LTF log files. This command requires a sub-command.

### `ltf logs info`

Parses a raw JSON log file and displays a summary of the test run.

**Usage:**

```bash
ltf logs info <path_to_log | latest>
```

### Arguments

* `path_to_log | latest` (required): Either the literal string `latest` to parse the most recent log, or the file path to a specific `test_run_[...]_raw.json` file.

### Example

```bash
# Get a summary of the last test run
ltf logs info latest
```

---

## `ltf eval`

Runs specified lua file with access to LTF libraries. Useful for scripts.

**Usage:**

```bash
ltf eval <file.lua> -- [args...]
```

### Arguments

* `file.lua`: Lua script to run
* `args`: any amount of arguments passed to the Lua in the `args` table

### Example

```lua
--- script.lua
local ltf = require("ltf")

print("Hello, " .. args[1])
print(args[2])
```

```bash
ltf eval script.lua LTF test
```

**Output:**

```
Hello, LTF
test
```
