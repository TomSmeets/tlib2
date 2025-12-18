# Tom's Awesome C Library 2.0 (Not a framework)

## Usage
- Use this library by copying only the '.h' files you use into your project, or alternatively add this project as a submodule.
- When copying files, you could implement a two way sync using 'unison'

## Rewrite

Why rewrite?
- Starting from scratch helps with new approaches.
- Keep The good ideas, skip the bad ideas
- Each tlib rewrite was better than the last.

What is different?
- This is not a framework!
- Should work with both Clang and Gcc
- Most basic entry point possible
- No single 'Global' type, let 'hot' handle that
- Decrease dependency depth
- Keep C code simple, so just C strings where possible
- Memory allocation should support ANY size
- Fully unit test
- Adding a new program should be no effort
- Thread safe

What stays the same
- No 'const', such as in 'const char *' does not have a lot of added value.

Ideas:
- no sub dirs, only single files, but generate a nice table like below
- To use Just copy those files where you want


Constness:
- const char *

## Contents

TODO: Generate this table based on file headers


| File     | Description                                  | Depends  |
|----------|----------------------------------------------|----------|
| type     | Basic Types                                  | -        |
| str      | String helpers                               | type     |
| linux    | Linux syscalls and Types                     | type     |
| os_api   | Basic interface for talking to an OS         | type     |
| os_linux | OS API implementation for Linux              | os_api   |
| os       | OS API implementations                       | os_linux |
| chunk    | Memory allocator for big power-of-two chunks | os       |
| mem      | Memory allocator                             | chunk    |
| fmt      | Text Formatter                               | mem      |
| elf      | ELF file parser                              | os       |
| hot      | Hot reloading building blocks                | elf      |
| dwarf    | DWARF Debug symbol parser                    | elf      |
