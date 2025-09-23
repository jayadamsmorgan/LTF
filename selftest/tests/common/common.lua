local ltf = require("ltf")

ltf.test({
	name = "Test common LTF test",
	tags = { "common" },
	body = function()
		ltf.log_info("common")
	end,
})
