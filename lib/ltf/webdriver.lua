local tm = require("ltf-main")
local proc = require("ltf.proc")
local json = require("ltf.json")
local http = require("ltf.http")

local M = {}

-- Internal W3C constant for element reference key
M.ELEM_KEY = "element-6066-11e4-a52e-4f735466cecf"

--- @alias webdriver
--- | '"chromedriver"'
--- | '"geckodriver"'
--- | '"safaridriver"'
--- | '"msedgedriver"'
--- | '"iedriver"'
--- | '"operadriver"'
--- | '"webkitdriver"'
--- | '"wpedriver"'

--- @alias api_method
--- | '"PUT"'
--- | '"POST"'
--- | '"DELETE"'
--- | '"GET"'

--- @class wd_session_opts
--- @field port integer
--- @field url string? webdriver url (optional, defaults to `http://localhost`)
--- @field request_object table? optionally provide full body for the webdriver session request
--- @field headless boolean? create webdriver session in headless state. Defaults to 'false'
--- @field headless_implementation webdriver? specify implementation for creating headless session if supported. Defaults to "chromedriver"|"operadriver"

--- @class wd_session_cmd_opts
--- @field method api_method
--- @field endpoint string
--- @field payload table? payload if method is "post" or "put"

--- @class wd_session_wait_until_visible_opts
--- @field using string selector strategy (css selector, xpath...)
--- @field value string selector
--- @field timeout integer? milliseconds (default 5000)

--- @class wd_session_input_text_opts
--- @field element_id string
--- @field text string
--- @field clear_first boolean? (default: true)

--- @class wd_session_drag_and_drop_opts
--- @field source_id string element to grab
--- @field target_id string element to drop on

--- @class wd_session_execute_opts
--- @field script string JS source
--- @field args table? array of arguments

--- @class wd_session_send_keys_opts
--- @field element_id string
--- @field text string

--- @class wd_session_resize_window_opts
--- @field width number
--- @field height number

--- @class wd_session_find_elements_opts
--- @field using string e.g. "css selector", "xpath"
--- @field value string

--- @class wd_session
--- @field base_url string
--- @field port integer
--- @field id string webdriver session id
--- @field headless boolean true if the session was created in headless mode with opts.headless = true
---
--- Click on an element
--- @field click fun(self: wd_session, element_id: string): raw_result: table
---
--- Drag-and-drop. Uses W3C Actions (§17) with a single mouse pointer device.
--- @field drag_and_drop fun(self: wd_session, opts: wd_session_drag_and_drop_opts): raw_result: table
---
--- Execute synchronous JavaScript in the page
--- @field execute fun(self: wd_session, opts: wd_session_execute_opts): raw_result: table
---
--- Find the first element matchinhg a selector
--- @field find_element fun(self: wd_session, opts: wd_session_find_elements_opts): element_id: string
---
--- Find *all* elements matching a selector
--- @field find_elements fun(self: wd_session, opts: wd_session_find_elements_opts): element_ids: string[]
---
--- Get the current URL
--- @field get_current_url fun(self: wd_session): url: string
---
--- Retrieve the visible text of an element
--- @field get_text fun(self: wd_session, element_id: string): text: string
---
--- Get the page title
--- @field get_title fun(self: wd_session): title: string
---
--- Go back in history
--- @field go_back fun(self: wd_session): raw_result: table
---
--- Go forward in history
--- @field go_forward fun(self: wd_session): raw_result: table
---
--- Input text
--- @field input_text fun(self: wd_session, opts: wd_session_input_text_opts): raw_result: table
---
--- Open URL
--- @field open_url fun(self: wd_session, url: string): raw_result: table
---
--- Reload the page
--- @field refresh fun(self: wd_session): raw_result: table
---
--- Resize browser window. If opts is nil, maximizes the window
--- @field resize_window fun(self: wd_session, opts: wd_session_resize_window_opts?): raw_result: table
---
--- Send keystrokes to an element
--- @field send_keys fun(self: wd_session, opts: wd_session_send_keys_opts): raw_result: table
---
--- Take a full-page screenshot. Returns PNG in base64 format.
--- @field screenshot fun(self: wd_session): base64_png: string?
---
--- Scroll element into view
--- @field scroll_into_view fun(self: wd_session, element_id: string): raw_result: table
---
--- Wait until element *visible*. Polls `/displayed` endpoint until it returns true or timeout
--- @field wait_until_visible fun(self: wd_session, opts: wd_session_wait_until_visible_opts): element_id: string
---
---
--- Low level helper
--- @field cmd fun(self: wd_session, opts: wd_session_cmd_opts): raw_result: table
---
---
--- Close the session
--- @field close fun(self: wd_session)

--- @class wd_spawn_opts
--- @field webdriver string|webdriver path or name of the webdriver executable. default: "chromedriver"
--- @field port integer port to open webdriver with
--- @field extraflags [string]? optional array of additional arguments passed to webdriver executable

--- Spawn webdriver instance
---
--- @param opts wd_spawn_opts
---
--- @return proc_handle process handle for the spawned webdriver
M.spawn_webdriver = function(opts)
	local args = opts.extraflags or {}
	local webdriver = opts.webdriver or "chromedriver"
	table.insert(args, "--port=" .. assert(opts.port, "opts.port is required"))
	local handle = proc.spawn({
		exe = webdriver,
		args = args,
	})

	return handle
end

--- @param url string
--- @param body string
---
--- @return string result
local wd_post_json = function(url, body)
	local result = ""

	local handle = http.new()
	tm:defer(function()
		handle:cleanup()
	end)

	handle
		:setopt(http.OPT_URL, url)
		:setopt(http.OPT_POST, 1) -- force POST
		:setopt(http.OPT_POSTFIELDS, body)
		:setopt(http.OPT_POSTFIELDSIZE, #body) -- avoid chunked/odd sizing
		:setopt(http.OPT_HTTPHEADER, {
			"Content-Type: application/json; charset=utf-8",
			"Accept: application/json",
			"Expect:", -- disable 100-continue
		})
		:setopt(http.OPT_WRITEFUNCTION, function(chunk, n)
			result = result .. chunk
			return n
		end)

	handle:perform()
	return result
end

--- @param url string
--- @param body string -- JSON string
--- @return string result
local wd_put_json = function(url, body)
	local result = ""

	local handle = http.new()
	tm:defer(function()
		handle:cleanup()
	end)

	handle
		:setopt(http.OPT_URL, url)
		:setopt(http.OPT_CUSTOMREQUEST, "PUT")
		:setopt(http.OPT_POSTFIELDS, body)
		:setopt(http.OPT_POSTFIELDSIZE, #body)
		:setopt(http.OPT_HTTPHEADER, {
			"Content-Type: application/json; charset=utf-8",
			"Accept: application/json",
			"Expect:",
		})
		:setopt(http.OPT_WRITEFUNCTION, function(chunk, n)
			result = result .. chunk
			return n
		end)

	handle:perform()
	return result
end

--- @param url string
--- @return string result
local wd_get_json = function(url)
	local result = ""

	local handle = http.new()
	tm:defer(function()
		handle:cleanup()
	end)

	handle
		:setopt(http.OPT_URL, url)
		:setopt(http.OPT_HTTPHEADER, {
			"Accept: application/json",
			"Expect:",
		})
		:setopt(http.OPT_WRITEFUNCTION, function(chunk, n)
			result = result .. chunk
			return n
		end)

	handle:perform()
	return result
end

--- @param url string
--- @return string result
local wd_delete_json = function(url)
	local result = ""

	local handle = http.new()
	tm:defer(function()
		handle:cleanup()
	end)

	handle
		:setopt(http.OPT_URL, url)
		:setopt(http.OPT_CUSTOMREQUEST, "DELETE")
		:setopt(http.OPT_HTTPHEADER, {
			"Accept: application/json",
			"Expect:",
		})
		:setopt(http.OPT_WRITEFUNCTION, function(chunk, n)
			result = result .. chunk
			return n
		end)

	handle:perform()
	return result
end

--- @param session wd_session
--- @param url string
---
--- @return table result
local open_url = function(session, url)
	local payload = {
		url = url,
	}
	local res = session:cmd({
		method = "POST",
		endpoint = "url",
		payload = payload,
	})
	return res
end

--- Go back in history.
--- @param session wd_session
---
--- @return table result
local go_back = function(session)
	local res = session:cmd({
		method = "POST",
		endpoint = "back",
	})
	return res
end

--- Go forward in history.
--- @param session wd_session
---
--- @return table result
local go_forward = function(session)
	local res = session:cmd({
		method = "POST",
		endpoint = "forward",
	})
	return res
end

--- Reload the page.
--- @param session wd_session
---
--- @return table result
local refresh = function(session)
	local res = session:cmd({
		method = "POST",
		endpoint = "refresh",
	})
	return res
end

--- Get the current URL.
--- @param session wd_session
---
--- @return string url
local get_current_url = function(session)
	local res = session:cmd({
		method = "GET",
		endpoint = "url",
	})
	return res.value
end

--- Get the page title.
---
--- @return string title
local get_title = function(session)
	local res = session:cmd({
		method = "GET",
		endpoint = "title",
	})
	return res.value
end

--- Find the first element matching a selector.
---
--- @param session wd_session
--- @param using string   e.g. "css selector", "xpath"
--- @param value string
---
--- @return string element_id
local find_element = function(session, using, value)
	local res = session:cmd({
		method = "POST",
		endpoint = "element",
		payload = {
			using = using,
			value = value,
		},
	})
	local v = res and res.value
	if not v then
		error("find_element: no value in response")
	end
	-- Prefer W3C key; fall back to legacy JSON Wire
	return v[M.ELEM_KEY] or v.ELEMENT
end

--- Find *all* elements matching a selector.
--- @param session wd_session
--- @param using string   e.g. "css selector", "xpath"
--- @param value string
---
--- @return string[] element_ids
local find_elements = function(session, using, value)
	local res = session:cmd({
		method = "POST",
		endpoint = "elements",
		payload = {
			using = using,
			value = value,
		},
	})
	local arr = res and res.value
	if type(arr) ~= "table" then
		error("find_elements: value is not an array")
	end
	local out = {}
	for i, entry in ipairs(arr) do
		out[i] = entry[M.ELEM_KEY] or entry.ELEMENT
	end
	return out
end

--- @param session wd_session
--- @param width number|nil
--- @param height number|nil
--- @return table result
local resize_window = function(session, width, height)
	local res
	if width and height then
		res = session:cmd({
			method = "POST",
			endpoint = "window/rect",
			payload = {
				width = width,
				height = height,
			},
		})
	else
		res = session:cmd({
			method = "POST",
			endpoint = "window/maximize",
		})
	end
	if res.value ~= nil and res.value.error then
		error("resize failed: " .. tostring(res.value.message))
	end
	return res
end

--- Click on an element.
---
--- @param session wd_session
--- @param element_id string
--- @return table result
local click = function(session, element_id)
	local res = session:cmd({
		method = "POST",
		endpoint = ("element/%s/click"):format(element_id),
	})
	if res.value ~= nil and res.value.error then
		error("click failed: " .. tostring(res.value.message))
	end
	return res
end

--- Send keystrokes to an element.
---
--- @param session wd_session
--- @param element_id string
--- @param text string
---
--- @return table result
local send_keys = function(session, element_id, text)
	-- W3C: prefer "text"; keep "value" for legacy drivers
	local chars = {}
	for i = 1, #text do
		chars[i] = text:sub(i, i)
	end

	local payload = { text = text, value = chars }
	local res = session:cmd({
		method = "POST",
		endpoint = ("element/%s/value"):format(element_id),
		payload = payload,
	})

	-- Surface driver errors (some return {value={error=..., message=...}})
	local v = res and res.value
	if v and v.error then
		error("send_keys failed: " .. tostring(v.message))
	end
	return res
end

--- Retrieve the visible text of an element.
---
--- @param session wd_session
--- @param element_id string
---
--- @return string text
local get_text = function(session, element_id)
	local res = session:cmd({
		method = "GET",
		endpoint = ("element/%s/text"):format(element_id),
	})
	return res.value
end

--- Execute synchronous JavaScript in the page.
---
--- @param session wd_session
--- @param script string JS source
--- @param args table? array of arguments
---
--- @return table result
local execute = function(session, script, args)
	local res = session:cmd({
		method = "POST",
		endpoint = "execute/sync",
		payload = {
			script = script,
			args = args or {},
		},
	})
	return res
end

--- Take a full-page screenshot.
---
--- @param session wd_session
---
--- @return string? base64_png
local screenshot = function(session)
	local res = session:cmd({
		method = "GET",
		endpoint = "screenshot",
	})
	return res.value
end

---
---
--- @param session wd_session
--- @param source_id string element to grab
--- @param target_id string element to drop on
---
--- @return table raw WebDriver response
local drag_and_drop = function(session, source_id, target_id)
	local BUTTON_LMB = 0 -- left mouse button for pointer actions
	local payload = {
		actions = {
			{
				type = "pointer",
				id = "mouse",
				parameters = { pointerType = "mouse" },
				actions = {
					{ type = "pointerMove", origin = { [M.ELEM_KEY] = source_id }, x = 0, y = 0 },
					{ type = "pointerDown", button = BUTTON_LMB },
					{ type = "pause", duration = 100 },
					{ type = "pointerMove", origin = { [M.ELEM_KEY] = target_id }, x = 0, y = 0 },
					{ type = "pointerUp", button = BUTTON_LMB },
				},
			},
		},
	}
	return session:cmd({
		method = "POST",
		endpoint = "actions",
		payload = payload,
	})
end

--- Input text
---
--- @param session wd_session
--- @param element_id string
--- @param text string
--- @param clear_first boolean? (default: true)
---
--- @return table raw WebDriver response
local input_text = function(session, element_id, text, clear_first)
	if clear_first ~= false then
		session:cmd({
			method = "POST",
			endpoint = ("element/%s/clear"):format(element_id),
		})
	end
	return session:send_keys({
		element_id = element_id,
		text = text,
	})
end

-- Wait until element *visible*.
-- Polls `/displayed` endpoint until it returns true or timeout.
--
--- @param session wd_session
--- @param using string   selector strategy   (css selector, xpath…)
--- @param value string   selector
--- @param timeout integer? milliseconds (default 5000)
---
--- @return string element_id (throws error on timeout)
local wait_until_visible = function(session, using, value, timeout)
	timeout = timeout or 5000
	local start = tm.millis()
	local elem_id

	while tm.millis() - start < timeout do
		local ok, id = pcall(session.find_element, session, {
			using = using,
			value = value,
		})
		if ok then
			local res = session:cmd({
				method = "GET",
				endpoint = ("element/%s/displayed"):format(id),
			})
			if res.value == true then
				elem_id = id
				break
			end
		end
		tm.sleep(100) -- small poll interval (ms)
	end

	if not elem_id then
		error(("wait_until_visible timeout after %d ms for selector %s:%s"):format(timeout, using, value))
	end
	return elem_id
end

--- Scroll element into view
---
--- @param session wd_session
--- @param element_id string
---
--- @return table raw WebDriver response
local scroll_into_view = function(session, element_id)
	local js = "arguments[0].scrollIntoView({block:'center',inline:'nearest'});"
	return session:execute({
		script = js,
		args = {
			{ [M.ELEM_KEY] = element_id },
		},
	})
end

--- Low level helper
---
--- @param session wd_session
--- @param method api_method
--- @param endpoint string
--- @param payload table? payload if method is "post" or "put"
---
--- @return table response
local session_cmd = function(session, method, endpoint, payload)
	payload = payload or {}
	local url = session.base_url .. ":" .. session.port .. "/session/" .. session.id .. "/" .. endpoint
	local body = json.serialize(payload)
	if method == "PUT" or method == "put" then
		local result = wd_put_json(url, body)
		return json.deserialize(result)
	end
	if method == "POST" or method == "post" then
		local result = wd_post_json(url, body)
		return json.deserialize(result)
	end
	if method == "DELETE" or method == "delete" then
		local result = wd_delete_json(url)
		return json.deserialize(result)
	end
	if method == "GET" or method == "get" then
		local result = wd_get_json(url)
		return json.deserialize(result)
	end

	error("Unknown request " .. method)
end

--- End webdriver session
---
--- @param session wd_session
local session_end = function(session)
	local url = session.base_url .. ":" .. session.port .. "/session/" .. session.id
	wd_delete_json(url)
end

--- Start webdriver session
---
--- @param opts wd_session_opts opts to open session with
---
--- @return wd_session
M.new_session = function(opts)
	opts.url = opts.url or "http://localhost"
	assert(opts.port, "opts.port is required")

	-- Start from user body or empty
	local body = opts.request_object or {}

	-- Normalize into W3C shape: body.capabilities.{alwaysMatch, firstMatch}
	body.capabilities = body.capabilities or {}

	-- If caller used legacy top-level fields, merge them into capabilities
	local legacy_always = body.alwaysMatch or {}
	local legacy_first = body.firstMatch or body["firstMatch"] or {}

	-- Ensure existence
	body.capabilities.alwaysMatch = body.capabilities.alwaysMatch or {}
	body.capabilities.firstMatch = body.capabilities.firstMatch or (next(legacy_first) and legacy_first or { {} })

	-- Merge legacy alwaysMatch into capabilities.alwaysMatch (without overwriting user keys)
	for k, v in pairs(legacy_always) do
		if body.capabilities.alwaysMatch[k] == nil then
			body.capabilities.alwaysMatch[k] = v
		end
	end

	-- Keep legacy desiredCapabilities if user provided (helps older drivers)
	body.desiredCapabilities = body.desiredCapabilities or body["desiredCapabilities"]

	opts.headless = opts.headless or false

	-- Convenience handle into caps
	local caps = body.capabilities
	local am = caps.alwaysMatch

	if opts.headless then
		opts.headless_implementation = opts.headless_implementation or "chromedriver"
		if opts.headless_implementation == "chromedriver" or opts.headless_implementation == "operadriver" then
			am.browserName = am.browserName or "chrome"
			am["goog:chromeOptions"] = am["goog:chromeOptions"] or {}
			am["goog:chromeOptions"].args = am["goog:chromeOptions"].args or {}
			table.insert(am["goog:chromeOptions"].args, "--headless=new")
			table.insert(am["goog:chromeOptions"].args, "--disable-gpu")
		elseif opts.headless_implementation == "geckodriver" then
			am.browserName = am.browserName or "firefox"
			am["moz:firefoxOptions"] = am["moz:firefoxOptions"] or {}
			am["moz:firefoxOptions"].args = am["moz:firefoxOptions"].args or {}
			table.insert(am["moz:firefoxOptions"].args, "-headless")
		elseif opts.headless_implementation == "msedgedriver" then
			-- Microsoft Edge (Chromium)
			am.browserName = am.browserName or "MicrosoftEdge"
			am["ms:edgeOptions"] = am["ms:edgeOptions"] or {}
			am["ms:edgeOptions"].args = am["ms:edgeOptions"].args or {}
			table.insert(am["ms:edgeOptions"].args, "--headless=new")
			table.insert(am["ms:edgeOptions"].args, "--disable-gpu")
		else
			error(
				("No default headless implementation for %s, modify opts.request_object instead"):format(
					tostring(opts.headless_implementation)
				)
			)
		end
	end

	-- Default browser if none chosen by user or headless path
	am.browserName = am.browserName or "chrome"

	-- Serialize and send
	local body_str = json.serialize(body)
	local url = ("%s:%d/session"):format(opts.url, opts.port)
	local result = wd_post_json(url, body_str)

	if result == "" or result == nil then
		error("Unable to start a session: empty result from server")
	end

	-- Try to tolerate different response shapes
	local decoded = json.deserialize(result)
	local value = decoded and decoded.value or decoded

	if not value then
		error("Unable to start a session: no `value` field in response")
	end

	-- If an error surfaced, surface its message (some drivers return {value={error=..., message=...}})
	if value.error then
		error("Unable to start a session: " .. (tostring(value.message) or tostring(value.error)))
	end

	local sessionId = value.sessionId or decoded.sessionId
	if not sessionId then
		error("Unable to start a session: sessionId is not present")
	end

	local session = {
		base_url = opts.url,
		port = opts.port,
		id = sessionId,
		headless = opts.headless,
	}

	local mt = {}
	mt.__index = mt

	function mt:click(element_id)
		return click(self, element_id)
	end
	--- @param mt_opts wd_session_drag_and_drop_opts
	function mt:drag_and_drop(mt_opts)
		return drag_and_drop(self, mt_opts.source_id, mt_opts.target_id)
	end
	--- @param mt_opts wd_session_execute_opts
	function mt:execute(mt_opts)
		return execute(self, mt_opts.script, mt_opts.args)
	end
	--- @param mt_opts wd_session_find_elements_opts
	function mt:find_element(mt_opts)
		return find_element(self, mt_opts.using, mt_opts.value)
	end
	--- @param mt_opts wd_session_find_elements_opts
	function mt:find_elements(mt_opts)
		return find_elements(self, mt_opts.using, mt_opts.value)
	end
	function mt:get_current_url()
		return get_current_url(self)
	end
	function mt:get_text(element_id)
		return get_text(self, element_id)
	end
	function mt:get_title()
		return get_title(self)
	end
	function mt:go_back()
		return go_back(self)
	end
	function mt:go_forward()
		return go_forward(self)
	end
	--- @param mt_opts wd_session_input_text_opts
	function mt:input_text(mt_opts)
		return input_text(self, mt_opts.element_id, mt_opts.text, mt_opts.clear_first)
	end
	function mt:open_url(url_str)
		return open_url(self, url_str)
	end
	function mt:refresh()
		return refresh(self)
	end
	--- @param mt_opts wd_session_resize_window_opts
	function mt:resize_window(mt_opts)
		if mt_opts then
			return resize_window(self, mt_opts.width, mt_opts.height)
		else
			return resize_window(self)
		end
	end
	--- @param mt_opts wd_session_send_keys_opts
	function mt:send_keys(mt_opts)
		return send_keys(self, mt_opts.element_id, mt_opts.text)
	end
	function mt:screenshot()
		return screenshot(self)
	end
	function mt:scroll_into_view(element_id)
		return scroll_into_view(self, element_id)
	end
	--- @param mt_opts wd_session_wait_until_visible_opts
	function mt:wait_until_visible(mt_opts)
		return wait_until_visible(self, mt_opts.using, mt_opts.value, mt_opts.timeout)
	end
	--- @param mt_opts wd_session_cmd_opts
	function mt:cmd(mt_opts)
		return session_cmd(self, mt_opts.method, mt_opts.endpoint, mt_opts.payload)
	end
	function mt:close()
		return session_end(self)
	end

	setmetatable(session, mt)

	return session
end

return M
