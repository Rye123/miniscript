str1 = "apple"
str2 = "banana"

print str1 + 1 // expect: apple1
print str1 + str2 // expect: applebanana
print str1 * 3 // expect: appleappleapple
print str1 * 0.6 // expect: app
print str2 * 0.5 + str1 // expect: banapple
print str1 * 0 == "" // expect: 1
print "banana.txt" - ".txt"     // expect: banana
print "banana" - "na" * 2       // expect: ba
print "banana" - "anana"        // expect: b
print "banana" - "abanana"      // expect: banana
print "banana" - "bananaa"      // expect: banana
print "banana" - "bananarama"  // expect: banana
print "hello" / 2      // expect: he
print "hello" / (1/2)  // expect: hellohello
print "hello" / 4      // expect: h
print "hello" / 8      // expect: (empty string)
