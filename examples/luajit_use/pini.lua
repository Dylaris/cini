local ffi = require("ffi")

-- declaration
ffi.cdef[[
    typedef enum Pini_Value_Type {
        PINI_VALUE_NUMBER,
        PINI_VALUE_STRING,
        PINI_VALUE_BOOLEAN
    } Pini_Value_Type;

    typedef struct Pini_Value {
        Pini_Value_Type type;
        union {
            double number;
            char *string;
            bool boolean;
        } as;
    } Pini_Value;

    typedef struct Pini_Context Pini_Context;

    bool pini_load(Pini_Context *ctx, const char *filename);
    void pini_unload(Pini_Context *ctx);
    void pini_dump(Pini_Context *ctx);
    bool pini_has(Pini_Context *ctx, const char *section_name, const char *key_name);

    Pini_Context *pini_new_context(void);
    void pini_free_context(Pini_Context *ctx);
    Pini_Value *pini_lookup(Pini_Context *ctx, const char *section_name, const char *key_name);
]]

-- load dynamic library
local cpini
if ffi.os == "Windows" then
    cpini = ffi.load("./pini.dll")
else
    cpini = ffi.load("./libpini.so")
end

local pini = {}
pini.__index = pini

function pini.new()
    local obj = {}
    setmetatable(obj, pini)
    obj.ctx = cpini.pini_new_context()
    if not obj.ctx then
        return nil, "failed to create pini context"
    end
    return obj
end

function pini:load(filename)
    if not cpini.pini_load(self.ctx, filename) then
        return false, "failed to load config file: " .. (filename or "nil")
    end
    return true
end

function pini:lookup(section, key)
    local ok, valptr = pcall(function()
        return cpini.pini_lookup(self.ctx, section, key)
    end)

    -- only 'valptr == nil' means 'valptr == NULL', do not use 'not valptr'
    if not ok or valptr == nil then
        return nil, string.format("config entry [%s:%s] not found", section, key)
    end

    local value = valptr[0] -- dereference

    if value.type == ffi.C.PINI_VALUE_NUMBER then
        return value.as.number
    elseif value.type == ffi.C.PINI_VALUE_STRING then
        return ffi.string(value.as.string)
    elseif value.type == ffi.C.PINI_VALUE_BOOLEAN then
        return value.as.boolean
    else
        return nil, string.format("unknown value type for [%s:%s]", section, key)
    end
end

function pini:has(section, key)
    return cpini.pini_has(self.ctx, section, key)
end

function pini:dump()
    cpini.pini_dump(self.ctx)
end

function pini:close()
    if self.ctx then
        cpini.pini_free_context(self.ctx)
        self.ctx = nil
    end
end

pini.__gc = pini.close

return pini
