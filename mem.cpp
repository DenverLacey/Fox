//
//  mem.cpp
//  Fox
//
//  Created by Denver Lacey on 3/9/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#include "mem.h"
#include <assert.h>

String_Allocator::~String_Allocator() {
    clear();
}

char *String_Allocator::allocate(size_t size) {
    if (current < size) {
        allocate_chunk(size);
    }
    Chunk c = current_chunk();
    current -= size;
    return &c[current];
}

char *String_Allocator::duplicate(const char *s) {
    return duplicate(s, strlen(s));
}

char *String_Allocator::duplicate(const char *s, size_t size) {
    char *_s = allocate(size + 1);
    memcpy(_s, s, size);
    _s[size] = 0;
    return _s;
}

bool String_Allocator::deallocate(char *s) {
    return deallocate(s, strlen(s));
}

bool String_Allocator::deallocate(char *s, size_t size) {
    if (s != &current_chunk()[current]) {
        return false;
    }
    current += size + 1; // plus one for null-terminator
    return true;
}

void String_Allocator::clear() {
    for (Chunk b : blocks) {
        free(b);
    }
    blocks.clear();
}
    
void String_Allocator::allocate_chunk(size_t size) {
    size_t alloc_size = Minimum_Chunk_Size;
    if (alloc_size < size) alloc_size = size;
    Chunk chunk = (Chunk)malloc(alloc_size);
    blocks.push_front(chunk);
    current = alloc_size;
}

String_Allocator::Chunk String_Allocator::current_chunk() {
    return blocks.front();
}

Mem_Allocator::~Mem_Allocator() {
    clear();
}

void *Mem_Allocator::allocate(size_t size) {
    if (current < size) {
        allocate_bucket(size);
    }
    Bucket b = current_bucket();
    current -= size;
    return &b[current];
}

void Mem_Allocator::clear() {
    for (Bucket b : buckets) {
        free(b);
    }
    buckets.clear();
}

Mem_Allocator::Bucket Mem_Allocator::current_bucket() {
    return buckets.front();
}

void Mem_Allocator::allocate_bucket(size_t size) {
    size_t alloc_size = Minimum_Bucket_Size;
    if (alloc_size < size) alloc_size = size;
    Bucket bucket = (Bucket)malloc(alloc_size);
    buckets.push_front(bucket);
    current = alloc_size;
}
