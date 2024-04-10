print true == true    // expect: 1
print true == false   // expect: 0
print false == true   // expect: 0
print false == false  // expect: 1

// with num
print true == 1        // expect: 1
print false == 0       // expect: 1

// with string
print true == "true"   // expect: 0
print false == "false" // expect: 0
print false == ""      // expect: 0

print true != true    // expect: 0
print true != false   // expect: 1
print false != true   // expect: 1
print false != false  // expect: 0

// with num
print true != 1        // expect: 0
print false != 0       // expect: 0

// with string
print true != "true"   // expect: 1
print false != "false" // expect: 1
print false != ""      // expect: 1
