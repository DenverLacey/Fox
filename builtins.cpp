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

void builtin_alloc(Stack &stack, Address arg_start) {
    runtime::Int size = stack.pop<runtime::Int>();
    runtime::Pointer allocation = malloc(size);
    stack.push(allocation);
}

void builtin_free(Stack &stack, Address arg_start) {
    runtime::Pointer pointer = stack.pop<runtime::Pointer>();
    free(pointer);
}

void builtin_panic(Stack &stack, Address arg_start) {
    runtime::String err = stack.pop<runtime::String>();
    printf("Panic! %.*s\n", err.len, err.s);
    exit(EXIT_FAILURE);
}

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
    interp->builtins.add_builtin("alloc", {
        builtin_alloc,
        value_types::func(value_types::ptr_to(const_cast<Value_Type *>(&value_types::Void)), value_types::Int)
    });
    
    interp->builtins.add_builtin("free", {
        builtin_free,
        value_types::func(value_types::Void, value_types::ptr_to(const_cast<Value_Type *>(&value_types::Void)))
    });
    
    interp->builtins.add_builtin("panic", {
        builtin_panic,
        value_types::func(value_types::Void, value_types::Str)
    });
    
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
