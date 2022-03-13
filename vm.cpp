//
//  vm.cpp
//  Fox
//
//  Created by Denver Lacey on 9/7/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#include "vm.h"

#include "mem.h"
#include "compiler.h"
#include "error.h"
#include "parser.h"
#include "tokenizer.h"
#include "typer.h"
#include "builtins.h"
#include "definitions.h"

void Stack::alloc(size_t size) {
    verify(_top + size <= Stack::Size, Code_Location{ 0,0,"<NO-LOC>" }, "Out of memory!");
    _top += size;
}

void Stack::calloc(size_t size) {
    verify(_top + size <= Stack::Size, Code_Location{ 0,0,"<NO-LOC>" }, "Out of memory!");
    memset(&_buffer[_top], 0, size);
    _top += size;
}

void Stack::push(void *data, size_t size) {
    void *dest = &_buffer[_top];
    alloc(size);
    memcpy(dest, data, size);
}

void *Stack::pop(size_t size) {
    assert(_top - size >= 0);
    _top -= size;
    return &_buffer[_top];
}

void *Stack::top(size_t size) {
    return &_buffer[_top - size];
}

void *Stack::get(Address address) {
    return &_buffer[address];
}

void *Stack::get(VM *vm, Address address) {
    return get(vm->frames.top().stack_bottom + address);
}

VM::VM(Data_Section &constants, Data_Section &str_constants)
  : constants(constants),
    str_constants(str_constants)
{
}

void VM::run() {
    #define READ(type, frame) *reinterpret_cast<type *>(&(*frame->instructions)[frame->pc]); frame->pc += sizeof(type)
    #define UNOP(type, op) { \
        type a = stack.pop<type>(); \
        stack.push(op(a)); \
    } break
    #define BIOP(type, op) { \
        type b = stack.pop<type>(); \
        type a = stack.pop<type>(); \
        stack.push(a op b); \
    } break
    #define BIOP_CHECK_FOR_ZERO(type, op, op_str) { \
        type b = stack.pop<type>(); \
        type a = stack.pop<type>(); \
        verify(b != 0, Code_Location{ 0,0,"<NO-LOC>" }, "Second operand detected as zero which is disallowed for operator " op_str "."); \
        stack.push(a op b); \
    } break
    
    Call_Frame *frame = &frames.top();
    while (frame->pc < frame->instructions->size()) {
        Opcode op = READ(Opcode, frame);
        switch (op) {
            // Literals
            case Opcode::Lit_True: {
                stack.push<runtime::Bool>(true);
            } break;
            case Opcode::Lit_False: {
                stack.push<runtime::Bool>(false);
            } break;
            case Opcode::Lit_0: {
                stack.push<runtime::Int>(0);
            } break;
            case Opcode::Lit_1: {
                stack.push<runtime::Int>(1);
            } break;
            case Opcode::Lit_Char: {
                runtime::Char c = READ(runtime::Char, frame);
                stack.push<runtime::Char>(c);
            } break;
            case Opcode::Lit_Int: {
                runtime::Int value = READ(runtime::Int, frame);
                stack.push<runtime::Int>(value);
            } break;
            case Opcode::Lit_Float: {
                runtime::Float value = READ(runtime::Float, frame);
                stack.push<runtime::Float>(value);
            } break;
            case Opcode::Lit_Pointer: {
                runtime::Pointer value = READ(runtime::Pointer, frame);
                stack.push<runtime::Pointer>(value);
            } break;
                
            // Constants
            case Opcode::Load_Const: {
                Size size = READ(Size, frame);
                size_t constant_index = READ(size_t, frame);
                void *constant = &constants[constant_index];
                stack.push(constant, size);
            } break;
            case Opcode::Load_Const_String: {
                size_t constant = READ(size_t, frame);
                size_t len = *reinterpret_cast<size_t *>(&str_constants[constant]);
                char *s = reinterpret_cast<char *>(&str_constants[constant + sizeof(size_t)]);
                stack.push<runtime::String>({ s, static_cast<runtime::Int>(len) });
            } break;
                
            // Arithmetic Operations
            case Opcode::Int_Add:   BIOP(runtime::Int, +);
            case Opcode::Int_Sub:   BIOP(runtime::Int, -);
            case Opcode::Int_Mul:   BIOP(runtime::Int, *);
            case Opcode::Int_Div:   BIOP_CHECK_FOR_ZERO(runtime::Int, /, "/");
            case Opcode::Int_Neg:   UNOP(runtime::Int, -);
            case Opcode::Mod:       BIOP_CHECK_FOR_ZERO(runtime::Int, %, "%%");
            case Opcode::Inc: {
                runtime::Int *n = stack.pop<runtime::Int *>();
                (*n)++;
            } break;
            case Opcode::Dec: {
                runtime::Int *n = stack.pop<runtime::Int *>();
                (*n)--;
            } break;
                
            case Opcode::Float_Add: BIOP(runtime::Float, +);
            case Opcode::Float_Sub: BIOP(runtime::Float, -);
            case Opcode::Float_Mul: BIOP(runtime::Float, *);
            case Opcode::Float_Div: BIOP_CHECK_FOR_ZERO(runtime::Float, /, "/");
            case Opcode::Float_Neg: UNOP(runtime::Float, -);
                
            case Opcode::Str_Add:
                todo("Str_Add not yet implemented.");
                break;
                
            // Bitwise Operations
            case Opcode::Bit_Not:       UNOP(runtime::Int, ~);
            case Opcode::Shift_Left:    BIOP(runtime::Int, <<);
            case Opcode::Shift_Right:   BIOP(runtime::Int, >>);
            case Opcode::Bit_And:       BIOP(runtime::Int, &);
            case Opcode::Xor:           BIOP(runtime::Int, ^);
            case Opcode::Bit_Or:        BIOP(runtime::Int, |);
                
            // Logical Operations
            case Opcode::And:   BIOP(runtime::Bool, &&);
            case Opcode::Or:    BIOP(runtime::Bool, ||);
            case Opcode::Not:   UNOP(runtime::Bool, !);
                
            // Equality Operations
            case Opcode::Equal: {
                Size size = READ(Size, frame);
                void *a = stack.pop(size);
                void *b = stack.pop(size);
                bool c = memcmp(a, b, size) == 0;
                stack.push<runtime::Bool>(c);
            } break;
            case Opcode::Not_Equal: {
                Size size = READ(Size, frame);
                void *a = stack.pop(size);
                void *b = stack.pop(size);
                bool c = memcmp(a, b, size) != 0;
                stack.push<runtime::Bool>(c);
            } break;
            case Opcode::Str_Equal: {
                runtime::String b = stack.pop<runtime::String>();
                runtime::String a = stack.pop<runtime::String>();
                bool c = a.len == b.len && memcmp(a.s, b.s, a.len) == 0;
                stack.push<runtime::Bool>(c);
            } break;
            case Opcode::Str_Not_Equal: {
                runtime::String b = stack.pop<runtime::String>();
                runtime::String a = stack.pop<runtime::String>();
                bool c = a.len == b.len && memcmp(a.s, b.s, a.len) != 0;
                stack.push<runtime::Bool>(c);
            } break;
                
            // Relational Operations
            case Opcode::Int_Less_Than:         BIOP(runtime::Int, <);
            case Opcode::Int_Less_Equal:        BIOP(runtime::Int, <=);
            case Opcode::Int_Greater_Than:      BIOP(runtime::Int, >);
            case Opcode::Int_Greater_Equal:     BIOP(runtime::Int, >=);
            case Opcode::Float_Less_Than:       BIOP(runtime::Float, <);
            case Opcode::Float_Less_Equal:      BIOP(runtime::Float, <=);
            case Opcode::Float_Greater_Than:    BIOP(runtime::Float, >);
            case Opcode::Float_Greater_Equal:   BIOP(runtime::Float, >=);
                
            // Stack Operations
            case Opcode::Move: {
                Size size = READ(Size, frame);
                runtime::Pointer dest = stack.pop<runtime::Pointer>();
                void *src = stack.top(size);
                if (dest != src) {
                    memcpy(dest, src, size);
                    stack.pop(size);
                }
            } break;
            case Opcode::Move_Push_Pointer: {
                Size size = READ(Size, frame);
                runtime::Pointer dest = stack.pop<runtime::Pointer>();
                void *src = stack.top(size);
                if (dest != src) {
                    memcpy(dest, src, size);
                    stack.pop(size);
                }
                stack.push<runtime::Pointer>(dest);
            } break;
            case Opcode::Copy: {
                Size size = READ(Size, frame);
                runtime::Pointer dest = stack.pop<runtime::Pointer>();
                runtime::Pointer src = stack.pop<runtime::Pointer>();
                if (dest != src) {
                    memcpy(dest, src, size);
                }
            } break;
            case Opcode::Load: {
                Size size = READ(Size, frame);
                runtime::Pointer data = stack.pop<runtime::Pointer>();
                stack.push(data, size);
            } break;
            case Opcode::Push_Pointer: {
                Address address = READ(Address, frame);
                runtime::Pointer p = stack.get(this, address);
                stack.push<runtime::Pointer>(p);
            } break;
            case Opcode::Push_Value: {
                Size size = READ(Size, frame);
                Address address = READ(Address, frame);
                void *data = stack.get(this, address);
                stack.push(data, size);
            } break;
            case Opcode::Push_Global_Pointer: {
                Address address = READ(Address, frame);
                runtime::Pointer p = stack.get(address);
                stack.push<runtime::Pointer>(p);
            } break;
            case Opcode::Push_Global_Value: {
                Size size = READ(Size, frame);
                Address address = READ(Address, frame);
                void *data = stack.get(address);
                stack.push(data, size);
            } break;
            case Opcode::Pop: {
                Size size = READ(Size, frame);
                stack.pop(size);
            } break;
            case Opcode::Allocate: {
                Size size = READ(Size, frame);
                stack.alloc(size);
            } break;
            case Opcode::Clear_Allocate: {
                Size size = READ(Size, frame);
                stack.calloc(size);
            } break;
            case Opcode::Flush: {
                Address flush_point = READ(Address, frame);
                flush_point += frame->stack_bottom;
                stack._top = static_cast<int>(flush_point);
            } break;
            
            // Branching Operations
            case Opcode::Jump: {
                size_t jump = READ(size_t, frame);
                frame->pc += static_cast<int>(jump);
            } break;
            case Opcode::Loop: {
                size_t jump = READ(size_t, frame);
                frame->pc -= static_cast<int>(jump);
            } break;
            case Opcode::Jump_True: {
                size_t jump = READ(size_t, frame);
                runtime::Bool cond = stack.pop<runtime::Bool>();
                if (cond) frame->pc += static_cast<int>(jump);
            } break;
            case Opcode::Jump_False: {
                size_t jump = READ(size_t, frame);
                runtime::Bool cond = stack.pop<runtime::Bool>();
                if (!cond) frame->pc += static_cast<int>(jump);
            } break;
            case Opcode::Jump_True_No_Pop: {
                size_t jump = READ(size_t, frame);
                runtime::Bool cond = stack.top<runtime::Bool>();
                if (cond) frame->pc += static_cast<int>(jump);
            } break;
            case Opcode::Jump_False_No_Pop: {
                size_t jump = READ(size_t, frame);
                runtime::Bool cond = stack.top<runtime::Bool>();
                if (!cond) frame->pc += static_cast<int>(jump);
            } break;
                
            // Invocation
            case Opcode::Call: {
                Size arg_size = READ(Size, frame);
                Function_Definition *defn = stack.pop<Function_Definition *>();
                call(defn, arg_size);
                frame = &frames.top();
            } break;
            case Opcode::Call_Builtin: {
                Builtin builtin = READ(Builtin, frame);
                Size arg_size = READ(Size, frame);
                Address arg_start = stack._top - arg_size;
                builtin(stack, arg_start);
            } break;
                
            // Cast
            case Opcode::Cast_Bool_Int: {
                runtime::Bool value = stack.pop<runtime::Bool>();
                stack.push<runtime::Int>(value ? 1 : 0);
            } break;
            case Opcode::Cast_Char_Int: {
                runtime::Char value = stack.pop<runtime::Char>();
                stack.push(static_cast<runtime::Int>(value));
            } break;
            case Opcode::Cast_Int_Float: {
                runtime::Int value = stack.pop<runtime::Int>();
                stack.push(static_cast<runtime::Float>(value));
            } break;
            case Opcode::Cast_Float_Int: {
                runtime::Float value = stack.pop<runtime::Float>();
                stack.push(static_cast<runtime::Int>(value));
            } break;

            case Opcode::Return: {
                if (frames.size() - 1 == 0)
                    return;
                
                Size size = READ(Size, frame);
                void *result = stack.pop(size);

                stack._top = frame->stack_bottom;
                
                stack.push(result, size);
                
                frames.pop();
                frame = &frames.top();
            } break;    
            case Opcode::Variadic_Return: {
                if (frames.size() - 1 == 0)
                    return;
                
                Size size = READ(Size, frame);
                void *result = stack.pop(size);
                
                runtime::Int ret_addr = *reinterpret_cast<runtime::Int *>(stack.get(this, -static_cast<int>(value_types::Int.size())));
                ret_addr += value_types::Int.size();
                stack._top = frame->stack_bottom - static_cast<int>(ret_addr);
                
                stack.push(result, size);
                
                frames.pop();
                frame = &frames.top();
            } break;
                
            default:
                internal_error("Unknown opcode: %d.", op);
                break;
        }
    }
    
    #undef READ
    #undef UNOP
    #undef BIOP
    #undef BIOP_CHECK_FOR_ZERO
}

void VM::call(Function_Definition *fn, int arg_size) {
    Call_Frame frame;
    frame.pc = 0;
    frame.stack_bottom = stack._top - arg_size;
    frame.instructions = &fn->instructions;
    frames.push(frame);
}

void VM::print_stack() {
    for (size_t i = 0; i < stack._top; i++) {
        uint8_t byte = stack._buffer[i];
        printf("%03zu: %hhX\n", i, byte);
    }
}

void print_code(std::vector<uint8_t> &code, Data_Section &constants, Data_Section &str_constants) {
    #define IDX "%04zX: "
    #define READ(type, i) *reinterpret_cast<type *>(&code[i]); i += sizeof(type)
    #define MARK(i) size_t mark = i++
    
    size_t i = 0;
    while (i < code.size()) {
        Opcode op = static_cast<Opcode>(code[i]);
        switch (op) {
            // Literals
            case Opcode::Lit_True:
                printf(IDX "Lit_True\n", i);
                i++;
                break;
            case Opcode::Lit_False:
                printf(IDX "Lit_False\n", i);
                i++;
                break;
            case Opcode::Lit_0:
                printf(IDX "Lit_0\n", i);
                i++;
                break;
            case Opcode::Lit_1:
                printf(IDX "Lit_1\n", i);
                i++;
                break;
            case Opcode::Lit_Char: {
                MARK(i);
                runtime::Char c = READ(runtime::Char, i);
                printf(IDX "Lit_Char '%s'\n", mark, utf8char_t::from_char32(c).buf);
            } break;
            case Opcode::Lit_Int: {
                MARK(i);
                runtime::Int constant = READ(runtime::Int, i);
                printf(IDX "Lit_Int (%lld)\n", mark, constant);
            } break;
            case Opcode::Lit_Float: {
                MARK(i);
                runtime::Float constant = READ(runtime::Float, i);
                printf(IDX "Lit_Float (%f)\n", mark, constant);
            } break;
            case Opcode::Lit_Pointer: {
                MARK(i);
                runtime::Pointer constant = READ(runtime::Pointer, i);
                printf(IDX "Lit_Pointer (%p)\n", mark, constant);
            } break;
                
            // Constants
            case Opcode::Load_Const: {
                MARK(i);
                Size size = READ(Size, i);
                size_t constant = READ(size_t, i);
                printf(IDX "Load_Const %ub [%zu]\n", mark, size * 8, constant);
            } break;
            case Opcode::Load_Const_String: {
                MARK(i);
                size_t constant = READ(size_t, i);
                size_t len = *reinterpret_cast<size_t *>(&str_constants[constant]);
                char *s = reinterpret_cast<char *>(&str_constants[constant + sizeof(size_t)]);
                printf(IDX "Load_Const_String [%zu] \"%.*s\"\n", mark, constant, len, s);
            } break;
                
            // Arithmetic
            case Opcode::Int_Add:
                printf(IDX "Int_Add\n", i);
                i++;
                break;
            case Opcode::Int_Sub:
                printf(IDX "Int_Sub\n", i);
                i++;
                break;
            case Opcode::Int_Mul:
                printf(IDX "Int_Mul\n", i);
                i++;
                break;
            case Opcode::Int_Div:
                printf(IDX "Int_Div\n", i);
                i++;
                break;
            case Opcode::Int_Neg:
                printf(IDX "Int_Neg\n", i);
                i++;
                break;
            case Opcode::Mod:
                printf(IDX "Mod\n", i);
                i++;
                break;
            case Opcode::Inc:
                printf(IDX "Inc\n", i);
                i++;
                break;
            case Opcode::Dec:
                printf(IDX "Dec\n", i);
                i++;
                break;
            case Opcode::Float_Add:
                printf(IDX "Float_Add\n", i);
                i++;
                break;
            case Opcode::Float_Sub:
                printf(IDX "Float_Sub\n", i);
                i++;
                break;
            case Opcode::Float_Mul:
                printf(IDX "Float_Mul\n", i);
                i++;
                break;
            case Opcode::Float_Div:
                printf(IDX "Float_Div\n", i);
                i++;
                break;
            case Opcode::Float_Neg:
                printf(IDX "Float_Neg\n", i);
                i++;
                break;
            case Opcode::Str_Add:
                printf(IDX "Str_Add\n", i);
                i++;
                break;
                
            // Bitwise
            case Opcode::Bit_Not:
                printf(IDX "Bit_Not\n", i);
                i++;
                break;
            case Opcode::Shift_Left:
                printf(IDX "Shift_Left\n", i);
                i++;
                break;
            case Opcode::Shift_Right:
                printf(IDX "Shift_Right\n", i);
                i++;
                break;
            case Opcode::Bit_And:
                printf(IDX "Bit_And\n", i);
                i++;
                break;
            case Opcode::Xor:
                printf(IDX "Xor\n", i);
                i++;
                break;
            case Opcode::Bit_Or:
                printf(IDX "Bit_Or\n", i);
                i++;
                break;
                
            // Logical
            case Opcode::And:
                printf(IDX "And\n", i);
                i++;
                break;
            case Opcode::Or:
                printf(IDX "Or\n", i);
                i++;
                break;
            case Opcode::Not:
                printf(IDX "Not\n", i);
                i++;
                break;
                
            // Equality
            case Opcode::Equal: {
                MARK(i);
                Size size = READ(Size, i);
                printf(IDX "Equal %ub\n", mark, size * 8);
            } break;
            case Opcode::Not_Equal: {
                MARK(i);
                Size size = READ(Size, i);
                printf(IDX "Not_Equal %ub\n", mark, size * 8);
            } break;
            case Opcode::Str_Equal:
                printf(IDX "Str_Equal\n", i);
                i++;
                break;
            case Opcode::Str_Not_Equal:
                printf(IDX "Str_Not_Equal\n", i);
                i++;
                break;
                
            // Relational
            case Opcode::Int_Less_Than:
                printf(IDX "Int_Less_Than\n", i);
                i++;
                break;
            case Opcode::Int_Less_Equal:
                printf(IDX "Int_Less_Equal\n", i);
                i++;
                break;
            case Opcode::Int_Greater_Than:
                printf(IDX "Int_Greater_Than\n", i);
                i++;
                break;
            case Opcode::Int_Greater_Equal:
                printf(IDX "Int_Greater_Equal\n", i);
                i++;
                break;
            case Opcode::Float_Less_Than:
                printf(IDX "Float_Less_Than\n", i);
                i++;
                break;
            case Opcode::Float_Less_Equal:
                printf(IDX "Float_Less_Equal\n", i);
                i++;
                break;
            case Opcode::Float_Greater_Than:
                printf(IDX "Float_Greater_Than\n", i);
                i++;
                break;
            case Opcode::Float_Greater_Equal:
                printf(IDX "Float_Greater_Equal\n", i);
                i++;
                break;
                
            // Stack
            case Opcode::Move: {
                MARK(i);
                Size size = READ(Size, i);
                printf(IDX "Move %ub\n", mark, size * 8);
            } break;
            case Opcode::Move_Push_Pointer: {
                MARK(i);
                Size size = READ(Size, i);
                printf(IDX "Move_Push_Pointer %ub\n", mark, size * 8);
            } break;
            case Opcode::Copy: {
                MARK(i);
                Size size = READ(Size, i);
                printf(IDX "Copy %ub\n", mark, size * 8);
            } break;
            case Opcode::Load: {
                MARK(i);
                Size size = READ(Size, i);
                printf(IDX "Load %ub\n", mark, size * 8);
            } break;
            case Opcode::Push_Pointer: {
                MARK(i);
                Address address = READ(Address, i);
                printf(IDX "Push_Pointer [%zu]\n", mark, address);
            } break;
            case Opcode::Push_Value: {
                MARK(i);
                Size size = READ(Size, i);
                Address address = READ(Address, i);
                printf(IDX "Push_Value %ub [%zu]\n", mark, size * 8, address);
            } break;
            case Opcode::Push_Global_Pointer: {
                MARK(i);
                Address address = READ(Address, i);
                printf(IDX "Push_Global_Pointer [%zu]\n", mark, address);
            } break;
            case Opcode::Push_Global_Value: {
                MARK(i);
                Size size = READ(Size, i);
                Address address = READ(Address, i);
                printf(IDX "Push_Global_Value %ub [%zu]\n", mark, size * 8, address);
            } break;
            case Opcode::Pop: {
                MARK(i);
                Size size = READ(Size, i);
                printf(IDX "Pop %ub\n", mark, size * 8);
            } break;
            case Opcode::Allocate: {
                MARK(i);
                Size size = READ(Size, i);
                printf(IDX "Allocate %ub\n", mark, size * 8);
            } break;
            case Opcode::Clear_Allocate: {
                MARK(i);
                Size size = READ(Size, i);
                printf(IDX "Clear_Allocate %ub\n", mark, size * 8);
            } break;
            case Opcode::Flush: {
                MARK(i);
                Address flush_point = READ(Address, i);
                printf(IDX "Flush => %zu\n", mark, flush_point);
            } break;
            case Opcode::Return: {
                MARK(i);
                Size size = READ(Size, i);
                printf(IDX "Return %ub\n", mark, size * 8);
            } break;
            case Opcode::Variadic_Return: {
                MARK(i);
                Size size = READ(Size, i);
                printf(IDX "Variadic_Return %ub\n", mark, size * 8);
            } break;
                
            // Branching
            case Opcode::Jump: {
                MARK(i);
                size_t jump = READ(size_t, i);
                size_t dest = mark + jump + 9; // add 9 for instruction
                printf(IDX "Jump => %zX\n", mark, dest);
            } break;
            case Opcode::Loop: {
                MARK(i);
                size_t jump = READ(size_t, i);
                size_t dest = mark - jump + 9; // add 9 for instruction
                printf(IDX "Loop => %zX\n", mark, dest);
            } break;
            case Opcode::Jump_True: {
                MARK(i);
                size_t jump = READ(size_t, i);
                size_t dest = mark + jump + 9; // add 9 for instruction
                printf(IDX "Jump_True => %zX\n", mark, dest);
            } break;
            case Opcode::Jump_False: {
                MARK(i);
                size_t jump = READ(size_t, i);
                size_t dest = mark + jump + 9; // add 9 for instruction
                printf(IDX "Jump_False => %zX\n", mark, dest);
            } break;
            case Opcode::Jump_True_No_Pop: {
                MARK(i);
                size_t jump = READ(size_t, i);
                size_t dest = mark + jump + 9; // add 9 for instruction
                printf(IDX "Jump_True_No_Pop => %zX\n", mark, dest);
            } break;
            case Opcode::Jump_False_No_Pop: {
                MARK(i);
                size_t jump = READ(size_t, i);
                size_t dest = mark + jump + 9; // add 9 for instruction
                printf(IDX "Jump_False_No_Pop => %zX\n", mark, dest);
            } break;
                
            // Invocation
            case Opcode::Call: {
                MARK(i);
                Size arg_size = READ(Size, i);
                printf(IDX "Call %ub\n", mark, arg_size * 8);
            } break;
            case Opcode::Call_Builtin: {
                MARK(i);
                Builtin builtin = READ(Builtin, i);
                Size arg_size = READ(Size, i);
                printf(IDX "Call_Builtin %p %ub\n", mark, builtin, arg_size * 8);
            } break;
                
            // Cast
            case Opcode::Cast_Bool_Int:
                printf(IDX "Cast_Bool_Int\n", i);
                i++;
                break;
            case Opcode::Cast_Char_Int:
                printf(IDX "Cast_Char_Int\n", i);
                i++;
                break;
            case Opcode::Cast_Int_Float:
                printf(IDX "Cast_Int_Float\n", i);
                i++;
                break;
            case Opcode::Cast_Float_Int:
                printf(IDX "Cast_Int_Float\n", i);
                i++;
                break;
                
            default:
                internal_error("Invalid opcode: %d.", op);
                break;
        }
    }
    
    printf(IDX "END\n", i);
    
    #undef IDX
    #undef READ
    #undef MARK
}
