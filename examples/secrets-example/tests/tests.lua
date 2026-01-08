local ltf = require("ltf")

ltf.test({
	name = "Secrets example test",
	body = function()
		local secret1 = ltf.get_secret("secret1")
		local secret2 = ltf.get_secret("secret2")

		ltf.log_info("secret1: " .. secret1)
		ltf.log_info("secret2: " .. secret2)
	end,
})
