//
//  definitions.cpp
//  Fox
//
//  Created by Denver Lacey on 17/10/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#include "definitions.h"
#include "vm.h"

bool Struct_Definition::has_field(String id) {
    for (auto &f : fields) {
        if (f.id == id) {
            return true;
        }
    }
    return false;
}

Struct_Field *Struct_Definition::find_field(String id) {
    for (auto &f : fields) {
        if (f.id == id) {
            return &f;
        }
    }
    return nullptr;
}

bool Struct_Definition::has_method(String id) {
    auto sid = id.str();
    auto it = methods.find(sid);
    return it != methods.end();
}

bool Struct_Definition::find_method(String id, Method &out_method) {
    auto sid = id.str();
    auto it = methods.find(sid);
    if (it != methods.end()) {
        out_method = it->second;
        return true;
    }
    return false;
}

Enum_Variant *Enum_Definition::find_variant(String id) {
    for (auto &v : variants) {
        if (v.id == id) {
            return &v;
        }
    }
    return nullptr;
}

Enum_Variant *Enum_Definition::find_variant_by_tag(runtime::Int tag) {
    for (auto &v : variants) {
        if (v.tag == tag) {
            return &v;
        }
    }
    return nullptr;
}

bool Enum_Definition::has_method(String id) {
    auto sid = id.str();
    auto it = methods.find(sid);
    return it != methods.end();
}

bool Enum_Definition::find_method(String id, Method &out_method) {
    auto sid = id.str();
    auto it = methods.find(sid);
    if (it != methods.end()) {
        out_method = it->second;
        return true;
    }
    return false;
}
