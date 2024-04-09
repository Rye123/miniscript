if 2+2 == 4 then
 print "math works"
else if 2 > 3 then
 print "second"
else if 5 > 4 then
 print 3
else
 print 4
end if
// expect: math works

if 2+3 == 4 then
 print "math doesn't works"
else if 2 < 3 then
 print "second"
else if 5 > 4 then
 print 3
else
 print 4
end if
// expect: second

if 2+3 == 4 then
 print "math doesn't works"
else if 2 > 3 then
 print "second"
else if 5 > 4 then
 print 3
else
 print 4
end if
// expect: 3

if 2+3 == 4 then
 print "math doesn't works"
else if 2 > 3 then
 print "second"
else if 5 < 4 then
 print 3
else
 print 4
end if
// expect: 4
