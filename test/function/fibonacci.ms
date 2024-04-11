fib = function(n)
  if n < 2 then
    return n
  end if
  return fib(n - 1) + fib(n - 2)
end function

print fib(8) // expect: 21
