s = "Hello
world!"
// Lexer Error: missing closing quote (") [line 1]

x = 5$&@*!
// Compiler Error: got Unknown($) where EOL is required
// note: miniscript interpreter says the above is a parsing error, but for us it's a lexing error
