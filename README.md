# Custom Memory Allocator (malloc, free, calloc, realloc)

A simplified custom implementation of memory allocation functions in C, developed as part of an academic operating systems course.

## Overview

This project re-implements essential memory allocation functions from the C standard library—`malloc()`, `free()`, `calloc()`, and `realloc()`—without using any built-in allocation mechanisms. It is designed to enhance understanding of how memory is managed at a low level in modern systems.

## Features

- `malloc(size_t size)` – Allocates a memory block of the given size.
- `free(void *ptr)` – Frees a previously allocated memory block.
- `calloc(size_t nmemb, size_t size)` – Allocates zero-initialized memory for an array.
- `realloc(void *ptr, size_t size)` – Changes the size of a previously allocated block.

### Additional Details

- No usage of `malloc()`, `free()`, or similar system/library memory allocators.
- Uses `mmap()`, `munmap()`, and other memory-related system calls to manage memory.
- Implements memory metadata management (e.g., headers, size tracking).
- Optional part (if implemented): Optimizations or advanced features for extra credit.
