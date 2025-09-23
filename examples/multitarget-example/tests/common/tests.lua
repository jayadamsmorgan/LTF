local ltf = require("ltf")

local target = ltf.get_current_target()
local custom_lib = require("my-custom-lib")
local variables = require("variables." .. target)

ltf.test({
	name = "Testing multitarget project",
	body = function()
		local baudrate = variables.serial.baudrate
		print("Serial baudrate for target '" .. ltf.get_current_target() .. "' is " .. baudrate)
		local result = custom_lib.keyword2("this is ", "test ")
		print(result)
		if target == "target1" then
			assert(baudrate == 115200)
			assert(result == "this is test ")
		elseif target == "target2" then
			assert(baudrate == 1500000)
			assert(result == "this is test hello")
		else
			-- Should never be reached
			ltf.log_critical("Unknown target '" .. target .. "'")
		end
	end,
})
