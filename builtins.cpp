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

void print_struct(Struct_Definition *defn, void *ptr);
void print_enum(Enum_Definition *defn, void *ptr);

void builtin_alloc(Stack &stack, Address arg_start) {
    runtime::Int size = stack.pop<runtime::Int>();
    runtime::Pointer allocation = malloc(size);
    stack.push(allocation);
}

void builtin_free_pointer(Stack &stack, Address arg_start) {
    runtime::Pointer pointer = stack.pop<runtime::Pointer>();
    free(pointer);
}

void builtin_free_slice(Stack &stack, Address arg_start) {
    runtime::Slice slice = stack.pop<runtime::Slice>();
    free(slice.data);
}

void builtin_free_str(Stack &stack, Address arg_start) {
    runtime::String str = stack.pop<runtime::String>();
    free(str.s);
}

void builtin_panic(Stack &stack, Address arg_start) {
    runtime::String err = stack.pop<runtime::String>();
    printf("Panic! %.*s\n", err.len, err.s);
    exit(EXIT_FAILURE);
}

void print_byte(runtime::Byte value) {
    printf("%d", value);
}

void print_bool(runtime::Bool value) {
    printf("%s", value ? "true" : "false");
}

void print_char(runtime::Char value) {
    auto utf_value = utf8char_t::from_char32(value);
    printf("%s", utf_value.buf);
}

void print_int(runtime::Int value) {
    printf("%lld", value);
}

void print_float(runtime::Float value) {
    printf("%f", value);
}

void print_str(runtime::String value) {
    printf("%.*s", value.len, value.s);
}

void print_value(Value_Type type, void *ptr) {
    switch (type.kind) {
        case Value_Type_Kind::Byte:
            print_byte(*reinterpret_cast<runtime::Byte *>(ptr));
            break;
        case Value_Type_Kind::Bool:
            print_bool(*reinterpret_cast<runtime::Bool *>(ptr));
            break;
        case Value_Type_Kind::Char:
            print_char(*reinterpret_cast<runtime::Char *>(ptr));
            break;
        case Value_Type_Kind::Int:
            print_int(*reinterpret_cast<runtime::Int *>(ptr));
            break;
        case Value_Type_Kind::Float:
            print_float(*reinterpret_cast<runtime::Float *>(ptr));
            break;
        case Value_Type_Kind::Str:
            print_str(*reinterpret_cast<runtime::String *>(ptr));
            break;
        case Value_Type_Kind::Struct:
            print_struct(type.data.struct_.defn, ptr);
            break;
        case Value_Type_Kind::Enum:
            print_enum(type.data.enum_.defn, ptr);
            break;

        default:
            printf("%s", type.display_str());
            break;
    }
}

void print_struct(Struct_Definition *defn, void *ptr) {
    printf("%.*s{ ", defn->name.size(), defn->name.c_str());

    for (size_t i = 0; i < defn->fields.size(); i++) {
        auto &field = defn->fields[i];
        
        printf("%.*s: ", field.id.size(), field.id.c_str());

        void *field_ptr = reinterpret_cast<uint8_t *>(ptr) + field.offset;
        print_value(field.type, field_ptr);

        if (i + 1 < defn->fields.size()) {
            printf(", ");
        }
    }

    printf(" }");
}

void print_enum(Enum_Definition *defn, void *ptr) {
    runtime::Int tag = *reinterpret_cast<runtime::Int *>(ptr);
    auto variant = defn->find_variant_by_tag(tag);
    internal_verify(variant, "Invalid variant tag for type `%.*s`: %lld.", defn->name.size(), defn->name.c_str(), tag);

    printf("%.*s", variant->id.size(), variant->id.c_str());

    if (!variant->payload.empty()) {
        printf("(");

        for (size_t i = 0; i < variant->payload.size(); i++) {
            auto &p = variant->payload[i];

            void *p_ptr = reinterpret_cast<uint8_t *>(ptr) + p.offset;
            print_value(p.type, p_ptr);

            if (i + 1 < variant->payload.size()) {
                printf(", ");
            }
        }

        printf(")");
    }
}

void builtin_puts_byte(Stack &stack, Address arg_start) {
    runtime::Byte value = stack.pop<runtime::Byte>();
    print_byte(value);
}

void builtin_puts_bool(Stack &stack, Address arg_start) {
    runtime::Bool value = stack.pop<runtime::Bool>();
    print_bool(value);
}

void builtin_puts_char(Stack &stack, Address arg_start) {
    runtime::Char value = stack.pop<runtime::Char>();
    print_char(value);
}

void builtin_puts_int(Stack &stack, Address arg_start) {
    runtime::Int value = stack.pop<runtime::Int>();
    print_int(value);
}

void builtin_puts_float(Stack &stack, Address arg_start) {
    runtime::Float value = stack.pop<runtime::Float>();
    print_float(value);
}

void builtin_puts_str(Stack &stack, Address arg_start) {
    runtime::String value = stack.pop<runtime::String>();
    print_str(value);
}

void builtin_puts_struct(Stack &stack, Address arg_start) {
    auto defn = stack.pop<Struct_Definition *>();
    auto data = stack.pop(defn->size);
    print_struct(defn, data);
}

void builtin_puts_enum(Stack &stack, Address arg_start) {
    auto defn = stack.pop<Enum_Definition *>();
    auto data = stack.pop(defn->size);

    print_enum(defn, data);
}

void builtin_print_byte(Stack &stack, Address arg_start) {
    builtin_puts_byte(stack, arg_start);
    printf("\n");
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
    
    interp->builtins.add_builtin("<free-ptr>", {
        builtin_free_pointer,
        value_types::func(value_types::Void, value_types::ptr_to(const_cast<Value_Type *>(&value_types::Void)))
    });

    interp->builtins.add_builtin("<free-slice>", {
        builtin_free_slice,
        value_types::func(value_types::Void, value_types::slice_of(const_cast<Value_Type *>(&value_types::Void)))
    });

    interp->builtins.add_builtin("<free-str>", {
        builtin_free_str,
        value_types::func(value_types::Void, value_types::Str)
    });
    
    interp->builtins.add_builtin("panic", {
        builtin_panic,
        value_types::func(value_types::Void, value_types::Str)
    });

    interp->builtins.add_builtin("<puts-byte>", {
        builtin_puts_byte,
        value_types::func(value_types::Void, value_types::Byte)
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

    interp->builtins.add_builtin("<print-byte>", {
        builtin_print_byte,
        value_types::func(value_types::Void, value_types::Byte)
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
