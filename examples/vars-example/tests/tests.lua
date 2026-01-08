local ltf = require("ltf")

ltf.test({
	name = "Example test",
	body = function()
		local var1 = ltf.get_var("var1")
		local var2 = ltf.get_var("var2")
		local var3 = ltf.get_var("var3")
		local var4 = ltf.get_var("var4")
		local var5 = ltf.get_var("var5")
		ltf.log_info("var1: " .. var1)
		ltf.log_info("var2: " .. var2)
		ltf.log_info("var3: " .. var3)
		ltf.log_info("var4: " .. var4)
		ltf.log_info("var5: " .. var5)
	end,
})
