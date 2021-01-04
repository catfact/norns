local r = {}
r.__index = r

local lcg = {}
lcg.__index = lcg

function lcg.new()
   local obj = setmetatable({}, lcg)
    obj.x = 1; obj.a = 1597334677; obj.c = 1289706101	
    return obj
end

function lcg:update()
    self.x = (self.x * self.a + self.c) & 0x7fffffff
end

function lcg:int()
    self:update()
    return self.x
end

function lcg:float()
    self:update()
    return self.x / 0x7fffffff
end

local gauss = {}
gauss.__index = gauss
function gauss:new()
    local obj = setmetatable({}, gauss)
    obj.lcg = lcg.new()
    obj.mean = 0
    obj.stddev = 0.5
    return obj
end

function gauss:next()
    local rand_val = self.lcg:float()
    return (((math.sqrt(-2 * math.log(rand_val)) * math.sin(rand_val * 2 * math.pi)) * self.stddev) + self.mean)
end

-- to be continued..

r = {}
r.lcg = lcg
r.gauss = gauss

return r