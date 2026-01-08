local ltf = require("ltf")

ltf.test({
	name = "First test",
	body = function()
		ltf.log_info("First test")
	end,
})

ltf.test({
	name = "Second test",
	body = function()
		ltf.log_info("Second test")
	end,
})
