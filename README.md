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

All documentation lives [here](https://jayadamsmorgan.github.io/LTF/).

### Quick Links

* **Getting Started:** [Project Setup & Your First Test](https://jayadamsmorgan.github.io/LTF/PROJECT_SETUP/)
* **CLI Reference:** [Command-Line Usage](https://jayadamsmorgan.github.io/LTF/CLI/)
* **Core Libraries:** [API Reference](https://jayadamsmorgan.github.io/LTF/LTF_LIBS/)

### Key Concepts

* [Logging System](https://jayadamsmorgan.github.io/LTF/TESTS/LOGGING/)
* [Raw Log JSON Schema](https://jayadamsmorgan.github.io/LTF/raw_log_json_schema.json)
* [The Tag System](https://jayadamsmorgan.github.io/LTF/TESTS/TAG_SYSTEM/)
* [Test Variables](https://jayadamsmorgan.github.io/LTF/TESTS/TEST_VARIABLES/)
* [Test Scenarios](https://jayadamsmorgan.github.io/LTF/TESTS/TEST_SCENARIOS/)
* [Test Secrets](https://jayadamsmorgan.github.io/LTF/TESTS/TEST_SECRETS/)
* [Hooks](https://jayadamsmorgan.github.io/LTF/HOOKS/HOOKS/)
* [Test Teardown with `defer`](https://jayadamsmorgan.github.io/LTF/TESTS/TEST_TEARDOWN/)
* [Test Overriding Behavior](https://jayadamsmorgan.github.io/LTF/TESTS/TEST_OVERRIDING/)
* [Test Description](https://jayadamsmorgan.github.io/LTF/TESTS/TEST_DESCRIPTION/)

## Installation

### Build from source

See **[Build Instructions](./BUILD.md)**.

## Contributing

Contributions are welcome — bug reports, feature requests, docs improvements, and PRs.

If you’re not sure where to start, open an issue with:

* what you’re trying to do
* your OS + LTF version (`ltf --version`)
* a minimal repro (if it’s a bug)

We’d love to hear from you.
