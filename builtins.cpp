//
//  intrinsics.cpp
//  Fox
//
//  Created by Denver Lacey on 16/10/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#include "builtins.h"

#include "interpreter.h"
#include "vm.h"

void builtin_printb(Stack &stack, Address arg_start) {
    runtime::Bool value = stack.pop<runtime::Bool>();
    printf("%s\n", value ? "true" : "false");
}

void builtin_printc(Stack &stack, Address arg_start) {
    runtime::Char value = stack.pop<runtime::Char>();
    auto utf_value = utf8char_t::from_char32(value);
    printf("%s\n", utf_value.buf);
}

void builtin_printd(Stack &stack, Address arg_start) {
    runtime::Int value = stack.pop<runtime::Int>();
    printf("%lld\n", value);
}

void builtin_printf(Stack &stack, Address arg_start) {
    runtime::Float value = stack.pop<runtime::Float>();
    printf("%f\n", value);
}

void builtin_prints(Stack &stack, Address arg_start) {
    runtime::String value = stack.pop<runtime::String>();
    printf("%.*s\n", value.len, value.s);
}

void load_builtins(Interpreter *interp) {
    interp->builtins.add_builtin("printb", {
        builtin_printb,
        value_types::func(value_types::Void, value_types::Bool)
    });
    
    interp->builtins.add_builtin("printc", {
        builtin_printc,
        value_types::func(value_types::Void, value_types::Char)
    });
    
    interp->builtins.add_builtin("printd", {
        builtin_printd,
        value_types::func(value_types::Void, value_types::Int)
    });
    
    interp->builtins.add_builtin("printf", {
        builtin_printf,
        value_types::func(value_types::Void, value_types::Float)
    });
    
    interp->builtins.add_builtin("prints", {
        builtin_prints,
        value_types::func(value_types::Void, value_types::Str)
    });
}
