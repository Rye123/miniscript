i = 0
while i < 20
	i = i + 1
	if i > 10 then
	  break
	end if

	if i % 3 == 0 then
	   continue
	end if
	print "Iteration" * 0.5 + ": " + i
end while
