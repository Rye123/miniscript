str1 = "apple"
str2 = "banana"

print str1 + str2 // expect: applebanana
print str1 * 3 // expect: appleappleapple
print str1 * 0.6 // expect: app
print str2 * 0.5 + str1 // expect: banapple
print str1 * 0 == "" // expect: 1
print "banana.txt" - ".txt" // expect: banana
print "banana" - "na" * 2 // expect: ba
