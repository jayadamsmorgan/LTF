# HTTP Client (`ltf.http`)

The `ltf.http` library provides a low-level, powerful interface for making HTTP(S) and other network requests. It is a thin wrapper around the ubiquitous **libcurl** library, giving you fine-grained control over every aspect of a network transfer.

Due to its low-level nature, familiarity with libcurl concepts is beneficial.

## Getting Started

The `http` library is exposed as a submodule of the main `ltf` object.

```lua
local ltf = require("ltf")
local http = ltf.http -- Access the HTTP module
```

## API Reference

The basic workflow for any request follows the libcurl pattern:
1.  Create a handle with `http.new()`.
2.  Configure the handle with one or more `handle:setopt()` calls.
3.  Execute the request with `handle:perform()`.
4.  Clean up resources with `handle:cleanup()`, ideally using `ltf.defer`.

### Handle Management

#### `ltf.http.new()`

Creates and returns a new HTTP handle. A handle represents a single transfer session and is the starting point for any request.

**Returns:**
*   (`http_handle`): A new `http_handle` object.

---

### The `http_handle` Object

The `http_handle` object is the main interface for building and executing a request.

#### `handle:setopt(curlopt, value)`

Sets an option on the handle. This is the primary method for configuring all aspects of a request, such as the URL, request method, headers, and data callbacks. This method is chainable.

**Parameters:**
*   `curlopt` (`integer`): An option constant from the `ltf.http.OPT_*` list (e.g., `http.OPT_URL`).
*   `value` (`boolean`|`integer`|`string`|`table`|`function`): The value for the option. The required type depends on the specific option being set.

**Returns:**
*   (`http_handle`): The handle itself, allowing for chained calls.

#### `handle:perform()`

Executes the transfer as configured by all previous `setopt()` calls. This is a synchronous, blocking operation that completes when the transfer is finished or has failed.

#### `handle:cleanup()`

Releases all resources used by the handle. It is critical to call this for every handle you create to prevent memory leaks. The garbage collector will also attempt to call this, but using `ltf.defer` is the recommended and safest approach.

---

### Option Constants (`http.OPT_*`)

The `ltf.http` module exposes a large number of constants that map directly to libcurl's `CURLOPT_` options. These are used with `handle:setopt()` to configure the request.

For a complete list and detailed explanation of what each option does, you must refer to the official **[`curl_easy_setopt` documentation](https://curl.se/libcurl/c/curl_easy_setopt.html)**.

Below are some of the most common options to get you started.

| Constant | Value Type | Description |
| :--- | :--- | :--- |
| `OPT_URL` | `string` | The URL for the request. |
| `OPT_FOLLOWLOCATION` | `boolean` | Set to `true` to follow HTTP redirects. |
| `OPT_VERBOSE` | `boolean` | Set to `true` for detailed operational logs from curl. |
| `OPT_TIMEOUT_MS` | `integer` | The maximum time in milliseconds for the entire operation to take. |
| `OPT_POST` | `boolean` | Set to `true` to make a POST request. |
| `OPT_POSTFIELDS` | `string` | The data to send in the body of a POST request. |
| `OPT_HTTPHEADER` | `table` of `string` | A list of custom HTTP headers (e.g., `{"Content-Type: application/json"}`). |
| `OPT_CUSTOMREQUEST`| `string` | Sets a custom request method (e.g., `"PUT"`, `"DELETE"`). |
| `OPT_WRITEFUNCTION`| `function` | A callback function `function(data)` that receives the response body, chunk by chunk. |
| `OPT_HEADERFUNCTION`| `function` | A callback function `function(data)` that receives the response headers, line by line. |
| `OPT_SSL_VERIFYPEER`| `boolean` | Set to `false` to disable SSL certificate verification (use with caution). |

### Full Examples

#### Simple GET Request

This example performs a GET request and captures the response body into a variable.

```lua
ltf.test("Simple GET request", function()
    local response_body_parts = {}
    local handle = http.new()
    
    -- ALWAYS defer cleanup to prevent resource leaks
    ltf.defer(handle.cleanup, handle)

    handle:setopt(http.OPT_URL, "https://api.github.com/zen")
    handle:setopt(http.OPT_USERAGENT, "LTF-HTTP-Client/1.0")
    
    -- Set a function to capture the response body
    handle:setopt(http.OPT_WRITEFUNCTION, function(data)
        table.insert(response_body_parts, data)
        return true -- Must return true to continue receiving data
    end)
    
    ltf.log_info("Performing GET request...")
    handle:perform()
    ltf.log_info("Request finished.")
    
    local response_body = table.concat(response_body_parts)
    ltf.print("Response:", response_body)
end)
```

#### POST Request with JSON and Headers

This example shows how to send a POST request with a JSON body and custom headers, while capturing both the response headers and body.

```lua
ltf.test("POST JSON data", function()
    local response_headers = {}
    local response_body = ""
    
    local post_data = ltf.json.encode({
        title = "My LTF Test Post",
        body = "This is a test from the LTF framework.",
        userId = 1,
    })
    
    local handle = http.new()
    ltf.defer(handle.cleanup, handle)
    
    -- Chain setopt calls for cleaner code
    handle:setopt(http.OPT_URL, "https://jsonplaceholder.typicode.com/posts")
        :setopt(http.OPT_POST, true)
        :setopt(http.OPT_POSTFIELDS, post_data)
        :setopt(http.OPT_HTTPHEADER, {
            "Content-Type: application/json; charset=UTF-8",
            "Accept: application/json"
        })
        :setopt(http.OPT_HEADERFUNCTION, function(header)
            table.insert(response_headers, header:gsub("[\r\n]", "")) -- Clean up newlines
            return true
        end)
        :setopt(http.OPT_WRITEFUNCTION, function(data)
            response_body = response_body .. data
            return true
        end)
    
    ltf.log_info("Performing POST request...")
    handle:perform()
    ltf.log_info("Request finished.")
    
    ltf.log_debug("--- Response Headers ---")
    for _, h in ipairs(response_headers) do
        if #h > 0 then ltf.log_debug(h) end
    end
    ltf.log_debug("--- Response Body ---")
    ltf.print(response_body)

    local decoded_response = ltf.json.decode(response_body)
    if decoded_response and decoded_response.id then
        ltf.log_info("Successfully created post with ID:", decoded_response.id)
    else
        ltf.log_error("Failed to parse response or find ID.")
    end
end)
```
