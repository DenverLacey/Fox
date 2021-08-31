//
//  mem.h
//  Fox
//
//  Created by Denver Lacey on 7/7/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#pragma once

#include <memory>

template<typename T>
using Ref = std::unique_ptr<T>;

template<typename T, typename ...Args>
Ref<T> make(Args &&...args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename T>
Ref<T> take(T *raw_ptr) {
    return std::unique_ptr<T>(raw_ptr);
}

template<typename To, typename From>
Ref<To> cast(Ref<From> &from) {
    To *p = dynamic_cast<To *>(from.get());
    if (p) from.release();
    return take(p);
}

template<typename To, typename From>
Ref<To> cast(Ref<From> &&from) {
    return cast<To>(from);
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
