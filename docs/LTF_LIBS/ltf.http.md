# HTTP Client (`ltf.http`)

`ltf.http` is a low-level HTTP client built as a thin wrapper around **libcurl**. You create a handle, configure it using `setopt()`, perform the transfer, and then clean up the handle.

## Getting started

`http` is exposed as a submodule of `ltf`:

```lua
local ltf = require("ltf")
local http = ltf.http
```

## API reference

The typical request flow is:

1. Create a handle with `http.new()`
2. Configure it with `handle:setopt(...)`
3. Execute with `handle:perform()`
4. Release resources with `handle:cleanup()` (recommended via `ltf.defer`)

---

### Handle management

#### `ltf.http.new()`

Creates and returns a new HTTP handle.

**Returns:**

* (`http_handle`): a new `http_handle` object

---

### The `http_handle` object

#### `handle:setopt(curlopt, value) -> http_handle`

Sets a libcurl option on the handle. This method is chainable.

**Parameters:**

* `curlopt` (`integer`): one of the `http.OPT_*` constants
* `value` (`boolean|integer|string|string[]|function`): option value

**Returns:**

* (`http_handle`): the same handle (for chaining)

#### `handle:perform()`

Performs the transfer (blocking).

#### `handle:cleanup()`

Frees resources associated with the handle. You should call this for every handle you create.

---

### Option constants (`http.OPT_*`)

`ltf.http` exports many `OPT_*` constants that map directly to libcurl `CURLOPT_*` options.

For detailed meaning of each option, refer to the official curl docs for `curl_easy_setopt`.

Below are some commonly used options:

* `OPT_URL` (`string`) — request URL
* `OPT_FOLLOWLOCATION` (`boolean|integer`) — follow redirects
* `OPT_VERBOSE` (`boolean|integer`) — enable verbose curl output
* `OPT_TIMEOUT_MS` (`integer`) — total timeout in milliseconds
* `OPT_POST` (`boolean|integer`) — enable POST
* `OPT_POSTFIELDS` (`string`) — POST body
* `OPT_HTTPHEADER` (`string[]`) — list of headers (`{ "Header: value", ... }`)
* `OPT_CUSTOMREQUEST` (`string`) — custom method (`"PUT"`, `"DELETE"`, ...)
* `OPT_WRITEFUNCTION` (`function`) — body callback
* `OPT_HEADERFUNCTION` (`function`) — header callback
* `OPT_SSL_VERIFYPEER` (`boolean|integer`) — enable/disable cert verification

> Note: some options are defined as “long” by curl and may expect `0/1` instead of Lua booleans. In practice, both are commonly accepted by wrappers—if you hit type issues, pass `0` or `1`.

---

## Examples

### Simple GET request

This example captures the response body into a Lua string.

```lua
local ltf = require("ltf")
local http = ltf.http

ltf.test({
  name = "Simple GET request",
  body = function()
    local parts = {}

    local handle = http.new()
    ltf.defer(handle.cleanup, handle)

    handle:setopt(http.OPT_URL, "https://api.github.com/zen")
    handle:setopt(http.OPT_USERAGENT, "LTF-HTTP-Client/1.0")

    handle:setopt(http.OPT_WRITEFUNCTION, function(data)
      table.insert(parts, data)
      return #data
    end)

    ltf.log_info("Performing GET request...")
    handle:perform()
    ltf.log_info("Request finished.")

    local body = table.concat(parts)
    ltf.print("Response:", body)
  end,
})
```

### POST JSON with headers

This example posts JSON and captures both headers and body.

```lua
local ltf = require("ltf")
local http = ltf.http

ltf.test({
  name = "POST JSON data",
  body = function()
    local response_headers = {}
    local response_body_parts = {}

    local post_data = ltf.json.serialize({
      title = "My LTF Test Post",
      body = "This is a test from the LTF framework.",
      userId = 1,
    })

    local handle = http.new()
    ltf.defer(handle.cleanup, handle)

    handle:setopt(http.OPT_URL, "https://jsonplaceholder.typicode.com/posts")
      :setopt(http.OPT_POST, 1)
      :setopt(http.OPT_POSTFIELDS, post_data)
      :setopt(http.OPT_HTTPHEADER, {
        "Content-Type: application/json; charset=UTF-8",
        "Accept: application/json",
      })
      :setopt(http.OPT_HEADERFUNCTION, function(line)
        table.insert(response_headers, (line:gsub("[\r\n]", "")))
        return true
      end)
      :setopt(http.OPT_WRITEFUNCTION, function(data)
        table.insert(response_body_parts, data)
        return #data
      end)

    ltf.log_info("Performing POST request...")
    handle:perform()
    ltf.log_info("Request finished.")

    local response_body = table.concat(response_body_parts)

    ltf.log_debug("--- Response Headers ---")
    for _, h in ipairs(response_headers) do
      if #h > 0 then
        ltf.log_debug(h)
      end
    end

    ltf.log_debug("--- Response Body ---")
    ltf.print(response_body)

    local decoded = ltf.json.deserialize(response_body)
    if decoded and decoded.id then
      ltf.log_info("Successfully created post with ID:", decoded.id)
    else
      ltf.log_error("Failed to parse response or find ID.")
    end
  end,
})
```

---

### Low-level access

#### `ltf.http.low`

`http.low` exposes the underlying low-level module (`require("ltf-http")`). Most users should use `http.new()` + `handle:setopt()` + `handle:perform()`.

## Type reference (Lua annotations)

```lua
--- @class http_handle
--- @field setopt fun(self: http_handle, curlopt: integer, value: boolean|integer|string|string[]|function): http_handle
--- @field perform fun(self: http_handle)
--- @field cleanup fun(self: http_handle)
```

