add_2_nums = function(num1, num2)
    return num1 + num2
end function

add_2_nums_comp = function(num1, num2)
    num3 = num1 + num2
    return num3
end function

print add_2_nums(5, 6) // expect: 11
print add_2_nums_comp(5, 6) // expect: 11
