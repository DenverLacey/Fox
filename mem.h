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
