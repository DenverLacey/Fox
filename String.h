//
//  String.h
//  Fox
//
//  Created by Denver Lacey on 7/7/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#pragma once
#include "typedefs.h"

class String {
    char *_data;
    size_t _size;
    
public:
    String() = default;
    String(char *data, size_t size);
    String(char *data);
    
    static String with_size(size_t size);
    static String copy(const char *data);
    static String copy(const char *data, size_t size);
    void free();
    String clone() const;
    
public:
    char *begin();
    char *end();
    
    char *c_str();
    const char *c_str() const;
    size_t size() const;
    size_t len() const;
    
public:
    bool operator==(const String &other) const;
    bool operator!=(const String &other) const;
    bool operator==(const char *other) const;
    bool operator!=(const char *other) const;
};
