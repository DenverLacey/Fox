//
//  typer.h
//  Fox
//
//  Created by Denver Lacey on 23/8/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#pragma once

#include "mem.h"
#include "value.h"
#include <vector>

enum class Typed_AST_Kind {
    // literals
    Bool,
    Char,
    Float,
    Ident,
    Int,
    Str,
    
    // unary
    Not,
    Address_Of,
    Address_Of_Mut,
    Deref,
    
    // binary
    Addition,
    Subtraction,
    Multiplication,
    Division,
    Mod,
    Assignment,
    Equal,
    Not_Equal,
    Less,
    Less_Eq,
    Greater,
    Greater_Eq,
    While,
    And,
    Or,
    
    // ternary
    
    // block
    Block,
    Comma,
    
    // unique
    If,
    Let,
    Type_Signiture,
};

struct Compiler;

struct Typed_AST {
    Typed_AST_Kind kind;
    Value_Type type;
    
    virtual ~Typed_AST() = default;
    void print() const;
    virtual void compile(Compiler &c) = 0;
};

struct Typed_AST_Bool : public Typed_AST {
    bool value;
    
    Typed_AST_Bool(bool value);
    void compile(Compiler &c) override;
};

struct Typed_AST_Char : public Typed_AST {
    char32_t value;
    
    Typed_AST_Char(char32_t value);
    void compile(Compiler &c) override;
};

struct Typed_AST_Float : public Typed_AST {
    double value;
    
    Typed_AST_Float(double value);
    void compile(Compiler &c) override;
};

struct Typed_AST_Ident : public Typed_AST {
    String id;
    
    Typed_AST_Ident(String id, Value_Type type);
    ~Typed_AST_Ident() override;
    void compile(Compiler &c) override;
};

struct Typed_AST_Int : public Typed_AST {
    int64_t value;
    
    Typed_AST_Int(int64_t value);
    void compile(Compiler &c) override;
};

struct Typed_AST_Str : public Typed_AST {
    String value;
    
    Typed_AST_Str(String value);
    ~Typed_AST_Str() override;
    void compile(Compiler &c) override;
};

struct Typed_AST_Unary : public Typed_AST {
    Ref<Typed_AST> sub;
    
    Typed_AST_Unary(Typed_AST_Kind kind, Value_Type type, Ref<Typed_AST> sub);
    void compile(Compiler &c) override;
};

struct Typed_AST_Binary : public Typed_AST {
    Ref<Typed_AST> lhs;
    Ref<Typed_AST> rhs;
    
    Typed_AST_Binary(Typed_AST_Kind kind, Value_Type type, Ref<Typed_AST> lhs, Ref<Typed_AST> rhs);
    void compile(Compiler &c) override;
};

struct Typed_AST_Ternary : public Typed_AST {
    Ref<Typed_AST> lhs;
    Ref<Typed_AST> mid;
    Ref<Typed_AST> rhs;
    
    Typed_AST_Ternary(Typed_AST_Kind kind, Value_Type type, Ref<Typed_AST> lhs, Ref<Typed_AST> mid, Ref<Typed_AST> rhs);
    void compile(Compiler &c) override;
};

struct Typed_AST_Block : public Typed_AST {
    std::vector<Ref<Typed_AST>> nodes;
    
    Typed_AST_Block(Typed_AST_Kind kind);
    void add(Ref<Typed_AST> node);
    void compile(Compiler &c) override;
};

struct Typed_AST_If : public Typed_AST {
    Ref<Typed_AST> cond;
    Ref<Typed_AST> then;
    Ref<Typed_AST> else_;
    
    Typed_AST_If(Value_Type type, Ref<Typed_AST> cond, Ref<Typed_AST> then, Ref<Typed_AST> else_);
    void compile(Compiler &c) override;
};

struct Typed_AST_Type_Signiture : public Typed_AST {
    Ref<Value_Type> value_type;
    
    Typed_AST_Type_Signiture(Ref<Value_Type> value_type);
    void compile(Compiler &c) override;
};

struct Typed_AST_Let : public Typed_AST {
    String id;
    bool is_mut;
    Ref<Typed_AST_Type_Signiture> specified_type;
    Ref<Typed_AST> initializer;
    
    Typed_AST_Let(String id, bool is_mut, Ref<Typed_AST_Type_Signiture> specified_type, Ref<Typed_AST> initializer);
    ~Typed_AST_Let() override;
    void compile(Compiler &c) override;
};

struct Untyped_AST_Block;
Ref<Typed_AST_Block> typecheck(Untyped_AST_Block *node);
