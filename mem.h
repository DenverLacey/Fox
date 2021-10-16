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
#include <forward_list>

template<typename T>
class Ref {
    T *ptr;
    
    template<typename>
    friend class Ref;
    
public:
    Ref() : ptr(nullptr) {}
    Ref(std::nullptr_t) : ptr(nullptr) {}
    Ref(const Ref<T> &other) = default;
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
    T &      operator*()       { return *ptr; }
    const T &operator*() const { return *ptr; }
    
    T *      operator->()       { return ptr; }
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
    
    T *      as_ptr()       { return ptr; }
    const T *as_ptr() const { return ptr; }
};

class String_Allocator {
    static constexpr size_t Minimum_Chunk_Size = 1024;
    using Chunk = char *;
    
    size_t current;
    std::forward_list<Chunk> blocks;
    
public:
    String_Allocator() = default;
    ~String_Allocator();
    String_Allocator(const String_Allocator &) = delete;
    String_Allocator(String_Allocator &&) = delete;
    
public:
    char *allocate(size_t size);
    char *duplicate(const char *s);
    char *duplicate(const char *s, size_t size);
    bool deallocate(char *s);
    bool deallocate(char *s, size_t size);
    void clear();
    
private:
    void allocate_chunk(size_t size);
    Chunk current_chunk();
};

inline String_Allocator SMem{};

class Mem_Allocator {
    static constexpr size_t Minimum_Bucket_Size = 1024;
    using Bucket = uint8_t *;
    
    uint8_t *current;
    uint8_t *previous;
    uint8_t *end_of_current_bucket;
    std::forward_list<Bucket> buckets;
    
public:
    Mem_Allocator() = default;
    ~Mem_Allocator();
    Mem_Allocator(const Mem_Allocator &) = delete;
    Mem_Allocator(Mem_Allocator &&) = delete;
    
public:
    void clear();
    
    template<typename T>
    Ref<T> allocate(size_t n) {
        return Ref<T>((T *)allocate(n * sizeof(T)));
    }
    
    template<typename T, typename ...Args>
    Ref<T> make(Args &&...args) {
        Ref<T> r = allocate<T>(1);
        new(r.as_ptr()) T { std::forward<Args>(args)... };
        return r;
    }
    
    template<typename T>
    Ref<T> reallocate(Ref<T> ref, size_t new_n) {
        return Ref<T>((T *)reallocate(ref.as_ptr(), new_n * sizeof(T)));
    }
    
    template<typename T>
    bool deallocate(Ref<T> ref) {
        return deallocate(ref.as_ptr());
    }
    
private:
    void *allocate(size_t size);
    void *reallocate(void *ptr, size_t new_size);
    bool deallocate(void *ptr);
    void allocate_bucket(size_t size);
};

inline Mem_Allocator Mem{};

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

template<typename Head, typename ...Tail>
struct head {
    using type = Head;
};

template<typename T>
void unpack(T *buffer) {}

template<typename T>
void unpack(T *buffer, T head) {
    *buffer = head;
}

template<typename T, typename ...Ts>
void unpack(T *buffer, T head, Ts ...tail) {
    *buffer = head;
    unpack(buffer + 1, tail...);
}

template<typename ...Ts>
auto unpack(Ts ...elements) {
    size_t num_elements = sizeof...(Ts);
    using T = typename std::remove_reference_t<typename head<Ts...>::type>;
    
    T *buffer = Mem.allocate<T>(num_elements).as_ptr();
    unpack(buffer, elements...);
    
    return buffer;
}
