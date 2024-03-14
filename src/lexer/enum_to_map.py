"""
Converts the TokenType enum of token.h into a map of strings.
For instance, the following enum:
```
    // comment
    TOKEN_IF, TOKEN_ELSE

    // comment 2
    TOKEN_AND,
```

will be converted to:

```
{"TOKEN_IF", "TOKEN_ELSE", "TOKEN_AND"}
```

We read the token.h file, and directly extract the contents of the enum.
The TokenType enum should be the FIRST enum defined, for this to work!
"""
from pathlib import Path

TOKEN_H_FILE = Path("./token.h")

if __name__ == "__main__":
    if not TOKEN_H_FILE.is_file():
        print("token.h file not found")
        exit(1)

    fp = TOKEN_H_FILE.open('r')
    contents = fp.readlines()
    
    # Extract enum
    enum_start_i = -1
    enum_end_i = -1
    for i, content in enumerate(contents):
        if "typedef enum {" in content and enum_start_i == -1: # (to ignore further typedefs)
            enum_start_i = i + 1
        if "} TokenType;" in content and enum_end_i == -1 and enum_start_i != -1:
            enum_end_i = i
            break
    assert enum_start_i < enum_end_i

    enum_contents = contents[enum_start_i:enum_end_i]

    # Remove comments
    token_str = ""
    for enum_content in enum_contents:
        try:
            comment_start = enum_content.index("//")
            enum_content = enum_content[:comment_start]
        except ValueError:
            pass
        enum_content = enum_content.strip()
        if len(enum_content) > 0:
            token_str += enum_content

    # Split to individual token strings
    tokens = [tok.strip() for tok in token_str.split(",")]
    new_strs = []
    for tok in tokens:
        tok = tok.strip()
        if len(tok) > 0:
            new_strs.append(f"\"{tok}\"")

    # Generate output
    out_str = ", " .join(new_strs)
    print("{" + out_str + "}")
    
