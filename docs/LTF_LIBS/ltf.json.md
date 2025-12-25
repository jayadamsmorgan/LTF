# JSON Utilities (`ltf.json`)

`ltf.json` provides fast helpers for serializing Lua values to JSON strings and deserializing JSON strings back into Lua tables.

## Getting started

`json` is exposed as a submodule of `ltf`:

```lua
local ltf = require("ltf")
local json = ltf.json
```

## API reference

### `ltf.json.serialize(object, opts)`

Converts a Lua value (typically a table) into a JSON string.

**Parameters:**

* `object` (`any`): Lua value to serialize
* `opts` (`json_serialize_opts`, optional): formatting/behavior options

**Returns:**

* (`string`): JSON string

#### `json_serialize_opts` (table)

| Field              | Type      | Default | Description                                               |
| ------------------ | --------- | ------- | --------------------------------------------------------- |
| `pretty`           | `boolean` | `false` | Pretty-print with indentation (2 spaces).                 |
| `pretty_tab`       | `boolean` | `false` | If `pretty` is true, indent with tabs instead of spaces.  |
| `spaced`           | `boolean` | `false` | Add extra spaces where appropriate (more human-readable). |
| `color`            | `boolean` | `false` | Add terminal escape codes to colorize the JSON output.    |
| `no_trailing_zero` | `boolean` | `false` | Remove trailing zeros from floats (e.g. `1.200` → `1.2`). |
| `slash_escape`     | `boolean` | `false` | Escape `/` characters (e.g. `/` → `\/`).                  |

**Example:**

```lua
local ltf = require("ltf")
local json = ltf.json

ltf.test({
  name = "JSON serialization",
  body = function()
    local my_data = {
      name = "LTF Test",
      id = 123,
      active = true,
      tags = { "core", "api" },
      path = "c:/temp/data",
    }

    local compact = json.serialize(my_data)
    ltf.print("Compact JSON:", compact)

    local pretty = json.serialize(my_data, { pretty = true, slash_escape = true })
    ltf.print("Pretty JSON with escaped slashes:")
    ltf.print(pretty)
  end,
})
```

---

### `ltf.json.deserialize(str)`

Parses a JSON string into a Lua table.

**Parameters:**

* `str` (`string`): JSON string to parse

**Returns:**

* (`table`): parsed Lua object

**Errors:**

* Raises an error if the JSON is invalid.

**Example:**

```lua
local ltf = require("ltf")
local json = ltf.json

ltf.test({
  name = "JSON deserialization",
  body = function()
    local json_string = '{"user":"test_user","permissions":["read","write"],"session_id":98765}'
    local data = json.deserialize(json_string)

    ltf.print("User:", data.user)
    ltf.print("First permission:", data.permissions[1])

    if data.session_id > 90000 then
      ltf.log_info("High session ID detected.")
    end
  end,
})
```

---

### Low-level access

#### `ltf.json.low`

`json.low` exposes the underlying low-level module (`require("ltf-json")`). Most users should use `json.serialize()` / `json.deserialize()`.

## Type reference (Lua annotations)

```lua
--- @class json_serialize_opts
--- @field spaced boolean?
--- @field pretty boolean?
--- @field pretty_tab boolean?
--- @field no_trailing_zero boolean?
--- @field slash_escape boolean?
--- @field color boolean?
```

