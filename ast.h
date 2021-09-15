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
    Array,
    Slice,
    
    // unary
    Negation,
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
    And,
    Or,
    While,
    Dot,
    Dot_Tuple,
    Subscript,
    Range,
    Inclusive_Range,
    
    // ternary
    
    // multiary
    Block,
    Comma,
    Tuple,
    
    // patterns
    Pattern_Underscore,
    Pattern_Ident,
    Pattern_Tuple,
    
    // unique
    If,
    For,
    Let,
    Type_Signiture,
    Generic_Specialization,
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

struct Untyped_AST_Multiary : public Untyped_AST {
    std::vector<Ref<Untyped_AST>> nodes;
    
    Untyped_AST_Multiary(Untyped_AST_Kind kind);
    void add(Ref<Untyped_AST> node);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Array : public Untyped_AST {
    size_t count;
    Ref<Value_Type> array_type;
    Ref<Untyped_AST_Multiary> element_nodes;
    
    Untyped_AST_Array(Untyped_AST_Kind kind, size_t count, Ref<Value_Type> array_type, Ref<Untyped_AST_Multiary> element_nodes);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Pattern : public Untyped_AST {
    bool are_all_variables_mut();
};

struct Untyped_AST_Pattern_Underscore : public Untyped_AST_Pattern {
    Untyped_AST_Pattern_Underscore();
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Pattern_Ident : public Untyped_AST_Pattern {
    bool is_mut;
    String id;
    
    Untyped_AST_Pattern_Ident(bool is_mut, String id);
    ~Untyped_AST_Pattern_Ident();
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Pattern_Tuple : public Untyped_AST_Pattern {
    std::vector<Ref<Untyped_AST_Pattern>> sub_patterns;
    
    Untyped_AST_Pattern_Tuple();
    void add(Ref<Untyped_AST_Pattern> sub);
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

struct Untyped_AST_For : public Untyped_AST {
    Ref<Untyped_AST_Pattern> target;
    String counter;
    Ref<Untyped_AST> iterable;
    Ref<Untyped_AST_Multiary> body;
    
    Untyped_AST_For(Ref<Untyped_AST_Pattern> target, String counter, Ref<Untyped_AST> iterable, Ref<Untyped_AST_Multiary> body);
    ~Untyped_AST_For();
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
    bool is_const;
    Ref<Untyped_AST_Pattern> target;
    Ref<Untyped_AST_Type_Signiture> specified_type;
    Ref<Untyped_AST> initializer;
    
    Untyped_AST_Let(bool is_const, Ref<Untyped_AST_Pattern> target, Ref<Untyped_AST_Type_Signiture> specified_type, Ref<Untyped_AST> initializer);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Generic_Specialization : public Untyped_AST {
    String id;
    std::vector<Ref<Untyped_AST_Type_Signiture>> params;
    
    Untyped_AST_Generic_Specialization(String id, std::vector<Ref<Untyped_AST_Type_Signiture>> &&params);
    ~Untyped_AST_Generic_Specialization() override;
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};
