local ltf = require("ltf")
local webdriver = ltf.webdriver

ltf.test({
	name = "testing web session start",
	body = function()
		local webdriver_port = ltf.get_var_number("port")
		local webdriver_name = ltf.get_var("webdriver")

		ltf.log_info("Using webdriver '" .. webdriver_name .. "' at port " .. webdriver_port)

		-- Spawn webdriver instance on port 9515
		local proc_handle = webdriver.spawn_webdriver({
			webdriver = webdriver_name,
			port = webdriver_port,
		})

		-- Kill webdriver instance on test finish
		ltf.defer(function()
			proc_handle:kill()
		end)

		ltf.sleep(1000)

		-- Start webdriver session on port 9515
		local session = webdriver.new_session({
			port = webdriver_port,
			headless = true,
		})

		ltf.log_info("Webdriver session id: " .. session.id)

		ltf.defer(function()
			session:close()
		end)

		ltf.sleep(1000)

		-- Open google.com
		session:open_url("https://google.com/")

		ltf.sleep(1000)

		-- Open yahoo.com
		session:open_url("https://github.com/")

		ltf.sleep(1000)

		-- Go back to google.com
		session:go_back()

		ltf.sleep(1000)

		-- Go forward to yahoo.com
		session:go_forward()

		ltf.sleep(1000)

		-- Refresh the site
		session:refresh()

		ltf.sleep(1000)

		-- Get the site url
		local url = session:get_current_url()
		assert(url == "https://github.com/")

		-- Get the site title
		local title
		title = session:get_title()
		ltf.log_info(title)

		ltf.sleep(500)
	end,
})
