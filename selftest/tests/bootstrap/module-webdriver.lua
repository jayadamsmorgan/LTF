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

		-- Just making sure it runs:
		ltf.sleep(5000)
		local status = proc_handle:wait()
		assert(status == nil)

		-- Actual testing:
		local session = wd.new_session({
			port = port,
			headless = true,
		})
		ltf.defer(function()
			session:close()
		end)
		session:open_url("https://github.com/")
		ltf.log_info(session:get_current_url())
		ltf.log_info(session:get_title())
	end,
})
