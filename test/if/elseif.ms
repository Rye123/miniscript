if 2+2 == 4 then
 print 1
else if 2 > 3 then
 print 2
else if 5 > 4 then
 print 3
else
 print 4
end if
// expect: 1

if 2+3 == 4 then
 print 1
else if 2 < 3 then
 print 2
else if 5 > 4 then
 print 3
else
 print 4
end if
// expect: 2

if 2+3 == 4 then
 print 1
else if 2 > 3 then
 print 2
else if 5 > 4 then
 print 3
else
 print 4
end if
// expect: 3

if 2+3 == 4 then
 print 1
else if 2 > 3 then
 print 2
else if 5 < 4 then
 print 3
else
 print 4
end if
// expect: 4
