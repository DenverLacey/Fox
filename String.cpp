//
//  String.cpp
//  Fox
//
//  Created by Denver Lacey on 7/7/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#include "String.h"
#include <cstddef>
#include <string.h>
#include <stdlib.h>

#include "utfcpp/utf8.h"
#include "mem.h"

String::String(char *data, size_t size)
    : _data(data), _size(size)
{
}

String::String(char *data)
    : _data(data), _size(strlen(data))
{
}

String String::with_size(size_t size) {
    return { SMem.allocate(size), size };
}

String String::copy(const char *data) {
    return { SMem.duplicate(data), strlen(data) };
}

String String::copy(const char *data, size_t size) {
    return { SMem.duplicate(data, size), size };
}

void String::free() {
    SMem.deallocate(_data, _size);
}

String String::clone() const {
    return copy(_data, _size);
}

char *String::begin() {
    return _data;
}

char *String::end() {
    return _data + _size;
}

char *String::c_str() {
    return _data;
}

char *String::c_str() const {
    return _data;
}

size_t String::size() const {
    return _size;
}

size_t String::len() const {
    auto begin = _data;
    auto end = _data + _size;
    return utf8::distance(begin, end);
}

bool String::operator==(const String &other) const {
    return _size == other._size && memcmp(_data, other._data, _size) == 0;
}

bool String::operator!=(const String &other) const {
    return !(*this == other);
}

bool String::operator==(const char *other) const {
    return _size == strlen(other) && memcmp(_data, other, _size) == 0;
}

bool String::operator!=(const char *other) const {
    return !(*this == other);
}
