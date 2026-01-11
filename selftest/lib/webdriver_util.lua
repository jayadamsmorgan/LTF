local M = {}

local ltf = require("ltf")
local wd = ltf.webdriver

--- @return wd_session
M.setup = function()
	local port = 9515
	local proc_handle = wd.spawn_webdriver({
		webdriver = "chromedriver",
		port = port,
		extraflags = { "--headless=new", "--window-size=1920,1080" },
	})
	ltf.defer(function()
		proc_handle:kill()
	end)

	-- Just making sure it runs:
	ltf.sleep(5000)
	local status = proc_handle:wait()
	if status ~= nil then
		local stderr = proc_handle:read("stderr", nil)
		error("Unable to start webdriver: " .. stderr)
	end

	local session = wd.new_session({
		port = port,
		headless = true,
	})
	session:execute({
		script = [[
    document.body.style.zoom = "50%";
    ]],
	})
	ltf.defer(function()
		session:close()
	end)

	return session
end

return M
