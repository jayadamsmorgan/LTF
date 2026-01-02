<div align="center">

# LTF
**A Modern Testing & Automation Framework in Lua**

</div>

LTF is a framework for writing functional tests and automations (AT, ATDD, RPA, etc.) using the simple and powerful Lua programming language. It is designed for both developer and QA workflows, providing the tools needed for robust testing of APIs, web frontends, and external hardware.

Leveraging the simplicity of Lua for test scripting and the raw performance of a C-based core engine, **LTF is a reliable, modern, and _blazingly fast_ testing framework.**

## Key Features

* **💻 Interactive Terminal UI:** Real-time, organized view of test execution, including progress, timings, and live log output.
* **✍️ Simple & Expressive Syntax:** Define tests with a clear and minimal API (`ltf.test(...)`) that gets out of your way.
* **📚 Batteries-Included Libraries:** Built-in modules for common automation tasks:
  * **WebDriver:** Browser automation for end-to-end testing (`ltf.webdriver`).
  * **HTTP Client:** Low-level HTTP client for API testing (`ltf.http`).
  * **Process Management:** Run and interact with external command-line tools (`ltf.proc`).
  * **Serial Communication:** Test embedded devices and hardware (`ltf.serial`).
  * **SSH + SFTP:** Remote command execution and file transfer (`ltf.ssh`).
  * **JSON Utilities:** Fast serialize/deserialize helpers (`ltf.json`).
* **🏷️ Flexible Tagging System:** Categorize tests with tags and selectively run suites from the command line.
* **🧹 Guaranteed Teardown:** Use `ltf.defer` to reliably clean up resources whether a test passes or fails.
* **🗂️ Detailed Logging:** Human-readable output logs and machine-readable raw JSON logs for every run (ideal for CI, reporting, and tooling).

> [!IMPORTANT]
>
> **Please note that this is `alpha` release**.
>
> Most features are still in **experimental** state, though API and documented behavior will not change.

## Overview

![](overview.gif)

## Quickstart

### 1) Initialize a project

```bash
ltf init my_project
cd my_project
````

### 2) Write your first test

Create `tests/hello_world_test.lua`:

```lua
local ltf = require("ltf")

ltf.test({
  name = "Hello World",
  tags = { "smoke" },
  body = function()
    ltf.log_info("Hello from LTF!")
  end,
})
```

### 3) Run tests

```bash
ltf test
```

Run only tagged tests:

```bash
ltf test --tags smoke
```

> Multi-target projects are supported via `ltf init --multitarget` and `ltf test <target_name>`.

## Documentation

All documentation lives under [`docs/`](./docs).

### Quick Links

* **Getting Started:** [Project Setup & Your First Test](./docs/PROJECT_SETUP.md)
* **CLI Reference:** [Command-Line Usage](./docs/CLI.md)
* **Core Libraries:** [API Reference](./docs/LTF_LIBS/README.md)

### Key Concepts

* [Logging System](./docs/LOGGING.md)
* [Raw Log JSON Schema](./docs/raw_log_json_schema.json)
* [The Tag System](./docs/TAG_SYSTEM.md)
* [Test Variables](./docs/TEST_VARIABLES.md)
* [Test Scenarios](./docs/TEST_SCENARIOS.md)
* [Test Secrets](./docs/TEST_SECRETS.md)
* [Hooks](./docs/HOOKS.md)
* [Test Teardown with `defer`](./docs/TEST_TEARDOWN.md)
* [Test Overriding Behavior](./docs/TEST_OVERRIDING.md)
* [Test Description](./docs/TEST_DESCRIPTION.md)

## Installation

### Homebrew (macOS / Linux)

```bash
brew tap jayadamsmorgan/ltf
brew install ltf
```

### Build from source

See **[Build Instructions](./BUILD.md)**.

## Contributing

Contributions are welcome — bug reports, feature requests, docs improvements, and PRs.

If you’re not sure where to start, open an issue with:

* what you’re trying to do
* your OS + LTF version (`ltf --version`)
* a minimal repro (if it’s a bug)

We’d love to hear from you.
