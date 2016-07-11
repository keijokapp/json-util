Lightweight standalone CLI utility to read and manipulate JSON documents in semantically safe way.

This is mainly meant for use in shell scripts. I'm also planning to write analogous tool for YAML but it wouldn't be as easy and I don't have too much time for this.

The tool is not tested for all possible cases but I try to keep it standard-compliant and bug-free. Please raise an issue for any bugs and misbehaving edge cases, as well as wording and grammatical errors.


**TODO:**
 * Tests tests tests ...
 * Flexible error & warning reporting
 * Unicode tests
 * Deleting object properties
 * Implement proper help and perhaps verbose mode
 * Implement `splice` action for adding elements to array
 * Implement `slice` action for removing elements from array
 * Implement `merge` action for merging two objects or arrays (according to [rfc7396](https://tools.ietf.org/html/rfc7396)?)
 * Add [rfc6901](https://tools.ietf.org/html/rfc6901) (JSON Pointers) support? (I found this RFC too late and don't really like that)
 * Configurable formatting and/or preserve whitespaces


## Compiling


This should be enough on any sane platform (well, actually, it just works on my machine):

```
gcc main.c -ojson-util
```


## Usage

```
json-util ACTION [ARGUMENT]
```

If action requires JSON input, it could be given through `stdin`. If multiple input values are needed (`set`, `splice` and `merge` actions),
they could be concatenated with recommended whitespace between them (for numbers and literal values). In that
case, if any value comes from untrusted source, it is recommended to pass the value through `check` action
so it does not interfere with other values (unterminated objects, string, arrays, ...).
Everything after required number of input values is ignored.

If action outputs JSON, it will be printed to `stdout` as raw JSON value. Objects and arrays will be reformatted
with tab as indentation character (for the sake of simplicity and because I like it that way) but that will probably change
in the future. Duplicate keys are *not* removed.

Example:
```
$ printf '{"a":{"b":["c"]}} ignored text' | json-util get a.b.0
"c"
```

Currently only these actions are supported:

 * `check`

 Check that input contains 1 and exacltly 1 valid JSON value and nothing more. Prints `"ERROR"` to standard output
 if that is *not* the case, otherwise prints nothing. Exit code will be 0 in both cases.

 * `type`
 
 Print type name (`object`, `array`, `string`, `number`, `boolean` or `null`) of input JSON value.

 * `get` *`pathname`*
 
 Get a JSON value at given path. Path is period-delimited sequence of path components. If path component
 contains period (`.`) or backslash (`\`), they must be escaped with backslash(`\`). If any object at path
 contains duplicate key, the *last* key/value will be used. Output will be empty if path cannot be resolved.

 * `set` *`pathname`*
 
 Set JSON value at given path. Requires 2 JSON input values - first is the document to be modified and
 second is assigned value. If path cannot be resolved, document will be unmodified. If any object at path
 contains duplicate key, the *last* key/value will be used. Previous key/value pairs won't be affected.
 
 It's not currently possible to delete items. However, you can mark items as `null`.

 * `decode-string`
 
 Decode JSON encoded string into UTF-8 encoded byte sequence.

 * `encode-string`
 
 Encode data from `stdin` to be safe for use in JSON string.

 * `encode-key` *`path-component`*
 
 Escape periods (`.`) and backslashes (`\`) with backslash (`\`).

## License

MIT
