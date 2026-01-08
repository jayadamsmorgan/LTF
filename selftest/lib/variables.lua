local ltf = require("ltf")

ltf.register_vars({
	constant = "just a simple variable",
	any = {},
	any_default = {
		default = "variable with a default value",
	},
	enum = {
		values = {
			"value1",
			"value2",
			"value3",
		},
	},
	enum_default = {
		default = "value2",
		values = {
			"value1",
			"value2",
			"value3",
		},
	},
	number = "3.14",
})
