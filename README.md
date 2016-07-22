Lightweight standalone CLI utility to read and manipulate JSON documents in a semantically safe way.

This is mainly meant for use in shell scripts. I'm also planning to write analogous tool for YAML but it wouldn't be as easy and I don't have too much time for this.

The tool is not tested for all possible cases but I try to keep it standard-compliant and bug-free. Please raise an issue for any bugs and misbehaving edge cases, as well as wording and grammatical errors.


**TODO:**
 * Tests tests tests ...
 * Flexible error & warning reporting
 * Unicode tests
 * Implement `merge` action for merging two objects or arrays (according to [rfc7396](https://tools.ietf.org/html/rfc7396)?)
 * Add [rfc6901](https://tools.ietf.org/html/rfc6901) (JSON Pointers) support? (I found this RFC too late and don't really like that)
 * Configurable formatting and/or keeping whitespaces


## Compiling


This should be enough on any sane platform (well, actually, it just works on my machine):

```
gcc main.c -ojson-util
```


## Usage

```
json-util ACTION [ARGUMENTS...]
```

If action requires JSON input, it could be given via `stdin`. If multiple input values are needed (`set`, `splice` and `merge` actions),
they could be concatenated with recommended whitespace between them (for numbers and literal values). In that
case, if any value comes from untrusted source, it is recommended to pass the value through `check` action
so it does not interfere with other values (unterminated objects, string, arrays, ...).
Everything after required number of input values is ignored.

If action outputs JSON, it will be printed to `stdout` as raw JSON value. Objects and arrays will be reformatted
with tab as indentation character (for the sake of simplicity and because I like it that way) but that will probably change
in the future. Duplicate keys are *not* removed.

In case of invalid input (bad JSON, unexpected value type, bad argument, integer overflow), exit code will be `1`. Otherwise
it will be `0` even if action fails for any other reason (e.g. unreferencabe/non-existent value).

Example:
```
$ printf '{"a":{"b":["c"]}} ignored text' | json-util get a.b.0
"c"
```

Currently supported actions:

 * `check`
 
 Check that input contains 1 and exacltly 1 valid JSON value and nothing more. Prints `"ERROR"` to standard output
 if that is *not* the case, otherwise prints nothing. Exit code will be 0 in both cases.

 * `type`
 
 Print type name (`object`, `array`, `string`, `number`, `boolean` or `null`) of input JSON value.

 * `get` *`pathname`*
 
 Get a JSON value at given path. Path is period-delimited sequence of path components. If path component
 contains period (`.`) or backslash (`\`), they must be escaped with backslash(`\`). If any object at path
 contains duplicate key, the *last* key/value will be used. Output will be empty if path cannot be resolved.

 * `keys`

 Expects single JSON object as input and outputs list of keys in object, one per line, encoded as JSON string.
 Keys are listed in same order they appear in input object. No deduplication is done.

 * `set` *`pathname`*
 
 Set JSON value at given path. Accepts 2 JSON input values - first is the document to be modified and
 second (optional) is assigned value. If path cannot be resolved, document will be unmodified. If any object at path
 contains duplicate key, the *last* key/value will be used. Previous key/value pairs won't be affected.
 
 If value pointed by path is an array and index is out of range, the gap will be filled with `null` values.
 
 Property can be deleted by not giving the second value. In this case, if the value pointed by path is an array,
 the behaviour will be the same as assigning `null` value. (Similar to Javascript)
 
 * `splice` *`[index]`* *`[count]`*
 
 Similar to Javascript `Array#splice` method. Accepts array as first input value and inserts other values at given `index`,
 while removing up to `count` old element at given `index`. If `index` is not given or is out of range,
 it will be set to the length of the array. If `count` is not given, it will be set to `0`.

 * `decode-string`
 
 Decode JSON encoded string into UTF-8 encoded byte sequence.

 * `encode-string`
 
 Encode data from `stdin` to be safe for use in JSON string.

 * `encode-key` *`path-component`*
 
 Escape periods (`.`) and backslashes (`\`) with backslash (`\`).

## License

MIT
