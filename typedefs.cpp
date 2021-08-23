//
//  typedefs.cpp
//  Fox
//
//  Created by Denver Lacey on 7/7/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#include "typedefs.h"
#include "utfcpp/utf8.h"

utf8char_t utf8char_t::from_char32(char32_t c) {
    utf8char_t _c{0};
    utf8::append(c, _c.buf);
    return _c;
}
