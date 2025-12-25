# Hooks

Hooks let you run custom Lua code at specific points during a test run (before/after each test, and at the beginning/end of the run). This is useful for setup/cleanup, diagnostics, custom reporting, or collecting extra artifacts.

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
* `context.log_dir` points to the directory where LTF stores logs/artifacts for the run

```lua
--- @class context_t
--- @field test_run test_run_context_t
--- @field test test_context_t
--- @field log_dir string
```

## Context types (Lua annotations)

```lua
--- @class test_run_context_t
--- @field project_name string
--- @field ltf_version string
--- @field started string
--- @field finished string? -- nil until the run is finished
--- @field os string
--- @field os_version string
--- @field target string?
--- @field tags string[]

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
--- @field children test_keyword_t[]

--- @class test_context_t
--- @field test_file string
--- @field name string
--- @field description string
--- @field started string
--- @field finished string? -- nil until the test is finished
--- @field status "passed"|"failed"|nil -- nil until the test is finished
--- @field tags string[]
--- @field vars table<string, string>
--- @field secrets table<string, string>
--- @field output test_output_t[]
--- @field failure_reasons test_output_t[]
--- @field teardown_output test_output_t[]
--- @field teardown_errors test_output_t[]
--- @field keywords test_keyword_t[]
```

## `context.test`

`context.test` is always a last started test, i.e. it is updated every `test_started` hook and is nil on `test_run_started`

### Keywords

`context.test.keywords` is essentially structured “call stack” / step information for the test: nested keywords with start/finish times and source locations. This is useful for building custom summaries or debugging timelines.

## Example

[See project example](./../examples/hooks-example)
