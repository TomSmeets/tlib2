# Tom's Awesome C Library 2.0 (Not a framework)

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


Ideas:
- no sub dirs, only single files, but generate a nice table like below
- To use Just copy those files where you want

## Contents

| Folder               | Description                                       | Depends          |
|----------------------|---------------------------------------------------|------------------|
| [core](./src/core)   | Basic types, OS abstraction and memory allocation |                  |
| [std](./src/std)     | Advanced functions                                | core             |
| [elf](./src/elf)     | ELF and DWARF file parsing                        | core, std        |
| [build](./src/build) | Helpers for writing a build system                | core, std, elf   |
| [app](./src/app)     | Example applications                              | \* |


TODO: Generate this tabe based on file headers


## Core

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

## Std

| File     | Description                                  | Depends  |
|----------|----------------------------------------------|----------|
| fmt      | Text Formatter                               | mem      |

## ELF

| File     | Description                                  | Depends  |
|----------|----------------------------------------------|----------|
| elf      | ELF file parser                              | os       |
| hot      | Hot reloading building blocks                | elf      |
| dwarf    | DWARF Debug symbol parser                    | elf      |
