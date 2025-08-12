# Proof-of-concept: a zero-copy, resizeable vector.

This code demonstrates a silly idea: Linux hides the physical address from user
space and 'hands out' memory in pages. Internally, it keeps something akin to a
loop-up table that translates virtual address to physical address.

The realization is that a "contiguous vector", e.g. std::vector, is not
contiguous in physical address space. At every page table it jumps.

Hence, there's no strict requirement to allocate a completely new array
and copy everything across.

## Compile

This is a simplistic PoC and uses a Makefile:

    make && ./mmap_vector

## Why is it silly?

It breaks tools like Valgrind. Hence, it's unlikely to ever be worth the cost,
but it's entertaining.
