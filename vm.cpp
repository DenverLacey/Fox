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

#include <fstream>

#define PRINT_DEBUG_DIAGNOSTICS 1
#define COMPILE_AST 1
#define RUN_VIRTUAL_MACHINE 1

void Stack::alloc(size_t size) {
    verify(_top + size <= Stack::Size, "Out of memory!");
    _top += size;
}

void Stack::calloc(size_t size) {
    verify(_top + size <= Stack::Size, "Out of memory!");
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

VM::VM(Data_Section &&constants, Data_Section &&str_constants)
    : constants(constants), str_constants(str_constants)
{
}

void VM::run() {
    #define READ(type, frame) *(type *)&(*frame->bytecode)[frame->pc]; frame->pc += sizeof(type)
    #define UNOP(type, op) { \
        type a = stack.pop<type>(); \
        stack.push(op(a)); \
    } break
    #define BIOP(type, op) { \
        type b = stack.pop<type>(); \
        type a = stack.pop<type>(); \
        stack.push(a op b); \
    } break
    
    Call_Frame *frame = &frames.top();
    while (frame->pc < frame->bytecode->size()) {
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
                
            // Constants
            case Opcode::Load_Const_Int: {
                size_t constant = READ(size_t, frame);
                runtime::Int value = *(runtime::Int *)&constants[constant];
                stack.push<runtime::Int>(value);
            } break;
            case Opcode::Load_Const_Float: {
                size_t constant = READ(size_t, frame);
                runtime::Float value = *(runtime::Float *)&constants[constant];
                stack.push<runtime::Float>(value);
            } break;
            case Opcode::Load_Const_String: {
                size_t constant = READ(size_t, frame);
                size_t len = *(size_t *)&str_constants[constant];
                char *s    = (char *)&str_constants[constant + sizeof(size_t)];
                stack.push<runtime::String>({ s, (runtime::Int)len });
            } break;
            case Opcode::Load_Const: {
                Size size = READ(Size, frame);
                void *constant = &(*frame->bytecode)[frame->pc];
                frame->pc += size;
                stack.push(constant, size);
            } break;
                
            // Arithmetic Operations
            case Opcode::Int_Add:   BIOP(runtime::Int, +);
            case Opcode::Int_Sub:   BIOP(runtime::Int, -);
            case Opcode::Int_Mul:   BIOP(runtime::Int, *);
            case Opcode::Int_Div:   BIOP(runtime::Int, /);
            case Opcode::Int_Neg:   UNOP(runtime::Int, -);
            case Opcode::Mod:       BIOP(runtime::Int, %);
            case Opcode::Float_Add: BIOP(runtime::Float, +);
            case Opcode::Float_Sub: BIOP(runtime::Float, -);
            case Opcode::Float_Mul: BIOP(runtime::Float, *);
            case Opcode::Float_Div: BIOP(runtime::Float, /);
            case Opcode::Float_Neg: UNOP(runtime::Float, -);
                
            case Opcode::Str_Add:
                assert(false);
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
            case Opcode::Indirect_Move:
                assert(false);
                break;
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
            case Opcode::Flush: {
                Address flush_point = READ(Address, frame);
                flush_point += frame->stack_bottom;
                stack._top = (int)flush_point;
            } break;
            
            // Branching Operations
            case Opcode::Jump: {
                size_t jump = READ(size_t, frame);
                frame->pc += (int)jump;
            } break;
            case Opcode::Loop: {
                size_t jump = READ(size_t, frame);
                frame->pc -= (int)jump;
            } break;
            case Opcode::Jump_True: {
                size_t jump = READ(size_t, frame);
                runtime::Bool cond = stack.pop<runtime::Bool>();
                if (cond) frame->pc += (int)jump;
            } break;
            case Opcode::Jump_False: {
                size_t jump = READ(size_t, frame);
                runtime::Bool cond = stack.pop<runtime::Bool>();
                if (!cond) frame->pc += (int)jump;
            } break;
            case Opcode::Jump_True_No_Pop:
                assert(false);
                break;
            case Opcode::Jump_False_No_Pop:
                assert(false);
                break;
                
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
                
            default:
                internal_error("Unknown opcode: %d.", op);
                break;
        }
    }
}
#undef READ
#undef UNOP
#undef BIOP

void VM::call(Function_Definition *fn, int arg_size) {
    Call_Frame frame;
    frame.pc = 0;
    frame.stack_bottom = stack._top - arg_size;
    frame.bytecode = &fn->bytecode;
    frames.push(frame);
}

void VM::print_stack() {
    for (size_t i = 0; i < stack._top; i++) {
        uint8_t byte = stack._buffer[i];
        printf("%03zu: %hhX\n", i, byte);
    }
}

static String read_entire_file(const char *path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    verify(file.is_open(), "'%s' could not be opened.", path);
    
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    auto source = String::with_size(size);
    file.read(source.c_str(), size);
    verify(file.good(), "Could not read from '%s'.", path);
    
    return source;
}

void print_code(Chunk &code, Data_Section &constants, Data_Section &str_constants) {
    #define READ(type, i) *(type *)&code[i]; i += sizeof(type)
    #define MARK(i) size_t mark = i++
    
    size_t i = 0;
    while (i < code.size()) {
        Opcode op = code[i];
        switch (op) {
            // Literals
            case Opcode::Lit_True:
                printf("%03zu: Lit_True\n", i);
                i++;
                break;
            case Opcode::Lit_False:
                printf("%03zu: Lit_False\n", i);
                i++;
                break;
            case Opcode::Lit_0:
                printf("%03zu: Lit_0\n", i);
                i++;
                break;
            case Opcode::Lit_1:
                printf("%03zu: Lit_1\n", i);
                i++;
                break;
            case Opcode::Lit_Char: {
                MARK(i);
                runtime::Char c = READ(runtime::Char, i);
                printf("%03zu: Lit_Char (%s)\n", mark, utf8char_t::from_char32(c).buf);
            } break;
                
            // Constants
            case Opcode::Load_Const_Int: {
                MARK(i);
                size_t constant = READ(size_t, i);
                runtime::Int k = *(runtime::Int *)&constants[constant];
                printf("%03zu: Load_Const_Int [%zu] (%lld)\n", mark, constant, k);
            } break;
            case Opcode::Load_Const_Float: {
                MARK(i);
                size_t constant = READ(size_t, i);
                runtime::Float f = *(runtime::Float *)&constants[constant];
                printf("%03zu: Load_Const_Float [%zu] (%f)\n", mark, constant, f);
            } break;
            case Opcode::Load_Const_String: {
                MARK(i);
                size_t constant = READ(size_t, i);
                size_t len = *(size_t *)&str_constants[constant];
                char *s    = (char *)&str_constants[constant + sizeof(size_t)];
                printf("%03zu: Load_Const_String [%zu] (%.*s)\n", mark, constant, len, s);
            } break;
            case Opcode::Load_Const: {
                MARK(i);
                size_t size = READ(size_t, i);
                uint8_t *constant = (uint8_t *)&code[i];
                i += size;
                printf("%03zu: Load_Const %zub (", mark, size * 8);
                for (size_t it = 0; it < size; it++, constant++) {
                    printf("%X", *constant);
                }
                printf(")\n");
            } break;
                
            // Arithmetic
            case Opcode::Int_Add:
                printf("%03zu: Int_Add\n", i);
                i++;
                break;
            case Opcode::Int_Sub:
                printf("%03zu: Int_Sub\n", i);
                i++;
                break;
            case Opcode::Int_Mul:
                printf("%03zu: Int_Mul\n", i);
                i++;
                break;
            case Opcode::Int_Div:
                printf("%03zu: Int_Div\n", i);
                i++;
                break;
            case Opcode::Int_Neg:
                printf("%03zu: Int_Neg\n", i);
                i++;
                break;
            case Opcode::Mod:
                printf("%03zu: Mod\n", i);
                i++;
                break;
            case Opcode::Float_Add:
                printf("%03zu: Float_Add\n", i);
                i++;
                break;
            case Opcode::Float_Sub:
                printf("%03zu: Float_Sub\n", i);
                i++;
                break;
            case Opcode::Float_Mul:
                printf("%03zu: Float_Mul\n", i);
                i++;
                break;
            case Opcode::Float_Div:
                printf("%03zu: Float_Div\n", i);
                i++;
                break;
            case Opcode::Float_Neg:
                printf("%03zu: Float_Neg\n", i);
                i++;
                break;
            case Opcode::Str_Add:
                printf("%03zu: Str_Add\n", i);
                i++;
                break;
                
            // Bitwise
            case Opcode::Bit_Not:
                printf("%03zu: Bit_Not\n", i);
                i++;
                break;
            case Opcode::Shift_Left:
                printf("%03zu: Shift_Left\n", i);
                i++;
                break;
            case Opcode::Shift_Right:
                printf("%03zu: Shift_Right\n", i);
                i++;
                break;
            case Opcode::Bit_And:
                printf("%03zu: Bit_And\n", i);
                i++;
                break;
            case Opcode::Xor:
                printf("%03zu: Xor\n", i);
                i++;
                break;
            case Opcode::Bit_Or:
                printf("%03zu: Bit_Or\n", i);
                i++;
                break;
                
            // Logical
            case Opcode::And:
                printf("%03zu: And\n", i);
                i++;
                break;
            case Opcode::Or:
                printf("%03zu: Or\n", i);
                i++;
                break;
            case Opcode::Not:
                printf("%03zu: Not\n", i);
                i++;
                break;
                
            // Equality
            case Opcode::Equal: {
                MARK(i);
                Size size = READ(Size, i);
                printf("%03zu: Equal %ub\n", mark, size * 8);
            } break;
            case Opcode::Not_Equal: {
                MARK(i);
                Size size = READ(Size, i);
                printf("%03zu: Not_Equal %ub\n", mark, size * 8);
            } break;
            case Opcode::Str_Equal:
                printf("%03zu: Str_Equal\n", i);
                i++;
                break;
            case Opcode::Str_Not_Equal:
                printf("%03zu: Str_Not_Equal\n", i);
                i++;
                break;
                
            // Relational
            case Opcode::Int_Less_Than:
                printf("%03zu: Int_Less_Than\n", i);
                i++;
                break;
            case Opcode::Int_Less_Equal:
                printf("%03zu: Int_Less_Equal\n", i);
                i++;
                break;
            case Opcode::Int_Greater_Than:
                printf("%03zu: Int_Greater_Than\n", i);
                i++;
                break;
            case Opcode::Int_Greater_Equal:
                printf("%03zu: Int_Greater_Equal\n", i);
                i++;
                break;
            case Opcode::Float_Less_Than:
                printf("%03zu: Float_Less_Than\n", i);
                i++;
                break;
            case Opcode::Float_Less_Equal:
                printf("%03zu: Float_Less_Equal\n", i);
                i++;
                break;
            case Opcode::Float_Greater_Than:
                printf("%03zu: Float_Greater_Than\n", i);
                i++;
                break;
            case Opcode::Float_Greater_Equal:
                printf("%03zu: Float_Greater_Equal\n", i);
                i++;
                break;
                
            // Stack
            case Opcode::Move: {
                MARK(i);
                Size size = READ(Size, i);
                printf("%03zu: Move %ub\n", mark, size * 8);
            } break;
            case Opcode::Indirect_Move:
                assert(false);
                break;
            case Opcode::Load: {
                MARK(i);
                Size size = READ(Size, i);
                printf("%03zu: Load %ub\n", mark, size * 8);
            } break;
            case Opcode::Push_Pointer: {
                MARK(i);
                Address address = READ(Address, i);
                printf("%03zu: Push_Pointer [%zu]\n", mark, address);
            } break;
            case Opcode::Push_Value: {
                MARK(i);
                Size size = READ(Size, i);
                Address address = READ(Address, i);
                printf("%03zu: Push_Value %ub [%zu]\n", mark, size * 8, address);
            } break;
            case Opcode::Push_Global_Pointer: {
                MARK(i);
                Address address = READ(Address, i);
                printf("%03zu: Push_Global_Pointer [%zu]\n", mark, address);
            } break;
            case Opcode::Push_Global_Value: {
                MARK(i);
                Size size = READ(Size, i);
                Address address = READ(Address, i);
                printf("%03zu: Push_Global_Value %ub [%zu]\n", mark, size * 8, address);
            } break;
            case Opcode::Pop: {
                MARK(i);
                Size size = READ(Size, i);
                printf("%03zu: Pop %ub\n", mark, size);
            } break;
            case Opcode::Flush: {
                MARK(i);
                Address flush_point = READ(Address, i);
                printf("%03zu: Flush => %zu\n", mark, flush_point);
            } break;
            case Opcode::Return: {
                MARK(i);
                Size size = READ(Size, i);
                printf("%03zu: Return %ub\n", mark, size);
            } break;
                
            // Branching
            case Opcode::Jump: {
                MARK(i);
                size_t jump = READ(size_t, i);
                size_t dest = mark + jump + 9; // add 9 for bytecode
                printf("%03zu: Jump => %zu\n", i, dest);
            } break;
            case Opcode::Loop: {
                MARK(i);
                size_t jump = READ(size_t, i);
                size_t dest = mark - jump + 9; // add 9 for bytecode
                printf("%03zu: Loop => %zu\n", i, dest);
            } break;
            case Opcode::Jump_True: {
                MARK(i);
                size_t jump = READ(size_t, i);
                size_t dest = mark + jump + 9; // add 9 for bytecode
                printf("%03zu: Jump_True => %zu\n", i, dest);
            } break;
            case Opcode::Jump_False: {
                MARK(i);
                size_t jump = READ(size_t, i);
                size_t dest = mark + jump + 9; // add 9 for bytecode
                printf("%03zu: Jump_False => %zu\n", i, dest);
            } break;
            case Opcode::Jump_True_No_Pop: {
                MARK(i);
                size_t jump = READ(size_t, i);
                size_t dest = mark + jump + 9; // add 9 for bytecode
                printf("%03zu: Jump_True_No_Pop => %zu\n", i, dest);
            } break;
            case Opcode::Jump_False_No_Pop: {
                MARK(i);
                size_t jump = READ(size_t, i);
                size_t dest = mark + jump + 9; // add 9 for bytecode
                printf("%03zu: Jump_False_No_Pop => %zu\n", i, dest);
            } break;
                
            default:
                internal_error("Invalid opcode: %d.", op);
                break;
        }
    }
}
#undef READ

void interpret(const char *path) {
    String source = read_entire_file(path);
    auto tokens = tokenize(source);
    auto ast = parse(tokens);
    
#if PRINT_DEBUG_DIAGNOSTICS
    ast->print();
#endif

    auto typed_ast = typecheck(ast.get());
    
#if PRINT_DEBUG_DIAGNOSTICS
    printf("\n------\n\n");
    typed_ast->print();
#endif
    
#if COMPILE_AST
    Function_Definition program;
    Compiler global(&program);
    global.compile(typed_ast.get());
    
#if PRINT_DEBUG_DIAGNOSTICS
    printf("\n------\n\n");
    print_code(program.bytecode, global.constants, global.str_constants);
#endif
    
#if RUN_VIRTUAL_MACHINE
    VM vm(std::move(global.constants), std::move(global.str_constants));
    vm.call(&program, 0);
    vm.run();
    
#if PRINT_DEBUG_DIAGNOSTICS
    printf("\n------\n\n");
    vm.print_stack();
#endif
    
#endif // RUN_VIRTUAL_MACHINE
#endif // COMPILE_AST
}
