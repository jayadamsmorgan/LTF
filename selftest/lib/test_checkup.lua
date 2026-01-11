local M = {}

local ltf = require("ltf")
local json = ltf.json
local proc = ltf.proc

--- @alias status_t
--- | '"PASSED"'
--- | '"FAILED"'

--- @alias log_level_t
--- | '"CRITICAL"'
--- | '"ERROR"'
--- | '"WARNING"'
--- | '"INFO"'
--- | '"DEBUG"'
--- | '"TRACE"'

--- @class test_t
--- @field name string
--- @field description string
--- @field started string
--- @field finished string
--- @field status status_t
--- @field tags [string]
--- @field output [output_t]
--- @field failure_reasons [output_t]? -- present only when status is "failed"
--- @field teardown_output [output_t]
--- @field teardown_errors [output_t]

--- @class output_t
--- @field file string
--- @field line integer
--- @field date_time string
--- @field level log_level_t
--- @field msg string

--- @param s string
--- @return boolean
M.is_valid_datetime = function(s)
	local MM, DD, YY, hh, mm, ss = s:match("^(%d%d)%.(%d%d)%.(%d%d)%-(%d%d):(%d%d):(%d%d)$")
	if not MM then
		return false
	end

	MM, DD, YY, hh, mm, ss = tonumber(MM), tonumber(DD), tonumber(YY), tonumber(hh), tonumber(mm), tonumber(ss)

	if MM < 1 or MM > 12 then
		return false
	end
	if hh > 23 or mm > 59 or ss > 59 then
		return false
	end

	local dim = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
	if YY % 4 == 0 then
		dim[2] = 29
	end

	if DD < 1 or DD > dim[MM] then
		return false
	end

	return true
end

--- @param test test_t
--- @param message string
M.test_error = function(test, message)
	ltf.log_error(("Test '%s': %s"):format(test.name, message))
end

--- @param condition boolean
--- @param test test_t
--- @param message string
M.error_if = function(condition, test, message)
	if condition == true then
		M.test_error(test, message)
	end
end

--- @param tag_test test_t
--- @param tag_array [string]
M.test_tags = function(tag_test, tag_array)
	if tag_test.tags == nil then
		return
	end
	M.error_if(#tag_test.tags ~= #tag_array, tag_test, "Tags not match")
	for i = 1, #tag_array do
		M.error_if(
			tag_test.tags[i] ~= tag_array[i],
			tag_test,
			("Tags not match: expected '%s' but got '%s'"):format(tag_array[i], tag_test.tags[i])
		)
	end
end

--- @class log_obj_t
--- @field ltf_version string
--- @field os string
--- @field os_version string
--- @field started string
--- @field finished string
--- @field target string?
--- @field tags [string]
--- @field tests [test_t]

--- @param args [string]
--- @return log_obj_t
M.load_log = function(args)
	local proc_handle = proc.spawn({
		exe = "../build/ltf",
		args = args,
	})
	while proc_handle:wait() == nil do
		proc_handle:read() -- flush stdout to not hang on large buffers
	end
	proc_handle:kill()

	local log_file = io.open("logs/bootstrap/test_run_latest_raw.json", "r")

	assert(log_file)

	local str = log_file:read("a")
	log_file:close()
	assert(str)
	assert(#str ~= 0)

	local log_obj = json.deserialize(str)

	assert(log_obj.ltf_version ~= nil)
	assert(log_obj.ltf_version ~= "")

	assert(log_obj.os ~= nil)
	assert(log_obj.os == "macos" or log_obj.os == "linux")

	assert(log_obj.os_version ~= nil)
	assert(log_obj.os_version ~= "")

	assert(log_obj.started ~= nil)
	assert(M.is_valid_datetime(log_obj.started))

	assert(log_obj.finished ~= nil)
	assert(M.is_valid_datetime(log_obj.finished))

	assert(log_obj.target == "bootstrap") -- It should not be present on single target projects, but here we check it

	return log_obj
end

--- @param test test_t?
--- @param expected_name string
--- @param expected_status status_t
M.check_test = function(test, expected_name, expected_status)
	if test == nil then
		ltf.log_error(("Test '%s': %s"):format(expected_name, "test is nil"))
		return
	end

	if test.name == nil then
		ltf.log_error(("Test '%s': %s"):format(expected_name, "test.name is nil"))
		return
	end
	if test.name ~= expected_name then
		ltf.log_error(
			("Test '%s': %s"):format(
				expected_name,
				("test.name is '%s', expected '%s"):format(test.name, expected_name)
			)
		)
		return
	end

	M.error_if(
		test.status ~= expected_status,
		test,
		("test.status is '%s', expected '%s'"):format(test.status, expected_status)
	)
	if test.status == "failed" and test.failure_reasons == nil then
		M.test_error(test, "test.status is 'failed' but test.failure_reasons is nil")
	end
	if test.status == "failed" and #test.failure_reasons == 0 then
		M.test_error(test, "test.status is 'failed' but test.failure_reasons is empty")
	end

	M.error_if(test.output == nil, test, "test.output is nil")
	M.error_if(
		test.started == nil or M.is_valid_datetime(test.started) == false,
		test,
		"test.started is nil or not valid"
	)
	M.error_if(
		test.finished == nil or M.is_valid_datetime(test.finished) == false,
		test,
		"test.finished is nil or not valid"
	)

	M.error_if(test.tags == nil, test, "test.tags is nil")
	M.error_if(test.teardown_output == nil, test, "test.teardown_output is nil")
	M.error_if(test.teardown_errors == nil, test, "test.teardown_errors is nil")
end

--- @param test test_t
--- @param output output_t?
--- @param expected_message string?
--- @param expected_level log_level_t?
--- @param contains boolean?
M.check_output = function(test, output, expected_message, expected_level, contains)
	contains = contains or false
	if not output then
		M.test_error(test, "output is nil")
		return
	end

	M.error_if(
		output.date_time == nil or M.is_valid_datetime(output.date_time) == false,
		test,
		"output.date_time is nil or not valid"
	)
	M.error_if(output.file == nil, test, "output.file is nil")
	M.error_if(#output.file == 0, test, "output.file is empty")
	M.error_if(output.line == nil, test, "output.line is nil")
	M.error_if(output.line == 0, test, "output.line is 0")
	M.error_if(output.level == nil, test, "output.level is nil")
	M.error_if(
		output.level ~= expected_level,
		test,
		("output.level is '%s', expected '%s'"):format(output.level, expected_level)
	)
	M.error_if(output.msg == nil, test, "output.msg is nil")
	if contains and expected_message then
		M.error_if(
			output.msg:find(expected_message, nil, true) == nil,
			test,
			("\noutput.msg is:\n'%s\ndoes not contain:\n'%s'"):format(output.msg, expected_message)
		)
	elseif expected_message then
		M.error_if(
			output.msg ~= expected_message,
			test,
			("output.msg is '%s', expected '%s'"):format(output.msg, expected_message)
		)
	end

	return nil, output
end

return M
