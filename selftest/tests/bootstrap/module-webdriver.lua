local ltf = require("ltf")
local wd = ltf.webdriver

ltf.test({
	name = "Test minimal webdriver possibilities",
	tags = { "module-webdriver" },
	body = function()
		local port = 9515
		local proc_handle = wd.spawn_webdriver({
			webdriver = "chromedriver",
			port = port,
		})
		ltf.defer(function()
			proc_handle:kill()
		end)
		ltf.sleep(5000) -- wait for the webdriver to start just to make sure
		local session = wd.session_start({
			port = port,
			headless = true,
		})
		ltf.defer(function()
			wd.session_end(session)
		end)
		wd.open_url(session, "https://github.com/")
		ltf.log_info(wd.get_current_url(session))
		ltf.log_info(wd.get_title(session))
	end,
})
