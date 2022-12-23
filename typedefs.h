//
//  typedefs.h
//  Fox
//
//  Created by Denver Lacey on 7/7/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#pragma once

#include <stddef.h>
#include <stdint.h>

using Size = uint16_t;
using Address = uint16_t;
using UUID = uint64_t;

struct utf8char_t {
    char buf[5]; // 5 for null-terminator
    static utf8char_t from_char32(char32_t c);
};
