# Tom's Awesome C Library 2.0 (Not a framework)

Why rewrite?
- Starting from scratch helps with new approaches.
- Keep The good ideas, skip the bad ideas
- Each tlibc rewrite was better than the last.

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
