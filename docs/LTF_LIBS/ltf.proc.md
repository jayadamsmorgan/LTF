# Process Management (`ltf.proc`)

`ltf.proc` provides helpers for running and interacting with external system processes. You can:

* Run a command and wait for it to finish (`proc.run`)
* Spawn a process and interact with it asynchronously (`proc.spawn`)
* Access the low-level backend module via `proc.low`

## Getting started

`proc` is exposed as a submodule of `ltf`:

```lua
local ltf = require("ltf")
local proc = ltf.proc
```

## API reference

### High-level execution

#### `ltf.proc.run(opts, timeout, sleepinterval)`

Runs an external command, waits for it to complete (optionally with a timeout), and returns captured `stdout`, `stderr`, and `exitcode`.

**Parameters:**

* `opts` (`run_opts`): executable + args
* `timeout` (`integer`, optional): timeout in milliseconds. If `nil`, waits indefinitely.
* `sleepinterval` (`integer`, optional): interval (ms) between timeout checks. Default: `20`.

**Returns:**

* `result` (`run_result`): `{ stdout, stderr, exitcode }`

**Errors:**

* Throws a Lua error with message `"timeout"` if the timeout is reached.

**Example:**

```lua
local ltf = require("ltf")
local proc = ltf.proc

ltf.test({
  name = "Run a git command",
  body = function()
    local ok, result_or_err = pcall(function()
      return proc.run({
        exe = "git",
        args = { "--version" },
      }, 2000) -- 2-second timeout
    end)

    if not ok then
      if result_or_err == "timeout" then
        ltf.log_critical("Command timed out")
      else
        ltf.log_critical("Command failed:", result_or_err)
      end
    end

    local result = result_or_err
    if result.exitcode == 0 then
      ltf.log_info("Git command successful!")
      ltf.print("Output:", result.stdout)
    else
      ltf.log_error("Git command failed with code:", result.exitcode)
      ltf.print("Error output:", result.stderr)
    end
  end,
})
```

---

### Asynchronous spawning

#### `ltf.proc.spawn(opts)`

Spawns an external process and returns a `proc_handle` for interacting with it while it is running.

**Parameters:**

* `opts` (`run_opts`): executable + args

**Returns:**

* (`proc_handle`): process handle with `read`, `write`, `wait`, `kill`

**Example:**

```lua
local ltf = require("ltf")
local proc = ltf.proc

ltf.test({
  name = "Interact with a running process",
  body = function()
    local handle = proc.spawn({
      exe = "grep",
      args = { "Hello" },
    })

    -- Ensure cleanup
    ltf.defer(handle.kill, handle)

    handle:write("Line 1\n")
    handle:write("Hello World\n")
    handle:write("Line 3\n")

    -- Close stdin (platform/process dependent; example kept as-is)
    handle:write("")

    while handle:wait() == nil do
      ltf.sleep(50)
    end

    local output = handle:read("stdout")
    ltf.log_info("Grep found:", output)
  end,
})
```

---

### The `proc_handle` object

Returned by `ltf.proc.spawn()`.

#### `handle:read(stream?, want?) -> string`

Reads from stdout/stderr.

**Parameters:**

* `stream` (`proc_output_stream`, optional): `"stdout"` (default) or `"stderr"`
* `want` (`integer`, optional): number of bytes requested (default `4096`)

**Returns:**

* (`string`): bytes read

> Note: `read()` must not be called after `kill()`.

#### `handle:write(buf) -> integer`

Writes to stdin (if the process is still alive).

**Parameters:**

* `buf` (`string`): data to write

**Returns:**

* (`integer`): number of bytes written

#### `handle:wait() -> integer?`

Non-blocking status check.

**Returns:**

* (`integer`): exit code if process has finished
* (`nil`): if still running

#### `handle:kill()`

Sends `SIGINT` to the process if it’s still running.

---

### Low-level access

#### `ltf.proc.low`

`proc.low` exposes the underlying low-level module (`require("ltf-proc")`). Most users should prefer `proc.spawn()` / `proc.run()`.

## Data structures & types

### `run_opts` (table)

* `exe` (`string`): executable path or name (if on `PATH`)
* `args` (`string[]`, optional): command-line arguments

### `run_result` (table)

* `stdout` (`string`): captured stdout
* `stderr` (`string`): captured stderr
* `exitcode` (`integer`): process exit code

### `proc_output_stream` (alias)

* `"stdout"` (default)
* `"stderr"`

