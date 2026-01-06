local M = {}

local ltf = require("ltf")
local wd = ltf.webdriver

--- @return wd_session
M.setup = function()
	local port = 9515
	local proc_handle = wd.spawn_webdriver({
		webdriver = "chromium.chromedriver",
		port = port,
	})
	ltf.defer(function()
		proc_handle:kill()
	end)

	-- Just making sure it runs:
	ltf.sleep(5000)
	local status = proc_handle:wait()
	assert(status == nil)

	local session = wd.new_session({
		port = port,
		headless = true,
	})
	ltf.defer(function()
		session:close()
	end)

	return session
end

return M
