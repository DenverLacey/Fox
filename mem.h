//
//  mem.h
//  Fox
//
//  Created by Denver Lacey on 7/7/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#pragma once

#include <memory>
#include <type_traits>
#include <vector>

template<typename T>
class Ref {
    T *ptr;
    
    template<typename>
    friend class Ref;
    
public:
    Ref() : ptr(nullptr) {}
    Ref(std::nullptr_t) : ptr(nullptr) {}
    Ref(const Ref<T> &other) : ptr(other.ptr) {}
    Ref(Ref<T> &&other) : ptr(std::exchange(other.ptr, nullptr)) {}
    
    template<typename Derived>
    Ref(Derived *ptr) : ptr(ptr) {
        static_assert(std::is_base_of_v<T, Derived>);
    }
    
    Ref<T> &operator=(T *ptr) {
        this->ptr = ptr;
        return *this;
    }
    
    Ref<T> &operator=(const Ref<T> &other) {
        ptr = other.ptr;
        return *this;
    }
    
    Ref<T> &operator=(Ref<T> &&other) {
        ptr = std::exchange(other.ptr, nullptr);
        return *this;
    }
    
    template<typename Derived>
    Ref<T> &operator=(Derived *ptr) {
        static_assert(std::is_base_of_v<T, Derived>);
        this->ptr = ptr;
        return *this;
    }
    
    template<typename Derived>
    Ref<T> &operator=(const Ref<Derived> &other) {
        static_assert(std::is_base_of_v<T, Derived>);
        ptr = other.ptr;
        return *this;
    }
    
    template<typename Derived>
    Ref<T> &operator=(Ref<Derived> &&other) {
        static_assert(std::is_base_of_v<T, Derived>);
        ptr = std::exchange(other.ptr, nullptr);
        return *this;
    }
    
public:
    T &operator*() { return *ptr; }
    const T &operator*() const { return *ptr; }
    T *operator->() { return ptr; }
    const T *operator->() const { return ptr; }
    
    operator bool() const { return ptr != nullptr; }
    
    template<typename U>
    operator Ref<U>() {
        static_assert(std::is_base_of_v<T, U> || std::is_base_of_v<U, T>);
        return Ref<U>(ptr);
    }
    
    template<typename U>
    operator const Ref<U>() const {
        static_assert(std::is_base_of_v<T, U> || std::is_base_of_v<U, T>);
        return Ref<U>(ptr);
    }
    
public:
    template<typename U>
    Ref<U> cast() {
        U *p = dynamic_cast<U *>(ptr);
        return Ref<U>(p);
    }
    
    template<typename U>
    const Ref<U> cast() const {
        U *p = dynamic_cast<U *>(ptr);
        return Ref<U>(p);
    }
    
    T *raw() {
        return ptr;
    }
    
    const T *raw() const {
        return ptr;
    }
};

//
// @INCOMPLETE:
//      Eventually this will be a method on an allocator instead of just 'new'ing.
//
template<typename T, typename ...Args>
Ref<T> make(Args &&...args) {
    T *p = new T{ args... };
    return Ref<T>(p);
}

template<typename = void> struct remove_ref;
template<typename T> struct remove_ref<Ref<T>> { using type = T; };
template<typename T> struct remove_ref<T*> { using type = T; };

template<typename C>
auto flatten(const C &container) {
    using R = typename std::remove_reference<
                            decltype(std::declval<C>()[size_t{}])
                        >::type;
    using T = typename remove_ref<R>::type;
    T *buffer = new T[container.size()];
    for (size_t i = 0; i < container.size(); i++) {
        buffer[i] = *container[i];
    }
    return buffer;
}

template<typename C, typename F>
auto flatten(const C &container, F producer) {
    using T = decltype(producer(container, size_t{}));
    T *buffer = new T[container.size()];
    for (size_t i = 0; i < container.size(); i++) {
        buffer[i] = producer(container, i);
    }
    return buffer;
}

class String_Allocator {
    struct Chunk {
        Chunk *next;
    };
    
    static constexpr size_t Minimum_Allocation_Size = 32 * sizeof(Chunk);
    
    Chunk *current;
    std::vector<Chunk *> blocks;
    
public:
    String_Allocator() = default;
    ~String_Allocator();
    String_Allocator(const String_Allocator&) = delete;
    String_Allocator(String_Allocator&&) = delete;
    
public:
    char *allocate(size_t size);
    char *duplicate(const char *s);
    char *duplicate(const char *s, size_t size);
    void deallocate(char *s);
    void deallocate(char *s, size_t size);
    
private:
    Chunk *allocate_block(size_t size);
    size_t allign_size(size_t size);
};

inline String_Allocator SA{};
