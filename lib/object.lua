--[[
  @overview: frome luvit-0.7.0 core.lua
]]

local Object = {}
Object.meta = {__index = Object}
 
--[[
  @overview: create a new instance of this object
]]
function Object:create()
  local meta = rawget(self, 'meta')
  if not meta then error('Cannot inherit form instance object') end
  return setmetatable({}, meta)
end

--[[
  @overview: Creates a new instance and calls obj:initialize(...) if it exists.
  @example:
    local Rectangle = Object:extend()
    function Rectangle:initialize(w, h)
      self.w = w
      self.h = h
    end
    function Rectangle:getArea()
      return self.w * self.h
    end
    local rect = Rectangle:new(3, 4)
    print(rect:getArea())
]]
function Object:new(...)
  local obj = self:create()
  if type(obj.initialize) == "function" then
    obj:initialize(...)
  end
  return obj
end

--[[
  @overview: Creates a new sub-class.
  @example:
    local Square = Rectangle:extend()
    function Square:initialize(w)
      self.w = w
      self.h = h
    end
]]
function Object:extend()
  local obj = self:create()
  local meta = {}
  -- move the meta methods defined in our ancestors meta into our own
  -- to preserve expected behavior in children (like __tostring, __add, etc)
  for k, v in pairs(self.meta) do
    meta[k] = v
  end
  meta.__index = obj
  meta.super = self
  obj.meta = meta
  return obj
end

return Object
