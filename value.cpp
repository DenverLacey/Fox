//
//  value.cpp
//  Fox
//
//  Created by Denver Lacey on 8/7/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#include "value.h"

#include <iostream>
#include <sstream>
#include <assert.h>

char *Value_Type::debug_str() const {
    std::ostringstream s;
    if (is_mut) {
        s << "mut ";
    }
    
    switch (kind) {
        case Value_Type_Kind::Unresolved_Type:
            s << "<?>";
            break;
        case Value_Type_Kind::None:
            s << "<NONE>";
            break;
        case Value_Type_Kind::Void:
            s << "void";
            break;
        case Value_Type_Kind::Bool:
            s << "bool";
            break;
        case Value_Type_Kind::Char:
            s << "char";
            break;
        case Value_Type_Kind::Int:
            s << "int";
            break;
        case Value_Type_Kind::Float:
            s << "float";
            break;
        case Value_Type_Kind::Str:
            s << "str";
            break;
        case Value_Type_Kind::Ptr:
            s << "*";
            s << data.ptr.subtype->debug_str();
            break;
        case Value_Type_Kind::Struct:
            assert(false);
            break;
        case Value_Type_Kind::Enum:
            assert(false);
            break;
    }
    
    char *debug_str = strndup(s.str().c_str(), s.str().size()); // @Leak
    return debug_str;
}

bool operator==(const Value_Type &a, const Value_Type &b) {
    if (a.kind != b.kind) {
        return false;
    }
    
    switch (a.kind) {
        case Value_Type_Kind::Ptr:
            return *a.data.ptr.subtype == *b.data.ptr.subtype;
        case Value_Type_Kind::Struct:
            assert(false);
        case Value_Type_Kind::Enum:
            assert(false);
    }
    
    return true;
}
