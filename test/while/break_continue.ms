i = 0
while i < 20
	i = i + 1
	if i > 10 then
	  break
	end if

	if i % 3 == 0 then
	   continue
	end if
	print i
end while

// Also test break before else
while i > 0
	if i < 7 then
		break
	else
		print i
	end if
	i = i - 1
end while

// expect: 1
// expect: 2
// expect: 4
// expect: 5
// expect: 7
// expect: 8
// expect: 10
// expect: 11
// expect: 10
// expect: 9
// expect: 8
// expect: 7
