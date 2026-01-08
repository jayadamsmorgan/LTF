local ltf = require("ltf")

local target = ltf.get_current_target()
local custom_lib = require("my-custom-lib")

ltf.test({
	name = "Testing multitarget project",
	body = function()
		local result = custom_lib.keyword2("this is ", "test ")
		print(result)
		if target == "target1" then
			assert(result == "this is test ")
		elseif target == "target2" then
			assert(result == "this is test hello")
		else
			-- Should never be reached
			ltf.log_critical("Unknown target '" .. target .. "'")
		end
	end,
})
