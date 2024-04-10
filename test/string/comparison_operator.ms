// Check that our strings comparisons are properly case-sensitive.
print "q" >= "A" and "q" <= "Z" // expect: 0
print "Q" >= "A" and "Q" <= "Z" // expect: 1
print "foo" == "FOO" // expect: 0
print "foo" == "foo" // expect: 1

// Mixed-type equality testing.
print 1 == "1"  // expect: 0
print "foo" == 0 // expect: 0

print "a" > "A"   // expect: 1
print "aA" > "A"  // expect: 1
print "Aa" > "A"  // expect: 1
print "aa" > "A"  // expect: 1
print "a" > "AA"  // expect: 1
print "aA" > "Aa" // expect: 1

print "ABCa" > "ABCA" // expect: 1
print "ABCa" > "ABC" // expect: 1
print "aaaa" > "aaa" // expect: 1
print "aaa" < "aaaa" // expect: 1

print "A" > "a"  // expect: 0
print "aa" < "A" // expect: 0
print "A" > "aa" // expect: 0
print "Aa" > "aA" // expect: 0
print "AAAa" > "aaa" // expect: 0
