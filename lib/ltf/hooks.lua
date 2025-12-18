local th = require("ltf-hooks")

local M = {}

--- @class context_t
--- @field test_run test_run_context_t
--- @field test test_context_t
--- @field log_dir string

--- @class test_run_context_t
--- @field project_name string
--- @field ltf_version string
--- @field started string
--- @field finished string? nil if the test run isn't finished
--- @field os string
--- @field os_version string
--- @field target string?
--- @field tags [string]

--- @class test_output_t
--- @field file string
--- @field line integer
--- @field date_time string
--- @field level "CRITICAL"|"ERROR"|"WARNING"|"INFO"|"DEBUG"|"TRACE"
--- @field msg string

--- @class test_keyword_t
--- @field name string
--- @field started string
--- @field finished string
--- @field file string
--- @field line integer
--- @field children [test_keyword_t]

--- @class test_context_t
--- @field test_file string
--- @field name string
--- @field started string
--- @field finished string? nil if the test isn't finished
--- @field status "passed"|"failed"|nil nil if the test isn't finished
--- @field tags [string]
--- @field output [test_output_t]
--- @field failure_reasons [test_output_t]
--- @field teardown_output [test_output_t]
--- @field teardown_errors [test_output_t]
--- @field keywords [test_keyword_t]

--- @alias hooks_fn fun(context: context_t)

--- Register 'test_run_started' hook
--- @param fn hooks_fn
M.test_run_started = function(fn)
	th:register_test_run_started(fn)
end

--- Register 'test_started' hook
--- @param fn hooks_fn
M.test_started = function(fn)
	th:register_test_started(fn)
end

--- Register 'test_started' hook
--- @param fn hooks_fn
M.test_finished = function(fn)
	th:register_test_finished(fn)
end

--- Register 'test_run_finished' hook
--- @param fn hooks_fn
M.test_run_finished = function(fn)
	th:register_test_run_finished(fn)
end

return M
