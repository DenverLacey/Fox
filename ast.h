//
//  ast.h
//  Fox
//
//  Created by Denver Lacey on 7/7/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#pragma once

#include <optional>
#include <vector>
#include "mem.h"
#include "value.h"

enum class Untyped_AST_Kind {
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
    Less,
    Less_Eq,
    Greater,
    Greater_Eq,
    And,
    Or,
    While,
    
    // ternary
    
    // block
    Block,
    Comma,
    
    // unique
    If,
    Let,
    Type_Signiture,
};

struct Typed_AST;
struct Typer;

struct Untyped_AST {
    Untyped_AST_Kind kind;
    
    virtual ~Untyped_AST() = default;
    void print() const;
    virtual Ref<Typed_AST> typecheck(Typer &t) = 0;
    virtual Ref<Untyped_AST> clone() = 0;
};

struct Untyped_AST_Bool : public Untyped_AST {
    bool value;
    
    Untyped_AST_Bool(bool value);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Char : public Untyped_AST {
    char32_t value;
    
    Untyped_AST_Char(char32_t value);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Float : public Untyped_AST {
    double value;
    
    Untyped_AST_Float(double value);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Ident : public Untyped_AST {
    String id;
    
    Untyped_AST_Ident(String id);
    ~Untyped_AST_Ident() override;
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Int : public Untyped_AST {
    int64_t value;
    
    Untyped_AST_Int(int64_t value);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Str : public Untyped_AST {
    String value;
    
    Untyped_AST_Str(String value);
    ~Untyped_AST_Str() override;
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Unary : public Untyped_AST {
    Ref<Untyped_AST> sub;
    
    Untyped_AST_Unary(Untyped_AST_Kind kind, Ref<Untyped_AST> sub);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Binary : public Untyped_AST {
    Ref<Untyped_AST> lhs;
    Ref<Untyped_AST> rhs;
    
    Untyped_AST_Binary(Untyped_AST_Kind kind, Ref<Untyped_AST> lhs, Ref<Untyped_AST> rhs);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Ternary : public Untyped_AST {
    Ref<Untyped_AST> lhs;
    Ref<Untyped_AST> mid;
    Ref<Untyped_AST> rhs;
    
    Untyped_AST_Ternary(Untyped_AST_Kind kind, Ref<Untyped_AST> lhs, Ref<Untyped_AST> mid, Ref<Untyped_AST> rhs);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Block : public Untyped_AST {
    std::vector<Ref<Untyped_AST>> nodes;
    
    Untyped_AST_Block(Untyped_AST_Kind kind);
    void add(Ref<Untyped_AST> node);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_If : public Untyped_AST {
    Ref<Untyped_AST> cond;
    Ref<Untyped_AST> then;
    Ref<Untyped_AST> else_;
    
    Untyped_AST_If(Ref<Untyped_AST> cond, Ref<Untyped_AST> then, Ref<Untyped_AST> else_);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Type_Signiture : public Untyped_AST {
    Ref<Value_Type> value_type;
    
    Untyped_AST_Type_Signiture(Ref<Value_Type> value_type);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Let : public Untyped_AST {
    String id;
    bool is_mut;
    Ref<Untyped_AST_Type_Signiture> specified_type;
    Ref<Untyped_AST> initializer;
    
    Untyped_AST_Let(String id, bool is_mut, Ref<Untyped_AST_Type_Signiture> specified_type, Ref<Untyped_AST> initializer);
    ~Untyped_AST_Let() override;
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};
