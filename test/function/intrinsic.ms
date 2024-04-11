abs = function(x)
    return x - (x % 1)
end function

pi = function()
    return 3.1415926535898
end function

// number chosen by randomly flipping a coin
rnd = function(seed)
    return 1
end function

print pi()
print abs(pi())
print rnd(5)
