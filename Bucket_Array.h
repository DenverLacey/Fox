//
//  Bucket_Array.h
//  Fox
//
//  Created by Denver Lacey on 20/9/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#pragma once

#include <assert.h>

template<typename T>
class Bucket_Array {
public:
    static constexpr size_t Bucket_Size = 32;
    
private:
    struct Bucket {
        T data[Bucket_Size];
    };
    
    Bucket *head = nullptr;
    size_t next = Bucket_Size + 1;
    std::vector<Bucket *> buckets;
    
public:
    Bucket_Array() = default;
    Bucket_Array(const Bucket_Array<T> &) = delete;
    Bucket_Array(Bucket_Array<T> &&) = default;
    
public:
    inline T &operator[](size_t idx) {
        size_t bucket_idx = idx / Bucket_Size;
        size_t item_idx = idx % Bucket_Size;
        return buckets[bucket_idx]->data[item_idx];
    }
    
    inline const T &operator[](size_t idx) const {
        size_t bucket_idx = idx / Bucket_Size;
        size_t item_idx = idx % Bucket_Size;
        return buckets[bucket_idx].data[item_idx];
    }
    
    inline size_t size() const {
        return (buckets.size() - 1) * Bucket_Size + next;
    }
    
public:
    void add(const T &item) {
        if (next > Bucket_Size) {
            allocate_new_bucket();
        }
        head->data[next++] = item;
    }
    
    void add(T &&item) {
        if (next > Bucket_Size) {
            allocate_new_bucket();
        }
        head->data[next++] = std::forward<T>(item);
    }
    
    T &last() {
        assert(next > 0);
        return head->data[next - 1];
    }
    
    const T &last() const {
        assert(next > 0);
        return head->data[next - 1];
    }
    
private:
    void allocate_new_bucket() {
        Bucket *new_bucket = new Bucket;
        buckets.push_back(new_bucket);
        next = 0;
        head = new_bucket;
    }
};
