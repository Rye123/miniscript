// * has higher precedence than +.
print 2 + 3 * 4 // expect: 14

// * has higher precedence than -.
print 20 - 3 * 4 // expect: 8

// / has higher precedence than +.
print 2 + 6 / 3 // expect: 4

// / has higher precedence than -.
print 2 - 6 / 3 // expect: 0

// < has higher precedence than ==.
print 1 == 2 < 1 // expect: true

// > has higher precedence than ==.
print 0 == 1 > 2 // expect: true

// <= has higher precedence than ==.
print 0 == 2 <= 1 // expect: true

// >= has higher precedence than ==.
print 0 == 1 >= 2 // expect: true

// 1 - 1 is not space-sensitive.
print 1 - 1 // expect: 0
print 1 -1  // expect: 0
print 1- 1  // expect: 0
print 1-1   // expect: 0

// Using () for grouping.
print (2 * (6 - (2 + 2))) // expect: 4
print  (-3+4) * (7-5) / (1+2) // expect: 0.666667
print (-1+2*(4-6))/(3-7)+8    // expect: 9.25
