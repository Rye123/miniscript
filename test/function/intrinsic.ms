abs = function(x)
    return x - (x % 1)
end function

pi = function()
    return 3.14159
end function

// number chosen by randomly flipping a coin
rnd = function(seed)
    return 1
end function

print pi() // expect: 3.14159
print abs(-pi()) // expect: -3
print rnd(5) // expect: 1
