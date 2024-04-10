// basics
print 1 and 1 and 1 // expect: 1
print 1 and 0 and 1 // expect: 0
print 0 and 1 and 1 // expect: 0
print 1 or 1 or 1 // expect: 1
print 1 or 0 or 1 // expect: 1
print 0 or 1 or 1 // expect: 1
print 1 and null // expect: 0
print 1 or null // expect: 1

// beyond standard 0-1 range
print -1 and 0 // expect: 0
print 0 and -1 // expect: 0
print -1 or 0 // expect: 1
print 0 or -1 // expect: 1
print 5 and 10 // expect: 1
print 5 or 10 // expect: 1

// with string
print 1 and "ryan" // expect: 1
print 0 or "" // expect: 0
print 0 or "" or 0 // expect: 0

// Return the first non-true argument.
print 1 and 2 and "" // expect: 0

// Return the first true argument.
print 1 or "full string" // expect: 1
print "" or 1 // expect: 1
print 0 or "" or true // expect: 1


