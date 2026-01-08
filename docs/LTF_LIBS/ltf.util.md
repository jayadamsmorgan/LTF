````md
# File Utilities (`ltf.util`)

`ltf.util` contains small helpers that don’t belong to a specific subsystem. Right now it provides:

* `file_info(path)` — inspect a filesystem path
* `resolve_symlink(path)` — resolve a symlink target (with clear error/`nil` behavior)

## Getting started

`util` is exposed as a submodule of `ltf`:

```lua
local ltf = require("ltf")
local util = ltf.util
````

## API reference

### `ltf.util.file_info(path) -> file_info?`

Returns information about a filesystem path.

**Parameters:**

* `path` (`string`): file/directory/symlink path (relative or absolute)

**Returns:**

* `file_info` if the path exists
* `nil` if the path does not exist

**Example:**

```lua
local ltf = require("ltf")
local util = ltf.util

ltf.test({
  name = "Inspect a path",
  body = function()
    local info = util.file_info("./build/output.bin")
    if not info then
      ltf.log_error("Path does not exist")
      return
    end

    ltf.log_info("Path:", info.path)
    ltf.log_info("Type:", info.type)
    ltf.log_info("Size:", info.size)
    ltf.log_info("Permissions:", info.permissions)

    if info.type == "symlink" then
      local resolved = util.resolve_symlink(info.path)
      if resolved then
        ltf.log_info("Resolved to:", resolved)
      else
        ltf.log_warning("Symlink is dangling (target does not exist)")
      end
    end
  end,
})
```

---

### `ltf.util.resolve_symlink(path) -> string?`

Resolves a symlink target.

**Behavior:**

* **Throws an error** if the symlink path does not exist (or is not a symlink, depending on implementation).
* **Returns `nil`** if the symlink exists but is **dangling** (target does not exist).
* **Returns `string`** (resolved path) if the link resolves to an existing target.

**Parameters:**

* `path` (`string`): path to the symlink (relative or absolute)

**Returns:**

* `string?` resolved path, or `nil` if dangling

**Example:**

```lua
local ltf = require("ltf")
local util = ltf.util

ltf.test({
  name = "Resolve symlink",
  body = function()
    local resolved = util.resolve_symlink("./latest.log")
    if resolved == nil then
      ltf.log_warning("Symlink exists, but target is missing (dangling).")
      return
    end

    ltf.log_info("Symlink points to:", resolved)
  end,
})
```

---

## Data structures & types

### `file_info` (table)

Returned by `ltf.util.file_info()`.

| Field         | Type             | Description                                                                                                 |
| ------------- | ---------------- | ----------------------------------------------------------------------------------------------------------- |
| `type`        | `file_info_type` | The type of the path.                                                                                       |
| `path`        | `string`         | **Absolute** path to the file/directory/symlink.                                                            |
| `size`        | `integer`        | Size in bytes (for directories this may be implementation-defined). For symlinks, this is the symlink size. |
| `permissions` | `integer`        | File permissions (platform-dependent numeric mode).                                                         |

### `file_info_type`

Accepted values:

* `"file"`
* `"directory"`
* `"symlink"`

---

### Low-level access

#### `ltf.util.low`

If exposed by your build, `ltf.util.low` provides the underlying low-level module (`require("ltf-util")`). Most users should use the high-level helpers above.

```
```

