fizzBuzz = function(n)
    i = 1
    while i <= n
        s = "Fizz" * (i % 3 == 0) + "Buzz" * (i % 5 == 0)
        if s == "" then
            print i
        else
            print s
        end if
        s = ""
        i = i + 1
    end while
end function

fizzBuzz(15)
