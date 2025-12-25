# File Utilities (`ltf.util`)

`ltf.util` contains small helpers that don’t belong to a specific subsystem. Right now it provides a simple file inspection helper.

## Getting started

`util` is exposed as a submodule of `ltf`:

```lua
local ltf = require("ltf")
local util = ltf.util
```

## API reference

### `ltf.util.file_info(path)`

Returns information about a file system path.

**Parameters:**

* `path` (`string`): file/directory path (relative or absolute)

**Returns:**

* (`file_info?`): `file_info` if the path exists, otherwise `nil`

**Example:**

```lua
local ltf = require("ltf")
local util = ltf.util

ltf.test({
  name = "Inspect a path",
  body = function()
    local info = util.file_info("./build/output.bin")
    if not info then
      ltf.log_error("File does not exist")
      return
    end

    ltf.log_info("Absolute path:", info.absolute_path)
    ltf.log_info("Type:", info.type)
    ltf.log_info("Size:", info.size)

    if info.is_symlink then
      ltf.log_info("Resolved path:", info.resolved_path)
    end
  end,
})
```

---

## Data structures & types

### `file_info` (table)

Returned by `ltf.util.file_info()`.

| Field           | Type             | Description                                                           |
| --------------- | ---------------- | --------------------------------------------------------------------- |
| `absolute_path` | `string`         | Absolute path for the input `path`.                                   |
| `type`          | `file_info_type` | `"file"` or `"directory"`.                                            |
| `size`          | `integer`        | File size in bytes (for directories, this is implementation-defined). |
| `is_symlink`    | `boolean`        | Whether the original path is a symlink.                               |
| `resolved_path` | `string`         | If `is_symlink` is true, the resolved/target path.                    |

### `file_info_type`

Accepted values:

* `"file"`
* `"directory"`

---

### Low-level access

#### `ltf.util.low`

`util.low` exposes the underlying low-level module (`require("ltf-util")`). Most users should use `util.file_info()` directly.
