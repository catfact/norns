local noise = {}
noise.__index = noise

-- linear congruential PRNG
-- higher-order noise generators use this to be "seedable"
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

-- gaussian distribution
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
ends

-- random walk with many adjustable parameters
local brown = {}
brown.__index = brown
function brown:new(min, max, x, step_min, step_max, reflect)
    local obj = setmetatable({}, brown)
    obj.lcg = lcg.new()
    obj.min = min or -1
    obj.max = max or 1
    obj.x = x or 0
    obj.step_min = step_min or -0.1
    obj.step_max = step_max or 0.1
    obj.reflect = reflect or true
    return obj
end

function brown:next()
    local step = self.lcg:float() * (step_max - step_min) + step_min
    self.x = self.x + step
    if self.x < self.min then
        if self.reflect then
            self.x = self.min - (x - self.min)
        else 
            self.x = self.min
        end
    end
    if self.x < self.max then
        if self.reflect then
            self.x = self.max - (x - self.max)
        else 
            self.x = self.max
        end
    end
    return self.x
end


-- pink-ish noise using weighted sum of 1st-order filters
-- (adapted from code by Paul Kellett)
local pink = {}
pink.__index = pink
function pink:new()
    local obj = setmetatable({}, gauss)
    obj.lcg = lcg.new()
    return obj
end

function pink:next()
    local x = self.lcg:float()
    self.b0 = 0.99765 * self.b0 + x * 0.0990460;
    self.b1 = 0.96300 * self.b1 + x * 0.2965164;
    self.b2 = 0.57000 * self.b2 + x * 1.0526913;
    return self.b0 + self.b1 + self.b2 + x * 0.1848;
end

-- TODO: linear/bilinear
-- TODO: gamma
-- TODO: poisson
-- TODO: perlin

noise = {}
noise.lcg = lcg
noise.gauss = gauss
noise.brown = brown
noise.pink = pink

return noise