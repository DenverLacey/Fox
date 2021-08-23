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

#include "String.h"

enum class Opcode : uint8_t {
    None = 0,
    
    // LITERAL
    Lit_True,
    Lit_False,
    Lit_0,
    Lit_1,
    
    // CONSTANTS
    Load_Const_Char,
    Load_Const_Int,
    Load_Const_Float,
    Load_Const_String,
    
    Load_Const,
    
    // ARITHMETIC
    Int_Add,
    Int_Sub,
    Int_Mul,
    Int_Div,
    Int_Neg,
    Mod,
    
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
    Indirect_Move,  // BYTE_RMOVE,
//    BYTE_WB_WRITE,
//    BYTE_WB_WRITE_FROM_POINTER,
//    BYTE_WB_READ,
//    BYTE_COPY,
//    BYTE_DUPLICATE,
//    BYTE_SHIFT,
    Load,   // BYTE_LOAD,
    Push_Pointer,   // BYTE_PUSH_POINTER,
    Push_Value,   // BYTE_PUSH_VALUE,
    Push_Global_Pointer,    // BYTE_PUSH_GLOBAL_POINTER,
    Push_Global_Value,  // BYTE_PUSH_GLOBAL_VALUE,
    Pop,    // BYTE_POP,
//    BYTE_ALLOCATE,
//    BYTE_HEAP_ALLOCATE,
//    BYTE_ZERO,
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
//    BYTE_CALL,
//    BYTE_CALL_NATIVE,
};

using Chunk = std::vector<Opcode>;

//struct StructMember {
//    TypeSize offset;
//    int len;
//    const char *ident;
//    ValueType inferred_type;
//};

//struct StructDefinition {
//    bool has_initialiser;
//    int depth;
//    int len_ident;
//    const char *ident;
//    TypeSize size;
//    StructDefinition *super;
//    std::vector<StructMember> members;
//    std::vector<AST *> initialiser;
//};

//struct EnumPayloadMember {
//    TypeSize offset;
//    ValueType type;
//};

//struct EnumMember {
//    int len;
//    const char *ident;
//    Integer tag;
//    std::vector<EnumPayloadMember> payload;
//};

//struct EnumDefinition {
//    bool is_sumtype;
//    int len_ident;
//    const char *ident;
//    TypeSize size;
//    std::vector<EnumMember> members;
//};

struct Function_Definition {
    String ident;
//    FunctionType type;
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
        return *(T *)p;
    }

    template<typename T>
    T top() {
        return *(T *)&_buffer[_top - sizeof(T)];
    }
};

//#define WB_SIZE 512
//using Workbench = uint8_t[WB_SIZE];

struct Compiler;
struct VM {
    Data_Section constants;
    Data_Section str_constants;
    Call_Stack frames;
    Stack stack;
//    Workbench workbench;
    
    void run();
    void call(Function_Definition* fn, int arg_size);
    void print_bytecode(Compiler *c);
    void print_stack();
};

void interpret(const char *filepath);
