//
//  Array.h
//  Fox
//
//  Created by Denver Lacey on 31/8/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#pragma once

#include "typedefs.h"
#include "mem.h"

template<typename T>
class Array {
    size_t _size;
    T *_data;
    
public:
    Array() = default;
    Array(size_t size, T *data) : _size(size), _data(data) {}
    
    inline static Array with_size(size_t size) {
        return Array { size, Mem.allocate<T>(size).as_ptr() };
    }
    
    static Array copy(size_t size, const T *data) {
        T *copy_data = Mem.allocate<T>(size).as_ptr();
        for (size_t i = 0; i < size; i++) {
            copy_data[i] = data[i];
        }
        return Array { size, copy_data };
    }
    
    void free() {
        Mem.deallocate(_data);
        _size = 0;
        _data = nullptr;
    }
    
    inline Array clone() const {
        return copy(_size, _data);
    }
    
public:
    inline T *begin() { return _data; }
    inline T *end() { return _data + _size; }
    inline const T *begin() const { return _data; }
    inline const T *end() const { return _data + _size; }
    inline T *data() { return _data; }
    inline const T *data() const { return _data; }
    inline size_t size() const { return _size; }
    
    inline T &operator[](size_t idx) { return _data[idx]; }
    inline const T &operator[](size_t idx) const { return _data[idx]; }
    
public:
    void add(const T &item) {
        _size++;
        _data = Mem.reallocate<T>(_data, _size);
        _data[_size-1] = item;
    }
    
    void add(T &&item) {
        _size++;
        _data = Mem.reallocate<T>(_data, _size);
        _data[_size-1] = std::forward<T>(item);
    }
    
    size_t reserve(size_t additional) {
        size_t size = _size;
        _size += additional;
        _data = Mem.reallocate<T>(_data, _size);
        return size;
    }
};
