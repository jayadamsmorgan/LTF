local ltf = require("ltf")
local wd_util = require("webdriver_util")

ltf.test({
	name = "Test page navigation",
	tags = { "module-webdriver" },
	body = function()
		local session = wd_util.setup()

		session:open_url("https://github.com/")

		session:open_url("https://google.com/")

		session:go_back()

		ltf.log_info(session:get_current_url())
		ltf.log_info(session:get_title())

		session:go_forward()
		session:refresh()

		ltf.log_info(session:get_current_url())
		ltf.log_info(session:get_title())
	end,
})

ltf.test({
	name = "Test elements find and interaction",
	tags = { "module-webdriver" },
	body = function()
		-- start new web session
		local session = wd_util.setup()

		-- go to site for testing
		session:open_url("https://demoqa.com/elements")

		ltf.sleep(1000)

		-- open 'elements'
		local elements_id = session:find_element({
			using = "xpath",
			value = "//body/div[@id='app']/div[contains(@class,'body-height')]/div[contains(@class,'container playgound-body')]/div[contains(@class,'row')]/div[contains(@class,'col-md-3')]/div[contains(@class,'left-pannel')]/div[contains(@class,'accordion')]/div[1]/span[1]/div[1]/div[1]",
		})

		session:click(elements_id)

		-- click Check box
		local checkbox_id = session:find_element({ using = "xpath", value = "//span[normalize-space()='Check Box']" })
		session:click(checkbox_id)

		session:wait_until_visible({
			using = "xpath",
			value = "//span[contains(@class,'rct-node-icon')]",
			timeout = 4000,
		})

		ltf.sleep(1000)

		-- check checkbox interaction
		local interact =
			session:find_element({ using = "xpath", value = "//span[@class='rct-checkbox']//*[name()='svg']" })
		session:click(interact)

		-- check css selector search via @class
		local suc = session:find_elements({ using = "css selector", value = ".text-success" })

		-- check correctness of get_text
		for name, value in pairs(suc) do
			ltf.log_info(session:get_text(value))
			break
		end

		-- check correcteness of searching via @class and text
		local sucq = session:find_element({
			using = "xpath",
			value = "//*[contains(@class, 'text-success') and contains(.,'notes')]",
		})

		ltf.log_info(session:get_text(sucq))

		-- check drag and drop and scroll_into_view
		local interactions_id = session:find_element({
			using = "xpath",
			value = "//span[contains(@class,'group-header') and contains(.,'Interactions')]",
		})

		session:scroll_into_view(interactions_id)
		ltf.sleep(1000)
		session:click(interactions_id)
		local dropable_id = session:find_element({ using = "xpath", value = "//span[normalize-space()='Droppable']" })
		session:scroll_into_view(dropable_id)

		ltf.sleep(1000)
		session:click(dropable_id)

		local dragable = session:find_element({ using = "xpath", value = "//div[@id='draggable']" })
		local dropable =
			session:find_element({ using = "xpath", value = "//div[@id='simpleDropContainer']//div[@id='droppable']" })

		session:drag_and_drop({ source_id = dragable, target_id = dropable })
		ltf.sleep(1000)

		local highlighted =
			session:find_element({ using = "xpath", value = "//*[contains(@class, 'ui-state-highlight')]" })
		ltf.log_info(session:get_text(highlighted))
	end,
})
