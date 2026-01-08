local ltf = require("ltf")

ltf.register_vars({
	port_name = {},
	baudrate = {
		default = "1500000",
	},
	bits = {
		default = "8",
	},
	parity = {
		default = "none",
	},
	stopbits = {
		default = "1",
	},
})
