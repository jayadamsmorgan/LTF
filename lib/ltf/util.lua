local util = require("ltf-util")

local M = {}

--- @alias file_info_type
--- | '"directory"'
--- | '"file"'

--- @class file_info
--- @field absolute_path string
--- @field type file_info_type
--- @field size integer
---
--- @field is_symlink boolean
--- @field resolved_path string

--- Returns file_info if file exists, otherwise returns nil
--- @param path string
---
--- @return file_info?
M.file_info = function(path)
	return util.file_info(path)
end

return M
