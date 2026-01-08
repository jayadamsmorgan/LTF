# LTF Logging

LTF provides a comprehensive logging system that automatically records the details of every test run. These logs are essential for debugging failures, reviewing test output, and archiving test results.

## Log File Generation

LTF generates log files automatically in the `your_project/logs/` directory after every test run.

*   To disable log file creation for a specific run, use the `--no-logs` or `-n` command-line option:
    ```bash
    ltf test --no-logs
    ```
*   For **multi-target projects**, logs are organized into subdirectories for each target: `your_project/logs/your_target/`.

LTF produces two types of log files for each run.

### 📝 Output Log

This file contains the human-readable console output from the test run, exactly as it appears in the LTF Terminal UI (TUI). It's perfect for a quick review of what happened during the test.

*   **Filename:** `test_run_[DATE]-[TIME]_output.log`
*   **Latest Symlink:** A convenience symlink named `test_run_latest_output.log` is always created, pointing to the log file from the most recent test run.

### Raw Log (JSON)

This file is a machine-readable JSON document containing a complete, structured record of the entire test run. It includes every log message regardless of level, test timings, statuses, and other metadata. It is ideal for programmatic analysis, reporting, or integration with other tools.

*   **Filename:** `test_run_[DATE]-[TIME]_raw.json`
*   **Latest Symlink:** A symlink named `test_run_latest_raw.json` always points to the latest raw log.
*   **Schema:** [The schema for the raw log format can be found here.](../raw_log_json_schema.json)

#### What’s inside `*_raw.json`

A raw log file is a single JSON object with:

* **Run metadata**: project name, LTF version, OS, timestamps, selected target, selected tags, and resolved variables.
* **All discovered/selected tests** (in execution order), each with:
  * timestamps and final status
  * tags
  * complete output captured during the test
  * structured failure reasons
  * teardown output/errors (from `ltf.defer`)
  * keyword/step timeline (nested “call stack”-like structure)

This makes `*_raw.json` a good source of truth for:
* building custom HTML reports
* CI summaries and dashboards
* “what changed?” comparisons between runs
* post-processing output into JUnit, Allure, etc.

#### Top-level fields (overview)

At the top level you’ll usually see:

* `project_name` — project directory name
* `ltf_version` — LTF build version string
* `os`, `os_version` — runtime platform information
* `started`, `finished` — timestamps for the overall run
* `target` — selected target (for multi-target) or project name
* `variables` — resolved variables for this run (after CLI/scenario overrides)
* `tags` — tags that filtered this run (if any)
* `tests` — array of per-test entries
* `total_amount`, `passed_amount`, `failed_amount`, `finished_amount` — summary counters

> **Timestamp format:** LTF timestamps use a compact string form like `MM.DD.YY-HH:MM:SS` (example: `12.25.25-00:29:10`). This is documented and validated in the JSON schema.

#### Per-test entries

Each item in `tests[]` contains:

* `name` — test name (unique identifier)
* `started`, `finished`
* `status` — `"PASSED"` / `"FAILED"`
* `tags` — tags attached to the test
* `output[]` — all log lines produced during the test body
* `failure_reasons[]` — log entries that represent the failure reason(s)
* `teardown_output[]` / `teardown_errors[]` — output/errors produced during deferred teardown
* `keywords[]` — nested keyword timeline (steps)

##### `output[]` / `failure_reasons[]` / `teardown_*[]`

These arrays all share the same entry structure:

* `file` — source file path where the message was emitted
* `line` — line number
* `date_time` — timestamp
* `level` — `"CRITICAL"|"ERROR"|"WARNING"|"INFO"|"DEBUG"|"TRACE"`
* `msg` — message string

Because the raw log captures **all** levels, it’s normal for `*_raw.json` to contain more detail than the TUI/output log at lower verbosity levels.

##### `keywords[]` (step timeline)

`keywords` is a structured timeline that represents nested “steps” inside a test.

Each keyword contains:

* `name`
* `started`, `finished`
* `file`, `line` (source location of the keyword)
* `children[]` (nested keywords)

This is useful for:
* measuring step durations
* building expandable timelines in reports
* debugging “where did the time go?” inside long tests

#### Using the schema

The raw log schema exists for tooling and integration. It helps you:

* validate log files in CI (catch format regressions)
* generate typed models (TypeScript/Python/Go, etc.)
* build parsers without guessing field names

Example validation (using `ajv`):

```bash
ajv validate -s docs/raw_log_json_schema.json -d logs/test_run_latest_raw.json
````

> Note: the schema is intentionally strict (`additionalProperties: false`) so that accidental format changes are detected early.

---

## 📶 Log Levels

LTF supports six hierarchical log levels, allowing you to control the verbosity of the output in the TUI and the [Output Log](#-output-log).

The levels, from most to least severe, are:

* `critical`
* `error`
* `warning`
* `info` (Default)
* `debug`
* `trace`

You can set the minimum log level for a run using the `--log-level` (or `-l`) option. Only messages of the specified level or higher will be displayed.

```bash
# Show only warnings, errors, and critical messages
ltf test --log-level warning

# Or using the short-form
ltf test -l w
```

> **Important:** Changing the log level only affects the TUI and the `output.log` file. The `raw.json` log **always contains all messages from all levels**, making it a complete record for debugging.

By default, LTF runs with the `info` log level. This means `debug` and `trace` messages are hidden from the console and output log unless a more verbose level is explicitly set.

---

## The `ltf logs` Utility

LTF includes a command-line utility for quickly parsing and viewing information from log files.

### `ltf logs info`

This command parses a [Raw Log](#raw-log-json) file and presents a concise summary of the test run, including test counts, pass/fail rates, and duration.

**Usage:**

```bash
ltf logs info <path_to_raw_log.json | latest>
```

**Examples:**

```bash
# Show info from the most recent test run
ltf logs info latest

# Show info from a specific log file
ltf logs info logs/test_run_2023-10-27-143000_raw.json
```
