local ltf = require("ltf")

ltf.register_vars({
	var1 = "just a simple variable",
	var2 = {},
	var3 = {
		default = "variable with a default value",
	},
	var4 = {
		values = {
			"value1",
			"value2",
			"value3",
		},
	},
	var5 = {
		default = "value2",
		values = {
			"value1",
			"value2",
			"value3",
		},
	},
})
