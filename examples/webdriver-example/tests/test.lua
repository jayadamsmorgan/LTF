local ltf = require("ltf")
local webdriver = ltf.webdriver

ltf.test({
	name = "testing web session start",
	body = function()
		local webdriver_port = 9515

		-- Spawn webdriver instance on port 9515
		local proc_handle = webdriver.spawn_webdriver({
			webdriver = "chromedriver",
			port = webdriver_port,
		})

		-- Kill webdriver instance on test finish
		ltf.defer(function()
			proc_handle:kill()
		end)

		ltf.sleep(1000)

		-- Start webdriver session on port 9515
		local session = webdriver.session_start({
			port = webdriver_port,
			headless = true,
		})

		print("Webdriver session id: " .. session.id)

		ltf.defer(function()
			webdriver.session_end(session)
		end)

		ltf.sleep(1000)

		-- Open google.com
		webdriver.open_url(session, "https://google.com/")

		ltf.sleep(1000)

		-- Open yahoo.com
		webdriver.open_url(session, "https://github.com/")

		ltf.sleep(1000)

		-- Go back to google.com
		webdriver.go_back(session)

		ltf.sleep(1000)

		-- Go forward to yahoo.com
		webdriver.go_forward(session)

		ltf.sleep(1000)

		-- Refresh the site
		webdriver.refresh(session)

		ltf.sleep(1000)

		-- Get the site url
		local url = webdriver.get_current_url(session)
		assert(url == "https://github.com/")

		-- Get the site title
		local title
		title = webdriver.get_title(session)
		print(title)
		assert(title == "GitHub · Build and ship software on a single, collaborative platform · GitHub")

		ltf.sleep(500)
	end,
})
