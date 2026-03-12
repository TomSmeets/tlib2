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

- app: Small single source file applications
- core: Core functionality
- elf: Reading/writing elf files and Dwarf
- gfx: 2d and 3d graphics and game engine
- snake: A sample snake game
