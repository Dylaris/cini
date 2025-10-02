-- usage: luajit main.lua

local pini = require("pini")

local ctx = pini.new()
local ok, err = ctx:load("../config.ini")
if not ok then
    print(err)
    os.exit(1)
end

print("=== Application Configuration ===\n");

-- Database configuration
print("Database Settings:");
print(string.format("  Host: %s",     ctx:lookup("database", "host")))
print(string.format("  Port: %.0f",   ctx:lookup("database", "port")))
print(string.format("  Username: %s", ctx:lookup("database", "username")))
print(string.format("  Use SSL: %s",  ctx:lookup("database", "use_ssl") and "Yes" or "No"))
