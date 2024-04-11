isEven = function(n)
  if n == 0 then
    return true
  end if
  return isOdd(n - 1)
end function

isOdd = function(n)
  if n == 0 then
    return false
  end if
  return isEven(n - 1)
end function

print isEven(4) // expect: 1
print isOdd(3) // expect: 1
