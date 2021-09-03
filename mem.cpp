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
    for (Chunk *b : blocks) {
        free(b);
    }
}

char *String_Allocator::allocate(size_t size) {
    if (current == nullptr) {
        current = allocate_block(size);
    }
    Chunk *free_chunk = current;
    size_t n = allign_size(size) / sizeof(Chunk);
    for (size_t i = 0; i < n; i++) {
      current = current->next;
    }
    return (char *)free_chunk;
}

char *String_Allocator::duplicate(const char *s) {
    return duplicate(s, strlen(s));
}

char *String_Allocator::duplicate(const char *s, size_t size) {
    char *_s = allocate(size);
    memcpy(_s, s, size);
    _s[size] = 0;
    return _s;
}

void String_Allocator::deallocate(char *s) {
    deallocate(s, strlen(s));
}

void String_Allocator::deallocate(char *s, size_t size) {
    Chunk *new_current = (Chunk *)s;
    Chunk *chunk = new_current;
    size_t alligned_size = allign_size(size);
    
    for (size_t i = 0; i < alligned_size - sizeof(Chunk); i += sizeof(Chunk)) {
        chunk->next = (Chunk *)(((char *)chunk) + sizeof(Chunk));
        chunk = chunk->next;
    }
    
    chunk->next = current;
    current = chunk;
}
    
String_Allocator::Chunk *String_Allocator::allocate_block(size_t size) {
    size_t alligned_size = allign_size(size);
    size_t block_size = Minimum_Allocation_Size;
    if (block_size < alligned_size) block_size = alligned_size;
    
    Chunk *block_begin = (Chunk *)malloc(block_size);
    blocks.push_back(block_begin);
    
    Chunk *chunk = block_begin;
    for (size_t i = 0; i < block_size - sizeof(Chunk); i += sizeof(Chunk)) {
      chunk->next = (Chunk *)(((char *)chunk) + sizeof(Chunk));
      chunk = chunk->next;
    }
    chunk->next = nullptr;
    
    return block_begin;
}

size_t String_Allocator::allign_size(size_t size) {
    return (((size + sizeof(Chunk) - 1)) / sizeof(Chunk)) * sizeof(Chunk);
}

Mem_Allocator::~Mem_Allocator() {
    deallocate();
}

void *Mem_Allocator::allocate(size_t size) {
    if (current < size) {
        allocate_bucket(size);
    }
    Bucket b = current_bucket();
    current -= size;
    return &b[current];
}

void Mem_Allocator::deallocate() {
    for (Bucket b : buckets) {
        free(b);
    }
    buckets.clear();
}

Mem_Allocator::Bucket Mem_Allocator::current_bucket() {
    return buckets.front();
}

void Mem_Allocator::allocate_bucket(size_t size) {
    size_t alloc_size = Minimum_Size;
    if (alloc_size < size) alloc_size = size;
    Bucket bucket = (Bucket)malloc(alloc_size);
    buckets.push_front(bucket);
    current = alloc_size;
}
