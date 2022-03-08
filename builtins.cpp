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
#include "error.h"

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

void builtin_puts_bool(Stack &stack, Address arg_start) {
    runtime::Bool value = stack.pop<runtime::Bool>();
    printf("%s", value ? "true" : "false");
}

void builtin_puts_char(Stack &stack, Address arg_start) {
    runtime::Char value = stack.pop<runtime::Char>();
    auto utf_value = utf8char_t::from_char32(value);
    printf("%s", utf_value.buf);
}

void builtin_puts_int(Stack &stack, Address arg_start) {
    runtime::Int value = stack.pop<runtime::Int>();
    printf("%lld", value);
}

void builtin_puts_float(Stack &stack, Address arg_start) {
    runtime::Float value = stack.pop<runtime::Float>();
    printf("%f", value);
}

void builtin_puts_str(Stack &stack, Address arg_start) {
    runtime::String value = stack.pop<runtime::String>();
    printf("%.*s", value.len, value.s);
}

void builtin_puts_struct(Stack &stack, Address arg_start) {
    auto defn = stack.pop<Struct_Definition *>();
    auto data = stack.pop(defn->size);

    printf("(value of struct %.*s at %p)", defn->name.size(), defn->name.c_str(), data);
}

void builtin_puts_enum(Stack &stack, Address arg_start) {
    auto defn = stack.pop<Enum_Definition *>();
    auto data = stack.pop(defn->size);

    if (defn->is_sumtype) {
        printf("(value of enum %.*s at %p)", defn->name.size(), defn->name.c_str(), data);
    } else {
        runtime::Int tag = *reinterpret_cast<runtime::Int *>(data);
        Enum_Variant *var = defn->find_variant_by_tag(tag);
        internal_verify(var, "Invalid tag value for %s: %lld.", defn->name.c_str(), tag);

        printf("%.*s", var->id.size(), var->id.c_str());
    }
}

void builtin_print_bool(Stack &stack, Address arg_start) {
    builtin_puts_bool(stack, arg_start);
    printf("\n");
}

void builtin_print_char(Stack &stack, Address arg_start) {
    builtin_puts_char(stack, arg_start);
    printf("\n");
}

void builtin_print_int(Stack &stack, Address arg_start) {
    builtin_puts_int(stack, arg_start);
    printf("\n");
}

void builtin_print_float(Stack &stack, Address arg_start) {
    builtin_puts_float(stack, arg_start);
    printf("\n");
}

void builtin_print_str(Stack &stack, Address arg_start) {
    builtin_puts_str(stack, arg_start);
    printf("\n");
}

void builtin_print_struct(Stack &stack, Address arg_start) {
    builtin_puts_struct(stack, arg_start);
    printf("\n");
}

void builtin_print_enum(Stack &stack, Address arg_start) {
    builtin_puts_enum(stack, arg_start);
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

    interp->builtins.add_builtin("<puts-bool>", {
        builtin_puts_bool,
        value_types::func(value_types::Void, value_types::Bool)
    });
    
    interp->builtins.add_builtin("<puts-char>", {
        builtin_puts_char,
        value_types::func(value_types::Void, value_types::Char)
    });
    
    interp->builtins.add_builtin("<puts-int>", {
        builtin_puts_int,
        value_types::func(value_types::Void, value_types::Int)
    });
    
    interp->builtins.add_builtin("<puts-float>", {
        builtin_puts_float,
        value_types::func(value_types::Void, value_types::Float)
    });
    
    interp->builtins.add_builtin("<puts-str>", {
        builtin_puts_str,
        value_types::func(value_types::Void, value_types::Str)
    });

    interp->builtins.add_builtin("<puts-struct>", {
        builtin_puts_struct,
        value_types::func(value_types::Void, Value_Type{ Value_Type_Kind::Struct })
    });

    interp->builtins.add_builtin("<puts-enum>", {
        builtin_puts_enum,
        value_types::func(value_types::Void, Value_Type{ Value_Type_Kind::Enum })
    });
    
    interp->builtins.add_builtin("<print-bool>", {
        builtin_print_bool,
        value_types::func(value_types::Void, value_types::Bool)
    });
    
    interp->builtins.add_builtin("<print-char>", {
        builtin_print_char,
        value_types::func(value_types::Void, value_types::Char)
    });
    
    interp->builtins.add_builtin("<print-int>", {
        builtin_print_int,
        value_types::func(value_types::Void, value_types::Int)
    });
    
    interp->builtins.add_builtin("<print-float>", {
        builtin_print_float,
        value_types::func(value_types::Void, value_types::Float)
    });
    
    interp->builtins.add_builtin("<print-str>", {
        builtin_print_str,
        value_types::func(value_types::Void, value_types::Str)
    });

    interp->builtins.add_builtin("<print-struct>", {
        builtin_print_struct,
        value_types::func(value_types::Void, Value_Type{ Value_Type_Kind::Struct })
    });

    interp->builtins.add_builtin("<print-enum>", {
        builtin_print_enum,
        value_types::func(value_types::Void, Value_Type{ Value_Type_Kind::Enum })
    });
}
