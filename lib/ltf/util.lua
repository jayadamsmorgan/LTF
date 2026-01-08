local util = require("ltf-util")

local M = {}

--- @alias file_info_type
--- | '"directory"'
--- | '"file"'
--- | '"symlink"'

--- @class file_info
--- @field type file_info_type the type of the file
--- @field path string absolute path to the file
--- @field size integer size of the file/symlink
--- @field permissions integer file permissions

--- Returns file_info if file exists, otherwise returns nil
--- @param path string
---
--- @return file_info?
M.file_info = function(path)
	return util.file_info(path)
end

--- Returns resolved path of a symlink
--- Throws an error if symlink does not exist
--- Returns nil if dangling
---
--- @param path string path to the symlink
---
--- @return string? resolved_path
M.resolve_symlink = function(path)
	return util.resolve_symlink(path)
end

return M
