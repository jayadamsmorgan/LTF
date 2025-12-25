local ltf = require("ltf")
local hooks = ltf.hooks

hooks.test_run_started(function(context)
	ltf.print("Started test run at " .. context.test_run.started)
end)

hooks.test_started(function(context)
	ltf.print("Started test " .. context.test.name)
end)

hooks.test_finished(function(context)
	ltf.print("Finished test " .. context.test.name)
end)

hooks.test_run_finished(function(context)
	ltf.print("Finished test run at " .. context.test_run.finished)
end)
