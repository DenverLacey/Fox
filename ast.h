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
#include "definitions.h"

enum class Untyped_AST_Kind {
    // literals
    Bool,
    Char,
    Float,
    Ident,
    Path,
    Int,
    Str,
    Array,
    Slice,
    Struct,
    
    // nullary
    Noinit,
    
    // unary
    Negation,
    Not,
    Address_Of,
    Address_Of_Mut,
    Deref,
    Defer,
    Return,
    Break,
    Continue,
    
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
    Field_Access,
    Field_Access_Tuple,
    Subscript,
    Range,
    Inclusive_Range,
    Binding,
    Invocation,
    Match_Arm,
    Cast,
    
    // ternary
    
    // multiary
    Block,
    Comma,
    Tuple,
    
    // patterns
    Pattern_Underscore,
    Pattern_Ident,
    Pattern_Tuple,
    Pattern_Struct,
    Pattern_Enum,
    Pattern_Value,
    
    // declarations
    Let,
    Struct_Decl,
    Enum_Decl,
    Trait_Decl,
    Fn_Decl,
    Fn_Decl_Header,
    Method_Decl,
    Method_Decl_Header,
    Impl_Decl,
    Import_Decl,
    
    // builtins
    Builtin,
    Builtin_Sizeof,
    Builtin_Alloc,
    Builtin_Printlike,
    
    // unique
    If,
    For,
    Match,
    Type_Signature,
    Generic_Specification,
    Dot_Call,
};

struct Typed_AST;
struct Typer;

struct Untyped_AST {
    Untyped_AST_Kind kind;
    Code_Location location;
    
    virtual ~Untyped_AST() = default;
    void print() const;
    virtual Ref<Typed_AST> typecheck(Typer &t) = 0;
    virtual Ref<Untyped_AST> clone() = 0;
};

struct Untyped_AST_Bool : public Untyped_AST {
    bool value;
    
    Untyped_AST_Bool(bool value, Code_Location location);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Char : public Untyped_AST {
    char32_t value;
    
    Untyped_AST_Char(char32_t value, Code_Location location);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Float : public Untyped_AST {
    double value;
    
    Untyped_AST_Float(double value, Code_Location location);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Symbol : public Untyped_AST {
    const char *display_str() const;
    bool matches(const char *symbol) const;
};

struct Untyped_AST_Ident : public Untyped_AST_Symbol {
    String id;
    
    Untyped_AST_Ident(String id, Code_Location location);
    ~Untyped_AST_Ident() override;
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Path : public Untyped_AST_Symbol {
    Ref<Untyped_AST_Ident> lhs;
    Ref<Untyped_AST_Symbol> rhs;
    
    Untyped_AST_Path(Ref<Untyped_AST_Ident> lhs, Ref<Untyped_AST_Symbol> rhs, Code_Location location);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Int : public Untyped_AST {
    int64_t value;
    
    Untyped_AST_Int(int64_t value, Code_Location location);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Str : public Untyped_AST {
    String value;
    
    Untyped_AST_Str(String value, Code_Location location);
    ~Untyped_AST_Str() override;
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Nullary : Untyped_AST {
    Untyped_AST_Nullary(Untyped_AST_Kind kind, Code_Location location);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Unary : public Untyped_AST {
    Ref<Untyped_AST> sub;
    
    Untyped_AST_Unary(Untyped_AST_Kind kind, Ref<Untyped_AST> sub, Code_Location location);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Return : public Untyped_AST_Unary {
    Untyped_AST_Return(Ref<Untyped_AST> sub, Code_Location location);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Loop_Control : public Untyped_AST {
    String label;

    Untyped_AST_Loop_Control(Untyped_AST_Kind kind, String label, Code_Location location);
    ~Untyped_AST_Loop_Control();
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Binary : public Untyped_AST {
    Ref<Untyped_AST> lhs;
    Ref<Untyped_AST> rhs;
    
    Untyped_AST_Binary(Untyped_AST_Kind kind, Ref<Untyped_AST> lhs, Ref<Untyped_AST> rhs, Code_Location location);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Ternary : public Untyped_AST {
    Ref<Untyped_AST> lhs;
    Ref<Untyped_AST> mid;
    Ref<Untyped_AST> rhs;
    
    Untyped_AST_Ternary(Untyped_AST_Kind kind, Ref<Untyped_AST> lhs, Ref<Untyped_AST> mid, Ref<Untyped_AST> rh, Code_Location location);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Multiary : public Untyped_AST {
    std::vector<Ref<Untyped_AST>> nodes;
    
    Untyped_AST_Multiary(Untyped_AST_Kind kind, Code_Location location);
    void add(Ref<Untyped_AST> node);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Array : public Untyped_AST {
    size_t count;
    Ref<Value_Type> array_type;
    Ref<Untyped_AST_Multiary> element_nodes;
    
    Untyped_AST_Array(Untyped_AST_Kind kind, size_t count, Ref<Value_Type> array_type, Ref<Untyped_AST_Multiary> element_nodes, Code_Location location);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Struct_Literal : public Untyped_AST {
    Ref<Untyped_AST_Symbol> struct_id;
    Ref<Untyped_AST_Multiary> bindings;
    
    Untyped_AST_Struct_Literal(Ref<Untyped_AST_Symbol> struct_id, Ref<Untyped_AST_Multiary> bindings, Code_Location location);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Builtin : public Untyped_AST {
    String id;
    
    Untyped_AST_Builtin(String id, Code_Location location);
    ~Untyped_AST_Builtin();
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Builtin_Printlike : public Untyped_AST {
    enum Kind {
        Puts,
        Print,
    } printlike_kind;
    Ref<Untyped_AST> arg;

    Untyped_AST_Builtin_Printlike(Kind kind, Ref<Untyped_AST> arg, Code_Location location);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Field_Access : public Untyped_AST {
    Ref<Untyped_AST> instance;
    String field_id;
    
    Untyped_AST_Field_Access(Ref<Untyped_AST> instance, String field_id, Code_Location location);
    ~Untyped_AST_Field_Access() override;
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Pattern : public Untyped_AST {
    bool are_all_variables_mut();
    bool are_no_variables_mut();
};

struct Untyped_AST_Pattern_Underscore : public Untyped_AST_Pattern {
    Untyped_AST_Pattern_Underscore(Code_Location location);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Pattern_Ident : public Untyped_AST_Pattern {
    bool is_mut;
    String id;
    
    Untyped_AST_Pattern_Ident(bool is_mut, String id, Code_Location location);
    ~Untyped_AST_Pattern_Ident();
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Pattern_Tuple : public Untyped_AST_Pattern {
    std::vector<Ref<Untyped_AST_Pattern>> sub_patterns;
    
    Untyped_AST_Pattern_Tuple(Code_Location location);
    void add(Ref<Untyped_AST_Pattern> sub);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Pattern_Struct : public Untyped_AST_Pattern_Tuple {
    Ref<Untyped_AST_Symbol> struct_id;
    
    Untyped_AST_Pattern_Struct(Ref<Untyped_AST_Symbol> struct_id, Code_Location location);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Pattern_Enum : public Untyped_AST_Pattern_Tuple {
    Ref<Untyped_AST_Symbol> enum_id;
    
    Untyped_AST_Pattern_Enum(Ref<Untyped_AST_Symbol> enum_id, Code_Location location);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Pattern_Value : public Untyped_AST_Pattern {
    Ref<Untyped_AST> value;
    
    Untyped_AST_Pattern_Value(Ref<Untyped_AST> value, Code_Location location);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_If : public Untyped_AST {
    Ref<Untyped_AST> cond;
    Ref<Untyped_AST> then;
    Ref<Untyped_AST> else_;
    
    Untyped_AST_If(Ref<Untyped_AST> cond, Ref<Untyped_AST> then, Ref<Untyped_AST> else_, Code_Location location);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_While : public Untyped_AST {
    Ref<Untyped_AST_Ident> label;
    Ref<Untyped_AST> condition;
    Ref<Untyped_AST_Multiary> body;

    Untyped_AST_While(Ref<Untyped_AST_Ident> label, Ref<Untyped_AST> condition, Ref<Untyped_AST_Multiary> body, Code_Location location);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_For : public Untyped_AST {
    Ref<Untyped_AST_Ident> label;
    Ref<Untyped_AST_Pattern> target;
    String counter;
    Ref<Untyped_AST> iterable;
    Ref<Untyped_AST_Multiary> body;
    
    Untyped_AST_For(Ref<Untyped_AST_Ident> label, Ref<Untyped_AST_Pattern> target, String counter, Ref<Untyped_AST> iterable, Ref<Untyped_AST_Multiary> body, Code_Location location);
    ~Untyped_AST_For();
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Match : public Untyped_AST {
    Ref<Untyped_AST> cond;
    Ref<Untyped_AST> default_arm;
    Ref<Untyped_AST_Multiary> arms;
    
    Untyped_AST_Match(Ref<Untyped_AST> cond, Ref<Untyped_AST> default_arm, Ref<Untyped_AST_Multiary> arms, Code_Location location);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Type_Signature : public Untyped_AST {
    Ref<Value_Type> value_type;
    
    Untyped_AST_Type_Signature(Ref<Value_Type> value_type, Code_Location location);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Let : public Untyped_AST {
    bool is_const;
    Ref<Untyped_AST_Pattern> target;
    Ref<Untyped_AST_Type_Signature> specified_type;
    Ref<Untyped_AST> initializer;
    
    Untyped_AST_Let(bool is_const, Ref<Untyped_AST_Pattern> target, Ref<Untyped_AST_Type_Signature> specified_type, Ref<Untyped_AST> initializer, Code_Location location);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Generic_Specification : public Untyped_AST {
    Ref<Untyped_AST_Symbol> id;
    Ref<Untyped_AST_Multiary> type_params;
    
    Untyped_AST_Generic_Specification(Ref<Untyped_AST_Symbol> id, Ref<Untyped_AST_Multiary> type_params, Code_Location location);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Struct_Declaration : public Untyped_AST {
    struct Field {
        String id;
        Ref<Untyped_AST_Type_Signature> type;
    };
    
    String id;
    std::vector<Field> fields;
    
    Untyped_AST_Struct_Declaration(String id, Code_Location location);
    ~Untyped_AST_Struct_Declaration() override;
    void add_field(String id, Ref<Untyped_AST_Type_Signature> type);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Enum_Declaration : public Untyped_AST {
    struct Variant {
        String id;
        Ref<Untyped_AST_Multiary> payload;
    };
    
    String id;
    std::vector<Variant> variants;
    
    Untyped_AST_Enum_Declaration(String id, Code_Location location);
    ~Untyped_AST_Enum_Declaration() override;
    void add_variant(String id, Ref<Untyped_AST_Multiary> payload);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Trait_Declaration : public Untyped_AST {
    String id;
    Ref<Untyped_AST_Multiary> body;

    Untyped_AST_Trait_Declaration(String id, Ref<Untyped_AST_Multiary> body, Code_Location location);
    ~Untyped_AST_Trait_Declaration();
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Fn_Declaration_Header : public Untyped_AST {
    String id;
    Ref<Untyped_AST_Multiary> params;
    bool varargs;
    Ref<Untyped_AST_Type_Signature> return_type_signature;

    Untyped_AST_Fn_Declaration_Header(Untyped_AST_Kind kind, String id, Ref<Untyped_AST_Multiary> params, bool varargs, Ref<Untyped_AST_Type_Signature> return_type_signature, Code_Location location);
    ~Untyped_AST_Fn_Declaration_Header();
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Fn_Declaration : public Untyped_AST_Fn_Declaration_Header {
    Ref<Untyped_AST_Multiary> body;
    
    Untyped_AST_Fn_Declaration(Untyped_AST_Kind kind, String id, Ref<Untyped_AST_Multiary> params, bool varargs, Ref<Untyped_AST_Type_Signature> return_type_signature, Ref<Untyped_AST_Multiary> body, Code_Location location);
    ~Untyped_AST_Fn_Declaration();
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Impl_Declaration : public Untyped_AST {
    Ref<Untyped_AST_Symbol> target;
    Ref<Untyped_AST_Symbol> for_;
    Ref<Untyped_AST_Multiary> body;
    
    Untyped_AST_Impl_Declaration(Ref<Untyped_AST_Symbol> target, Ref<Untyped_AST_Symbol> for_, Ref<Untyped_AST_Multiary> body, Code_Location location);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Dot_Call : public Untyped_AST {
    Ref<Untyped_AST> receiver;
    Ref<Untyped_AST_Multiary> args;
    String method_id;
    
    Untyped_AST_Dot_Call(Ref<Untyped_AST> receiver, String method_id, Ref<Untyped_AST_Multiary> args, Code_Location location);
    ~Untyped_AST_Dot_Call();
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};

struct Untyped_AST_Import_Declaration : public Untyped_AST {
    Ref<Untyped_AST_Symbol> path;
    Ref<Untyped_AST_Ident> rename_id;
    
    Untyped_AST_Import_Declaration(Ref<Untyped_AST_Symbol> path, Ref<Untyped_AST_Ident> rename_id, Code_Location location);
    Ref<Typed_AST> typecheck(Typer &t) override;
    Ref<Untyped_AST> clone() override;
};
