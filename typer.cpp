//
//  typer.cpp
//  Fox
//
//  Created by Denver Lacey on 23/8/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#include "typer.h"

#include "ast.h"
#include "builtins.h"
#include "compiler.h"
#include "error.h"
#include "interpreter.h"

#include <forward_list>
#include <unistd.h>
#include <unordered_map>
#include <string>
#include <sstream>

Typed_AST_Bool::Typed_AST_Bool(bool value, Code_Location location) {
    kind = Typed_AST_Kind::Bool;
    type.kind = Value_Type_Kind::Bool;
    this->value = value;
    this->location = location;
}

bool Typed_AST_Bool::is_constant(Compiler &c) {
    return true;
}

Typed_AST_Char::Typed_AST_Char(char32_t value, Code_Location location) {
    kind = Typed_AST_Kind::Char;
    type.kind = Value_Type_Kind::Char;
    this->value = value;
    this->location = location;
}

bool Typed_AST_Char::is_constant(Compiler &c) {
    return true;
}

Typed_AST_Float::Typed_AST_Float(double value, Code_Location location) {
    kind = Typed_AST_Kind::Float;
    type.kind = Value_Type_Kind::Float;
    this->value = value;
    this->location = location;
}

bool Typed_AST_Float::is_constant(Compiler &c) {
    return true;
}

Typed_AST_Ident::Typed_AST_Ident(String id, Value_Type type, Code_Location location) {
    kind = Typed_AST_Kind::Ident;
    this->type = type;
    this->id = id;
    this->location = location;
}

Typed_AST_Ident::~Typed_AST_Ident() {
    id.free();
}

bool Typed_AST_Ident::is_constant(Compiler &c) {
    auto [status, _] = c.find_variable(id);
    return status == Find_Variable_Result::Found_Constant;
}

Typed_AST_UUID::Typed_AST_UUID(Typed_AST_Kind kind, UUID uuid, Value_Type type, Code_Location location) {
    this->kind = kind;
    this->uuid = uuid;
    this->type = type;
    this->location = location;
}

bool Typed_AST_UUID::is_constant(Compiler &c) {
    return true;
}

Typed_AST_Byte::Typed_AST_Byte(uint8_t value, Code_Location location) {
    kind = Typed_AST_Kind::Byte;
    type.kind = Value_Type_Kind::Byte;
    this->value = value;
    this->location = location;
}

bool Typed_AST_Byte::is_constant(Compiler &c) {
    return true;
}

Typed_AST_Int::Typed_AST_Int(int64_t value, Code_Location location) {
    kind = Typed_AST_Kind::Int;
    type.kind = Value_Type_Kind::Int;
    this->value = value;
    this->location = location;
}

bool Typed_AST_Int::is_constant(Compiler &c) {
    return true;
}

Typed_AST_Str::Typed_AST_Str(String value, Code_Location location) {
    kind = Typed_AST_Kind::Str;
    type.kind = Value_Type_Kind::Str;
    this->value = value;
    this->location = location;
}

Typed_AST_Str::~Typed_AST_Str() {
    value.free();
}

bool Typed_AST_Str::is_constant(Compiler &c) {
    return true;
}

Typed_AST_Ptr::Typed_AST_Ptr(void *value, Code_Location location) {
    kind = Typed_AST_Kind::Ptr;
    type.kind = Value_Type_Kind::Ptr;
    this->value = value;
    this->location = location;
}

bool Typed_AST_Ptr::is_constant(Compiler &c) {
    return true;
}

Typed_AST_Builtin::Typed_AST_Builtin(Builtin_Definition *defn, Value_Type *type, Code_Location location) {
    this->kind = Typed_AST_Kind::Builtin;
    this->defn = defn;
    this->location = location;
    
    if (type) {
        this->type = *type;
    } else {
        // @NOTE:
        //  This may cause some problems else where in the code that expects a Bulitin's type to
        // just be the return type of the builtin function.
        //
        // this->type = *defn->type.data.func.return_type;

        this->type = defn->type;
    }
}

bool Typed_AST_Builtin::is_constant(Compiler &c) {
    return true;
}

Typed_AST_Nullary::Typed_AST_Nullary(Typed_AST_Kind kind, Value_Type type, Code_Location location) {
    this->kind = kind;
    this->type = type;
    this->location = location;
}

bool Typed_AST_Nullary::is_constant(Compiler &c) {
    bool constant = true;
    switch (kind) {
        case Typed_AST_Kind::Allocate:
            break;
            
        default:
            internal_error("Invalid nullary kind: %d.", kind);
            break;
    }
 
    return constant;
}

Typed_AST_Unary::Typed_AST_Unary(Typed_AST_Kind kind, Value_Type type, Ref<Typed_AST> sub, Code_Location location) {
    this->kind = kind;
    this->type = type;
    this->sub = sub;
    this->location = location;
}

bool Typed_AST_Unary::is_constant(Compiler &c) {
    return sub->is_constant(c);
}

Typed_AST_Return::Typed_AST_Return(bool variadic, Ref<Typed_AST> sub, Code_Location location)
    : Typed_AST_Unary(Typed_AST_Kind::Return, value_types::None, sub, location)
{
    this->variadic = variadic;
}

bool Typed_AST_Return::is_constant(Compiler &c) {
    return sub ? sub->is_constant(c) : true;
}

Typed_AST_Loop_Control::Typed_AST_Loop_Control(Typed_AST_Kind kind, String label, Code_Location location) {
    this->kind = kind;
    this->label = label;
    this->location = location;
}

Typed_AST_Loop_Control::~Typed_AST_Loop_Control() {
    label.free();
}

bool Typed_AST_Loop_Control::is_constant(Compiler &c) {
    return true;
}

Typed_AST_Binary::Typed_AST_Binary(
    Typed_AST_Kind kind, 
    Value_Type type,
    Ref<Typed_AST> lhs, 
    Ref<Typed_AST> rhs, 
    Code_Location location) 
{
    this->kind = kind;
    this->type = type;
    this->lhs = lhs;
    this->rhs = rhs;
    this->location = location;
}

bool Typed_AST_Binary::is_constant(Compiler &c) {
    return lhs->is_constant(c) && rhs->is_constant(c);
}

Typed_AST_Ternary::Typed_AST_Ternary(
    Typed_AST_Kind kind, 
    Value_Type type, 
    Ref<Typed_AST> lhs, 
    Ref<Typed_AST> mid, 
    Ref<Typed_AST> rhs, 
    Code_Location location) 
{
    this->kind = kind;
    this->type = type;
    this->lhs = lhs;
    this->mid = mid;
    this->rhs = rhs;
    this->location = location;
}

bool Typed_AST_Ternary::is_constant(Compiler &c) {
    return lhs->is_constant(c) && mid->is_constant(c) && rhs->is_constant(c);
}

Typed_AST_Multiary::Typed_AST_Multiary(Typed_AST_Kind kind, Code_Location location) {
    this->kind = kind;
    this->type.kind = Value_Type_Kind::None;
    this->location = location;
}

void Typed_AST_Multiary::add(Ref<Typed_AST> node) {
    nodes.push_back(node);
}

bool Typed_AST_Multiary::is_constant(Compiler &c) {
    for (auto n : nodes) {
        if (!n->is_constant(c)) {
            return false;
        }
    }
    return true;
}

Typed_AST_Array::Typed_AST_Array(
    Value_Type type,
    Typed_AST_Kind kind,
    size_t count,
    Ref<Value_Type> array_type,
    Ref<Typed_AST_Multiary> element_nodes, 
    Code_Location location)
{
    this->type = type;
    this->kind = kind;
    this->count = count;
    this->array_type = array_type;
    this->element_nodes = element_nodes;
    this->location = location;
}

bool Typed_AST_Array::is_constant(Compiler &c) {
    return element_nodes->is_constant(c);
}

Typed_AST_Enum_Literal::Typed_AST_Enum_Literal(
    Value_Type enum_type,
    runtime::Int tag,
    Ref<Typed_AST_Multiary> payload, 
    Code_Location location)
{
    this->kind = Typed_AST_Kind::Enum;
    this->type = enum_type;
    this->tag = tag;
    this->payload = payload;
    this->location = location;
}

bool Typed_AST_Enum_Literal::is_constant(Compiler &c) {
    if (payload) {
        return payload->is_constant(c);
    }
    return true;
}

Typed_AST_If::Typed_AST_If(
    Value_Type type,
    Ref<Typed_AST> cond,
    Ref<Typed_AST> then,
    Ref<Typed_AST> else_, 
    Code_Location location)
{
    kind = Typed_AST_Kind::If;
    this->type = type;
    this->cond = cond;
    this->then = then;
    this->else_ = else_;
    this->location = location;
}

bool Typed_AST_If::is_constant(Compiler &c) {
    return cond->is_constant(c) && then->is_constant(c) && else_->is_constant(c);
}

Typed_AST_Type_Signature::Typed_AST_Type_Signature(Ref<Value_Type> value_type, Code_Location location) {
    this->kind = Typed_AST_Kind::Type_Signature;
    this->value_type = value_type;
    this->location = location;
}

bool Typed_AST_Type_Signature::is_constant(Compiler &c) {
    return true;
}

Typed_AST_Processed_Pattern::Typed_AST_Processed_Pattern(Code_Location location) {
    kind = Typed_AST_Kind::Processed_Pattern;
    this->location = location;
}

Typed_AST_Processed_Pattern::~Typed_AST_Processed_Pattern() {
    for (auto &b : bindings) {
        b.id.free();
    }
}

void Typed_AST_Processed_Pattern::add_binding(
    String id,
    Value_Type type,
    bool is_mut)
{
    type.is_mut = is_mut;
    bindings.push_back({ id, type });
}

bool Typed_AST_Processed_Pattern::is_constant(Compiler &c) {
    return true;
}

Typed_AST_Match_Pattern::Typed_AST_Match_Pattern(Code_Location location) {
    this->kind = Typed_AST_Kind::Match_Pattern;
    this->location = location;
}

Typed_AST_Match_Pattern::Binding::Binding() {
    kind = Binding_Kind::None;
    offset = 0;
}

bool Typed_AST_Match_Pattern::Binding::is_none() const {
    return kind == Binding_Kind::None;
}

void Typed_AST_Match_Pattern::add_none_binding() {
    bindings.emplace_back();
}

void Typed_AST_Match_Pattern::add_value_binding(Ref<Typed_AST> binding, Size offest) {
    Binding b;
    b.kind = Binding_Kind::Value;
    b.offset = offest;
    b.value_node = binding;
    bindings.push_back(b);
}

void Typed_AST_Match_Pattern::add_variable_binding(
    String id,
    Value_Type type,
    Size offset)
{
    Binding b;
    b.kind = Binding_Kind::Variable;
    b.offset = offset;
    b.variable_info = { id, type };
    bindings.push_back(b);
}

bool Typed_AST_Match_Pattern::is_simple_value_pattern() {
    for (auto b : bindings) {
        if (b.kind == Binding_Kind::None ||
            b.kind == Binding_Kind::Variable)
        {
            return false;
        }
    }
    return true;
}

bool Typed_AST_Match_Pattern::is_constant(Compiler &c) {
    for (auto b : bindings) {
        if (b.kind == Binding_Kind::Value &&
            !b.value_node->is_constant(c))
        {
            return false;
        }
    }
    return true;
}

Typed_AST_While::Typed_AST_While(
    Ref<Typed_AST_Ident> label,
    Ref<Typed_AST> condition,
    Ref<Typed_AST_Multiary> body,
    Code_Location location)
{
    this->kind = Typed_AST_Kind::While;
    this->label = label;
    this->condition = condition;
    this->body = body;
    this->location = location;
}

bool Typed_AST_While::is_constant(Compiler &c) {
    return condition->is_constant(c) && body->is_constant(c);
}

Typed_AST_For::Typed_AST_For(
    Typed_AST_Kind kind,
    Ref<Typed_AST_Ident> label,
    Ref<Typed_AST_Processed_Pattern> target,
    String counter,
    Ref<Typed_AST> iterable,
    Ref<Typed_AST_Multiary> body, 
    Code_Location location)
{
    this->kind = kind;
    this->label = label;
    this->target = target;
    this->counter = counter;
    this->iterable = iterable;
    this->body = body;
    this->location = location;
}

Typed_AST_For::~Typed_AST_For() {
    counter.free();
}

bool Typed_AST_For::is_constant(Compiler &c) {
    return iterable->is_constant(c) && body->is_constant(c);
}

Typed_AST_Forever::Typed_AST_Forever(
    Ref<Typed_AST_Ident> label,
    Ref<Typed_AST_Multiary> body, 
    Code_Location location)
{
    this->kind = Typed_AST_Kind::Forever;
    this->label = label;
    this->body = body;
    this->location = location;
}

bool Typed_AST_Forever::is_constant(Compiler &c) {
    return body->is_constant(c);
}

Typed_AST_Match::Typed_AST_Match(
    Ref<Typed_AST> cond,
    Ref<Typed_AST> default_arm,
    Ref<Typed_AST_Multiary> arms, 
    Code_Location location)
{
    this->kind = Typed_AST_Kind::Match;
    this->cond = cond;
    this->default_arm = default_arm;
    this->arms = arms;
    this->location = location;
}

bool Typed_AST_Match::is_constant(Compiler &c) {
    if (!cond->is_constant(c)) return false;
    if (default_arm && !default_arm->is_constant(c)) return false;
    return arms->is_constant(c);
}

Typed_AST_Let::Typed_AST_Let(
    bool is_const,
    Ref<Typed_AST_Processed_Pattern> target,
    Ref<Typed_AST_Type_Signature> specified_type,
    Ref<Typed_AST> initializer, 
    Code_Location location)
{
    kind = Typed_AST_Kind::Let;
    this->is_const = is_const;
    this->target = target;
    this->specified_type = specified_type;
    this->initializer = initializer;
    this->location = location;
}

bool Typed_AST_Let::is_constant(Compiler &c) {
    return initializer->is_constant(c);
}

Typed_AST_Field_Access::Typed_AST_Field_Access(
    Value_Type type,
    bool deref,
    Ref<Typed_AST> instance,
    Size field_offset, 
    Code_Location location)
{
    this->kind = Typed_AST_Kind::Field_Access;
    this->type = type;
    this->deref = deref;
    this->instance = instance;
    this->field_offset = field_offset;
    this->location = location;
}

bool Typed_AST_Field_Access::is_constant(Compiler &c) {
    return instance->is_constant(c);
}

Typed_AST_Fn_Declaration::Typed_AST_Fn_Declaration(
    Function_Definition *defn,
    Ref<Typed_AST_Multiary> body, 
    Code_Location location)
{
    this->kind = Typed_AST_Kind::Fn_Decl;
    this->defn = defn;
    this->body = body;
    this->location = location;
}

bool Typed_AST_Fn_Declaration::is_constant(Compiler &c) {
    return true;
}

Typed_AST_Cast::Typed_AST_Cast(
    Typed_AST_Kind kind,
    Value_Type type,
    Ref<Typed_AST> expr, 
    Code_Location location)
{
    this->kind = kind;
    this->type = type;
    this->expr = expr;
    this->location = location;
}

bool Typed_AST_Cast::is_constant(Compiler &c) {
    return expr->is_constant(c);
}

Typed_AST_Variadic_Call::Typed_AST_Variadic_Call(
    Value_Type type,
    Size varargs_size,
    Ref<Typed_AST> func,
    Ref<Typed_AST_Multiary> args,
    Ref<Typed_AST_Multiary> varargs, 
    Code_Location location)
{
    this->kind = Typed_AST_Kind::Variadic_Call;
    this->type = type;
    this->varargs_size = varargs_size;
    this->func = func;
    this->args = args;
    this->varargs = varargs;
    this->location = location;
}

bool Typed_AST_Variadic_Call::is_constant(Compiler &c) {
    return func->is_constant(c) && args->is_constant(c) && varargs->is_constant(c);
}

static Typed_AST_Kind to_typed(Untyped_AST_Kind kind) {
    switch (kind) {
        case Untyped_AST_Kind::Bool:            return Typed_AST_Kind::Bool;
        case Untyped_AST_Kind::Char:            return Typed_AST_Kind::Char;
        case Untyped_AST_Kind::Float:           return Typed_AST_Kind::Float;
        case Untyped_AST_Kind::Ident:           return Typed_AST_Kind::Ident;
        case Untyped_AST_Kind::Int:             return Typed_AST_Kind::Int;
        case Untyped_AST_Kind::Str:             return Typed_AST_Kind::Str;
        case Untyped_AST_Kind::Array:           return Typed_AST_Kind::Array;
        case Untyped_AST_Kind::Slice:           return Typed_AST_Kind::Slice;
        case Untyped_AST_Kind::Range:           return Typed_AST_Kind::Range;
        case Untyped_AST_Kind::Inclusive_Range:
            return Typed_AST_Kind::Inclusive_Range;
        case Untyped_AST_Kind::Negation:        return Typed_AST_Kind::Negation;
        case Untyped_AST_Kind::Not:             return Typed_AST_Kind::Not;
        case Untyped_AST_Kind::Address_Of:      return Typed_AST_Kind::Address_Of;
        case Untyped_AST_Kind::Address_Of_Mut:  return Typed_AST_Kind::Address_Of_Mut;
        case Untyped_AST_Kind::Deref:           return Typed_AST_Kind::Deref;
        case Untyped_AST_Kind::Return:          return Typed_AST_Kind::Return;
        case Untyped_AST_Kind::Break:           return Typed_AST_Kind::Break;
        case Untyped_AST_Kind::Continue:        return Typed_AST_Kind::Continue;
        case Untyped_AST_Kind::Addition:        return Typed_AST_Kind::Addition;
        case Untyped_AST_Kind::Subtraction:     return Typed_AST_Kind::Subtraction;
        case Untyped_AST_Kind::Multiplication:  return Typed_AST_Kind::Multiplication;
        case Untyped_AST_Kind::Division:        return Typed_AST_Kind::Division;
        case Untyped_AST_Kind::Mod:             return Typed_AST_Kind::Mod;
        case Untyped_AST_Kind::Assignment:      return Typed_AST_Kind::Assignment;
        case Untyped_AST_Kind::Equal:           return Typed_AST_Kind::Equal;
        case Untyped_AST_Kind::Not_Equal:       return Typed_AST_Kind::Not_Equal;
        case Untyped_AST_Kind::Less:            return Typed_AST_Kind::Less;
        case Untyped_AST_Kind::Less_Eq:         return Typed_AST_Kind::Less_Eq;
        case Untyped_AST_Kind::Greater:         return Typed_AST_Kind::Greater;
        case Untyped_AST_Kind::Greater_Eq:      return Typed_AST_Kind::Greater_Eq;
        case Untyped_AST_Kind::And:             return Typed_AST_Kind::And;
        case Untyped_AST_Kind::Or:              return Typed_AST_Kind::Or;
        case Untyped_AST_Kind::While:           return Typed_AST_Kind::While;
        case Untyped_AST_Kind::Field_Access:    return Typed_AST_Kind::Field_Access;
        case Untyped_AST_Kind::Field_Access_Tuple:
            return Typed_AST_Kind::Field_Access;
        case Untyped_AST_Kind::Subscript:       return Typed_AST_Kind::Subscript;
        case Untyped_AST_Kind::Invocation:      return Typed_AST_Kind::Function_Call;
        case Untyped_AST_Kind::Match_Arm:       return Typed_AST_Kind::Match_Arm;
        case Untyped_AST_Kind::Block:           return Typed_AST_Kind::Block;
        case Untyped_AST_Kind::Comma:           return Typed_AST_Kind::Comma;
        case Untyped_AST_Kind::Tuple:           return Typed_AST_Kind::Tuple;
        case Untyped_AST_Kind::If:              return Typed_AST_Kind::If;
        case Untyped_AST_Kind::For:             return Typed_AST_Kind::For;
        case Untyped_AST_Kind::Match:           return Typed_AST_Kind::Match;
        case Untyped_AST_Kind::Let:             return Typed_AST_Kind::Let;
        case Untyped_AST_Kind::Fn_Decl:         return Typed_AST_Kind::Fn_Decl;
        case Untyped_AST_Kind::Type_Signature:  return Typed_AST_Kind::Type_Signature;
            
        case Untyped_AST_Kind::Pattern_Underscore:
        case Untyped_AST_Kind::Pattern_Ident:
        case Untyped_AST_Kind::Pattern_Tuple:
        case Untyped_AST_Kind::Pattern_Struct:
            return Typed_AST_Kind::Processed_Pattern;
            
        case Untyped_AST_Kind::Pattern_Value:
            break;
            
        case Untyped_AST_Kind::Generic_Specification:
            break;
    }
    
    internal_error("Invalid Untyped_AST_Kind value: %d\n", kind);
}

constexpr size_t INDENT_SIZE = 2;
static void print_at_indent(Interpreter *interp, const Ref<Typed_AST> node, size_t indent);

static void print_sub_at_indent(Interpreter *interp, const char *name, const Ref<Typed_AST> sub, size_t indent) {
    printf("%*s%s: ", indent * INDENT_SIZE, "", name);
    print_at_indent(interp, sub, indent);
}

static void print_nullary(const char *id, Ref<Typed_AST_Nullary> n) {
    printf("(%s) %s\n", id, n->type.debug_str());
}

static void print_unary_at_indent(Interpreter *interp, const char *id, const Ref<Typed_AST_Unary> u, size_t indent) {
    printf("(%s) %s\n", id, u->type.debug_str());
    print_sub_at_indent(interp, "sub", u->sub, indent + 1);
}

static void print_binary_at_indent(Interpreter *interp, const char *id, const Ref<Typed_AST_Binary> b, size_t indent) {
    printf("(%s) %s\n", id, b->type.debug_str());
    print_sub_at_indent(interp, "lhs", b->lhs, indent + 1);
    print_sub_at_indent(interp, "rhs", b->rhs, indent + 1);
}

static void print_ternary_at_indent(Interpreter *interp, const char *id, const Ref<Typed_AST_Ternary> t, size_t indent) {
    printf("(%s) %s\n", id, t->type.debug_str());
    print_sub_at_indent(interp, "lhs", t->lhs, indent + 1);
    print_sub_at_indent(interp, "mid", t->mid, indent + 1);
    print_sub_at_indent(interp, "rhs", t->rhs, indent + 1);
}

static void print_multiary_at_indent(Interpreter *interp, const char *id, const Ref<Typed_AST_Multiary> m, size_t indent) {
    printf("(%s) %s\n", id, m->type.debug_str());
    for (size_t i = 0; i < m->nodes.size(); i++) {
        const Ref<Typed_AST> node = m->nodes[i];
        printf("%*s%zu: ", (indent + 1) * INDENT_SIZE, "", i);
        print_at_indent(interp, node, indent + 1);
    }
}

static void print_cast_at_indent(Interpreter *interp, const Ref<Typed_AST_Cast> c, size_t indent) {
    printf("(as) %s\n", c->type.debug_str());
    print_sub_at_indent(interp, "expr", c->expr, indent + 1);
}

static void print_at_indent(Interpreter *interp, const Ref<Typed_AST> node, size_t indent) {
    switch (node->kind) {
        case Typed_AST_Kind::Byte: {
            auto lit = node.cast<Typed_AST_Byte>();
            printf("%db\n", lit->value);
        } break;
        case Typed_AST_Kind::Bool: {
            Ref<Typed_AST_Bool> lit = node.cast<Typed_AST_Bool>();
            printf("%s\n", lit->value ? "true" : "false");
        } break;
        case Typed_AST_Kind::Char: {
            Ref<Typed_AST_Char> lit = node.cast<Typed_AST_Char>();
            printf("'%s'\n", utf8char_t::from_char32(lit->value).buf);
        } break;
        case Typed_AST_Kind::Float: {
            Ref<Typed_AST_Float> lit = node.cast<Typed_AST_Float>();
            printf("%f\n", lit->value);
        } break;
        case Typed_AST_Kind::Ident: {
            Ref<Typed_AST_Ident> id = node.cast<Typed_AST_Ident>();
            printf("%.*s :: %s\n", id->id.size(), id->id.c_str(), id->type.debug_str());
        } break;
        case Typed_AST_Kind::Ident_Struct: {
            auto uuid = node.cast<Typed_AST_UUID>();
            auto defn = uuid->type.data.type.type->data.struct_.defn;
            printf("%.*s :: %s\n", defn->name.size(), defn->name.c_str(), uuid->type.debug_str());
        } break;
        case Typed_AST_Kind::Ident_Enum: {
            auto uuid = node.cast<Typed_AST_UUID>();
            auto defn = uuid->type.data.type.type->data.enum_.defn;
            printf("%.*s :: %s\n", defn->name.size(), defn->name.c_str(), uuid->type.debug_str());
        } break;
        case Typed_AST_Kind::Ident_Trait: {
            auto uuid = node.cast<Typed_AST_UUID>();
            auto defn = uuid->type.data.type.type->data.trait.defn;
            printf("%.*s :: %s\n", defn->name.size(), defn->name.c_str(), uuid->type.debug_str());
        } break;
        case Typed_AST_Kind::Ident_Func: {
            auto uuid = node.cast<Typed_AST_UUID>();
            auto defn = interp->functions.get_func_by_uuid(uuid->uuid);
            printf("%.*s#%zu :: %s\n", defn->name.size(), defn->name.c_str(), uuid->uuid, defn->type.debug_str());
        } break;
        case Typed_AST_Kind::Ident_Module: {
            auto uuid = node.cast<Typed_AST_UUID>();
            auto module = interp->modules.get_module_by_uuid(uuid->uuid);
            printf("%s#%llu\n", module->module_path.c_str(), uuid->uuid);
        } break;
        case Typed_AST_Kind::Int: {
            Ref<Typed_AST_Int> lit = node.cast<Typed_AST_Int>();
            printf("%lld\n", lit->value);
        } break;
        case Typed_AST_Kind::Str: {
            Ref<Typed_AST_Str> lit = node.cast<Typed_AST_Str>();
            printf("\"%.*s\"\n", lit->value.size(), lit->value.c_str());
        } break;
        case Typed_AST_Kind::Ptr: {
            auto lit = node.cast<Typed_AST_Ptr>();
            printf("%p\n", lit->value);
        } break;
        case Typed_AST_Kind::Builtin: {
            auto builtin = node.cast<Typed_AST_Builtin>();
            printf("@%p :: %s\n", builtin->defn->builtin, builtin->type.debug_str());
        } break;
        case Typed_AST_Kind::Allocate: {
            print_nullary("allocate", node.cast<Typed_AST_Nullary>());
        } break;
        case Typed_AST_Kind::Negation: {
            print_unary_at_indent(interp, "-", node.cast<Typed_AST_Unary>(), indent);
        } break;
        case Typed_AST_Kind::Not: {
            print_unary_at_indent(interp, "!", node.cast<Typed_AST_Unary>(), indent);
        } break;
        case Typed_AST_Kind::Address_Of: {
            print_unary_at_indent(interp, "&", node.cast<Typed_AST_Unary>(), indent);
        } break;
        case Typed_AST_Kind::Address_Of_Mut: {
            print_unary_at_indent(interp, "&mut", node.cast<Typed_AST_Unary>(), indent);
        } break;
        case Typed_AST_Kind::Deref: {
            print_unary_at_indent(interp, "*", node.cast<Typed_AST_Unary>(), indent);
        } break;
        case Typed_AST_Kind::Defer: {
            print_unary_at_indent(interp, "defer", node.cast<Typed_AST_Unary>(), indent);
        } break;
        case Typed_AST_Kind::Return: {
            auto ret = node.cast<Typed_AST_Return>();
            printf("(ret)\n");
            if (ret->sub) {
                print_sub_at_indent(interp, "sub", ret->sub, indent + 1);
            } else {
                printf("%*ssub: nullptr\n", (indent + 1) * INDENT_SIZE, "");
            }
        } break;
        case Typed_AST_Kind::Break: {
            auto control = node.cast<Typed_AST_Loop_Control>();
            printf("(break)\n");
            if (control->label.size() != 0) {
                printf("%*slabel: %.*s\n", (indent + 1) * INDENT_SIZE, "", control->label.size(), control->label.c_str());
            }
        } break;
        case Typed_AST_Kind::Continue: {
            auto control = node.cast<Typed_AST_Loop_Control>();
            printf("(continue)\n");
            if (control->label.size() != 0) {
                printf("%*slabel: %.*s\n", (indent + 1) * INDENT_SIZE, "", control->label.size(), control->label.c_str());
            }
        } break;
        case Typed_AST_Kind::Addition: {
            print_binary_at_indent(interp, "+", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::Subtraction: {
            print_binary_at_indent(interp, "-", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::Multiplication: {
            print_binary_at_indent(interp, "*", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::Division: {
            print_binary_at_indent(interp, "/", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::Mod: {
            print_binary_at_indent(interp, "%", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::Assignment: {
            print_binary_at_indent(interp, "=", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::Equal: {
            print_binary_at_indent(interp, "==", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::Not_Equal: {
            print_binary_at_indent(interp, "!=", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::Less: {
            print_binary_at_indent(interp, "<", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::Less_Eq: {
            print_binary_at_indent(interp, "<=", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::Greater: {
            print_binary_at_indent(interp, ">", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::Greater_Eq: {
            print_binary_at_indent(interp, ">=", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::And: {
            print_binary_at_indent(interp, "and", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::Or: {
            print_binary_at_indent(interp, "or", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::Field_Access: {
            auto dot = node.cast<Typed_AST_Field_Access>();
            printf("(.) %s\n", dot->type.debug_str());
            print_sub_at_indent(interp, "instance", dot->instance, indent + 1);
            printf("%*soffset: %d\n", (indent + 1) * INDENT_SIZE, "", dot->field_offset);
        } break;
        case Typed_AST_Kind::Subscript:
        case Typed_AST_Kind::Negative_Subscript: {
            print_binary_at_indent(interp, "[]", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::Range: {
            print_binary_at_indent(interp, "..", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::Inclusive_Range: {
            print_binary_at_indent(interp, "...", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::Function_Call:
        case Typed_AST_Kind::Builtin_Call: {
            print_binary_at_indent(interp, "call", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::Match_Arm: {
            print_binary_at_indent(interp, "arm", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::If: {
            auto t = node.cast<Typed_AST_If>();
            printf("(if)\n");
            print_sub_at_indent(interp, "cond", t->cond, indent + 1);
            print_sub_at_indent(interp, "then", t->then, indent + 1);
            if (t->else_) {
                print_sub_at_indent(interp, "else", t->else_, indent + 1);
            }
        } break;
        case Typed_AST_Kind::While: {
            auto w = node.cast<Typed_AST_While>();
            printf("(while)\n");
            if (w->label) {
                printf("%*slabel: %.*s\n", (indent + 1) * INDENT_SIZE, "", w->label->id.size(), w->label->id.c_str());
            }
            print_sub_at_indent(interp, "cond", w->condition, indent + 1);
            print_sub_at_indent(interp, "body", w->body, indent + 1);
        } break;
        case Typed_AST_Kind::For:
        case Typed_AST_Kind::For_Range: {
            auto f = node.cast<Typed_AST_For>();
            printf("(for)\n");
            print_sub_at_indent(interp, "target", f->target, indent + 1);
            if (f->counter != "") {
                printf("%*scounter: %s\n", (indent + 1) * INDENT_SIZE, "", f->counter.c_str());
            }
            print_sub_at_indent(interp, "iterable", f->iterable, indent + 1);
            print_sub_at_indent(interp, "body", f->body, indent + 1);
        } break;
        case Typed_AST_Kind::Forever: {
            auto f = node.cast<Typed_AST_Forever>();
            printf("(forever)\n");
            if (f->label) print_sub_at_indent(interp, "label", f->label, indent + 1);
            print_sub_at_indent(interp, "body", f->body, indent + 1);
        } break;
        case Typed_AST_Kind::Match: {
            auto m = node.cast<Typed_AST_Match>();
            printf("(match)\n");
            print_sub_at_indent(interp, "cond", m->cond, indent + 1);
            if (m->default_arm) {
                print_sub_at_indent(interp, "default", m->default_arm, indent + 1);
            }
            print_sub_at_indent(interp, "arms", m->arms, indent + 1);
        } break;
        case Typed_AST_Kind::Block: {
            print_multiary_at_indent(interp, "block", node.cast<Typed_AST_Multiary>(), indent);
        } break;
        case Typed_AST_Kind::Comma: {
            print_multiary_at_indent(interp, ",", node.cast<Typed_AST_Multiary>(), indent);
        } break;
        case Typed_AST_Kind::Tuple: {
            print_multiary_at_indent(interp, "tuple", node.cast<Typed_AST_Multiary>(), indent);
        } break;
        case Typed_AST_Kind::Let: {
            auto let = node.cast<Typed_AST_Let>();
            printf("(%s)\n", let->is_const ? "const" : "let");
            
            print_sub_at_indent(interp, "target", let->target, indent + 1);
            if (let->specified_type) {
                print_sub_at_indent(interp, "type", let->specified_type, indent + 1);
            }
            if (let->initializer) {
                print_sub_at_indent(interp, "init", let->initializer, indent + 1);
            }
        } break;
        case Typed_AST_Kind::Type_Signature: {
            auto sig = node.cast<Typed_AST_Type_Signature>();
            printf("%s\n", sig->value_type->debug_str());
        } break;
        case Typed_AST_Kind::Processed_Pattern: {
            auto pp = node.cast<Typed_AST_Processed_Pattern>();
            printf("(pattern)\n");
            for (size_t i = 0; i < pp->bindings.size(); i++) {
                auto &b = pp->bindings[i];
                printf("%*s%zu: ", (indent + 1) * INDENT_SIZE, "", i);
                if (b.id == "") {
                    printf("_ :: %s\n", b.type.debug_str());
                } else {
                    printf("%.*s :: %s\n", b.id.size(), b.id.c_str(), b.type.debug_str());
                }
            }
        } break;
        case Typed_AST_Kind::Match_Pattern: {
            auto mp = node.cast<Typed_AST_Match_Pattern>();
            printf("(match-pattern)\n");
            for (size_t i = 0; i < mp->bindings.size(); i++) {
                auto b = mp->bindings[i];
                printf("%*s%zu: ", (indent + 1) * INDENT_SIZE, "", i);
                switch (b.kind) {
                    case Typed_AST_Match_Pattern::Binding_Kind::None:
                        printf("_\n");
                        break;
                    case Typed_AST_Match_Pattern::Binding_Kind::Value:
                        print_at_indent(interp, b.value_node, indent + 1);
                        break;
                    case Typed_AST_Match_Pattern::Binding_Kind::Variable:
                        printf("[%s, %u, %s]\n", b.variable_info.id.c_str(), b.offset, b.variable_info.type.debug_str());
                        break;
                        
                    default:
                        internal_error("Invalid Binding_Kind: %d.", b.kind);
                        break;
                }
            }
        } break;
        case Typed_AST_Kind::Array:
        case Typed_AST_Kind::Slice: {
            auto array = node.cast<Typed_AST_Array>();
            if (array->array_type->kind == Value_Type_Kind::Array) {
                printf("(array)\n");
            } else {
                printf("(slice)\n");
            }
            printf("%*scount: %zu\n", (indent + 1) * INDENT_SIZE, "", array->count);
            printf("%*stype: %s\n", (indent + 1) * INDENT_SIZE, "", array->array_type->debug_str());
            print_sub_at_indent(interp, "elems", array->element_nodes, indent + 1);
        } break;
        case Typed_AST_Kind::Enum: {
            auto enum_ = node.cast<Typed_AST_Enum_Literal>();
            printf("(enum) %s\n", enum_->type.debug_str());
            printf("%*stag: %lld\n", (indent + 1) * INDENT_SIZE, "", enum_->tag);
            if (enum_->payload) {
                print_sub_at_indent(interp, "payload", enum_->payload, indent + 1);
            }
        } break;
        case Typed_AST_Kind::Fn_Decl: {
            auto decl = node.cast<Typed_AST_Fn_Declaration>();
            printf("(fn-decl)\n");
            printf("%*sfn_id: #%zu\n", (indent + 1) * INDENT_SIZE, "", decl->defn->uuid);
            printf("%*sfn_type: %s\n", (indent + 1) * INDENT_SIZE, "", decl->defn->type.debug_str());
            print_sub_at_indent(interp, "body", decl->body, indent + 1);
        } break;
        case Typed_AST_Kind::Cast_Byte_Int:
        case Typed_AST_Kind::Cast_Byte_Float:
        case Typed_AST_Kind::Cast_Bool_Int:
        case Typed_AST_Kind::Cast_Char_Int:
        case Typed_AST_Kind::Cast_Int_Float:
        case Typed_AST_Kind::Cast_Float_Int:
            print_cast_at_indent(interp, node.cast<Typed_AST_Cast>(), indent);
            break;
        case Typed_AST_Kind::Variadic_Call: {
            auto call = node.cast<Typed_AST_Variadic_Call>();
            printf("(varargs-call)\n");
            printf("%*svarargs size: %d\n", (indent + 1) * INDENT_SIZE, "", call->varargs_size);
            print_sub_at_indent(interp, "func", call->func, indent + 1);
            print_sub_at_indent(interp, "args", call->args, indent + 1);
            print_sub_at_indent(interp, "varargs", call->varargs, indent + 1);
        } break;
            
        default:
            internal_error("Invalid Typed_AST_Kind value: %d\n", node->kind);
            break;
    }
}

void Typed_AST::print(Interpreter *interp) const {
    print_at_indent(interp, Ref<Typed_AST>(const_cast<Typed_AST *>(this)), 0);
}

struct Typer_Binding {
    Typer_Binding() {}
    
    enum {
        Variable,
        Type,
        Function,
        Module,
    } kind;
    
    struct Function_Binding {
        UUID uuid;
        Value_Type fn_type;
    };
    
    union {
        Value_Type value_type;
        Function_Binding func;
        ::Module *mod;
    };
    
    static Typer_Binding variable(Value_Type value_type) {
        Typer_Binding b;
        b.kind = Variable;
        b.value_type = value_type;
        return b;
    }
    
    static Typer_Binding type(Value_Type type) {
        Typer_Binding b;
        b.kind = Type;
        b.value_type = type;
        return b;
    }
    
    static Typer_Binding function(UUID uuid, Value_Type fn_type) {
        Typer_Binding b;
        b.kind = Function;
        b.func.uuid = uuid;
        b.func.fn_type = fn_type;
        return b;
    }
    
    static Typer_Binding module(::Module *module) {
        Typer_Binding b;
        b.kind = Module;
        b.mod = module;
        return b;
    }
};

struct Typer_Scope {
    std::unordered_map<std::string, Typer_Binding> bindings;
};

struct Typer {
    Interpreter *interp;
    Module *module;
    Typer_Scope *global_scope;
    Function_Definition *function;
    Typer *parent;
    bool has_return;
    std::forward_list<Typer_Scope> scopes;
    
    Typer(Interpreter *interp, Module *module) {
        this->interp = interp;
        this->module = module;
        this->function = nullptr;
        this->parent = nullptr;
        
        begin_scope(); // global scope
        global_scope = &current_scope();
    }
    
    Typer(Typer &t, Function_Definition *function) {
        this->interp = t.interp;
        this->module = t.module;
        this->global_scope = t.global_scope;
        this->function = function;
        this->has_return = false;
        this->parent = &t;
    }
    
    Typer_Scope &current_scope() {
        return scopes.front();
    }
    
    void begin_scope() {
        scopes.push_front(Typer_Scope());
    }
    
    void end_scope() {
        scopes.pop_front();
    }
    
    bool find_binding_by_id(const std::string &id, Typer_Binding &out_binding) {
        for (auto &s : scopes) {
            auto it = s.bindings.find(id);
            if (it == s.bindings.end()) continue;
            out_binding = it->second;
            return true;
        }

        if (parent) {
            if (find_non_variable_binding_by_id_in_parent(id, out_binding)) {
                return true;
            }
        }

        auto it = global_scope->bindings.find(id);
        if (it != global_scope->bindings.end()) {
            out_binding = it->second;
            return true;
        }

        return false;
    }
    
    bool find_non_variable_binding_by_id_in_parent(
        const std::string &id,
        Typer_Binding &out_binding)
    {
        for (auto &s : parent->scopes) {
            auto it = s.bindings.find(id);
            if (it == s.bindings.end()) continue;
            if (it->second.kind == Typer_Binding::Variable) continue;
            out_binding = it->second;
            return true;
        }
        
        if (parent->parent) {
            return parent->find_non_variable_binding_by_id_in_parent(id, out_binding);
        }
        
        return false;
    }
    
    void put_binding(const std::string &id, Typer_Binding binding, Code_Location location) {
        auto &cs = current_scope();
        
        auto it = cs.bindings.find(id);
        verify(it == cs.bindings.end() || it->second.kind == Typer_Binding::Variable, location, "Cannot shadow something other than a variable. Reused identifier was '%s'.", id.c_str());
        
        cs.bindings[id] = binding;
    }
    
    void bind_variable(const std::string &id, Value_Type type, bool is_mut, Code_Location location) {
        type.is_mut = is_mut;
        return put_binding(id, Typer_Binding::variable(type), location);
    }
    
    void bind_type(const std::string &id, Value_Type type, Code_Location location) {
        internal_verify(type.kind == Value_Type_Kind::Type, "Attempted to bind a type name to something other than a type.");
        return put_binding(id, Typer_Binding::type(type), location);
    }
    
    void bind_function(const std::string &id, UUID uuid, Value_Type fn_type, Code_Location location) {
        internal_verify(fn_type.kind == Value_Type_Kind::Function, "Attempted to bind a function name to a non-function Value_Type.");
        return put_binding(id, Typer_Binding::function(uuid, fn_type), location);
    }
    
    void bind_module(const std::string &id, Module *module, Code_Location location) {
        return put_binding(id, Typer_Binding::module(module), location);
    }
    
    void bind_module_members(Module *module, Code_Location location) {
        for (auto &[id, member] : module->members) {
            switch (member.kind) {
                case Module::Member::Struct: {
                    auto defn = interp->types.get_struct_by_uuid(member.uuid);
                    internal_verify(defn, "Failed to retrieve struct#%lld from types.", member.uuid);
                    
                    Value_Type *struct_type = Mem.make<Value_Type>().as_ptr();
                    struct_type->kind = Value_Type_Kind::Struct;
                    struct_type->data.struct_.defn = defn;
                    
                    Value_Type type_type;
                    type_type.kind = Value_Type_Kind::Type;
                    type_type.data.type.type = struct_type;
                    
                    bind_type(id, type_type, location);
                } break;
                case Module::Member::Enum: {
                    auto defn = interp->types.get_enum_by_uuid(member.uuid);
                    internal_verify(defn, "Failed to retrieve enum#%lld from types.", member.uuid);
                    
                    Value_Type *enum_type = Mem.make<Value_Type>().as_ptr();
                    enum_type->kind = Value_Type_Kind::Enum;
                    enum_type->data.enum_.defn = defn;
                    
                    Value_Type type_type;
                    type_type.kind = Value_Type_Kind::Type;
                    type_type.data.type.type = enum_type;
                    
                    bind_type(id, type_type, location);
                } break;
                case Module::Member::Function: {
                    auto func = interp->functions.get_func_by_uuid(member.uuid);
                    internal_verify(func, "Failed to retrieve function with UUID #%lld.", member.uuid);
                    
                    bind_function(id, func->uuid, func->type, location);
                } break;
                case Module::Member::Submodule: {
                    auto sub = interp->modules.get_module_by_uuid(member.uuid);
                    internal_verify(sub, "Failed to retrieve module#%lld from modules.", member.uuid);
                    
                    bind_module(id, sub, location);
                } break;
                    
                default:
                    internal_error("Invalid Module::Member kind: %d.", member.kind);
                    break;
            }
        }
    }
    
    void bind_pattern(
        Ref<Untyped_AST_Pattern> pattern,
        const Value_Type &type,
        Ref<Typed_AST_Processed_Pattern> out_pp)
    {
        switch (pattern->kind) {
            case Untyped_AST_Kind::Pattern_Underscore:
                out_pp->add_binding("", type, false);
                break;
            case Untyped_AST_Kind::Pattern_Ident: {
                auto ip = pattern.cast<Untyped_AST_Pattern_Ident>();
                out_pp->add_binding(ip->id.clone(), type, ip->is_mut);
                bind_variable(ip->id.str(), type, ip->is_mut, ip->location);
            } break;
            case Untyped_AST_Kind::Pattern_Tuple: {
                auto tp = pattern.cast<Untyped_AST_Pattern_Tuple>();
                verify(type.kind == Value_Type_Kind::Tuple &&
                       tp->sub_patterns.size() == type.data.tuple.child_types.size(),
                       tp->location,
                       "Cannot match tuple pattern with %s.", type.display_str());
                for (size_t i = 0; i < tp->sub_patterns.size(); i++) {
                    auto sub_pattern = tp->sub_patterns[i];
                    auto sub_type    = type.data.tuple.child_types[i];
                    bind_pattern(sub_pattern, sub_type, out_pp);
                }
            } break;
            case Untyped_AST_Kind::Pattern_Struct: {
                auto sp = pattern.cast<Untyped_AST_Pattern_Struct>();
                auto uuid = sp->struct_id->typecheck(*this).cast<Typed_AST_UUID>();
                internal_verify(uuid, "Failed to cast to UUID* in Typer::put_pattern().");
                
                verify(type.kind == Value_Type_Kind::Struct &&
                       type.data.struct_.defn->uuid == uuid->uuid,
                       sp->location,
                       "Cannot match %s struct pattern with %s.", sp->struct_id->display_str(), type.display_str());
                
                auto defn = type.data.struct_.defn;
                verify(defn->fields.size() == sp->sub_patterns.size(), sp->location, "Incorrect number of sub patterns in struct pattern for struct %s. Expected %zu but was given %zu.", type.display_str(), defn->fields.size(), sp->sub_patterns.size());
                
                for (size_t i = 0; i < sp->sub_patterns.size(); i++) {
                    auto sub_pattern = sp->sub_patterns[i];
                    auto field_type  = defn->fields[i].type;
                    bind_pattern(sub_pattern, field_type, out_pp);
                }
            } break;
                
            default:
                internal_error("Invalid target kind: %d\n", pattern->kind);
                break;
        }
    }
    
    void bind_match_pattern(
        Ref<Untyped_AST_Pattern> pattern,
        const Value_Type &type,
        Ref<Typed_AST_Match_Pattern> out_mp,
        Size offset)
    {
        switch (pattern->kind) {
            case Untyped_AST_Kind::Pattern_Underscore:
                out_mp->add_none_binding();
                break;
            case Untyped_AST_Kind::Pattern_Ident: {
                auto ip = pattern.cast<Untyped_AST_Pattern_Ident>();
                Value_Type id_type = type;
                id_type.is_mut = ip->is_mut;
                out_mp->add_variable_binding(ip->id, id_type, offset);
                bind_variable(ip->id.str(), id_type, ip->is_mut, ip->location);
            } break;
            case Untyped_AST_Kind::Pattern_Tuple: {
                auto tp = pattern.cast<Untyped_AST_Pattern_Tuple>();
                verify(type.kind == Value_Type_Kind::Tuple &&
                       tp->sub_patterns.size() == type.data.tuple.child_types.size(),
                       tp->location,
                       "Cannot match tuple pattern with %s.", type.display_str());
                Size new_offset = offset;
                for (size_t i = 0; i < tp->sub_patterns.size(); i++) {
                    auto sub_pattern = tp->sub_patterns[i];
                    auto sub_type    = type.data.tuple.child_types[i];
                    bind_match_pattern(sub_pattern, sub_type, out_mp, new_offset);
                    new_offset += sub_type.size();
                }
            } break;
            case Untyped_AST_Kind::Pattern_Struct: {
                auto sp = pattern.cast<Untyped_AST_Pattern_Struct>();
                auto uuid = sp->struct_id->typecheck(*this).cast<Typed_AST_UUID>();
                internal_verify(uuid, "Failed to cast to UUID* in Typer::bind_match_pattern().");
                
                verify(type.kind == Value_Type_Kind::Struct &&
                       type.data.struct_.defn->uuid == uuid->uuid, 
                       sp->location,
                       "Cannot match %s struct pattern with %s.", sp->struct_id->display_str(), type.display_str());
                
                auto defn = type.data.struct_.defn;
                verify(defn->fields.size() == sp->sub_patterns.size(), sp->location, "Incorrect number of sub patterns in struct pattern for struct %s. Expected %zu but was given %zu.", type.display_str(), defn->fields.size(), sp->sub_patterns.size());
                
                Size new_offset = offset;
                for (size_t i = 0; i < sp->sub_patterns.size(); i++) {
                    auto sub_pattern = sp->sub_patterns[i];
                    auto field_type  = defn->fields[i].type;
                    bind_match_pattern(sub_pattern, field_type, out_mp, new_offset);
                    new_offset += field_type.size();
                }
            } break;
            case Untyped_AST_Kind::Pattern_Enum: {
                auto ep = pattern.cast<Untyped_AST_Pattern_Enum>();
                auto lit = ep->enum_id->typecheck(*this).cast<Typed_AST_Enum_Literal>();
                internal_verify(lit, "Failed to cast to Enum_Literal* in Typer::bind_match_pattern().");
                
                auto defn = lit->type.data.enum_.defn;
                auto &variant = defn->variants[lit->tag];
                
                verify(variant.payload.size() == ep->sub_patterns.size(), ep->location, "Incorrect number of sub patterns in enum pattern for enum %s. Expected %zu but was given %zu.", type.display_str(), variant.payload.size(), ep->sub_patterns.size());
                
                auto tag = Mem.make<Typed_AST_Int>(lit->tag, ep->location);
                out_mp->add_value_binding(tag, offset);
                
                Size new_offset = offset + value_types::Int.size();
                for (size_t i = 0; i < ep->sub_patterns.size(); i++) {
                    auto sub_pattern = ep->sub_patterns[i];
                    auto field_type = variant.payload[i].type;
                    bind_match_pattern(sub_pattern, field_type, out_mp, new_offset);
                    new_offset += field_type.size();
                }
            } break;
            case Untyped_AST_Kind::Pattern_Value: {
                auto vp = pattern.cast<Untyped_AST_Pattern_Value>();
                auto value = vp->value->typecheck(*this);
                
                verify(value->type.eq_ignoring_mutability(type), vp->location, "Type mismatch in pattern. Expected '%s' but was given '%s'.", type.display_str(), value->type.display_str());
                
                out_mp->add_value_binding(value, offset);
            } break;
                
            default:
                internal_error("Invalid target kind: %d\n", pattern->kind);
                break;
        }
    }
    
    Value_Type resolve_value_type(Value_Type type) {
        if (type.is_resolved()) {
            return type;
        }
        
        Value_Type resolved;
        
        switch (type.kind) {
            case Value_Type_Kind::None:
                internal_error("Attempted to resolve a None Value_Type.");
                break;
            case Value_Type_Kind::Unresolved_Type: {
                auto type_typechecked = type.data.unresolved.symbol->typecheck(*this);
                verify(type_typechecked->type.kind == Value_Type_Kind::Type, type_typechecked->location, "Expected type name. '%s' is not a type.", type.data.unresolved.symbol->display_str());
                
                resolved = *type_typechecked->type.data.type.type;
            } break;
                
            case Value_Type_Kind::Ptr: {
                auto child_type = Mem.make<Value_Type>();
                *child_type = resolve_value_type(*type.data.ptr.child_type);
                resolved.kind = Value_Type_Kind::Ptr;
                resolved.data.ptr.child_type = child_type.as_ptr();
            } break;
            case Value_Type_Kind::Array: {
                auto element_type = Mem.make<Value_Type>();
                *element_type = resolve_value_type(*type.data.array.element_type);
                resolved.kind = Value_Type_Kind::Array;
                resolved.data.array.count = type.data.array.count;
                resolved.data.array.element_type = element_type.as_ptr();
            } break;
            case Value_Type_Kind::Slice: {
                auto element_type = Mem.make<Value_Type>();
                *element_type = resolve_value_type(*type.data.slice.element_type);
                resolved.kind = Value_Type_Kind::Slice;
                resolved.data.slice.element_type = element_type.as_ptr();
            } break;
            case Value_Type_Kind::Tuple: {
                auto child_types = Array<Value_Type>::with_size(type.data.tuple.child_types.size());
                for (size_t i = 0; i < child_types.size(); i++) {
                    child_types[i] = resolve_value_type(type.child_type()[i]);
                }
                resolved.kind = Value_Type_Kind::Tuple;
                resolved.data.tuple.child_types = child_types;
            } break;
                
            default:
                internal_error("Type's of kind %d shouldn't need resolution.", type.kind);
                break;
        }
        
        resolved.is_mut = type.is_mut;
        return resolved;
    }
};

Ref<Typed_AST_Multiary> typecheck(
    Interpreter &interp,
    Module *module,
    Ref<Untyped_AST_Multiary> node)
{
    auto t = Typer { &interp, module };
    
    auto typechecked = Mem.make<Typed_AST_Multiary>(to_typed(node->kind), node->location);
    for (auto &n : node->nodes) {
        if (auto tn = n->typecheck(t))
            typechecked->add(tn);
    }
    
    return typechecked;
}

Ref<Typed_AST> Untyped_AST_Bool::typecheck(Typer &t) {
    return Mem.make<Typed_AST_Bool>(value, location);
}

Ref<Typed_AST> Untyped_AST_Char::typecheck(Typer &t) {
    return Mem.make<Typed_AST_Char>(value, location);
}

Ref<Typed_AST> Untyped_AST_Float::typecheck(Typer &t) {
    return Mem.make<Typed_AST_Float>(value, location);
}

Ref<Typed_AST> Untyped_AST_Ident::typecheck(Typer &t) {
    auto sid = id.str();
    Typer_Binding binding;
    verify(t.find_binding_by_id(sid, binding), location, "Unresolved identifier '%s'.", sid.c_str());
    Ref<Typed_AST> ident;
    switch (binding.kind) {
        case Typer_Binding::Type: {
            switch (binding.value_type.data.type.type->kind) {
                case Value_Type_Kind::Struct:
                    ident = Mem.make<Typed_AST_UUID>(Typed_AST_Kind::Ident_Struct, binding.value_type.data.type.type->data.struct_.defn->uuid, binding.value_type, location);
                    break;
                case Value_Type_Kind::Enum:
                    ident = Mem.make<Typed_AST_UUID>(Typed_AST_Kind::Ident_Enum, binding.value_type.data.type.type->data.enum_.defn->uuid, binding.value_type, location);
                    break;
                case Value_Type_Kind::Trait:
                    ident = Mem.make<Typed_AST_UUID>(Typed_AST_Kind::Ident_Trait, binding.value_type.data.type.type->data.trait.defn->uuid, binding.value_type, location);
                    break;
                
                default:
                    internal_error("Invalid Value_Type_Kind for Type type.");
                    break;
            }
        } break;
        case Typer_Binding::Function: {
            ident = Mem.make<Typed_AST_UUID>(Typed_AST_Kind::Ident_Func, binding.func.uuid, binding.func.fn_type, location);
        } break;
        case Typer_Binding::Module: {
            ident = Mem.make<Typed_AST_UUID>(Typed_AST_Kind::Ident_Module, binding.mod->uuid, value_types::None, location);
        } break;
            
        default:
            ident = Mem.make<Typed_AST_Ident>(id.clone(), binding.value_type, location);
            break;
    }

    return ident;
}

template<typename Namespace>
Ref<Typed_AST> typecheck_ident_in_namespace(Typer &t, Namespace *namespace_, Ref<Untyped_AST_Ident> id);

template<>
Ref<Typed_AST> typecheck_ident_in_namespace<Struct_Definition>(
    Typer &t,
    Struct_Definition *defn,
    Ref<Untyped_AST_Ident> id)
{
    //
    // @TODO:
    //      Handle things that aren't methods.
    //
    
    Method method;
    verify(defn->find_method(id->id, method), id->location, "Struct type '%s' does not have a method called '%s'.", defn->name.c_str(), id->id.c_str());
    
    auto method_defn = t.interp->functions.get_func_by_uuid(method.uuid);
    internal_verify(method_defn, "Failed to retrieve method '%s' from funcbook with id #%zu.", id->id.c_str(), method.uuid);
    
    return Mem.make<Typed_AST_UUID>(
        Typed_AST_Kind::Ident_Func,
        method.uuid,
        method_defn->type,
        id->location
    );
}

template<>
Ref<Typed_AST> typecheck_ident_in_namespace<Enum_Definition>(
    Typer &t,
    Enum_Definition *defn,
    Ref<Untyped_AST_Ident> id)
{
    Ref<Typed_AST> typechecked;
    
    String variant_id = id->id;
    auto variant = defn->find_variant(variant_id);
    
    if (variant) {
        Value_Type enum_type;
        enum_type.kind = Value_Type_Kind::Enum;
        enum_type.data.enum_.defn = defn;
        
        typechecked = Mem.make<Typed_AST_Enum_Literal>(
            enum_type,
            variant->tag,
            nullptr,
            id->location
        );
    } else {
        Method method;
        verify(defn->find_method(variant_id, method), id->location, "'%s' does not exist within the '%s' enum type's namespace.", variant_id.c_str(), defn->name.c_str());
        
        auto method_defn = t.interp->functions.get_func_by_uuid(method.uuid);
        internal_verify(method_defn, "Failed to retrieve method defn from funcbook.");
        
        typechecked = Mem.make<Typed_AST_UUID>(
            Typed_AST_Kind::Ident_Func,
            method.uuid,
            method_defn->type,
            id->location
        );
    }
    
    return typechecked;
}

template<>
Ref<Typed_AST> typecheck_ident_in_namespace<Module>(
    Typer &t,
    Module *module,
    Ref<Untyped_AST_Ident> id)
{
    Module::Member m;
    verify(module->find_member_by_id(id->id.str(), m), id->location, "'%s' cannot be found in the '%s' module.", module->module_path.c_str());
    
    Ref<Typed_AST_UUID> typechecked;
    switch (m.kind) {
        case Module::Member::Struct: {
            auto defn = t.interp->types.get_struct_by_uuid(m.uuid);
            
            Value_Type *struct_type = Mem.make<Value_Type>().as_ptr();
            struct_type->kind = Value_Type_Kind::Struct;
            struct_type->data.struct_.defn = defn;
            
            Value_Type type_type;
            type_type.kind = Value_Type_Kind::Type;
            type_type.data.type.type = struct_type;
            
            typechecked = Mem.make<Typed_AST_UUID>(Typed_AST_Kind::Ident_Struct, m.uuid, type_type, id->location);
        } break;
        case Module::Member::Enum: {
            auto defn = t.interp->types.get_enum_by_uuid(m.uuid);
            
            Value_Type *enum_type = Mem.make<Value_Type>().as_ptr();
            enum_type->kind = Value_Type_Kind::Enum;
            enum_type->data.enum_.defn = defn;
            
            Value_Type type_type;
            type_type.kind = Value_Type_Kind::Type;
            type_type.data.type.type = enum_type;
            
            typechecked = Mem.make<Typed_AST_UUID>(Typed_AST_Kind::Ident_Enum, m.uuid, type_type, id->location);
        } break;
        case Module::Member::Function: {
            auto defn = t.interp->functions.get_func_by_uuid(m.uuid);
            
            typechecked = Mem.make<Typed_AST_UUID>(Typed_AST_Kind::Ident_Func, m.uuid, defn->type, id->location);
        } break;
        case Module::Member::Submodule: {
            typechecked = Mem.make<Typed_AST_UUID>(Typed_AST_Kind::Ident_Module, m.uuid, value_types::None, id->location);
        } break;
            
        default:
            internal_error("Invalid Module::Member kind: %d", m.kind);
            break;
    }
    
    return typechecked;
}

template<typename Namespace>
static Ref<Typed_AST> typecheck_symbol_in_namespace(
    Typer &t,
    Namespace *namespace_,
    Ref<Untyped_AST_Symbol> symbol)
{
    if (symbol->kind == Untyped_AST_Kind::Ident) {
        return typecheck_ident_in_namespace(
            t,
            namespace_,
            symbol.cast<Untyped_AST_Ident>()
        );
    }
    
    auto path = symbol.cast<Untyped_AST_Path>();
    Ref<Typed_AST> lhs = typecheck_ident_in_namespace(t, namespace_, path->lhs);
    
    Ref<Typed_AST> typechecked;
    switch (lhs->kind) {
        case Typed_AST_Kind::Ident_Struct: {
            auto uuid = lhs.cast<Typed_AST_UUID>();
            auto defn = t.interp->types.get_struct_by_uuid(uuid->uuid);
            typechecked = typecheck_symbol_in_namespace(t, defn, path->rhs);
        } break;
        case Typed_AST_Kind::Ident_Enum: {
            auto uuid = lhs.cast<Typed_AST_UUID>();
            auto defn = t.interp->types.get_enum_by_uuid(uuid->uuid);
            typechecked = typecheck_symbol_in_namespace(t, defn, path->rhs);
        } break;
        case Typed_AST_Kind::Ident_Module: {
            auto uuid = lhs.cast<Typed_AST_UUID>();
            auto module = t.interp->modules.get_module_by_uuid(uuid->uuid);
            typechecked = typecheck_symbol_in_namespace(t, module, path->rhs);
        } break;
            
        default:
            internal_error("Invalid Typed_AST_Kind: %d.", lhs->kind);
            break;
    }
    
    return typechecked;
}

Ref<Typed_AST> Untyped_AST_Path::typecheck(Typer &t) {
    auto namespace_ = lhs->typecheck(t);
    
    Ref<Typed_AST> typechecked;
    switch (namespace_->kind) {
        case Typed_AST_Kind::Ident_Struct: {
            auto uuid = namespace_.cast<Typed_AST_UUID>();
            auto defn = t.interp->types.get_struct_by_uuid(uuid->uuid);
            typechecked = typecheck_symbol_in_namespace(t, defn, rhs);
        } break;
        case Typed_AST_Kind::Ident_Enum: {
            auto uuid = namespace_.cast<Typed_AST_UUID>();
            auto defn = t.interp->types.get_enum_by_uuid(uuid->uuid);
            typechecked = typecheck_symbol_in_namespace(t, defn, rhs);
        } break;
        case Typed_AST_Kind::Ident_Module: {
            auto uuid = namespace_.cast<Typed_AST_UUID>();
            auto module = t.interp->modules.get_module_by_uuid(uuid->uuid);
            typechecked = typecheck_symbol_in_namespace(t, module, rhs);
        } break;
            
        default:
            internal_error("Invalid Typed_AST_Kind in typecheck_path().");
            break;
    }
    
    return typechecked;
}

Ref<Typed_AST> Untyped_AST_Byte::typecheck(Typer &t) {
    return Mem.make<Typed_AST_Byte>(value, location);
}

Ref<Typed_AST> Untyped_AST_Int::typecheck(Typer &t) {
    return Mem.make<Typed_AST_Int>(value, location);
}

Ref<Typed_AST> Untyped_AST_Str::typecheck(Typer &t) {
    return Mem.make<Typed_AST_Str>(value.clone(), location);
}

Ref<Typed_AST> Untyped_AST_Nullary::typecheck(Typer &t) {
    switch (kind) {
        case Untyped_AST_Kind::Noinit:
            error(location, "'noinit' only allowed as initializer expression of variable declaration.");
            break;
            
        default:
            internal_error("Invalid nullary kind: %d.", kind);
            break;
    }
    
    todo("Implement Untyped_AST_Nullary::typecheck().");
}

Ref<Typed_AST> Untyped_AST_Unary::typecheck(Typer &t) {
    auto sub = this->sub->typecheck(t);
    switch (kind) {
        case Untyped_AST_Kind::Negation:
            verify(sub->type.kind == Value_Type_Kind::Int ||
                   sub->type.kind == Value_Type_Kind::Float,
                   sub->location,
                   "(-) requires operand to be an 'int' or a 'float' but was given '%s'.", sub->type.display_str());
            return Mem.make<Typed_AST_Unary>(Typed_AST_Kind::Negation, sub->type, sub, location);
        case Untyped_AST_Kind::Not:
            verify(sub->type.kind == Value_Type_Kind::Bool, sub->location, "(!) requires operand to be a 'bool' but got a '%s'.", sub->type.display_str());
            return Mem.make<Typed_AST_Unary>(Typed_AST_Kind::Not, value_types::Bool, sub, location);
        case Untyped_AST_Kind::Address_Of: {
            verify(sub->type.kind != Value_Type_Kind::None, location, "Cannot take a pointer to something that doesn't return a value.");
            auto pty = value_types::ptr_to(&sub->type);
            pty.data.ptr.child_type->is_mut = false;
            return Mem.make<Typed_AST_Unary>(Typed_AST_Kind::Address_Of, pty, sub, location);
        }
        case Untyped_AST_Kind::Address_Of_Mut: {
            verify(sub->type.kind != Value_Type_Kind::None, location, "Cannot take a pointer to something that doesn't return a value.");
            verify(sub->type.is_mut, location, "Cannot take a mutable pointer to something that isn't itself mutable.");
            auto pty = value_types::ptr_to(&sub->type);
            return Mem.make<Typed_AST_Unary>(Typed_AST_Kind::Address_Of_Mut, pty, sub, location);
        }
        case Untyped_AST_Kind::Deref:
            verify(sub->type.kind == Value_Type_Kind::Ptr, location, "Cannot dereference something of type '%s' because it is not a pointer type.", sub->type.display_str());
            return Mem.make<Typed_AST_Unary>(Typed_AST_Kind::Deref, *sub->type.data.ptr.child_type, sub, location);
        case Untyped_AST_Kind::Defer:
            internal_verify(sub->type.kind == Value_Type_Kind::None || sub->type.kind == Value_Type_Kind::Void, "deferred statement would leave an orphaned value on the stack");
            return Mem.make<Typed_AST_Unary>(Typed_AST_Kind::Defer, value_types::None, sub, location);
        case Untyped_AST_Kind::Builtin_Sizeof: {
            auto type = sub.cast<Typed_AST_Type_Signature>();
            internal_verify(type, "Failed to cast 'rhs' to Type_Signature.");
            
            Size type_size = type->value_type->size();
            
            return Mem.make<Typed_AST_Int>(static_cast<int64_t>(type_size), location);
        } break;
        case Untyped_AST_Kind::Builtin_Free: {
            Builtin_Definition *defn = nullptr;
            switch (sub->type.kind) {
                case Value_Type_Kind::Ptr:
                    defn = t.interp->builtins.get_builtin("<free-ptr>");
                    internal_verify(defn, "Failed to retrieve <free-ptr> builtin.");
                    break;
                case Value_Type_Kind::Slice:
                    defn = t.interp->builtins.get_builtin("<free-slice>");
                    internal_verify(defn, "Failed to retrieve <free-slice> builtin.");
                    break;
                case Value_Type_Kind::Str:
                    defn = t.interp->builtins.get_builtin("<free-str>");
                    internal_verify(defn, "Failed to retrieve <free-str> builtin.");
                    break;

                default:
                    error(sub->location, "Cannot free something of type '%s'.", sub->type.display_str());
                    break;
            }

            auto builtin = Mem.make<Typed_AST_Builtin>(defn, nullptr, location);

            return Mem.make<Typed_AST_Binary>(
                Typed_AST_Kind::Builtin_Call,
                *builtin->type.data.func.return_type,
                builtin,
                sub,
                location
            );
        } break;
            
        default:
            internal_error("Invalid Unary Untyped_AST_Kind value: %d\n", kind);
            return nullptr;
    }
}

Ref<Typed_AST> Untyped_AST_Return::typecheck(Typer &t) {
    verify(t.function, location, "Return statement outside of function.");

    Ref<Typed_AST> sub = nullptr;
    if (t.function->type.kind == Value_Type_Kind::Void) {
        verify(!this->sub, location, "Return statement does not match function definition. Expected '%s' but was given '%s'.", t.function->type.data.func.return_type->display_str(), this->sub->typecheck(t)->type.size());
    } else {
        sub = this->sub->typecheck(t);
        verify(t.function->type.data.func.return_type->assignable_from(sub->type), location, "Return statement does not match function definition. Expected '%s' but was given '%s'.", t.function->type.data.func.return_type->display_str(), sub->type.display_str());
    }
    
    t.has_return = true;

    return Mem.make<Typed_AST_Return>(t.function->varargs, sub, location);
}

Ref<Typed_AST> Untyped_AST_Loop_Control::typecheck(Typer &t) {
    return Mem.make<Typed_AST_Loop_Control>(to_typed(kind), label.clone(), location);
}

enum class Skip_Receiver {
    Dont_Skip = 0,
    Do_Skip = 1,
};

static void typecheck_function_call_arguments(
    Typer &t,
    Function_Definition *defn,
    Ref<Typed_AST_Multiary> out_args,
    Ref<Typed_AST_Multiary> out_varargs, // can be null
    Ref<Untyped_AST_Multiary> rhs,
    Skip_Receiver skip_receiver = Skip_Receiver::Dont_Skip)
{
    internal_verify(defn->varargs == out_varargs, "Either no out parameter passed for varargs or it was passed when it wasn't needed.");
    
    size_t num_args = defn->type.data.func.arg_types.size() - (defn->varargs ? 1 : 0);
    out_args->nodes.reserve(num_args);
    for (size_t i = out_args->nodes.size(); i < num_args; i++) {
        out_args->nodes.push_back(nullptr);
    }
    
    bool began_named_args = false;
    size_t num_positional_args = static_cast<bool>(skip_receiver) ? 1 : 0;
    size_t i = 0;
    while (i < rhs->nodes.size()) {
        Ref<Untyped_AST> arg_node = rhs->nodes[i];
        
        Ref<Untyped_AST> arg_expr;
        size_t arg_pos;
        if (arg_node->kind == Untyped_AST_Kind::Binding) {
            began_named_args = true;
            
            auto arg_bin = arg_node.cast<Untyped_AST_Binary>();
            auto arg_id_node = arg_bin->lhs.cast<Untyped_AST_Ident>();
            String arg_id = arg_id_node->id;
            
            arg_pos = -1;
            for (size_t i = 0; i < defn->param_names.size(); i++) {
                if (arg_id == defn->param_names[i]) {
                    arg_pos = i;
                    break;
                }
            }
            verify(arg_pos != -1, arg_id_node->location, "Unknown parameter '%.*s'.", arg_id.size(), arg_id.c_str());
            
            arg_expr = arg_bin->rhs;
        } else if (began_named_args) {
            error(arg_node->location, "Cannot have positional argruments after named arguments in function call.");
        } else {
            arg_expr = arg_node;
            arg_pos = num_positional_args++;
        }
        
        if (out_varargs && arg_pos >= num_args) {
            Value_Type *vararg_type = defn->type.data.func.arg_types[defn->type.data.func.arg_types.size() - 1].data.slice.element_type;
            
            while (true) {
                auto typechecked_arg_expr = arg_expr->typecheck(t);
                
                verify(vararg_type->assignable_from(typechecked_arg_expr->type), typechecked_arg_expr->location, "Argument type mismatch. Expected '%s' but was given '%s'.", vararg_type->display_str(), typechecked_arg_expr->type.display_str());
                
                out_varargs->nodes.push_back(typechecked_arg_expr);
                
                if (i + 1 >= rhs->nodes.size() ||
                    rhs->nodes[i+1]->kind == Untyped_AST_Kind::Binding)
                {
                    break;
                }
                
                i++;
                arg_expr = rhs->nodes[i];
            }
        } else {
            auto typecheck_arg_expr = arg_expr->typecheck(t);
            verify(defn->type.data.func.arg_types[arg_pos]
                       .assignable_from(typecheck_arg_expr->type),
                    typecheck_arg_expr->location,
                   "Argument type mismatch. Expected '%s' but was given '%s'.", defn->type.data.func.arg_types[arg_pos].display_str(), typecheck_arg_expr->type.display_str());
            
            auto &arg = out_args->nodes[arg_pos];
            verify(!arg, typecheck_arg_expr->location, "Argument '%.*s' given more than once.", defn->param_names[arg_pos].size(), defn->param_names[arg_pos].c_str());
            arg = typecheck_arg_expr;
        }
        
        i++;
    }
}

static Ref<Typed_AST> typecheck_function_call(
    Typer &t,
    Ref<Typed_AST> func,
    Ref<Untyped_AST_Multiary> rhs,
    Code_Location location)
{
    // @TODO: Check for function pointer type of stuff
    verify(func->kind == Typed_AST_Kind::Ident_Func, func->location, "First operand of function call must be a function.");
    
    auto func_uuid = func.cast<Typed_AST_UUID>();
    auto defn = t.interp->functions.get_func_by_uuid(func_uuid->uuid);
    internal_verify(defn, "Failed to retrieve function with id #%zu.", func_uuid->uuid);
    
    Ref<Typed_AST> typechecked;
    if (defn->varargs) {
        verify(rhs->nodes.size() >= defn->type.data.func.arg_types.size() - 1, rhs->location, "Incorrect number of arguments for invocation. Expected at least %zu but was given %zu.", defn->type.data.func.arg_types.size() - 1, rhs->nodes.size());
        
        auto args = Mem.make<Typed_AST_Multiary>(Typed_AST_Kind::Comma, rhs->location);
        auto varargs = Mem.make<Typed_AST_Multiary>(Typed_AST_Kind::Comma, rhs->location);
        typecheck_function_call_arguments(t, defn, args, varargs, rhs);
        
        Size varargs_size = 0;
        for (auto n : varargs->nodes) {
            varargs_size += n->type.size();
        }
        
        typechecked = Mem.make<Typed_AST_Variadic_Call>(
            *defn->type.data.func.return_type,
            varargs_size,
            func,
            args,
            varargs,
            location
        );
    } else {
        // @TODO: default arguments stuff (if we do default arguments)
    
        verify(rhs->nodes.size() == defn->type.data.func.arg_types.size(), rhs->location, "Incorrect number of arguments for invocation. Expected %zu but was given %zu.", defn->type.data.func.arg_types.size(), rhs->nodes.size());
    
        auto args = Mem.make<Typed_AST_Multiary>(Typed_AST_Kind::Comma, rhs->location);
        typecheck_function_call_arguments(t, defn, args, nullptr, rhs);
    
        typechecked = Mem.make<Typed_AST_Binary>(
            Typed_AST_Kind::Function_Call,
            *defn->type.data.func.return_type,
            func,
            args,
            location
        );
    }
    
    return typechecked;
}

static Ref<Typed_AST_Enum_Literal> typecheck_enum_literal_with_payload(
    Typer &t,
    Ref<Typed_AST> lhs,
    Ref<Untyped_AST_Multiary> rhs)
{
    auto lit = lhs.cast<Typed_AST_Enum_Literal>();
    internal_verify(lit, "lhs passed to %s() was not an enum literal.", __func__);
    
    lit->payload = rhs->typecheck(t).cast<Typed_AST_Multiary>();
    
    return lit;
}

static Ref<Typed_AST_Binary> typecheck_builtin_call(
    Typer &t,
    Ref<Typed_AST> lhs,
    Ref<Untyped_AST_Multiary> rhs,
    Code_Location location)
{
    auto builtin = lhs.cast<Typed_AST_Builtin>();
    internal_verify(builtin, "lhs passed to %s() was not a builtin.", __func__);
    
    auto builtin_type = builtin->type.data.func;
    
    //
    // @NOTE:
    //      When/If builtins start to have type parameters this will
    //      need to change.
    //
    
    verify(rhs->nodes.size() == builtin_type.arg_types.size(), rhs->location, "Incorrect number of arguments. Expected %zu but was given %zu.", builtin_type.arg_types.size(), rhs->nodes.size());
    
    auto args = rhs->typecheck(t).cast<Typed_AST_Multiary>();
    internal_verify(args, "args didn't come back as a Multiary.");
    
    for (size_t i = 0; i < args->nodes.size(); i++) {
        auto expected = builtin_type.arg_types[i];
        auto given = args->nodes[i];
        verify(expected.assignable_from(given->type), given->location, "Type mismatch: Argument %zu of builtin call expected to be '%s' but was given '%s'.", i, expected.display_str(), given->type.display_str());
    }

    return Mem.make<Typed_AST_Binary>(
        Typed_AST_Kind::Builtin_Call,
        *builtin_type.return_type,
        builtin,
        args,
        location
    );
}

static Ref<Typed_AST> typecheck_invocation(
    Typer &t,
    Untyped_AST_Binary &call)
{
    auto lhs = call.lhs->typecheck(t);
    auto rhs = call.rhs.cast<Untyped_AST_Multiary>();
    internal_verify(rhs, "Failed to cast rhs to Multiary* in typecheck_invocation().");
    
    Ref<Typed_AST> typechecked;
    switch (lhs->kind) {
        case Typed_AST_Kind::Builtin:
            typechecked = typecheck_builtin_call(t, lhs, rhs, call.location);
            break;
            
        default:
            switch (lhs->type.kind) {
                case Value_Type_Kind::Function:
                    typechecked = typecheck_function_call(t, lhs, rhs, call.location);
                    break;
                case Value_Type_Kind::Enum:
                    typechecked = typecheck_enum_literal_with_payload(t, lhs, rhs);
                    break;
                    
                default:
                    error(lhs->location, "Type '%s' isn't invocable.", lhs->type.display_str());
                    break;
            }
    }
    
    return typechecked;
}

static Ref<Typed_AST_Multiary> typecheck_slice_literal(
    Typer &t,
    Untyped_AST_Binary &lit)
{
    auto element_type = lit.lhs->typecheck(t).cast<Typed_AST_Type_Signature>();
    internal_verify(element_type, "Failed to cast to Signature.");
    
    auto fields = lit.rhs->typecheck(t).cast<Typed_AST_Multiary>();
    internal_verify(fields, "Failed to cast to Multiary.");
    
    verify(fields->nodes.size() == 2, fields->location, "Incorrect number of arguments for slice literal.");
    
    auto pointer = fields->nodes[0];
    auto size = fields->nodes[1];
    
    verify(pointer->type.kind == Value_Type_Kind::Ptr, pointer->location, "Type mismatch! Expected '*%s' but was given '%s'.", element_type->value_type->display_str(), pointer->type.display_str());
    verify(element_type->value_type->eq(*pointer->type.data.ptr.child_type), pointer->location, "Type mismatch! Expected '*%s' but was given '%s'.", element_type->value_type->display_str(), pointer->type.display_str());
    
    verify(size->type.kind == Value_Type_Kind::Int, size->location, "Type mismatch! Expected 'int' but was given '%s'.", size->type.display_str());
    
    auto slice_type = value_types::slice_of(element_type->value_type.as_ptr());
    
    fields->type = slice_type;
    
    return fields;
}

static Ref<Typed_AST> typecheck_cast_from_byte(
    Typer &t,
    Ref<Typed_AST> lhs,
    Ref<Typed_AST_Type_Signature> sig,
    Code_Location location)
{
    Ref<Typed_AST> typechecked;

    switch (sig->value_type->kind) {
        case Value_Type_Kind::Int:
            typechecked = Mem.make<Typed_AST_Cast>(Typed_AST_Kind::Cast_Byte_Int, value_types::Int, lhs, location);
            break;
        case Value_Type_Kind::Float:
            typechecked = Mem.make<Typed_AST_Cast>(Typed_AST_Kind::Cast_Byte_Float, value_types::Float, lhs, location);
            break;

        default:
            error(sig->location, "Cannot cast from type 'byte' to type '%s'.", sig->value_type->display_str());
            break;
    }

    return typechecked;
}

static Ref<Typed_AST> typecheck_cast_from_bool(
    Typer &t,
    Ref<Typed_AST> lhs,
    Ref<Typed_AST_Type_Signature> sig,
    Code_Location location)
{
    verify(sig->value_type->kind == Value_Type_Kind::Int, sig->location, "Cannot cast from type 'bool' to type '%s'.", sig->value_type->display_str());
    
    Ref<Typed_AST> typechecked;
    if (lhs->kind == Typed_AST_Kind::Bool) {
        auto lit = lhs.cast<Typed_AST_Bool>();
        typechecked = Mem.make<Typed_AST_Int>(lit->value ? 1 : 0, location);
    } else {
        typechecked = Mem.make<Typed_AST_Cast>(Typed_AST_Kind::Cast_Bool_Int, value_types::Int, lhs, location);
    }
    
    return typechecked;
}

static Ref<Typed_AST> typecheck_cast_from_char(
    Typer &t,
    Ref<Typed_AST> lhs,
    Ref<Typed_AST_Type_Signature> sig,
    Code_Location location)
{
    verify(sig->value_type->kind == Value_Type_Kind::Int, sig->location, "Cannot cast from type 'char' to type '%s'.", sig->value_type->display_str());
    
    return Mem.make<Typed_AST_Cast>(
        Typed_AST_Kind::Cast_Char_Int,
        value_types::Int,
        lhs,
        location
    );
}

static Ref<Typed_AST> typecheck_cast_from_int(
    Typer &t,
    Ref<Typed_AST> lhs,
    Ref<Typed_AST_Type_Signature> sig,
    Code_Location location)
{
    Ref<Typed_AST> typechecked;
    
    switch (sig->value_type->kind) {
        case Value_Type_Kind::Float:
            typechecked = Mem.make<Typed_AST_Cast>(Typed_AST_Kind::Cast_Int_Float, value_types::Float, lhs, location);
            break;
        case Value_Type_Kind::Ptr:
            internal_verify(value_types::Int.size() == value_types::Ptr.size(), "This code expects sizeof(runtime::Int) == sizeof(runtime::Ptr).");
            typechecked = lhs;
            typechecked->type = *sig->value_type;
            typechecked->location = location;
            break;
            
        default:
            error(sig->location, "Cannot cast from type 'int' to type '%s'.", sig->value_type->display_str());
            break;
    }
    
    return typechecked;
}

static Ref<Typed_AST> typecheck_cast_from_float(
    Typer &t,
    Ref<Typed_AST> lhs,
    Ref<Typed_AST_Type_Signature> sig,
    Code_Location location)
{
    verify(sig->value_type->kind == Value_Type_Kind::Int, sig->location, "Cannot cast from type 'float' to type '%s'.", sig->value_type->display_str());
    
    return Mem.make<Typed_AST_Cast>(Typed_AST_Kind::Cast_Float_Int, value_types::Int, lhs, location);
}

static Ref<Typed_AST> typecheck_cast_from_str(
    Typer &t,
    Ref<Typed_AST> lhs,
    Ref<Typed_AST_Type_Signature> sig,
    Code_Location location)
{
    todo("Implement %s().", __func__);
}

static Ref<Typed_AST> typecheck_cast_from_ptr(
    Typer &t,
    Ref<Typed_AST> lhs,
    Ref<Typed_AST_Type_Signature> sig,
    Code_Location location)
{
    Ref<Typed_AST> typechecked;
    
    switch (sig->value_type->kind) {
        case Value_Type_Kind::Ptr:
            verify(!sig->value_type->data.ptr.child_type->is_mut || lhs->type.data.ptr.child_type->is_mut, lhs->location, "Cannot cast from type '%s' to type '%s'. Mutability mismatch.", lhs->type.display_str(), sig->value_type->display_str());
            typechecked = lhs;
            typechecked->type = *sig->value_type;
            typechecked->location = location;
            break;
        case Value_Type_Kind::Int:
            internal_verify(value_types::Int.size() == value_types::Ptr.size(), "This code expects sizeof(runtime::Int) == sizeof(runtime::Ptr).");
            typechecked = lhs;
            typechecked->type = value_types::Int;
            typechecked->location = location;
            break;
            
        default:
            error(lhs->location, "Cannot cast from type '%s' to type '%s'.", lhs->type.display_str(), sig->value_type->display_str());
            break;
    }
    
    return typechecked;
}

static Ref<Typed_AST> typecheck_cast_from_enum(
    Typer &t,
    Ref<Typed_AST> lhs,
    Ref<Typed_AST_Type_Signature> sig,
    Code_Location location)
{
    verify(sig->value_type->kind == Value_Type_Kind::Int, sig->location, "Cannot cast from type '%s' to type '%s'.", lhs->type.display_str(), sig->value_type->display_str());
    
    auto defn = lhs->type.data.enum_.defn;
    verify(!defn->is_sumtype, lhs->location, "Cannot cast from sum-type enum '%s' to 'int'.", lhs->type.display_str());
    
    auto typechecked = lhs;
    typechecked->type = value_types::Int;
    typechecked->location = location;
    return typechecked;
}

static Ref<Typed_AST> typecheck_cast_from_func(
    Typer &t,
    Ref<Typed_AST> lhs,
    Ref<Typed_AST_Type_Signature> sig,
    Code_Location location)
{
    todo("Implement %s().", __func__);
}

Ref<Typed_AST> Untyped_AST_Binary::typecheck(Typer &t) {
    switch (kind) {
        case Untyped_AST_Kind::Invocation:
            return typecheck_invocation(t, *this);
        case Untyped_AST_Kind::Slice:
            return typecheck_slice_literal(t, *this);
            
        default:
            break;
    }
    
    auto lhs = this->lhs->typecheck(t);
    auto rhs = this->rhs->typecheck(t);
    switch (kind) {
        case Untyped_AST_Kind::Addition:
            verify(lhs->type.kind == rhs->type.kind, lhs->location, "(+) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int ||
                   lhs->type.kind == Value_Type_Kind::Float ||
                   lhs->type.kind == Value_Type_Kind::Byte,
                   lhs->location, 
                   "(+) requires operands to be either 'int' or 'float' but was given '%s'.", lhs->type.display_str());
            verify(rhs->type.kind == Value_Type_Kind::Int ||
                   rhs->type.kind == Value_Type_Kind::Float ||
                   lhs->type.kind == Value_Type_Kind::Byte, 
                   rhs->location,
                   "(+) requires operands to be either 'int' or 'float' but was given '%s'.", rhs->type.display_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Addition, lhs->type, lhs, rhs, location);
        case Untyped_AST_Kind::Subtraction:
            verify(lhs->type.kind == rhs->type.kind, lhs->location, "(-) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int ||
                   lhs->type.kind == Value_Type_Kind::Float ||
                   lhs->type.kind == Value_Type_Kind::Byte, 
                   lhs->location,
                   "(-) requires operands to be either 'int' or 'float' but was given '%s'.", lhs->type.display_str());
            verify(rhs->type.kind == Value_Type_Kind::Int ||
                   rhs->type.kind == Value_Type_Kind::Float ||
                   rhs->type.kind == Value_Type_Kind::Byte, 
                   rhs->location,
                   "(-) requires operands to be either 'int' or 'float' but was given '%s'.", rhs->type.display_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Subtraction, lhs->type, lhs, rhs, location);
        case Untyped_AST_Kind::Multiplication:
            verify(lhs->type.kind == rhs->type.kind, lhs->location, "(*) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int ||
                   lhs->type.kind == Value_Type_Kind::Float ||
                   lhs->type.kind == Value_Type_Kind::Byte, 
                   lhs->location,
                   "(*) requires operands to be either 'int' or 'float' but was given '%s'.", lhs->type.display_str());
            verify(rhs->type.kind == Value_Type_Kind::Int ||
                   rhs->type.kind == Value_Type_Kind::Float ||
                   rhs->type.kind == Value_Type_Kind::Byte, 
                   rhs->location,
                   "(*) requires operands to be either 'int' or 'float' but was given '%s'.", rhs->type.display_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Multiplication, lhs->type, lhs, rhs, location);
        case Untyped_AST_Kind::Division:
            verify(lhs->type.kind == rhs->type.kind, lhs->location, "(/) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int ||
                   lhs->type.kind == Value_Type_Kind::Float ||
                   lhs->type.kind == Value_Type_Kind::Byte, 
                   lhs->location,
                   "(/) requires operands to be either 'int' or 'float' but was given '%s'.", lhs->type.display_str());
            verify(rhs->type.kind == Value_Type_Kind::Int ||
                   rhs->type.kind == Value_Type_Kind::Float ||
                   rhs->type.kind == Value_Type_Kind::Byte, 
                   rhs->location,
                   "(/) requires operands to be either 'int' or 'float' but was given '%s'.", rhs->type.display_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Division, lhs->type, lhs, rhs, location);
        case Untyped_AST_Kind::Mod:
            verify(lhs->type.kind == rhs->type.kind, lhs->location, "(+) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int ||
                   lhs->type.kind == Value_Type_Kind::Byte, 
                    lhs->location,
                    "(+) requires operands to be 'int' but was given '%s'.", lhs->type.display_str());
            verify(rhs->type.kind == Value_Type_Kind::Int ||
                   lhs->type.kind == Value_Type_Kind::Byte, 
                    rhs->location,
                    "(+) requires operands to be 'int' but was given '%s'.", rhs->type.display_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Mod, lhs->type, lhs, rhs, location);
        case Untyped_AST_Kind::Assignment:
            verify(
                lhs->type.is_mut,
                lhs->location,
                "Cannot assign to something of type '%s' because it is immutable.", lhs->type.display_str()
            );
            verify(
                lhs->type.assignable_from(rhs->type), 
                rhs->location,
                "(=) requires both operands to be the same type."
            );
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Assignment, value_types::None, lhs, rhs, location);
        case Untyped_AST_Kind::Equal:
            verify(lhs->type.eq_ignoring_mutability(rhs->type), lhs->location, "(==) requires both operands to be the same type.");
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Equal, value_types::Bool, lhs, rhs, location);
        case Untyped_AST_Kind::Not_Equal:
            verify(lhs->type.eq_ignoring_mutability(rhs->type), lhs->location, "(!=) requires both operands to be the same type.");
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Not_Equal, value_types::Bool, lhs, rhs, location);
        case Untyped_AST_Kind::Less:
            verify(lhs->type.eq_ignoring_mutability(rhs->type), lhs->location, "(<) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int ||
                   lhs->type.kind == Value_Type_Kind::Float ||
                   lhs->type.kind == Value_Type_Kind::Byte,
                   lhs->location,
                   "(<) requires operands to be 'int' or 'float' but was given '%s'.", lhs->type.display_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Less, value_types::Bool, lhs, rhs, location);
        case Untyped_AST_Kind::Greater:
            verify(lhs->type.eq_ignoring_mutability(rhs->type), lhs->location, "(>) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int ||
                   lhs->type.kind == Value_Type_Kind::Float ||
                   lhs->type.kind == Value_Type_Kind::Byte,
                   lhs->location,
                   "(>) requires operands to be 'int' or 'float' but was given '%s'.", lhs->type.display_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Greater, value_types::Bool, lhs, rhs, location);
        case Untyped_AST_Kind::Less_Eq:
            verify(lhs->type.eq_ignoring_mutability(rhs->type), lhs->location, "(<=) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int ||
                   lhs->type.kind == Value_Type_Kind::Float ||
                   lhs->type.kind == Value_Type_Kind::Byte,
                   lhs->location,
                   "(<=) requires operands to be 'int' or 'float' but was given '%s'.", lhs->type.display_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Less_Eq, value_types::Bool, lhs, rhs, location);
        case Untyped_AST_Kind::Greater_Eq:
            verify(lhs->type.eq_ignoring_mutability(rhs->type), lhs->location, "(>=) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int ||
                   lhs->type.kind == Value_Type_Kind::Float ||
                   lhs->type.kind == Value_Type_Kind::Byte,
                   lhs->location,
                   "(>=) requires operands to be 'int' or 'float' but was given '%s'.", lhs->type.display_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Greater_Eq, value_types::Bool, lhs, rhs, location);
        case Untyped_AST_Kind::And:
            verify(lhs->type.kind == Value_Type_Kind::Bool, lhs->location, "(and) requires first operand to be 'bool' but was given '%s'.", lhs->type.display_str());
            verify(rhs->type.kind == Value_Type_Kind::Bool, rhs->location, "(and) requires second operand to be 'bool' but was given '%s'.", rhs->type.display_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::And, value_types::Bool, lhs, rhs, location);
        case Untyped_AST_Kind::Or:
            verify(lhs->type.kind == Value_Type_Kind::Bool, lhs->location, "(or) requires first operand to be 'bool' but was given '%s'.", lhs->type.display_str());
            verify(rhs->type.kind == Value_Type_Kind::Bool, rhs->location, "(or) requires second operand to be 'bool' but was given '%s'.", rhs->type.display_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Or, value_types::Bool, lhs, rhs, location);
        case Untyped_AST_Kind::Field_Access_Tuple: {
            bool needs_deref = lhs->type.kind == Value_Type_Kind::Ptr;
            Value_Type &ty = needs_deref ? *lhs->type.child_type() : lhs->type;
            verify(ty.kind == Value_Type_Kind::Tuple, lhs->location, "(.) requires first operand to be a tuple but was given '%s'.", lhs->type.display_str());
            auto i = rhs.cast<Typed_AST_Int>();
            internal_verify(i, "Dot_Tuple got a rhs that wasn't an int.");
            verify(i->value < ty.data.tuple.child_types.size(), i->location, "Cannot access type %lld from a %s.", i->value, lhs->type.display_str());
            Value_Type child_ty = ty.data.tuple.child_types[i->value];
            child_ty.is_mut = ty.is_mut;
            return Mem.make<Typed_AST_Field_Access>(
                child_ty,
                needs_deref,
                lhs,
                lhs->type.data.tuple.offset_of_type(i->value),
                location
            );
        } break;
        case Untyped_AST_Kind::Subscript: {
            verify(lhs->type.kind == Value_Type_Kind::Array ||
                   lhs->type.kind == Value_Type_Kind::Slice,
                   lhs->location,
                   "([]) requires first operand to be an array or slice but was given '%s'.", lhs->type.display_str());
            verify(rhs->type.kind == Value_Type_Kind::Int ||
                   (rhs->type.kind == Value_Type_Kind::Range && rhs->type.child_type()->kind == Value_Type_Kind::Int),
                   rhs->location,
                   "([]) requires second operand to be 'int' or 'Range<int>' but was given '%s'.", rhs->type.display_str());
            
            Typed_AST_Kind kind = Typed_AST_Kind::Subscript;
            
            if (rhs->kind == Typed_AST_Kind::Int) {
                auto i = rhs.cast<Typed_AST_Int>();
                if (i->value < 0) {
                    if (lhs->type.kind == Value_Type_Kind::Array) {
                        i->value += lhs->type.data.array.count;
                    } else {
                        kind = Typed_AST_Kind::Negative_Subscript;
                    }
                }
            }

            Value_Type type;
            if (rhs->type.kind == Value_Type_Kind::Range) {
                type = value_types::slice_of(lhs->type.child_type());
            } else {
                type = *lhs->type.child_type();
            }
            
            return Mem.make<Typed_AST_Binary>(kind, type, lhs, rhs, location);
        }
        case Untyped_AST_Kind::Range:
            verify(lhs->type.eq_ignoring_mutability(rhs->type), lhs->location, "(..) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int, lhs->location, "(..) requires operands to be of type 'int' but was given '%s'.", lhs->type.display_str());
            return Mem.make<Typed_AST_Binary>(
                Typed_AST_Kind::Range, 
                value_types::range_of(false, &lhs->type), 
                lhs, 
                rhs,
                location
            );
        case Untyped_AST_Kind::Inclusive_Range:
            verify(lhs->type.eq_ignoring_mutability(rhs->type), lhs->location, "(...) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int, lhs->location, "(...) requires operands to be of type 'int' but was given '%s'.", lhs->type.display_str());
            return Mem.make<Typed_AST_Binary>(
                Typed_AST_Kind::Inclusive_Range,
                value_types::range_of(true, &lhs->type), 
                lhs, 
                rhs,
                location
            );
        case Untyped_AST_Kind::Cast: {
            Ref<Typed_AST> typechecked;
            
            auto sig = rhs.cast<Typed_AST_Type_Signature>();
            internal_verify(sig, "rhs to 'as' operator was not a type signature.");
            
            switch (lhs->type.kind) {
                case Value_Type_Kind::Byte:
                    typechecked = typecheck_cast_from_byte(t, lhs, sig, location);
                    break;
                case Value_Type_Kind::Bool:
                    typechecked = typecheck_cast_from_bool(t, lhs, sig, location);
                    break;
                case Value_Type_Kind::Char:
                    typechecked = typecheck_cast_from_char(t, lhs, sig, location);
                    break;
                case Value_Type_Kind::Int:
                    typechecked = typecheck_cast_from_int(t, lhs, sig, location);
                    break;
                case Value_Type_Kind::Float:
                    typechecked = typecheck_cast_from_float(t, lhs, sig, location);
                    break;
                case Value_Type_Kind::Str:
                    typechecked = typecheck_cast_from_str(t, lhs, sig, location);
                    break;
                case Value_Type_Kind::Ptr:
                    typechecked = typecheck_cast_from_ptr(t, lhs, sig, location);
                    break;
                case Value_Type_Kind::Enum:
                    typechecked = typecheck_cast_from_enum(t, lhs, sig, location);
                    break;
                case Value_Type_Kind::Function:
                    typechecked = typecheck_cast_from_func(t, lhs, sig, location);
                    break;
                    
                default:
                    break;
            }
            
            return typechecked;
        }
        case Untyped_AST_Kind::Builtin_Alloc: {
            auto type = lhs.cast<Typed_AST_Type_Signature>();
            internal_verify(type, "Failed to cast type to Type_Signature");
            
            verify(type->value_type->kind == Value_Type_Kind::Ptr, type->location, "'@alloc' must return a pointer type.");
            verify(rhs->type.kind == Value_Type_Kind::Int, rhs->location, "'@alloc' requires its second operand to be of type 'int' but was given '%s'.", rhs->type.display_str());
            
            auto defn = t.interp->builtins.get_builtin("alloc");
            internal_verify(defn, "Could't retrieve '@alloc' builtin.");
            auto alloc = Mem.make<Typed_AST_Builtin>(defn, type->value_type.as_ptr(), location);
            
            return Mem.make<Typed_AST_Binary>(
                Typed_AST_Kind::Builtin_Call,
                *type->value_type,
                alloc,
                rhs,
                rhs->location
            );
        }
            
        default:
            internal_error("Invalid Binary Untyped_AST_Kind value: %d\n", kind);
    }
    return nullptr;
}

Ref<Typed_AST> Untyped_AST_Ternary::typecheck(Typer &t) {
    auto lhs = this->lhs->typecheck(t);
    auto mid = this->mid->typecheck(t);
    auto rhs = this->rhs->typecheck(t);
    switch (kind) {
            
        default:
            internal_error("Invalid Ternary Untyped_AST_Kind value: %d\n", kind);
            return nullptr;
    }
}

Ref<Typed_AST> Untyped_AST_Multiary::typecheck(Typer &t) {
    if (kind == Untyped_AST_Kind::Block) t.begin_scope();
    auto multi = Mem.make<Typed_AST_Multiary>(to_typed(kind), location);
    for (auto &node : nodes) {
        if (auto typechecked = node->typecheck(t))
            multi->add(typechecked);
    }
    if (kind == Untyped_AST_Kind::Block) t.end_scope();
    
    if (kind == Untyped_AST_Kind::Tuple) {
        Value_Type *subtypes = flatten(multi->nodes, [](auto &c, size_t i) {
            return c[i]->type;
        });
        multi->type = value_types::tup_from(multi->nodes.size(), subtypes);
    }
    
    return multi;
}

//
// @NOTE:
//      Changing element_type here also modifies the Untyped node which might
//      not be ok in the future.
//
Ref<Typed_AST> Untyped_AST_Array::typecheck(Typer &t) {
    auto element_nodes = this->element_nodes->typecheck(t).cast<Typed_AST_Multiary>();
    Value_Type *element_type = array_type->child_type();
    internal_verify(element_type, "Could not get pointer to element type of array type.");
    if (element_type->kind == Value_Type_Kind::None) {
        verify(element_nodes->nodes.size() > 0, element_nodes->location, "Cannot infer element type of empty array literal.");
        bool is_mut = element_type->is_mut;
        *element_type = element_nodes->nodes[0]->type;
        element_type->is_mut = is_mut;
    }
    for (size_t i = 0; i < element_nodes->nodes.size(); i++) {
        verify(element_nodes->nodes[i]->type.eq_ignoring_mutability(*element_type), element_nodes->nodes[i]->location, "Element %zu in array literal does not match the expected type '%s'.", i+1, element_type->display_str());
    }
    return Mem.make<Typed_AST_Array>(*array_type, to_typed(kind), count, array_type, element_nodes, location);
}

Ref<Typed_AST> Untyped_AST_Struct_Literal::typecheck(Typer &t) {
    auto struct_uuid = this->struct_id->typecheck(t).cast<Typed_AST_UUID>();
    internal_verify(struct_uuid, "Failed to cast struct_id to UUID* in Struct_Literal::typecheck().");
    
    auto defn = t.interp->types.get_struct_by_uuid(struct_uuid->uuid);
    internal_verify(defn, "Failed to retrieve struct-defn #%zu from typebook.", struct_uuid->uuid);
    auto bindings = this->bindings;
    
    verify(defn->fields.size() == bindings->nodes.size(), bindings->location, "Incorrect number of arguments in struct literal. Expected %zu but was given %zu.", defn->fields.size(), bindings->nodes.size());
    
    auto args = Mem.make<Typed_AST_Multiary>(Typed_AST_Kind::Comma, bindings->location);
    args->type = *struct_uuid->type.data.type.type;
    
    for (size_t i = 0; i < defn->fields.size(); i++) {
        auto &field = defn->fields[i];
        auto binding = bindings->nodes[i];
        
        Ref<Typed_AST> arg = nullptr;
        switch (binding->kind) {
            case Untyped_AST_Kind::Ident: {
                auto bid = binding.cast<Untyped_AST_Ident>();
                verify(field.id == bid->id, bid->location, "Given identifier doesn't match name of field. Expected '%.*s' but was given '%.*s'. Please specify field.", field.id.size(), field.id.c_str(), bid->id.size(), bid->id.c_str());
                arg = bid->typecheck(t);
                verify(field.type.assignable_from(arg->type), arg->location, "Cannot assign to field '%.*s' because of mismatched types. Expected '%s' but was given '%s'.", field.id.size(), field.id.c_str(), field.type.display_str(), arg->type.display_str());
            } break;
            case Untyped_AST_Kind::Binding: {
                auto b = binding.cast<Untyped_AST_Binary>();
                auto bid = b->lhs.cast<Untyped_AST_Ident>();
                verify(field.id == bid->id, bid->location, "Given identifier doesn't match name of field. Expected '%.*s' but was given '%.*s'.", field.id.size(), field.id.c_str(), bid->id.size(), bid->id.c_str());
                arg = b->rhs->typecheck(t);
                verify(field.type.assignable_from(arg->type), arg->location, "Cannot assign to field '%.*s' because of mismatched types. Expected '%s' but was given '%s'.", field.id.size(), field.id.c_str(), field.type.display_str(), arg->type.display_str());
            } break;
                
            default:
                error(binding->location, "Expected either an identifier expression or binding in struct literal.");
                break;
        }
        
        args->add(arg);
    }
    
    return args;
}

Ref<Typed_AST> Untyped_AST_Builtin::typecheck(Typer &t) {
    auto sid = id.str();
    auto defn = t.interp->builtins.get_builtin(sid);
    verify(defn, location, "'@%s' is not a builtin.", sid.c_str());
    
    return Mem.make<Typed_AST_Builtin>(defn, nullptr, location);
}

Ref<Typed_AST> Untyped_AST_Builtin_Printlike::typecheck(Typer &t) {
    bool is_puts = this->printlike_kind == Untyped_AST_Builtin_Printlike::Puts;
    auto arg = this->arg->typecheck(t);

    Ref<Typed_AST> printlike = nullptr;
    switch (arg->type.kind) {
        case Value_Type_Kind::Byte: {
            auto defn = t.interp->builtins.get_builtin(is_puts ? "<puts-byte>" : "<print-byte>");
            internal_verify(defn, "Failed to retrieve builtin");
            printlike = Mem.make<Typed_AST_Builtin>(defn, nullptr, location);
        } break;
        case Value_Type_Kind::Bool: {
            auto defn = t.interp->builtins.get_builtin(is_puts ? "<puts-bool>" : "<print-bool>");
            internal_verify(defn, "Failed to retrieve builtin");
            printlike = Mem.make<Typed_AST_Builtin>(defn, nullptr, location);
        } break;
        case Value_Type_Kind::Char: {
            auto defn = t.interp->builtins.get_builtin(is_puts ? "<puts-char>" : "<print-char>");
            internal_verify(defn, "Failed to retrieve builtin");
            printlike = Mem.make<Typed_AST_Builtin>(defn, nullptr, location);
        } break;
        case Value_Type_Kind::Int: {
            auto defn = t.interp->builtins.get_builtin(is_puts ? "<puts-int>" : "<print-int>");
            internal_verify(defn, "Failed to retrieve builtin");
            printlike = Mem.make<Typed_AST_Builtin>(defn, nullptr, location);
        } break;
        case Value_Type_Kind::Float: {
            auto defn = t.interp->builtins.get_builtin(is_puts ? "<puts-float>" : "<print-float>");
            internal_verify(defn, "Failed to retrieve builtin");
            printlike = Mem.make<Typed_AST_Builtin>(defn, nullptr, location);
        } break;
        case Value_Type_Kind::Str: {
            auto defn = t.interp->builtins.get_builtin(is_puts ? "<puts-str>" : "<print-str>");
            internal_verify(defn, "Failed to retrieve builtin");
            printlike = Mem.make<Typed_AST_Builtin>(defn, nullptr, location);
        } break;
        case Value_Type_Kind::Ptr: {
            auto defn = t.interp->builtins.get_builtin(is_puts ? "<puts-ptr>" : "<print-ptr>");
            internal_verify(defn, "Failed to retrieve builtin");
            printlike = Mem.make<Typed_AST_Builtin>(defn, nullptr, location);
        } break;
        case Value_Type_Kind::Struct: {
            auto defn = t.interp->builtins.get_builtin(is_puts ? "<puts-struct>" : "<print-struct>");
            internal_verify(defn, "Failed to retrieve builtin");
            printlike = Mem.make<Typed_AST_Builtin>(defn, nullptr, location);

            printlike->type = printlike->type.clone();
            printlike->type.data.func.arg_types[0] = arg->type;

            auto struct_defn = arg->type.data.struct_.defn;
            auto push_defn = Mem.make<Typed_AST_Ptr>(struct_defn, location);

            auto args = Mem.make<Typed_AST_Multiary>(Typed_AST_Kind::Comma, arg->location);
            args->add(arg);
            args->add(push_defn);
            arg = args;
        } break;
        case Value_Type_Kind::Enum: {
            auto defn = t.interp->builtins.get_builtin(is_puts ? "<puts-enum>" : "<print-enum>");
            internal_verify(defn, "Failed to retrieve builtin");
            printlike = Mem.make<Typed_AST_Builtin>(defn, nullptr, location);

            printlike->type = printlike->type.clone();
            printlike->type.data.func.arg_types[0] = arg->type;

            auto enum_defn = arg->type.data.enum_.defn;
            auto push_defn = Mem.make<Typed_AST_Ptr>(enum_defn, location);

            auto args = Mem.make<Typed_AST_Multiary>(Typed_AST_Kind::Comma, arg->location);
            args->add(arg);
            args->add(push_defn);
            arg = args;
        } break;

        default:
            error(arg->location, "`@%s` does not take an argument of type `%s`.", is_puts ? "puts" : "print", arg->type.display_str());
            break;
    }

    return Mem.make<Typed_AST_Binary>(
        Typed_AST_Kind::Builtin_Call,
        *printlike->type.data.func.return_type,
        printlike,
        arg,
        location
    );
}

Ref<Typed_AST> Untyped_AST_Field_Access::typecheck(Typer &t) {
    auto instance = this->instance->typecheck(t);
    
    bool needs_deref = instance->type.kind == Value_Type_Kind::Ptr;
    Value_Type &ty = needs_deref ? *instance->type.child_type() : instance->type;
    verify(ty.kind == Value_Type_Kind::Struct, instance->location, "(.) requires first operand to be a struct type but was given '%s'.", instance->type.display_str());
    
    Struct_Field *field = ty.data.struct_.defn->find_field(field_id);
    verify(field, location, "'%.*s' is not a field of '%.*s'.", field_id.size(), field_id.c_str(), ty.data.struct_.defn->name.size(), ty.data.struct_.defn->name.c_str());
    
    Value_Type field_ty = field->type;
    field_ty.is_mut |= ty.is_mut; // |= because fields can be 'forced mut'
    
    Size field_offset = field->offset;
    
    return Mem.make<Typed_AST_Field_Access>(field_ty, needs_deref, instance, field_offset, location);
}

Ref<Typed_AST> Untyped_AST_Pattern_Underscore::typecheck(Typer &t) {
    internal_error("Call to Untyped_AST_Pattern_Underscore::typecheck() is disallowed.");
}

Ref<Typed_AST> Untyped_AST_Pattern_Ident::typecheck(Typer &t) {
    internal_error("Call to Untyped_AST_Pattern_Ident::typecheck() is disallowed.");
}

Ref<Typed_AST> Untyped_AST_Pattern_Tuple::typecheck(Typer &t) {
    internal_error("Call to Untyped_AST_Pattern_Tuple::typecheck() is disallowed.");
}

Ref<Typed_AST> Untyped_AST_Pattern_Struct::typecheck(Typer &t) {
    internal_error("Call to Untyped_AST_Pattern_Struct::typecheck() is disallowed.");
}

Ref<Typed_AST> Untyped_AST_Pattern_Enum::typecheck(Typer &t) {
    internal_error("Call to Untyped_AST_Pattern_Enum::typecheck() is disallowed.");
}

Ref<Typed_AST> Untyped_AST_Pattern_Value::typecheck(Typer &t) {
    internal_error("Call to Untyped_AST_Pattern_Value::typecheck() is disallowed.");
}

Ref<Typed_AST> Untyped_AST_If::typecheck(Typer &t) {
    auto cond = this->cond->typecheck(t);
    auto then = this->then->typecheck(t);
    
    t.has_return = false;
    
    auto else_ = this->else_ ? this->else_->typecheck(t) : nullptr;
    verify(!else_ || then->type.eq(else_->type), location, "Both branches of (if) must be the same. '%s' vs '%s'.", then->type.display_str(), else_->type.display_str());
    return Mem.make<Typed_AST_If>(then->type, cond, then, else_, location);
}

Ref<Typed_AST> Untyped_AST_Type_Signature::typecheck(Typer &t) {
    auto resolved_type = Mem.make<Value_Type>(t.resolve_value_type(*value_type));
    return Mem.make<Typed_AST_Type_Signature>(resolved_type, location);
}

Ref<Typed_AST> Untyped_AST_While::typecheck(Typer &t) {
    Ref<Typed_AST_Ident> label = nullptr;
    if (this->label) {
        label = Mem.make<Typed_AST_Ident>(this->label->id.clone(), value_types::None, this->label->location);
    }

    auto cond = condition->typecheck(t);
    auto body = this->body->typecheck(t).cast<Typed_AST_Multiary>();

    verify(cond->type.kind == Value_Type_Kind::Bool, cond->location, "(while) requires condition to be 'bool' but was given '%s'.", cond->type.display_str());
    
    t.has_return = false;
    
    return Mem.make<Typed_AST_While>(label, cond, body, location);
}

Ref<Typed_AST> Untyped_AST_For::typecheck(Typer &t) {
    Ref<Typed_AST_Ident> label = nullptr;
    if (this->label) {
        label = Mem.make<Typed_AST_Ident>(this->label->id.clone(), value_types::None, this->label->location);
    }

    auto iterable = this->iterable->typecheck(t);
    switch (iterable->type.kind) {
        case Value_Type_Kind::Array:
        case Value_Type_Kind::Slice:
//        case Value_Type_Kind::Str:
        case Value_Type_Kind::Range:
            break;
            
        default:
            error(iterable->location, "Cannot iterate over something of type '%s'.", iterable->type.display_str());
            break;
    }
    
    Value_Type *target_type = iterable->type.child_type();
    
    t.begin_scope();
    
    auto processed_target = Mem.make<Typed_AST_Processed_Pattern>(target->location);
    t.bind_pattern(target, *target_type, processed_target);
    
    if (counter != "") {
        t.bind_variable(counter.str(), value_types::Int, false, location);
    }
    
    auto body = this->body->typecheck(t);
    
    t.end_scope();
    
    t.has_return = false;
    
    return Mem.make<Typed_AST_For>(
        iterable->type.kind == Value_Type_Kind::Range ?
            Typed_AST_Kind::For_Range :
            Typed_AST_Kind::For,
        label,
        processed_target,
        counter.clone(),
        iterable,
        body.cast<Typed_AST_Multiary>(),
        location
    );
}

Ref<Typed_AST> Untyped_AST_Forever::typecheck(Typer &t) {
    Ref<Typed_AST_Ident> label = nullptr;
    if (this->label) {
        label = Mem.make<Typed_AST_Ident>(this->label->id.clone(), value_types::None, this->label->location);
    }

    auto body = this->body->typecheck(t).cast<Typed_AST_Multiary>();
    internal_verify(body, "body didn't typecheck to a Multiary.");

    return Mem.make<Typed_AST_Forever>(label, body, location);
}

Ref<Typed_AST> Untyped_AST_Match::typecheck(Typer &t) {
    auto cond = this->cond->typecheck(t);
    
    bool has_return = true;
    
    Ref<Typed_AST> default_arm = nullptr;
    if (this->default_arm) {
        default_arm = this->default_arm->typecheck(t);
        
        has_return = t.has_return;
        t.has_return = false;
    }
    
    auto arms = Mem.make<Typed_AST_Multiary>(Typed_AST_Kind::Comma, this->arms->location);
    for (auto arm : this->arms->nodes) {
        internal_verify(arm->kind == Untyped_AST_Kind::Match_Arm, "Arm node in match node is not an Untyped_AST_Kind::Match_Arm.");
        auto arm_bin = arm.cast<Untyped_AST_Binary>();
        
        t.begin_scope();
        
        auto pat = arm_bin->lhs.cast<Untyped_AST_Pattern>();
        auto match_pat = Mem.make<Typed_AST_Match_Pattern>(pat->location);
        t.bind_match_pattern(pat, cond->type, match_pat, 0);
        
        auto body = arm_bin->rhs->typecheck(t);
        
        t.end_scope();
        
        auto typechecked_arm = Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Match_Arm, value_types::None, match_pat, body, arm->location);
        arms->add(typechecked_arm);
        
        has_return &= t.has_return;
        t.has_return = false;
    }
    
    t.has_return = has_return && default_arm;
    
    return Mem.make<Typed_AST_Match>(
        cond,
        default_arm,
        arms,
        location
    );
}

Ref<Typed_AST> Untyped_AST_Let::typecheck(Typer &t) {
    Value_Type ty = value_types::None;
    
    Ref<Typed_AST_Type_Signature> sig = nullptr;
    if (specified_type) {
        sig = specified_type->typecheck(t).cast<Typed_AST_Type_Signature>();
        internal_verify(sig, "Failed to cast to Type_Sig in Untyped_AST_Let::typecheck().");
        ty = *sig->value_type;
    }
    
    Ref<Typed_AST> init = nullptr;
    if (initializer) {
        if (initializer->kind == Untyped_AST_Kind::Noinit) {
            init = Mem.make<Typed_AST_Nullary>(Typed_AST_Kind::Allocate, ty, initializer->location);
        } else {
            init = initializer->typecheck(t);
            if (sig) {
                verify(specified_type->value_type->assignable_from(init->type), init->location, "Given type '%s' does not match specified type '%s'.", init->type.display_str(), sig->value_type->display_str());
            } else {
                ty = init->type;
            }
        }
    }
    
    if (is_const) {
        verify(!(ty.is_mut || ty.is_partially_mutable()), location, "Constants must be completely immutable.");
    }

    auto processed_target = Mem.make<Typed_AST_Processed_Pattern>(target->location);
    t.bind_pattern(target, ty, processed_target);
    
    return Mem.make<Typed_AST_Let>(is_const, processed_target, sig, init, location);
}

Ref<Typed_AST> Untyped_AST_Generic_Specification::typecheck(Typer &t) {
    todo("Generic_Specialization::typecheck() not yet implemented.");
}

Ref<Typed_AST> Untyped_AST_Struct_Declaration::typecheck(Typer &t) {
    Struct_Definition defn;
    defn.module = t.module;
    defn.uuid = t.interp->next_uuid();
    defn.name = this->id.clone();
    
    Size current_offset = 0;
    for (auto &f : fields) {
        Struct_Field field;
        field.id = f.id.clone();
        verify(!defn.has_field(field.id), location, "Redefinition of field '%.*s'.", field.id.size(), field.id.c_str());
        field.offset = current_offset;
        field.type = t.resolve_value_type(*f.type->value_type);
        defn.fields.push_back(field);
        current_offset += field.type.size();
    }
    
    defn.size = current_offset;
    auto new_defn = t.interp->types.add_struct(defn);
    t.module->add_struct_member(new_defn);
    
    Value_Type *struct_type = Mem.make<Value_Type>().as_ptr();
    struct_type->kind = Value_Type_Kind::Struct;
    struct_type->data.struct_.defn = new_defn;
    
    Value_Type type;
    type.kind = Value_Type_Kind::Type;
    type.data.type.type = struct_type;
    
    String id = this->id.clone();
    t.bind_type(id.str(), type, location);
    
    return nullptr;
}

Ref<Typed_AST> Untyped_AST_Enum_Declaration::typecheck(Typer &t) {
    Enum_Definition defn;
    defn.is_sumtype = false;
    defn.module = t.module;
    defn.uuid = t.interp->next_uuid();
    defn.name = id.clone();
    defn.size = value_types::Int.size();
    
    bool is_sumtype = false;
    for (size_t i = 0; i < variants.size(); i++) {
        auto &v = variants[i];
        Enum_Variant defn_v;
        defn_v.tag = i;
        defn_v.id = v.id.clone();
        
        if (v.payload) {
            is_sumtype = true;
            
            Size field_offset = value_types::Int.size();
            for (auto n : v.payload->nodes) {
                switch (n->kind) {
                    case Untyped_AST_Kind::Type_Signature: {
                        auto sig = n->typecheck(t).cast<Typed_AST_Type_Signature>();
                        
                        Enum_Payload_Field pf;
                        pf.offset = field_offset;
                        pf.type = *sig->value_type;
                        defn_v.payload.push_back(pf);
                        
                        field_offset += pf.type.size();
                    } break;
                    case Untyped_AST_Kind::Binding:
                        todo("Enum payloads that are bindings.");
                        break;
                        
                    default:
                        internal_error("Invalid kind in Untyped_AST_Enum_Declaration::typecheck(): %d.", n->kind);
                        break;
                }
            }
        }
        
        defn.variants.push_back(defn_v);
    }
    
    if (is_sumtype) {
        defn.is_sumtype = true;
        
        Size max_payload_size = 0;
        for (auto &v : defn.variants) {
            Size payload_size = 0;
            for (auto &f : v.payload) {
                payload_size += f.type.size();
            }
            
            if (payload_size > max_payload_size) {
                max_payload_size = payload_size;
            }
        }
        
        defn.size += max_payload_size;
    }
    
    auto new_defn = t.interp->types.add_enum(defn);
    t.module->add_enum_member(new_defn);
    
    Value_Type *enum_type = Mem.make<Value_Type>().as_ptr();
    enum_type->kind = Value_Type_Kind::Enum;
    enum_type->data.enum_.defn = new_defn;
    
    Value_Type type;
    type.kind = Value_Type_Kind::Type;
    type.data.type.type = enum_type;
    
    String id = this->id.clone();
    t.bind_type(id.str(), type, location);
    
    return nullptr;
}

enum class Is_Method {
    No = 0,
    Yes = 1,
};

static Trait_Method typecheck_trait_fn_decl_header(
    Typer &t, 
    Untyped_AST_Fn_Declaration_Header &decl, 
    Is_Method is_method) 
{
    Trait_Method method;
    method.name = decl.id;
    method.variadic = decl.varargs;
    method.is_method = static_cast<bool>(is_method);

    if (decl.return_type_signature) {
        method.return_type = t.resolve_value_type(*decl.return_type_signature->value_type);
    } else {
        method.return_type = value_types::Void;
    }

    for (size_t i = 0; i < decl.params->nodes.size(); i++) {
        auto param = decl.params->nodes[i];

        //
        // @COPYPASTE(typecheck_fn_decl_header)
        //
        String param_name;
        Value_Type param_type;
        switch (param->kind) {
            case Untyped_AST_Kind::Assignment:
                todo("Default arguments not yet implemented.");
                break;
            case Untyped_AST_Kind::Binding: {
                auto b = param.cast<Untyped_AST_Binary>();
                internal_verify(b, "param in trait fn decl header not a binary node.");

                auto id = b->lhs.cast<Untyped_AST_Pattern_Ident>();
                param_name = id->id.clone();
                param_type = t.resolve_value_type(*b->rhs.cast<Untyped_AST_Type_Signature>()->value_type);
                param_type.is_mut = id->is_mut;
            } break;

            default:
                error(param->location, "Expected a parameter.");
                break;
        }
        
        if (method.variadic && i == decl.params->nodes.size() - 1) {
            verify(param_type.kind == Value_Type_Kind::Slice, param->location, "Variadic parameter must be a slice type but was given '%s'.", param_type.display_str());
        }

        method.params.push_back({ param_name, param_type });
    }

    return method;
}

Ref<Typed_AST> Untyped_AST_Trait_Declaration::typecheck(Typer &t) {
    Trait_Definition _defn;
    _defn.module = t.module;
    _defn.uuid = t.interp->next_uuid();
    _defn.name = id.clone();

    Trait_Definition *defn = t.interp->types.add_trait(_defn);

    t.begin_scope();
    Value_Type *trait_ty = Mem.make<Value_Type>(value_types::trait(defn, nullptr)).as_ptr();
    t.bind_type("Self", value_types::type_of(trait_ty), location);

    for (auto node : body->nodes) {
        switch (node->kind) {
            case Untyped_AST_Kind::Method_Decl_Header:
            case Untyped_AST_Kind::Fn_Decl_Header: {
                auto decl = node.cast<Untyped_AST_Fn_Declaration_Header>();
                internal_verify(decl, "Failed to cast to 'Fn_Decl_Header *'");

                auto is_method = static_cast<Is_Method>(node->kind == Untyped_AST_Kind::Method_Decl_Header);
                auto trait_method = typecheck_trait_fn_decl_header(t, *decl, is_method);
                defn->methods.push_back(trait_method);
            } break;

            case Untyped_AST_Kind::Method_Decl:
            case Untyped_AST_Kind::Fn_Decl: {
                error(node->location, "trait functions with default implementations not yet implemented.");
            } break;

            default:
                error(node->location, "This type of declaration is disallowed in trait bodies.");
                break;
        }
    }

    t.end_scope();

    return nullptr;
}

static Function_Definition *typecheck_fn_decl_header(
    Typer &t,
    Untyped_AST_Fn_Declaration_Header &decl)
{
    Function_Definition defn;
    defn.varargs = decl.varargs;
    defn.uuid = t.interp->next_uuid();
    defn.module = t.module;
    defn.name = decl.id.clone();

    Value_Type func_type;
    func_type.kind = Value_Type_Kind::Function;

    Value_Type *return_type = Mem.make<Value_Type>().as_ptr();
    if (decl.return_type_signature) {
        *return_type = decl.return_type_signature->typecheck(t)
            .cast<Typed_AST_Type_Signature>()->value_type->clone();
    } else {
        *return_type = value_types::Void;
    }

    func_type.data.func.return_type = return_type;

    auto param_types = Array<Value_Type>::with_size(decl.params->nodes.size());
    for (size_t i = 0; i < param_types.size(); i++) {
        auto param = decl.params->nodes[i];

        //
        // @TODO:
        //      Sort out default arguments. (If we do default arguments)
        //

        String param_name;
        Value_Type param_type;
        switch (param->kind) {
            case Untyped_AST_Kind::Assignment:
                todo("Default arguments not yet implemented.");
                break;
            case Untyped_AST_Kind::Binding: {
                auto b = param.cast<Untyped_AST_Binary>();
                auto id = b->lhs.cast<Untyped_AST_Pattern_Ident>();
                param_name = id->id.clone();
                param_type = *b->rhs->typecheck(t)
                    .cast<Typed_AST_Type_Signature>()->value_type;
                param_type.is_mut = id->is_mut;
            } break;

            default:
                error(param->location, "Expected a parameter.");
                break;
        }
        
        if (defn.varargs && i == param_types.size() - 1) {
            verify(param_type.kind == Value_Type_Kind::Slice, param->location, "Variadic parameter must be a slice type but was given '%s'.", param_type.display_str());
        }

        param_types[i] = param_type;
        defn.param_names.push_back(param_name);
    }

    func_type.data.func.arg_types = param_types;
    defn.type = func_type;

    return t.interp->functions.add_func(defn);
}

static Ref<Typed_AST_Fn_Declaration> typecheck_fn_decl_body(
    Typer &t,
    Untyped_AST_Fn_Declaration &decl,
    Function_Definition *defn)
{
    auto new_t = Typer { t, defn };

    new_t.begin_scope();
    
    new_t.bind_function(defn->name.str(), defn->uuid, defn->type, decl.location);
    
    for (size_t i = 0; i < defn->param_names.size(); i++) {
        String param_name = defn->param_names[i];
        Value_Type param_type = defn->type.data.func.arg_types[i];
        new_t.bind_variable(param_name.str(), param_type, param_type.is_mut, decl.params->nodes[i]->location);
    }

    auto body = decl.body->typecheck(new_t).cast<Typed_AST_Multiary>();

    verify(defn->type.data.func.return_type->kind == Value_Type_Kind::Void ||
           new_t.has_return, body->location, "Not all paths return a value in non-void function '%.*s'.", defn->name.size(), defn->name.c_str());
    
    return Mem.make<Typed_AST_Fn_Declaration>(defn, body, decl.location);
}

struct Typecheck_Fn_Decl_Result {
    Function_Definition *defn;
    Ref<Typed_AST_Fn_Declaration> typed_decl;
};

static Typecheck_Fn_Decl_Result typecheck_fn_decl(
    Typer &t,
    Untyped_AST_Fn_Declaration &decl)
{
    auto defn = typecheck_fn_decl_header(t, decl);
    auto typed_decl = typecheck_fn_decl_body(t, decl, defn);
    return { defn, typed_decl };
}

Ref<Typed_AST> Untyped_AST_Fn_Declaration::typecheck(Typer &t) {
    auto [defn, typed_decl] = typecheck_fn_decl(t, *this);
    t.bind_function(id.str(), defn->uuid, defn->type, location);
    t.module->add_func_member(defn);
    return typed_decl;
}

Ref<Typed_AST> Untyped_AST_Fn_Declaration_Header::typecheck(Typer &t) {
    internal_error("Call to Untyped_AST_Fn_Declaration_Header::typecheck() is disallowed.");
}

static Ref<Typed_AST_Multiary> typecheck_impl_declaration(
    Typer &t,
    const char *type_name,
    std::unordered_map<std::string, Method> &methods,
    Ref<Untyped_AST_Multiary> body)
{
    // Prepass so everything can refer to everything else regardless of order
    // in the impl block
    struct Prepass {
        Function_Definition *defn;
        Ref<Untyped_AST_Fn_Declaration> decl;
    };
    
    auto prepasses = std::vector<Prepass> { body->nodes.size() };
    for (int i = 0; i < body->nodes.size(); i++) {
        auto node = body->nodes[i];
        
        switch (node->kind) {
            case Untyped_AST_Kind::Fn_Decl:
            case Untyped_AST_Kind::Method_Decl: {
                auto decl = node.cast<Untyped_AST_Fn_Declaration>();
                auto defn = typecheck_fn_decl_header(t, *decl);
                
                verify(methods.find(defn->name.c_str()) == methods.end(), node->location, "Cannot have two methods of the same name for one type. Reused name '%s'. Type '%s'.", defn->name.c_str(), type_name);
                
                auto fn_sid = defn->name.str();
                methods[fn_sid] = {
                    node->kind == Untyped_AST_Kind::Fn_Decl,
                    defn->uuid
                };
                
                prepasses[i] = { defn, decl };
            } break;
                
            default:
                error(node->location, "Impl declaration bodies can only contain function declarations, for now.");
                break;
        }
    }
    
    
    //
    // @HACK:
    //      Giving this Typed_AST_Kind::Comma might cause problems in the future.
    //      It was made this way to prevent redundant Flush instructions from being
    //      emitted.
    //
    auto typed_body = Mem.make<Typed_AST_Multiary>(Typed_AST_Kind::Comma, body->location);
    for (auto [defn, decl] : prepasses) {
        auto typechecked_decl = typecheck_fn_decl_body(t, *decl, defn);
        typed_body->add(typechecked_decl);
    }
    
    return typed_body;
}

Ref<Typed_AST> typecheck_impl_for_type(Typer &t, Untyped_AST_Impl_Declaration &impl) {
    auto target = impl.target->typecheck(t).cast<Typed_AST_UUID>();
    internal_verify(target, "Failed to cast target to UUID* in Untyped_AST_Impl_Declaration::typecheck().");
    
    Ref<Typed_AST> typechecked = nullptr;
    
    switch (target->type.kind) {
        case Value_Type_Kind::Type:
            switch (target->type.data.type.type->kind) {
                case Value_Type_Kind::Struct: {
                    auto defn = t.interp->types.get_struct_by_uuid(target->uuid);
                    internal_verify(defn, "Failed to retrieve Struct_Definition from typebook.");
                    
                    t.begin_scope();
                    t.bind_type("Self", target->type, target->location);
                    typechecked = typecheck_impl_declaration(t, defn->name.c_str(), defn->methods, impl.body);
                    t.end_scope();
                } break;
                case Value_Type_Kind::Enum: {
                    auto defn = t.interp->types.get_enum_by_uuid(target->uuid);
                    internal_verify(defn, "Failed to retrieve Enum_Definition from typebook.");
                    
                    t.begin_scope();
                    t.bind_type("Self", target->type, target->location);
                    typechecked = typecheck_impl_declaration(t, defn->name.c_str(), defn->methods, impl.body);
                    t.end_scope();
                } break;
                    
                default:
                    internal_error("Invalid Value_Type_Kind: %d.", target->type.data.type.type->kind);
                    break;
            }
            break;
            
        default:
            error(target->location, "Cannot implement something that isn't a type.");
            break;
    }
    
    return typechecked;
}

Ref<Typed_AST> typecheck_impl_for_trait(Typer &t, Untyped_AST_Impl_Declaration &impl) {
    todo("Implement %s", __func__);
}

Ref<Typed_AST> Untyped_AST_Impl_Declaration::typecheck(Typer &t) {
    Ref<Typed_AST> typechecked;
    if (for_) {
        typechecked = typecheck_impl_for_trait(t, *this);
    } else {
        typechecked = typecheck_impl_for_type(t, *this);
    }
    return typechecked;
}

struct Module_Path {
    String filepath;
    std::vector<String> segments;
    
    String name() const {
        return segments.back();
    }
};

static Module_Path generate_module_path_from_symbol(Untyped_AST_Symbol &path) {
    std::stringstream s;
    Untyped_AST_Symbol *segment = &path;
    while (true) {
        if (segment->kind == Untyped_AST_Kind::Ident) {
            auto id = dynamic_cast<Untyped_AST_Ident *>(segment);
            s << id->id.c_str() << ".fox";
            break;
        }
        
        auto path = dynamic_cast<Untyped_AST_Path *>(segment);
        s << path->lhs->id.c_str() << "/";
        
        segment = path->rhs.as_ptr();
    }
    
    std::string cpp_path_str = s.str();
    String path_str = String {
        SMem.duplicate(cpp_path_str.c_str(), cpp_path_str.size()),
        cpp_path_str.size()
    };
    
    std::vector<String> segments;
    
    for (size_t i = 0; i < path_str.size(); i++) {
        char *seg_str = &path_str.c_str()[i];
        size_t len = 0;
        for (; i < path_str.size(); i++) {
            char c = path_str.c_str()[i];
            if (c == '/') {
                break;
            } else if (c == '.') {
                internal_verify(i == path_str.size() - 4, "Unexpected '.' in module path");
                i = path_str.size();
                break;
            }
            len++;
        }
        segments.push_back({ seg_str, len });
    }
    
    return Module_Path { path_str, segments };
}

Ref<Typed_AST> Untyped_AST_Import_Declaration::typecheck(Typer &t) {
    Module_Path module_path = generate_module_path_from_symbol(*path);
    Module *module = t.interp->compile_module(module_path.filepath);
    
    if (rename_id) {
        auto module_name = rename_id->id;
        if (module_name == "*") {
            t.bind_module_members(module, path->location);
        } else {
            t.bind_module(module_name.str(), module, path->location);
        }
    } else if (module_path.segments.size() > 1) {
        char *module_path_start = module_path.segments[0].begin();
        Module *parent_module = nullptr;
        Module *previous_module = nullptr;
        for (size_t i = 0; i < module_path.segments.size() - 1; i++) {
            String segment = module_path.segments[i];
            
            size_t len = segment.size() + segment.begin() - module_path_start;
            auto segment_path = String { module_path_start, len };
            
            Module *segment_module = t.interp->get_or_create_module(segment_path);
            
            if (previous_module) {
                std::string segment_name = segment.str();
                
                Module::Member member;
                if (previous_module->find_member_by_id(segment_name, member)) {
                    verify(member.kind == Module::Member::Submodule, location, "'%s' is not a submodule of '%s'.", segment_name.c_str(), previous_module->module_path.c_str());
                } else {
                    previous_module->add_submodule(segment_name, segment_module);
                }
            } else {
                parent_module = segment_module;
            }
            
            previous_module = segment_module;
        }
        
        internal_verify(previous_module, "'previous_module' is null when it should point to a Module.");
        
        std::string module_name = module_path.name().str();
        Module::Member member;
        if (previous_module->find_member_by_id(module_name, member)) {
            verify(member.kind == Module::Member::Submodule, location, "'%s' is not a submodule of '%s'.", module_name.c_str(), previous_module->module_path.c_str());
        } else {
            previous_module->add_submodule(module_name, module);
        }
        
        internal_verify(parent_module, "'parent_module' is null when it shouldn't be.");
        t.bind_module(module_path.segments[0].str(), parent_module, path->location);
    } else {
        t.bind_module(module_path.name().str(), module, path->location);
    }
    
    return nullptr;
}

static Ref<Typed_AST> typecheck_dot_call_for_string(
    Typer &t,
    Ref<Typed_AST> receiver,
    String method_id,
    Ref<Untyped_AST_Multiary> args,
    Code_Location location)
{
    Ref<Typed_AST> typechecked;
    
    if (method_id == "bytes") {
        verify(args->nodes.size() == 0, args->location, "Incorrect number of arguments passed to 'len'. Expected 0 byt was given %zu.", args->nodes.size());

        size_t offset = offsetof(runtime::String, s);

        Value_Type ty = value_types::slice_of(const_cast<Value_Type *>(&value_types::Byte));
        ty.is_mut = receiver->type.is_mut;
        
        bool deref = receiver->type.kind == Value_Type_Kind::Ptr;
        
        typechecked = Mem.make<Typed_AST_Field_Access>(
            ty,
            deref,
            receiver,
            static_cast<Size>(offset),
            location
        );
    } else if (method_id == "len") {
        verify(args->nodes.size() == 0, args->location, "Incorrect number of arguments passed to 'len'. Expected 0 but was given %zu.", args->nodes.size());
        
        size_t offset = offsetof(runtime::String, len);
        
        Value_Type ty = value_types::Int;
        ty.is_mut = receiver->type.is_mut;

        bool deref = receiver->type.kind == Value_Type_Kind::Ptr;
        
        typechecked = Mem.make<Typed_AST_Field_Access>(ty, deref, receiver, static_cast<Size>(offset), location);
    } else {
        error(receiver->location, "'%s' is not a method of 'str'.", method_id.c_str());
    }
    
    return typechecked;
}

static Ref<Typed_AST> typecheck_dot_call_for_slice(
    Typer &t,
    Ref<Typed_AST> receiver,
    String method_id,
    Ref<Untyped_AST_Multiary> args,
    Code_Location location)
{
    Ref<Typed_AST> typechecked;
    
    if (method_id == "data") {
        verify(args->nodes.size() == 0, args->location, "Incorrect number of arguments passed to 'data'. Expected 0 but was given %zu.", args->nodes.size());
        
        size_t offset = offsetof(runtime::Slice, data);
        bool deref = receiver->type.kind == Value_Type_Kind::Ptr;
        Value_Type type;
        if (deref) {
            type = value_types::ptr_to(receiver->type.data.ptr.child_type->data.slice.element_type);
        } else {
            type = value_types::ptr_to(receiver->type.data.slice.element_type);
        }
        
        typechecked = Mem.make<Typed_AST_Field_Access>(type, deref, receiver, static_cast<Size>(offset), location);
    } else if (method_id == "len") {
        verify(args->nodes.size() == 0, args->location, "Incorrect number of arguments passed to 'len'. Expected 0 but was given %zu.", args->nodes.size());
        
        size_t offset = offsetof(runtime::Slice, count);

        Value_Type ty = value_types::Int;
        ty.is_mut = receiver->type.is_mut;

        bool deref = receiver->type.kind == Value_Type_Kind::Ptr;
        
        typechecked = Mem.make<Typed_AST_Field_Access>(ty, deref, receiver, static_cast<Size>(offset), location);
    } else {
        error(receiver->location, "'%s' is not a method of '%s'.", method_id.c_str(), receiver->type.display_str());
    }
    
    return typechecked;
}

static Ref<Typed_AST> typecheck_dot_call_for_struct(
    Typer &t,
    Ref<Typed_AST> receiver,
    String method_id,
    Ref<Untyped_AST_Multiary> args,
    Code_Location location)
{
    Struct_Definition *defn;
    if (receiver->type.kind == Value_Type_Kind::Ptr) {
        defn = receiver->type.data.ptr.child_type->data.struct_.defn;
    } else {
        defn = receiver->type.data.struct_.defn;
    }
    
    Method method;
    verify(defn->find_method(method_id, method), receiver->location, "Struct type '%s' does not have a method called '%s'.", defn->name.c_str(), method_id.c_str());
    verify(!method.is_static, receiver->location, "Cannot call '%s' with dot call since the method does not take a receiver.", method_id.c_str());
    
    auto method_defn = t.interp->functions.get_func_by_uuid(method.uuid);
    internal_verify(method_defn, "Failed to retreive method from funcbook.");
    
    Value_Type method_type = method_defn->type;
    
    auto method_uuid = Mem.make<Typed_AST_UUID>(Typed_AST_Kind::Ident_Func, method.uuid, method_type, args->location);
    
    //
    // @TODO:
    //       Handle default arguments (if we do them)
    //
    if (receiver->type.kind != Value_Type_Kind::Ptr) {
        Value_Type ptr_ty;
        ptr_ty.kind = Value_Type_Kind::Ptr;
        ptr_ty.data.ptr.child_type = &receiver->type;
        receiver = Mem.make<Typed_AST_Unary>(Typed_AST_Kind::Address_Of, ptr_ty, receiver, receiver->location);
    }
    
    verify(method_type.data.func.arg_types[0].assignable_from(receiver->type), receiver->location, "Cannot call this method because the receiver's type does not match the parameter's type. Expected '%s' but was given '%s'.", method_type.data.func.arg_types[0].display_str(), receiver->type.display_str());
    
    auto typechecked_args = Mem.make<Typed_AST_Multiary>(Typed_AST_Kind::Comma, args->location);
    typechecked_args->add(receiver);
    
    if (method_defn->varargs) {
        auto typechecked_varargs = Mem.make<Typed_AST_Multiary>(Typed_AST_Kind::Comma, args->location);
        
        typecheck_function_call_arguments(t, method_defn, typechecked_args, typechecked_varargs, args, Skip_Receiver::Do_Skip);
        
        Size varargs_size = 0;
        for (auto n : typechecked_varargs->nodes) {
            varargs_size += n->type.size();
        }
        
        return Mem.make<Typed_AST_Variadic_Call>(
            *method_type.data.func.return_type,
            varargs_size,
            method_uuid,
            typechecked_args,
            typechecked_varargs,
            location
        );
    } else {
        typecheck_function_call_arguments(t, method_defn, typechecked_args, nullptr, args, Skip_Receiver::Do_Skip);
        
        return Mem.make<Typed_AST_Binary>(
            Typed_AST_Kind::Function_Call,
            *method_type.data.func.return_type,
            method_uuid,
            typechecked_args,
            location
        );
    }
}

static Ref<Typed_AST> typecheck_dot_call_for_enum(
    Typer &t,
    Ref<Typed_AST> receiver,
    String method_id,
    Ref<Untyped_AST_Multiary> args,
    Code_Location location)
{
    Enum_Definition *defn;
    if (receiver->type.kind == Value_Type_Kind::Ptr) {
        defn = receiver->type.data.ptr.child_type->data.enum_.defn;
    } else {
        defn = receiver->type.data.enum_.defn;
    }
    
    Method method;
    verify(defn->find_method(method_id, method), receiver->location, "Enum type '%s' does not have a method called '%s'.", defn->name.c_str(), method_id.c_str());
    verify(!method.is_static, receiver->location, "Cannot call '%s' with dot call since the method does not take a receiver.", method_id.c_str());
    
    auto method_defn = t.interp->functions.get_func_by_uuid(method.uuid);
    internal_verify(method_defn, "Failed to retreive method from funcbook.");
    
    Value_Type method_type = method_defn->type;
    
    auto method_uuid = Mem.make<Typed_AST_UUID>(Typed_AST_Kind::Ident_Func, method.uuid, method_type, args->location);
    
    //
    // @TODO:
    //       Handle default arguments (if we do them)
    //
    if (receiver->type.kind != Value_Type_Kind::Ptr) {
        Value_Type ptr_ty;
        ptr_ty.kind = Value_Type_Kind::Ptr;
        ptr_ty.data.ptr.child_type = &receiver->type;
        receiver = Mem.make<Typed_AST_Unary>(Typed_AST_Kind::Address_Of, ptr_ty, receiver, receiver->location);
    }
    
    verify(method_type.data.func.arg_types[0].assignable_from(receiver->type), receiver->location, "Cannot call this method because the receiver's type does not match the parameter's type. Expected '%s' but was given '%s'.", method_type.data.func.arg_types[0].display_str(), receiver->type.display_str());
    
    auto typechecked_args = Mem.make<Typed_AST_Multiary>(Typed_AST_Kind::Comma, args->location);
    typechecked_args->add(receiver);
    
    if (method_defn->varargs) {
        auto typechecked_varargs = Mem.make<Typed_AST_Multiary>(Typed_AST_Kind::Comma, args->location);
        
        typecheck_function_call_arguments(t, method_defn, typechecked_args, typechecked_varargs, args, Skip_Receiver::Do_Skip);
        
        Size varargs_size = 0;
        for (auto n : typechecked_varargs->nodes) {
            varargs_size += n->type.size();
        }
        
        return Mem.make<Typed_AST_Variadic_Call>(
            *method_type.data.func.return_type,
            varargs_size,
            method_uuid,
            typechecked_args,
            typechecked_varargs,
            location
        );
    } else {
        typecheck_function_call_arguments(t, method_defn, typechecked_args, nullptr, args, Skip_Receiver::Do_Skip);
        
        return Mem.make<Typed_AST_Binary>(
            Typed_AST_Kind::Function_Call,
            *method_type.data.func.return_type,
            method_uuid,
            typechecked_args,
            location
        );
    }
}

Ref<Typed_AST> Untyped_AST_Dot_Call::typecheck(Typer &t) {
    auto receiver = this->receiver->typecheck(t);
    
    Value_Type receiver_type = receiver->type;
    if (receiver_type.kind == Value_Type_Kind::Ptr) {
        receiver_type = *receiver_type.data.ptr.child_type;
    }
    
    Ref<Typed_AST> typechecked;
    switch (receiver_type.kind) {
        case Value_Type_Kind::Str:
            typechecked = typecheck_dot_call_for_string(t, receiver, method_id, args, location);
            break;
        case Value_Type_Kind::Slice:
            typechecked = typecheck_dot_call_for_slice(t, receiver, method_id, args, location);
            break;
        case Value_Type_Kind::Struct:
            typechecked = typecheck_dot_call_for_struct(t, receiver, method_id, args, location);
            break;
        case Value_Type_Kind::Enum:
            typechecked = typecheck_dot_call_for_enum(t, receiver, method_id, args, location);
            break;
            
        default:
            error(receiver->location, "Cannot use dot calls with something that isn't a struct or enum type, for now.");
            break;
    }
    
    return typechecked;
}
