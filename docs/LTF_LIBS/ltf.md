# LTF Core Library (`ltf`)

`ltf` is the main Lua entry point for writing tests. It provides:

* Test registration (`ltf.test`)
* Logging helpers (`ltf.log_*`, `ltf.print`)
* Timing and utilities (`ltf.sleep`, `ltf.millis`, `ltf.defer`)
* Run/test context helpers (`ltf.get_active_tags`, `ltf.get_active_test_tags`, `ltf.get_current_target`)
* Variable + secret APIs (`ltf.register_vars`, `ltf.get_var`, `ltf.get_vars`, `ltf.register_secrets`, `ltf.get_secret`, `ltf.get_secrets`)
* Submodules (`ltf.serial`, `ltf.http`, `ltf.ssh`, `ltf.webdriver`, etc.)

## Getting started

In any Lua test file:

```lua
local ltf = require("ltf")
```

## Modules

The `ltf` module exposes several submodules:

* `ltf.serial`
* `ltf.webdriver`
* `ltf.proc`
* `ltf.json`
* `ltf.http`
* `ltf.hooks`
* `ltf.ssh`
* `ltf.util`

Example:

```lua
local ltf = require("ltf")

local serial = ltf.serial
-- serial.open(...), etc.
```

## API reference

### Defining tests

#### `ltf.test(opts)`

Registers a new test.

**Parameters:**

* `opts` (`ltf_test_opts`): test options object:

  * `name` (`string`, required): test name (must be unique within a run)
  * `description` (`string`, optional): human-readable description
  * `tags` (`string[]`, optional): tags for filtering
  * `body` (`function`, required): test function

**Example:**

```lua
local ltf = require("ltf")

ltf.test({
  name = "My first test",
  tags = { "smoke", "api" },
  body = function()
    ltf.log_info("Hello from LTF")
  end,
})
```

---

### Logging

All logs are written to the log files and shown in the TUI.

#### `ltf.log(level, ...)`

Logs a message at the specified level.

**Parameters:**

* `level` (`log_level`): accepted values are:

  * Full names: `"critical"`, `"error"`, `"warning"`, `"info"`, `"debug"`, `"trace"`
  * First-letter shortcuts: `"c"`, `"e"`, `"w"`, `"i"`, `"d"`, `"t"`
  * Case-insensitive (e.g. `"INFO"`, `"C"`)
* `...` (`any`): values to log (like Lua `print()`)

**Behavior:**

* `"critical"`: fails the test immediately.
* `"error"`: marks the test failed but continues execution.
* Other levels only log.

**Example:**

```lua
ltf.test({
  name = "Logging demonstration",
  body = function()
    ltf.print("Same as info")
    ltf.log_debug("Debug details:", 123)

    ltf.log_error("This fails the test, but continues.")
    ltf.log_info("This still runs.")

    -- ltf.log_critical("Stops immediately.")
  end,
})
```

#### Convenience helpers

* `ltf.print(...)` — same as `ltf.log("i", ...)`
* `ltf.log_critical(...)` — same as `ltf.log("c", ...)` (fails immediately)
* `ltf.log_error(...)` — same as `ltf.log("e", ...)` (marks failed, continues)
* `ltf.log_warning(...)` — same as `ltf.log("w", ...)`
* `ltf.log_info(...)` — same as `ltf.log("i", ...)`
* `ltf.log_debug(...)` — same as `ltf.log("d", ...)`
* `ltf.log_trace(...)` — same as `ltf.log("t", ...)`

---

### Test control and utilities

#### `ltf.defer(fn, ...)`

Registers a function to be executed after the test finishes (teardown). This is useful for cleanup.

**Parameters:**

* `fn` (`function`): function to call during teardown
* `...` (`any`): arguments passed to `fn`

**Example:**

```lua
ltf.test({
  name = "Defer demonstration",
  body = function()
    local port = open_very_important_port()
    ltf.defer(close_port, port)

    ltf.defer(function()
      ltf.log_info("Always runs at the end of the test.")
    end)

    ltf.log_info("Test body runs here.")
  end,
})
```

> Note: defers execute after the test finishes. (If you rely on specific ordering like LIFO/FIFO, document the current behavior in your runtime, because it’s not defined by this Lua wrapper file.)

#### `ltf.sleep(ms)`

Sleeps for `ms` milliseconds.

* `ms` (`number`): milliseconds

```lua
ltf.sleep(250)
```

#### `ltf.millis() -> integer`

Returns milliseconds elapsed since the current test started.

```lua
local elapsed = ltf.millis()
ltf.log_info("Elapsed:", elapsed, "ms")
```

---

### Run context helpers

#### `ltf.get_active_tags() -> string[]`

Returns the active tags for the current **test run**.

#### `ltf.get_active_test_tags() -> string[]`

Returns the active tags for the **currently running test**.

#### `ltf.get_current_target() -> string`

Returns the current target name if the project is multi-target, otherwise returns `""`.

---

### Variables

Variables are named values used across tests and can be set via CLI/scenarios.

#### `ltf.register_vars(vars)`

Registers variables at Lua top level.

* `vars` (`table<string, ltf_var_reg_t|string>`)

A variable may be registered as a constant string or as a table with:

* `default` (`string`, optional)
* `values` (`string[]`, optional)

```lua
ltf.register_vars({
  -- constant
  device = "stm32wb55",

  -- required / open value
  serial_port = {},

  -- default
  baud = { default = "115200" },

  -- enum
  env = { values = { "dev", "staging", "prod" } },

  -- enum + default
  log_level = { default = "info", values = { "info", "debug", "trace" } },
})
```

#### `ltf.get_var(name) -> string`

Returns a single variable value.

```lua
local port = ltf.get_var("serial_port")
```

#### `ltf.get_var_number(name) -> number`

Returns a single variable value as a number.

```lua
local port_number = ltf.get_var_number("port_number")
```

#### `ltf.get_vars() -> table<string, string>`

Returns all variables as a map of `name -> value`.

---

### Secrets

Secrets are named sensitive values loaded from `.secrets`.

#### `ltf.register_secrets(names)`

Registers secret names at Lua top level.

* `names` (`string[]`)

```lua
ltf.register_secrets({ "api_token", "password" })
```

#### `ltf.get_secret(name) -> string`

Returns a single secret value.

```lua
local token = ltf.get_secret("api_token")
```

#### `ltf.get_secrets() -> table<string, string>`

Returns all secrets as a map of `name -> value`.
