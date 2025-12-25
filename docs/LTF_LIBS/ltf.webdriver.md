# Web Browser Automation (`ltf.webdriver`)

`ltf.webdriver` provides a W3C WebDriver client for automating browsers through a running WebDriver server (for example `chromedriver`, `geckodriver`). It helps you:

* spawn a driver process (`webdriver.spawn_webdriver`)
* start a browser session (`webdriver.new_session`)
* drive the browser via a `session` object (navigation, element actions, JS execution, screenshots, etc.)
* close the session (`session:close()`)

> This module is a **client**. You must have a WebDriver server binary installed (and typically available in your PATH), or provide an explicit executable path.

---

## Getting started

`webdriver` is exposed as a submodule of `ltf`:

```lua
local ltf = require("ltf")
local webdriver = ltf.webdriver
````

Typical flow:

1. Spawn a WebDriver server process.
2. Start a session (opens a browser).
3. Interact with pages/elements.
4. Close the session and kill the driver.

---

## Driver management

### `ltf.webdriver.spawn_webdriver(opts)`

Spawns a WebDriver server process listening on the given port.

**Parameters:**

* `opts` (`wd_spawn_opts`):

  * `port` (`integer`, required): port for the driver to listen on
  * `webdriver` (`string|webdriver`, optional): executable path/name. Default: `"chromedriver"`.
  * `extraflags` (`string[]`, optional): additional CLI flags passed to the driver

**Returns:**

* (`proc_handle`): process handle (from `ltf.proc.spawn()`)

**Notes:**

* `spawn_webdriver` always injects the port flag by appending `--port=<port>` to the driver arguments.
  Avoid passing your own `--port=...` in `extraflags` unless you want duplicates.

**Example:**

```lua
local ltf = require("ltf")
local webdriver = ltf.webdriver

local driver = webdriver.spawn_webdriver({
  webdriver = "chromedriver",
  port = 9515,
})

-- Always clean up the driver process
ltf.defer(driver.kill, driver)
```

---

## Session management

### `ltf.webdriver.new_session(opts)`

Connects to the WebDriver server and starts a new browser session (which opens a browser window).

**Parameters:**

* `opts` (`wd_session_opts`):

  * `port` (`integer`, required): WebDriver server port
  * `url` (`string`, optional): server URL. Default: `"http://localhost"`.
  * `request_object` (`table`, optional): full JSON body for the `POST /session` request (advanced)
  * `headless` (`boolean`, optional): create session in headless mode. Default: `false`.
  * `headless_implementation` (`webdriver`, optional): which driver’s default headless caps to apply.
    Default: `"chromedriver"`. Supported defaults: `"chromedriver"`, `"operadriver"`, `"geckodriver"`, `"msedgedriver"`.

**Returns:**

* (`wd_session`): session object (methods below)

**Notes:**

* If `opts.request_object` is provided, it is used as the base JSON request body, then normalized into W3C `capabilities` shape.
* For older drivers, `desiredCapabilities` (if provided in `request_object`) is preserved.
* If `opts.headless = true`, the module injects common headless flags into capabilities for supported implementations:

  * Chrome/Opera: adds `--headless=new` and `--disable-gpu` into `goog:chromeOptions.args`
  * Firefox: adds `-headless` into `moz:firefoxOptions.args`
  * Edge: adds `--headless=new` and `--disable-gpu` into `ms:edgeOptions.args`

### `session:close()`

Ends the session (closes the browser).

**Common pattern:**

```lua
local session = webdriver.new_session({ port = 9515 })
ltf.defer(session.close, session)
```

---

## The `session` object

The `session` returned by `webdriver.new_session()` is your handle to a browser instance.

### Fields

* `base_url` (`string`)
* `port` (`integer`)
* `id` (`string`): session id
* `headless` (`boolean`): true if created with `opts.headless = true`

### Error behavior

Some helpers surface WebDriver errors as Lua errors (notably: `new_session`, `click`, `send_keys`, `resize_window`, `wait_until_visible`).
If you want to handle failures without aborting the test, wrap calls in `pcall`.

---

## Browser navigation (session methods)

### `session:open_url(url)`

Navigates to `url`.

### `session:get_current_url() -> string`

Returns the current page URL.

### `session:get_title() -> string`

Returns the current page title.

### `session:go_back()`

Back in history.

### `session:go_forward()`

Forward in history.

### `session:refresh()`

Reloads the page.

### `session:resize_window(opts?)`

Resizes the browser window.

* If `opts` is `nil`: maximizes the window.
* If provided: `{ width = number, height = number }`

If the driver returns an error response, `resize_window` throws a Lua error.

---

## Finding and interacting with elements

### `session:find_element(opts) -> string`

Finds the first matching element.

**Parameters:**

* `opts` (`wd_session_find_elements_opts`):

  * `using` (`string`): `"css selector"`, `"xpath"`, etc.
  * `value` (`string`): selector value

**Returns:**

* (`string`): `element_id`

**Notes:**

* Element IDs are extracted using the W3C element key
  `"element-6066-11e4-a52e-4f735466cecf"` and fall back to legacy `"ELEMENT"` if needed.

### `session:find_elements(opts) -> string[]`

Finds all matching elements (returns array of `element_id`).

### `session:click(element_id)`

Clicks the element.

Throws a Lua error if the driver returns an error response.

### `session:get_text(element_id) -> string`

Gets visible text for the element.

### `session:send_keys(opts)`

Sends keystrokes to an element.

**Parameters:**

* `opts` (`wd_session_send_keys_opts`):

  * `element_id` (`string`)
  * `text` (`string`)

**Notes:**

* Uses W3C `text` and includes a legacy `value` array of characters for compatibility.

Throws a Lua error if the driver returns an error response.

### `session:input_text(opts)`

Convenience helper: clears the element (by default) and then sends keys.

**Parameters:**

* `opts` (`wd_session_input_text_opts`):

  * `element_id` (`string`)
  * `text` (`string`)
  * `clear_first` (`boolean`, optional, default: `true`)

### `session:wait_until_visible(opts) -> string`

Polls until an element is visible (or times out). Returns the element id.

**Parameters:**

* `opts` (`wd_session_wait_until_visible_opts`):

  * `using` (`string`)
  * `value` (`string`)
  * `timeout` (`integer`, optional, default: `5000`)

**Behavior:**

* Repeatedly calls `find_element` and then polls the element `.../displayed` endpoint until it becomes visible.
* Throws a Lua error on timeout.

### `session:scroll_into_view(element_id)`

Scrolls an element into view via JavaScript `scrollIntoView`.

### `session:drag_and_drop(opts)`

Drag-and-drop using W3C Actions with a single mouse pointer device.

**Parameters:**

* `opts` (`wd_session_drag_and_drop_opts`):

  * `source_id` (`string`): element to grab
  * `target_id` (`string`): element to drop on

---

## Advanced usage

### `session:execute(opts) -> table`

Executes synchronous JS (`POST /execute/sync`).

**Parameters:**

* `opts` (`wd_session_execute_opts`):

  * `script` (`string`): JS source
  * `args` (`table`, optional): array of arguments

**Notes:**

* To pass an element as an argument, provide a W3C element reference object:
  `{ ["element-6066-11e4-a52e-4f735466cecf"] = element_id }`

### `session:screenshot() -> string?`

Returns a full-page screenshot as base64 PNG (driver-dependent).

**Returns:**

* (`string?`): base64 PNG on success, or `nil` if not available.

**Note:**

* Most drivers return a base64 string. If the driver returns an unusual error shape, you may get a non-string `value`.
  Use `pcall` if you want to handle driver quirks gracefully.

### `session:cmd(opts) -> table`

Low-level helper to call arbitrary WebDriver session endpoints.

**Parameters:**

* `opts` (`wd_session_cmd_opts`):

  * `method` (`api_method`): `"GET"|"POST"|"PUT"|"DELETE"` (case-insensitive)
  * `endpoint` (`string`): endpoint suffix after `/session/<id>/` (examples: `"url"`, `"element"`, `"actions"`, `"execute/sync"`)
  * `payload` (`table`, optional): payload (used for POST/PUT; ignored for GET/DELETE)

**Returns:**

* (`table`): decoded JSON response (typically with a `value` field)

**Important:**

* `endpoint` should be a suffix like `"url"` — do **not** include `/session/<id>/` in it.

---

## Full example

```lua
local ltf = require("ltf")
local webdriver = ltf.webdriver

ltf.test({
  name = "Google Search with WebDriver",
  tags = { "e2e", "smoke" },
  body = function()
    -- 1) Spawn driver
    ltf.log_info("Spawning chromedriver on port 9515...")
    local driver = webdriver.spawn_webdriver({
      webdriver = "chromedriver",
      port = 9515,
    })
    ltf.defer(driver.kill, driver)

    -- Give the driver a moment to start
    ltf.sleep(2000)

    -- 2) Start session
    ltf.log_info("Starting new browser session...")
    local session = webdriver.new_session({
      port = 9515,
      headless = false, -- set true for headless mode
    })
    ltf.defer(session.close, session)

    -- 3) Interact
    session:open_url("https://www.google.com")
    ltf.log_info("Opened google.com. Title:", session:get_title())

    local search_box = session:wait_until_visible({
      using = "css selector",
      value = "textarea[name='q']",
      timeout = 5000,
    })

    session:input_text({
      element_id = search_box,
      text = "Test Automation Framework",
      clear_first = true,
    })

    -- Most stable: submit with Enter (instead of chasing dynamic button selectors)
    session:send_keys({
      element_id = search_box,
      text = "\n",
    })

    ltf.sleep(3000)

    local new_title = session:get_title()
    ltf.log_info("Results page title:", new_title)
    if not new_title:find("Test Automation Framework") then
      ltf.log_error("Page title did not update as expected.")
    end
  end,
})
```

---

## Data structures & types

### `wd_spawn_opts` (table)

* `webdriver` (`string|webdriver`, optional): executable path/name. Default: `"chromedriver"`
* `port` (`integer`, required)
* `extraflags` (`string[]`, optional): extra CLI flags passed to the driver (port flag is appended automatically)

### `wd_session_opts` (table)

* `port` (`integer`, required)
* `url` (`string`, optional): default `"http://localhost"`
* `request_object` (`table`, optional): raw `POST /session` request body
* `headless` (`boolean`, optional): default `false`
* `headless_implementation` (`webdriver`, optional): default `"chromedriver"`.
  Supported defaults: `"chromedriver"`, `"operadriver"`, `"geckodriver"`, `"msedgedriver"`.

### `wd_session_cmd_opts` (table)

* `method` (`api_method`): `"GET"|"POST"|"PUT"|"DELETE"` (case-insensitive)
* `endpoint` (`string`): endpoint suffix after `/session/<id>/`
* `payload` (`table`, optional): payload for POST/PUT

### `wd_session_find_elements_opts` (table)

* `using` (`string`): `"css selector"`, `"xpath"`, etc.
* `value` (`string`)

### `wd_session_send_keys_opts` (table)

* `element_id` (`string`)
* `text` (`string`)

### `wd_session_input_text_opts` (table)

* `element_id` (`string`)
* `text` (`string`)
* `clear_first` (`boolean`, optional, default: `true`)

### `wd_session_wait_until_visible_opts` (table)

* `using` (`string`)
* `value` (`string`)
* `timeout` (`integer`, optional, default: `5000`)

### `wd_session_drag_and_drop_opts` (table)

* `source_id` (`string`)
* `target_id` (`string`)

### `wd_session_execute_opts` (table)

* `script` (`string`)
* `args` (`table`, optional)

---

## Low-level notes

* Element IDs are extracted using the W3C element key:
  `"element-6066-11e4-a52e-4f735466cecf"`, with fallback to legacy `"ELEMENT"`.
* Driver errors are surfaced as Lua errors in helpers like `click`, `send_keys`, `resize_window`, `wait_until_visible`, and session creation.
* `session:cmd()` is the escape hatch for anything not wrapped by higher-level helpers.


