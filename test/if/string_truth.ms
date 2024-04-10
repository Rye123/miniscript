emptyString = ""
fullString = "foo"

if fullString then
print "A"
end if
// expect: A

if not fullString then
print "B"
end if
// expect nothing

if emptyString and fullString then
print "C"
end if
// expect nothing

if emptyString or fullString then
print "D"
end if
// expect: D
