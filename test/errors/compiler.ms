x = 5$&@*!
// Compiler Error: got Unknown($) where EOL is required [line 1]

a = 1-
// Compiler Error: got EOL where number, string, or identifier is required [line 4]

a = 1)
// Compiler Error: got RParen where EOL is required [line 7]

a = (1
// Compiler Error: got EOL where RParen is required [line 10]

a = *1
// Compiler Error: got OpTimes where number, string, or identifier is required [line 13]

print -
// Compiler Error: got EOL where number, string, or identifier is required [line 16]