local M = {}

local ltf = require("ltf")
local wd = ltf.webdriver

--- @return wd_session
M.setup = function()
	local port = 9515

	local session = wd.new_session({
		port = port,
		headless = false,
		headless_implementation = "geckodriver",
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
