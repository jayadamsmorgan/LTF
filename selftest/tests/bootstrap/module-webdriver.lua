local ltf = require("ltf")
local wd_util = require("webdriver_util")

ltf.test({
	name = "Test minimal webdriver possibilities",
	tags = { "module-webdriver" },
	body = function()
		local session = wd_util.setup()

		session:open_url("https://github.com/")

		session:open_url("https://google.com/")

		session:go_back()

		ltf.log_info(session:get_current_url())
		ltf.log_info(session:get_title())

		session:go_forward()

		ltf.log_info(session:get_current_url())
		ltf.log_info(session:get_title())
	end,
})
