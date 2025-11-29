# Tlibc 2.0

NOTE: This is not a framework!

- This is not a framework
- Should work with both Clang and Gcc
- Most basic entry point possible
- No 'Global' let 'hot' handle that
- Decreaese dependency depth
- So just c strings most of the time
- Mem should support any size



# Memory
- os_alloc will do it's best to allocate sequential chunkcs

AAABBB..CC.D.E..FF

block(next, size)
