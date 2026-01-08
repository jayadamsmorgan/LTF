local th = require("ltf-hooks")

local M = {}

--- @class context_t
--- @field test_run test_run_context_t context about current test run
--- @field test test_context_t current test context for `test_started`|`test_finished`
--- @field logs context_logs_t

--- @class context_logs_t
--- @field dir string log directory path
--- @field raw_log string raw log file path
--- @field output_log string output log file path

--- @class test_run_context_t
--- @field project_name string
--- @field ltf_version string
--- @field os string
--- @field os_version string
--- @field target string? nil if project is single-target
--- @field started string
--- @field finished string? nil if the test run hasn't finished yet
--- @field tags [string]
--- @field vars table<ltf_var_name, ltf_var_value>
--- @field secrets table<secret_name, secret_value>

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
--- @field name string
--- @field description string
--- @field started string
--- @field finished string? nil on `test_started`
--- @field teardown_start string? nil on `test_started`
--- @field teardown_end string? nil on `test_started`
--- @field status "passed"|"failed"|nil nil on `test_started`
--- @field tags [string]
--- @field outputs [test_output_t] populated only on `test_finished`
--- @field failure_reasons [test_output_t] populated only on `test_finished`
--- @field teardown_output [test_output_t] populated only on `test_finished`
--- @field teardown_errors [test_output_t] populated only on `test_finished`
--- @field keywords [test_keyword_t] populated only on `test_finished`

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
