# Hooks

Hooks let you run custom Lua code at specific points during a test run (before/after each test, and at the beginning/end of the run). This is useful for setup/cleanup, diagnostics, custom reporting, or collecting extra artifacts.

> **Note:** In hooks, only the following functions from the [LTF Core Library](./LTF_LIBS/ltf.md) can be used: sleep, millis, print, log. Using any others may lead to undefined behavior. 

## Registering hooks

Hooks are available under `ltf.hooks`. You can register hooks anywhere at Lua **top level**, but a common convention is to keep them in a dedicated folder `hooks/`.

```lua
local ltf = require("ltf")
local hooks = ltf.hooks

-- Runs once at the start of the whole test run
hooks.test_run_started(function(context)
  -- ...
end)

-- Runs before each test starts
hooks.test_started(function(context)
  -- ...
end)

-- Runs after each test finishes
hooks.test_finished(function(context)
  -- ...
end)

-- Runs once after all tests have finished
hooks.test_run_finished(function(context)
  -- ...
end)
```

### Multiple hooks per event

You can register as many hooks as you want for the same event (`test_started`, `test_finished`, etc.). They execute **in the order they were registered**.

## Hook context

Each hook receives a single argument: `context`, which has type `context_t`.

* `context.test_run` contains information about the overall run (project, target, tags, timestamps, OS, etc.)
* `context.test` contains information about the current test (name, file, vars, secrets, output, status, etc.)
* `context.logs` contains information about the paths of log files and directories

```lua
--- @class context_t
--- @field test_run test_run_context_t
--- @field test test_context_t
--- @field logs context_logs_t
```

## Context types (Lua annotations)

```lua
--- @class test_run_context_t
--- @field project_name string
--- @field ltf_version string
--- @field os string
--- @field os_version string
--- @field target string? nil if project is single-target
--- @field started string
--- @field finished string? nil if the test run hasn't finished yet
--- @field tags [string]
--- @field vars table<ltf_var_name, ltf_var_value>
--- @field secrets table<secret_name, secret_value>

--- @class test_output_t
--- @field file string
--- @field line integer
--- @field date_time string
--- @field level "CRITICAL"|"ERROR"|"WARNING"|"INFO"|"DEBUG"|"TRACE"
--- @field msg string

--- @class test_keyword_t
--- @field name string
--- @field started string
--- @field finished string
--- @field file string
--- @field line integer

--- @class test_context_t
--- @field name string
--- @field description string
--- @field started string
--- @field finished string? nil on `test_started`
--- @field teardown_start string? nil on `test_started`
--- @field teardown_end string? nil on `test_started`
--- @field status "passed"|"failed"|nil nil on `test_started`
--- @field tags [string]
--- @field outputs [test_output_t] populated only on `test_finished`
--- @field failure_reasons [test_output_t] populated only on `test_finished`
--- @field teardown_output [test_output_t] populated only on `test_finished`
--- @field teardown_errors [test_output_t] populated only on `test_finished`
--- @field keywords [test_keyword_t] populated only on `test_finished`

--- @class context_logs_t
--- @field dir string log directory path
--- @field raw_log string raw log file path
--- @field output_log string output log file path
```

> `context.test` is always a last started test, i.e. it is updated every `test_started` hook and is nil on `test_run_started`

> `context.test.keywords` is essentially structured “call stack” / step information for the test: nested keywords with start/finish times and source locations. This is useful for building custom summaries or debugging timelines.

## Example

[See project example](https://github.com/jayadamsmorgan/LTF/tree/master/examples/hooks-example)
