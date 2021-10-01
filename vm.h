//
//  vm.h
//  Fox
//
//  Created by Denver Lacey on 9/7/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#pragma once

#include <stack>
#include <vector>
#include <unordered_map>
#include <string>

#include "String.h"
#include "mem.h"
#include "typer.h"

struct Module;

enum class Opcode : uint8_t {
    None = 0,
    
    // LITERAL
    Lit_True,
    Lit_False,
    Lit_0,
    Lit_1,
    Lit_Char,
    Lit_Int,
    Lit_Float,
    Lit_Pointer,
    
    // CONSTANTS
    Load_Const,
    Load_Const_String,
    
    // ARITHMETIC
    Int_Add,
    Int_Sub,
    Int_Mul,
    Int_Div,
    Int_Neg,
    Mod,
    Inc,
    Dec,
    
    Float_Add,
    Float_Sub,
    Float_Mul,
    Float_Div,
    Float_Neg,
    
    Str_Add,
    
    // BITWISE
    Bit_Not,
    
    Shift_Left,
    Shift_Right,
    
    Bit_And,
    Xor,
    Bit_Or,
    
    // LOGIC
    And,
    Or,
    Not,
    
    // RELATIONAL
    Equal,
    Not_Equal,
    Str_Equal,
    Str_Not_Equal,
    
    Int_Less_Than,
    Int_Less_Equal,
    Int_Greater_Than,
    Int_Greater_Equal,
    
    Float_Less_Than,
    Float_Less_Equal,
    Float_Greater_Than,
    Float_Greater_Equal,
    
    // STACK
    Move,   // BYTE_MOVE,
    Move_Push_Pointer,
//    BYTE_WB_WRITE,
//    BYTE_WB_WRITE_FROM_POINTER,
//    BYTE_WB_READ,
    Copy,   // BYTE_COPY,
//    BYTE_DUPLICATE,
//    BYTE_SHIFT,
    Load,   // BYTE_LOAD,
    Push_Pointer,   // BYTE_PUSH_POINTER,
    Push_Value,   // BYTE_PUSH_VALUE,
    Push_Global_Pointer,    // BYTE_PUSH_GLOBAL_POINTER,
    Push_Global_Value,  // BYTE_PUSH_GLOBAL_VALUE,
    Pop,    // BYTE_POP,
    Allocate, //    BYTE_ALLOCATE,
    Clear_Allocate, //    BYTE_ZERO,
    Heap_Allocate,  //    BYTE_HEAP_ALLOCATE,
    Flush,  // BYTE_FLUSH,
    Return, // BYTE_RETURN,
    
    // BRANCHING
    Jump,
    Loop,
    Jump_True,
    Jump_False,
    Jump_True_No_Pop,
    Jump_False_No_Pop,
    
    // INVOCATION
    Call,   //    BYTE_CALL,
//    BYTE_CALL_NATIVE,
};

using Chunk = std::vector<Opcode>;

struct Function_Definition {
    UUID uuid;
    Module *module;
    String name;
    Value_Type type;
    std::vector<String> param_names;
//    CodeLocation location;
    Chunk bytecode;
};

struct Call_Frame {
    int pc;
    int stack_bottom;
    Chunk *bytecode;
};

using Data_Section = std::vector<uint8_t>;
using Call_Stack = std::stack<Call_Frame>;

struct VM;
struct Stack {
    static constexpr size_t Size = UINT16_MAX;
    
    int _top = 0;
    uint8_t _buffer[Size];
    
    void alloc(size_t size);
    void calloc(size_t size);
    void push(void *data, size_t size);
    void *pop(size_t size);
    void *top(size_t size);
    void *get(Address address);
    void *get(VM *vm, Address address);
    
    template<typename T>
    void push(T constant) {
        push(&constant, sizeof(T));
    }
    
    template<typename T>
    T pop() {
        void *p = pop(sizeof(T));
        return *reinterpret_cast<T *>(p);
    }

    template<typename T>
    T top() {
        return *reinterpret_cast<T *>(top(sizeof(T)));
    }
};

//#define WB_SIZE 512
//using Workbench = uint8_t[WB_SIZE];

struct Compiler;
struct VM {
    Data_Section &constants;
    Data_Section &str_constants;
    Call_Stack frames;
    Stack stack;
//    Workbench workbench;
    
    VM(Data_Section &constants, Data_Section &str_constants);
    
    void run();
    void call(Function_Definition* fn, int arg_size);
    void print_stack();
};

void print_code(Chunk &code, Data_Section &constants, Data_Section &str_constants);

struct Struct_Field {
    Size offset;
    String id;
    Value_Type type;
};

struct Struct_Definition {
    Size size;
    // Struct_Definition *super;
    UUID uuid;
    Module *module;
    String name;
    std::vector<Struct_Field> fields;
    std::unordered_map<std::string, UUID> methods;
    // std::vector<Ref<Typed_AST>> initializer;
    
    bool has_field(String id);
    Struct_Field *find_field(String id);
    bool has_method(String id);
    bool find_method(String id, UUID &out_uuid);
};

struct Enum_Payload_Field {
    Size offset;
    Value_Type type;
};

struct Enum_Variant {
    runtime::Int tag;
    String id;
    std::vector<Enum_Payload_Field> payload;
};

struct Enum_Definition {
    bool is_sumtype;
    Size size;
    UUID uuid;
    Module *module;
    String name;
    std::vector<Enum_Variant> variants;
    
    Enum_Variant *find_variant(String id);
};
