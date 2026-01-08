local ltf = require("ltf")

ltf.register_vars({
	ip = {
		default = "127.0.0.1",
	},
	port = {
		default = "22",
	},
})

ltf.register_secrets({
	"user",
	"password",
})
