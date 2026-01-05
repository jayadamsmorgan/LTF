# Test variables

LTF supports **test variables**: named values you register in Lua and optionally override from the CLI. Tests read them at runtime with `ltf.get_var()`.

## Registering variables

Call `ltf.register_vars()` at Lua **top level** (not inside a test body). You can place it anywhere, but it’s usually best to keep variables in one file (for example `lib/variables.lua`).

```lua
local ltf = require("ltf")

ltf.register_vars({
  project = "my-project",
})
```
> You can call `ltf.register_vars` as much as you need
> and do it across different files as long as the variables you are
> registering are unique.

## Using variables in a test

```lua
local ltf = require("ltf")

ltf.test({
  name = "Example Test",
  body = function()
    local project = ltf.get_var("project")
    ltf.log_info("Project is: " .. project)
  end,
})
```

## Variable forms

LTF variables can be registered in one of these **five forms**:

### 1) Constant

**Fixed value.** Cannot be overridden.

```lua
ltf.register_vars({
  device_type = "stm32",
})
```

Use when the test suite is tied to a specific value.

---

### 2) Required

**Must be provided** (typically via CLI). No default.

```lua
ltf.register_vars({
  serial_port = {},
})
```

Use when the value depends on the environment (like `/dev/ttyUSB0`).

---

### 3) Default

**Optional.** Uses `default` unless overridden.

```lua
ltf.register_vars({
  baudrate = { default = "115200" },
})
```

Use when most runs share a common value, but you occasionally change it.

---

### 4) Enum

**Restricted set.** Value must be one of `values`.

```lua
ltf.register_vars({
  log_level = {
    values = { "critical", "debug", "info", "warning", "error" },
  },
})
```

Use when you only want to allow known modes.

---

### 5) Enum with default

**Restricted set + default.** Default must be in `values`.

```lua
ltf.register_vars({
  env = {
    default = "dev",
    values = { "dev", "staging", "prod" },
  },
})
```

Use when there’s a standard choice, but you still want strict allowed values.

---

## Setting variables

Pass variables as `name=value` pairs, separated by commas:

```bash
ltf test --vars serial_port=/dev/ttyUSB0,env=staging,log_level=debug
```

Shorthand:

```bash
ltf test -v serial_port=/dev/ttyUSB0,env=staging
```

You can also set variables by multiple `-v/--vars`:

```bash
ltf test -v serial_port=/dev/ttyUSB0 -v env=staging
```

If you need to set variable which contains comma(s), just wrap it in quotes:

```bash
ltf test -v enumeration="one,two,three",serial_port=/dev/ttyUSB0
```


Alternatively, variables can also be set via [Test Scenarios](./TEST_SCENARIOS.md).

## Validation rules

LTF validates variables **before any tests run**:

* **Constant** variables cannot be overridden from the CLI.
* For **Enum** and **Enum with default**:
  * CLI values must be listed in `values`
  * `default` (if present) must also be listed in `values`
* If specified variable was not registered LTF will print **warning** message but continue executing tests.

If validation fails, LTF exits with an error before executing tests.

## Example

[See project example](./../examples/vars-example).
