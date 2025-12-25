# Hooks (`ltf.hooks`)

`ltf.hooks` lets you run your own Lua callbacks at key points during a test run. Hooks are useful for:

* Setting up / tearing down external resources for the whole run
* Per-test setup/cleanup
* Collecting extra logs or artifacts
* Inspecting the run/test context (tags, status, output, keywords, etc.)

## Getting started

`hooks` is exposed as a submodule of `ltf`:

```lua
local ltf = require("ltf")
local hooks = ltf.hooks
```

Hooks can be registered anywhere at Lua top level, but it’s a good idea to organize them in a dedicated folder (for example `hooks/`).

## API reference

All hook callbacks have the same signature:

* `fn(context)` where `context` is of type `context_t`

You can register as many hooks as you want for each hook type. They will run in the order they were registered.

---

### Hook registration

#### `ltf.hooks.test_run_started(fn)`

Registers a callback that runs once at the start of the whole test run.

**Parameters:**

* `fn` (`hooks_fn`): `function(context) ... end`

**Example:**

```lua
local ltf = require("ltf")
local hooks = ltf.hooks

hooks.test_run_started(function(ctx)
  ltf.log_info("Run started for project:", ctx.test_run.project_name)
  ltf.log_info("Tags:", table.concat(ctx.test_run.tags, ", "))
end)
```

#### `ltf.hooks.test_started(fn)`

Registers a callback that runs before each test starts.

**Parameters:**

* `fn` (`hooks_fn`)

**Example:**

```lua
local ltf = require("ltf")
local hooks = ltf.hooks

hooks.test_started(function(ctx)
  ltf.log_info("Starting test:", ctx.test.name)
end)
```

#### `ltf.hooks.test_finished(fn)`

Registers a callback that runs after each test finishes.

**Parameters:**

* `fn` (`hooks_fn`)

**Example:**

```lua
local ltf = require("ltf")
local hooks = ltf.hooks

hooks.test_finished(function(ctx)
  ltf.log_info("Finished test:", ctx.test.name, "status:", tostring(ctx.test.status))
end)
```

#### `ltf.hooks.test_run_finished(fn)`

Registers a callback that runs once after all tests have finished.

**Parameters:**

* `fn` (`hooks_fn`)

**Example:**

```lua
local ltf = require("ltf")
local hooks = ltf.hooks

hooks.test_run_finished(function(ctx)
  ltf.log_info("Run finished. Logs dir:", ctx.log_dir)
  ltf.log_info("Started:", ctx.test_run.started, "Finished:", tostring(ctx.test_run.finished))
end)
```

---

## Context types

### `context_t`

The object passed to every hook.

* `test_run` (`test_run_context_t`): run-level metadata
* `test` (`test_context_t`): current test metadata (for run hooks, this may reflect the “current”/last test depending on when it’s invoked)
* `log_dir` (`string`): path to the test run log directory

### `test_run_context_t`

Run-level metadata:

* `project_name` (`string`)
* `ltf_version` (`string`)
* `started` (`string`)
* `finished` (`string?`): `nil` until the run is finished
* `os` (`string`)
* `os_version` (`string`)
* `target` (`string?`): set if multi-target run
* `tags` (`string[]`): active run tags

### `test_context_t`

Per-test metadata:

* `test_file` (`string`)
* `name` (`string`)
* `started` (`string`)
* `finished` (`string?`): `nil` until finished
* `status` (`"passed"|"failed"|nil`): `nil` until finished
* `tags` (`string[]`)
* `output` (`test_output_t[]`)
* `failure_reasons` (`test_output_t[]`)
* `teardown_output` (`test_output_t[]`)
* `teardown_errors` (`test_output_t[]`)
* `keywords` (`test_keyword_t[]`)

### `test_output_t`

A single log/output entry:

* `file` (`string`)
* `line` (`integer`)
* `date_time` (`string`)
* `level` (`"CRITICAL"|"ERROR"|"WARNING"|"INFO"|"DEBUG"|"TRACE"`)
* `msg` (`string`)

### `test_keyword_t`

Keyword tree entries (call-stack-like information):

* `name` (`string`)
* `started` (`string`)
* `finished` (`string`)
* `file` (`string`)
* `line` (`integer`)
* `children` (`test_keyword_t[]`)

## Type reference (Lua annotations)

```lua
--- @alias hooks_fn fun(context: context_t)

--- @class context_t
--- @field test_run test_run_context_t
--- @field test test_context_t
--- @field log_dir string
```
