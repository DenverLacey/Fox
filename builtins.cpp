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

void builtin_putb(Stack &stack, Address arg_start) {
    runtime::Bool value = stack.pop<runtime::Bool>();
    printf("%s", value ? "true" : "false");
}

void builtin_putc(Stack &stack, Address arg_start) {
    runtime::Char value = stack.pop<runtime::Char>();
    auto utf_value = utf8char_t::from_char32(value);
    printf("%s", utf_value.buf);
}

void builtin_putd(Stack &stack, Address arg_start) {
    runtime::Int value = stack.pop<runtime::Int>();
    printf("%lld", value);
}

void builtin_putf(Stack &stack, Address arg_start) {
    runtime::Float value = stack.pop<runtime::Float>();
    printf("%f", value);
}

void builtin_puts(Stack &stack, Address arg_start) {
    runtime::String value = stack.pop<runtime::String>();
    printf("%.*s", value.len, value.s);
}

void builtin_printb(Stack &stack, Address arg_start) {
    builtin_putb(stack, arg_start);
    printf("\n");
}

void builtin_printc(Stack &stack, Address arg_start) {
    builtin_putc(stack, arg_start);
    printf("\n");
}

void builtin_printd(Stack &stack, Address arg_start) {
    builtin_putd(stack, arg_start);
    printf("\n");
}

void builtin_printf(Stack &stack, Address arg_start) {
    builtin_putf(stack, arg_start);
    printf("\n");
}

void builtin_prints(Stack &stack, Address arg_start) {
    builtin_puts(stack, arg_start);
    printf("\n");
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

    interp->builtins.add_builtin("putb", {
        builtin_putb,
        value_types::func(value_types::Void, value_types::Bool)
    });
    
    interp->builtins.add_builtin("putc", {
        builtin_putc,
        value_types::func(value_types::Void, value_types::Char)
    });
    
    interp->builtins.add_builtin("putd", {
        builtin_putd,
        value_types::func(value_types::Void, value_types::Int)
    });
    
    interp->builtins.add_builtin("putf", {
        builtin_putf,
        value_types::func(value_types::Void, value_types::Float)
    });
    
    interp->builtins.add_builtin("puts", {
        builtin_puts,
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
