# LTF Documentation

This folder contains the primary user-facing documentation for LTF: CLI usage, project setup, logging, tags, scenarios, hooks, and more.

## Core docs

- [Command-Line Interface (CLI)](./CLI.md)  
  How to use the `ltf` executable (`init`, `test`, `target`, `logs`, `eval`, etc.).

- [Project setup](./PROJECT_SETUP.md)  
  Single-target vs multi-target layout, initialization, and how test discovery works.

- [Logging](./LOGGING.md)  
  Log files, log levels, and how to use `ltf logs info`.

- [Tag system](./TAG_SYSTEM.md)  
  Add tags to tests and filter runs with `--tags/-t`.

- [Test description](./TEST_DESCRIPTION.md)  
  Add optional `description` text to tests (single-line or multiline).

- [Test teardown with `ltf.defer`](./TEST_TEARDOWN.md)  
  Guaranteed cleanup, LIFO order, and conditional teardown behavior.

- [Test variables](./TEST_VARIABLES.md)  
  Register variables, override from CLI, validation rules, and examples.

- [Test secrets](./TEST_SECRETS.md)  
  Register secrets, provide values via `.secrets`, multiline values, and validation.

- [Test overriding behavior](./TEST_OVERRIDING.md)  
  How duplicate test names override earlier definitions (especially useful for multi-target).

- [Test scenarios](./TEST_SCENARIOS.md)  
  Describe a run in JSON (vars/tags/log settings/order) and how include/append/remove works.

- [Hooks](./HOOKS.md)  
  Register lifecycle hooks and understand the hook context types.

## Internal libraries (API reference)

The API docs for built-in Lua libraries live under:

- [LTF_LIBS (core library API docs)](./LTF_LIBS/)  
  Overview and per-module documentation for `ltf.*` modules (HTTP, SSH, serial, webdriver, JSON, etc.).
