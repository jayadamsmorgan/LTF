local ltf = require("ltf")
local util = ltf.util

ltf.test({
	name = "Test util.file_info (file)",
	tags = { "module-util", "file-util" },
	body = function()
		local info = util.file_info(".ltf.json")
		assert(info)

		ltf.log_info(info.type)
		ltf.log_info(info.path)
		ltf.log_info(info.size)
		ltf.log_info(info.permissions)
	end,
})

ltf.test({
	name = "Test util.file_info (directory)",
	tags = { "module-util", "file-util" },
	body = function()
		local info = util.file_info("tests")
		assert(info)

		ltf.log_info(info.type)
		ltf.log_info(info.path)
		ltf.log_info(info.size)
		ltf.log_info(info.permissions)
	end,
})

ltf.test({
	name = "Test util.file_info (symlink)",
	tags = { "module-util", "file-util" },
	body = function()
		local info = util.file_info("logs/test_run_latest_raw.json")
		assert(info)

		ltf.log_info(info.type)
		ltf.log_info(info.path)
		ltf.log_info(info.size)
		ltf.log_info(info.permissions)
	end,
})

ltf.test({
	name = "Test util.file_info (no file present)",
	tags = { "module-util", "file-util" },
	body = function()
		local info = util.file_info("nonexistent_file.txt")
		assert(info == nil)
	end,
})

ltf.test({
	name = "Test util.resolve_symlink",
	tags = { "module-util", "file-util" },
	body = function()
		local resolved = util.resolve_symlink("logs/bootstrap/test_run_latest_raw.json")
		assert(resolved)

		ltf.log_info(resolved)
	end,
})

ltf.test({
	name = "Test util.resolve_symlink (no file present)",
	tags = { "module-util", "file-util" },
	body = function()
		-- Should error out
		local _ = util.resolve_symlink("nonexistent_file.txt")
	end,
})
