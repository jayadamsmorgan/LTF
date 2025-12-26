local ltf = require("ltf")
local hooks = ltf.hooks

hooks.test_run_started(function(context)
	ltf.log_info("Started test run at " .. context.test_run.started)
end)

hooks.test_started(function(context)
	ltf.log_info("Started test " .. context.test.name)
end)

hooks.test_finished(function(context)
	ltf.log_info("Finished test " .. context.test.name)
end)

hooks.test_run_finished(function(context)
	ltf.log_info("Finished test run for project " .. context.test_run.project_name)
end)
