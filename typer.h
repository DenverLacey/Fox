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
    Ident_Struct,
    Ident_Enum,
    Ident_Func,
    Ident_Module,
    Int,
    Str,
    Array,
    Slice,
    Enum,
    Builtin,
    
    // nullary
    Allocate,
    
    // unary
    Negation,
    Not,
    Address_Of,
    Address_Of_Mut,
    Deref,
    Return,
    
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
    Subscript,
    Negative_Subscript,
    Range,
    Inclusive_Range,
    Function_Call,
    Builtin_Call,
    Match_Arm,
    
    // ternary
    
    // multiary
    Block,
    Comma,
    Tuple,
    
    // declarations
    Let,
    Fn_Decl,
    
    // casts
    Cast_Bool_Int,
    Cast_Char_Int,
    Cast_Int_Float,
    Cast_Float_Int,
    
    // unique
    If,
    For,
    For_Range,
    Match,
    Type_Signature,
    Field_Access,
    Processed_Pattern,
    Match_Pattern,
};

struct Compiler;

struct Typed_AST {
    Typed_AST_Kind kind;
    Value_Type type;
    
    virtual ~Typed_AST() = default;
    void print(struct Interpreter *interp) const;
    virtual void compile(Compiler &c) = 0;
    virtual bool is_constant(Compiler &c) = 0;
};

struct Typed_AST_Bool : public Typed_AST {
    bool value;
    
    Typed_AST_Bool(bool value);
    void compile(Compiler &c) override;
    bool is_constant(Compiler &c) override;
};

struct Typed_AST_Char : public Typed_AST {
    char32_t value;
    
    Typed_AST_Char(char32_t value);
    void compile(Compiler &c) override;
    bool is_constant(Compiler &c) override;
};

struct Typed_AST_Float : public Typed_AST {
    double value;
    
    Typed_AST_Float(double value);
    void compile(Compiler &c) override;
    bool is_constant(Compiler &c) override;
};

struct Typed_AST_Ident : public Typed_AST {
    String id;
    
    Typed_AST_Ident(String id, Value_Type type);
    ~Typed_AST_Ident() override;
    void compile(Compiler &c) override;
    bool is_constant(Compiler &c) override;
};

struct Typed_AST_UUID : public Typed_AST {
    UUID uuid;
    
    Typed_AST_UUID(Typed_AST_Kind kind, UUID uuid, Value_Type type);
    void compile(Compiler &c) override;
    bool is_constant(Compiler &c) override;
};

struct Typed_AST_Int : public Typed_AST {
    int64_t value;
    
    Typed_AST_Int(int64_t value);
    void compile(Compiler &c) override;
    bool is_constant(Compiler &c) override;
};

struct Typed_AST_Str : public Typed_AST {
    String value;
    
    Typed_AST_Str(String value);
    ~Typed_AST_Str() override;
    void compile(Compiler &c) override;
    bool is_constant(Compiler &c) override;
};

struct Builtin_Definition;
struct Typed_AST_Builtin : public Typed_AST {
    Builtin_Definition *defn;
    
    Typed_AST_Builtin(Builtin_Definition *defn);
    void compile(Compiler &c) override;
    bool is_constant(Compiler &c) override;
};

struct Typed_AST_Nullary : public Typed_AST {
    Typed_AST_Nullary(Typed_AST_Kind kind, Value_Type type);
    void compile(Compiler &c) override;
    bool is_constant(Compiler &c) override;
};

struct Typed_AST_Unary : public Typed_AST {
    Ref<Typed_AST> sub;
    
    Typed_AST_Unary(Typed_AST_Kind kind, Value_Type type, Ref<Typed_AST> sub);
    void compile(Compiler &c) override;
    bool is_constant(Compiler &c) override;
};

struct Typed_AST_Return : public Typed_AST_Unary {
    Typed_AST_Return(Ref<Typed_AST> sub);
    void compile(Compiler &c) override;
    bool is_constant(Compiler &c) override;
};

struct Typed_AST_Binary : public Typed_AST {
    Ref<Typed_AST> lhs;
    Ref<Typed_AST> rhs;
    
    Typed_AST_Binary(Typed_AST_Kind kind, Value_Type type, Ref<Typed_AST> lhs, Ref<Typed_AST> rhs);
    void compile(Compiler &c) override;
    bool is_constant(Compiler &c) override;
};

struct Typed_AST_Ternary : public Typed_AST {
    Ref<Typed_AST> lhs;
    Ref<Typed_AST> mid;
    Ref<Typed_AST> rhs;
    
    Typed_AST_Ternary(Typed_AST_Kind kind, Value_Type type, Ref<Typed_AST> lhs, Ref<Typed_AST> mid, Ref<Typed_AST> rhs);
    void compile(Compiler &c) override;
    bool is_constant(Compiler &c) override;
};

struct Typed_AST_Multiary : public Typed_AST {
    std::vector<Ref<Typed_AST>> nodes;
    
    Typed_AST_Multiary(Typed_AST_Kind kind);
    void add(Ref<Typed_AST> node);
    void compile(Compiler &c) override;
    bool is_constant(Compiler &c) override;
};

struct Typed_AST_Array : public Typed_AST {
    size_t count;
    Ref<Value_Type> array_type;
    Ref<Typed_AST_Multiary> element_nodes;
    
    Typed_AST_Array(Value_Type type, Typed_AST_Kind kind, size_t count, Ref<Value_Type> array_type, Ref<Typed_AST_Multiary> element_nodes);
    void compile(Compiler &c) override;
    bool is_constant(Compiler &c) override;
};

struct Typed_AST_Enum_Literal : public Typed_AST {
    runtime::Int tag;
    Ref<Typed_AST_Multiary> payload;
    
    Typed_AST_Enum_Literal(Value_Type enum_type, runtime::Int tag, Ref<Typed_AST_Multiary> payload);
    void compile(Compiler &c) override;
    bool is_constant(Compiler &c) override;
};

struct Typed_AST_If : public Typed_AST {
    Ref<Typed_AST> cond;
    Ref<Typed_AST> then;
    Ref<Typed_AST> else_;
    
    Typed_AST_If(Value_Type type, Ref<Typed_AST> cond, Ref<Typed_AST> then, Ref<Typed_AST> else_);
    void compile(Compiler &c) override;
    bool is_constant(Compiler &c) override;
};

struct Typed_AST_Type_Signature : public Typed_AST {
    Ref<Value_Type> value_type;
    
    Typed_AST_Type_Signature(Ref<Value_Type> value_type);
    void compile(Compiler &c) override;
    bool is_constant(Compiler &c) override;
};

struct Typed_AST_Processed_Pattern : public Typed_AST {
    struct Binding {
        String id;
        Value_Type type;
    };
    std::vector<Binding> bindings;
    
    Typed_AST_Processed_Pattern();
    ~Typed_AST_Processed_Pattern();
    void add_binding(String id, Value_Type type, bool is_mut);
    void compile(Compiler &c) override;
    bool is_constant(Compiler &c) override;
};

struct Typed_AST_Match_Pattern : public Typed_AST {
    enum Binding_Kind : uint8_t {
        None,
        Value,
        Variable,
    };
    
    struct Binding {
        Binding_Kind kind;
        Size offset;
        union {
            Ref<Typed_AST> value_node;
            struct {
                String id;
                Value_Type type;
            } variable_info;
        };
        
        Binding();
        bool is_none() const;
    };
    
    std::vector<Binding> bindings;
    
    Typed_AST_Match_Pattern();
    void add_none_binding();
    void add_value_binding(Ref<Typed_AST> binding, Size offset);
    void add_variable_binding(String id, Value_Type type, Size offset);
    bool is_simple_value_pattern();
    void compile(Compiler &c) override;
    bool is_constant(Compiler &c) override;
};

struct Typed_AST_For : public Typed_AST {
    Ref<Typed_AST_Processed_Pattern> target;
    String counter;
    Ref<Typed_AST> iterable;
    Ref<Typed_AST_Multiary> body;
    
    Typed_AST_For(Typed_AST_Kind kind, Ref<Typed_AST_Processed_Pattern> target, String counter, Ref<Typed_AST> iterable, Ref<Typed_AST_Multiary> body);
    ~Typed_AST_For();
    void compile(Compiler &c) override;
    bool is_constant(Compiler &c) override;
};

struct Typed_AST_Match : public Typed_AST {
    Ref<Typed_AST> cond;
    Ref<Typed_AST> default_arm;
    Ref<Typed_AST_Multiary> arms;
    
    Typed_AST_Match(Ref<Typed_AST> cond, Ref<Typed_AST> default_arm, Ref<Typed_AST_Multiary> arms);
    void compile(Compiler &c) override;
    bool is_constant(Compiler &c) override;
};

struct Typed_AST_Let : public Typed_AST {
    bool is_const;
    Ref<Typed_AST_Processed_Pattern> target;
    Ref<Typed_AST_Type_Signature> specified_type;
    Ref<Typed_AST> initializer;
    
    Typed_AST_Let(bool is_const, Ref<Typed_AST_Processed_Pattern> target, Ref<Typed_AST_Type_Signature> specified_type, Ref<Typed_AST> initializer);
    void compile(Compiler &c) override;
    bool is_constant(Compiler &c) override;
};

struct Typed_AST_Field_Access : public Typed_AST {
    bool deref;
    Ref<Typed_AST> instance;
    Size field_offset;
    
    Typed_AST_Field_Access(Value_Type type, bool deref, Ref<Typed_AST> instance, Size field_offset);
    void compile(Compiler &c) override;
    bool is_constant(Compiler &c) override;
};

struct Typed_AST_Fn_Declaration : public Typed_AST {
    struct Function_Definition *defn;
    Ref<Typed_AST_Multiary> body;
    
    Typed_AST_Fn_Declaration(Function_Definition *defn, Ref<Typed_AST_Multiary> body);
    void compile(Compiler &c) override;
    bool is_constant(Compiler &c) override;
};

struct Typed_AST_Cast : public Typed_AST {
    Ref<Typed_AST> expr;
    
    Typed_AST_Cast(Typed_AST_Kind kind, Value_Type type, Ref<Typed_AST> expr);
    void compile(Compiler &c) override;
    bool is_constant(Compiler &c) override;
};

Ref<Typed_AST_Multiary> typecheck(struct Interpreter &interp, struct Module *module, Ref<struct Untyped_AST_Multiary> node);
