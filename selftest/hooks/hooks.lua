local ltf = require("ltf")
local hooks = ltf.hooks
local json = ltf.json

local output = {}
local tests_ran = 0

hooks.test_run_started(function(context)
	output.run_started_ctx = context
	output.test_started_ctxs = {}
	output.test_finished_ctxs = {}
    ltf.log_info("Hook test run started")
end)

hooks.test_started(function(context)
	tests_ran = tests_ran + 1
    ltf.log_info("Hook test started")
	output.test_started_ctxs[tests_ran] = context
end)

hooks.test_finished(function(context)
	output.test_finished_ctxs[tests_ran] = context
    ltf.log_info("Hook testfinished")
end)

hooks.test_run_finished(function(context)
	output.run_finished_ctx = context

	local result = json.serialize(output, { pretty = true, spaced = true })

	local output_file = io.open("hooks_output.json", "w")
	assert(output_file)
    ltf.log_info("Hook test run finished")
	output_file:write(result)
	output_file:flush()
	output_file:close()
end)
