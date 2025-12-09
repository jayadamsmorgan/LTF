local ltf = require("ltf")

local variables = {
	var1 = { default = "defaultvar1" },
	var2 = { default = "defaultvar2" },
	var3 = { default = "defaultvar3" },
	var4 = { default = "defaultvar4" },
}

ltf.register_vars(variables)

ltf.test({
	name = "First test",
	description = "First test",
	tags = { "tag1" },
	body = function()
		ltf.log_info("First test running!!!")

		local active_tags = ltf.get_active_tags()

		for i, tag in pairs(active_tags) do
			ltf.log_info("tag" .. i .. ": " .. tag)
		end

		local vars = ltf.get_vars()
		local order = { "var1", "var2", "var3", "var4" }
		for _, name in ipairs(order) do
			local val = vars[name]
			ltf.log_info("variable: " .. name .. " = " .. val)
		end
	end,
})

ltf.test({
	name = "Second test",
	description = "First test",
	tags = { "tag2" },
	body = function()
		ltf.log_info("Second test running!!!")

		local active_tags = ltf.get_active_tags()

		for i, tag in pairs(active_tags) do
			ltf.log_info("tag" .. i .. ": " .. tag)
		end
		local vars = ltf.get_vars()
		local order = { "var1", "var2", "var3", "var4" }
		for _, name in ipairs(order) do
			local val = vars[name]
			ltf.log_info("variable: " .. name .. " = " .. val)
		end
	end,
})

ltf.test({
	name = "Third test",
	description = "First test",
	tags = { "tag3" },
	body = function()
		ltf.log_info("Third test running!!!")

		local active_tags = ltf.get_active_tags()

		for i, tag in pairs(active_tags) do
			ltf.log_info("tag" .. i .. ": " .. tag)
		end
		local vars = ltf.get_vars()
		local order = { "var1", "var2", "var3", "var4" }
		for _, name in ipairs(order) do
			local val = vars[name]
			ltf.log_info("variable: " .. name .. " = " .. val)
		end
	end,
})

ltf.test({
	name = "Fourth test",
	description = "First test",
	tags = { "tag4" },
	body = function()
		ltf.log_info("Fourth test running!!!")

		local active_tags = ltf.get_active_tags()

		for i, tag in pairs(active_tags) do
			ltf.log_info("tag" .. i .. ": " .. tag)
		end
		local vars = ltf.get_vars()
		local order = { "var1", "var2", "var3", "var4" }
		for _, name in ipairs(order) do
			local val = vars[name]
			ltf.log_info("variable: " .. name .. " = " .. val)
		end
	end,
})
