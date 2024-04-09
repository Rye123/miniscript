// Check that our strings comparisons are properly case-sensitive.
print "q" >= "A" and "q" <= "Z" // expect: 0
print "Q" >= "A" and "Q" <= "Z" // expect: 1
print "foo" == "FOO" // expect: 0
print "foo" == "foo" // expect: 1

// Mixed-type equality testing.
print 1 == "1"  // expect: 0
print "foo" == 0 // expect: 0

