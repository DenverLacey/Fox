//
//  typer.cpp
//  Fox
//
//  Created by Denver Lacey on 23/8/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#include "typer.h"

#include "ast.h"
#include "compiler.h"
#include "error.h"
#include "interpreter.h"

#include <forward_list>
#include <unordered_map>
#include <string>

Typed_AST_Bool::Typed_AST_Bool(bool value) {
    kind = Typed_AST_Kind::Bool;
    type.kind = Value_Type_Kind::Bool;
    this->value = value;
}

bool Typed_AST_Bool::is_constant(Compiler &c) {
    return true;
}

Typed_AST_Char::Typed_AST_Char(char32_t value) {
    kind = Typed_AST_Kind::Char;
    type.kind = Value_Type_Kind::Char;
    this->value = value;
}

bool Typed_AST_Char::is_constant(Compiler &c) {
    return true;
}

Typed_AST_Float::Typed_AST_Float(double value) {
    kind = Typed_AST_Kind::Float;
    type.kind = Value_Type_Kind::Float;
    this->value = value;
}

bool Typed_AST_Float::is_constant(Compiler &c) {
    return true;
}

Typed_AST_Ident::Typed_AST_Ident(String id, Value_Type type) {
    kind = Typed_AST_Kind::Ident;
    this->type = type;
    this->id = id;
}

Typed_AST_Ident::~Typed_AST_Ident() {
    id.free();
}

bool Typed_AST_Ident::is_constant(Compiler &c) {
    auto [status, _] = c.find_variable(id);
    return status == Find_Variable_Result::Found_Constant;
}

Typed_AST_UUID::Typed_AST_UUID(Typed_AST_Kind kind, UUID uuid, Value_Type type) {
    this->kind = kind;
    this->uuid = uuid;
    this->type = type;
}

bool Typed_AST_UUID::is_constant(Compiler &c) {
    return true;
}

Typed_AST_Int::Typed_AST_Int(int64_t value) {
    kind = Typed_AST_Kind::Int;
    type.kind = Value_Type_Kind::Int;
    this->value = value;
}

bool Typed_AST_Int::is_constant(Compiler &c) {
    return true;
}

Typed_AST_Str::Typed_AST_Str(String value) {
    kind = Typed_AST_Kind::Str;
    type.kind = Value_Type_Kind::Str;
    this->value = value;
}

Typed_AST_Str::~Typed_AST_Str() {
    value.free();
}

bool Typed_AST_Str::is_constant(Compiler &c) {
    return true;
}

Typed_AST_Nullary::Typed_AST_Nullary(Typed_AST_Kind kind, Value_Type type) {
    this->kind = kind;
    this->type = type;
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

Typed_AST_Unary::Typed_AST_Unary(Typed_AST_Kind kind, Value_Type type, Ref<Typed_AST> sub) {
    this->kind = kind;
    this->type = type;
    this->sub = sub;
}

bool Typed_AST_Unary::is_constant(Compiler &c) {
    return sub->is_constant(c);
}

Typed_AST_Return::Typed_AST_Return(Ref<Typed_AST> sub)
    : Typed_AST_Unary(Typed_AST_Kind::Return, value_types::None, sub)
{
}

bool Typed_AST_Return::is_constant(Compiler &c) {
    return sub ? sub->is_constant(c) : true;
}

Typed_AST_Binary::Typed_AST_Binary(Typed_AST_Kind kind, Value_Type type, Ref<Typed_AST> lhs, Ref<Typed_AST> rhs) {
    this->kind = kind;
    this->type = type;
    this->lhs = lhs;
    this->rhs = rhs;
}

bool Typed_AST_Binary::is_constant(Compiler &c) {
    return lhs->is_constant(c) && rhs->is_constant(c);
}

Typed_AST_Ternary::Typed_AST_Ternary(Typed_AST_Kind kind, Value_Type type, Ref<Typed_AST> lhs, Ref<Typed_AST> mid, Ref<Typed_AST> rhs) {
    this->kind = kind;
    this->type = type;
    this->lhs = lhs;
    this->mid = mid;
    this->rhs = rhs;
}

bool Typed_AST_Ternary::is_constant(Compiler &c) {
    return lhs->is_constant(c) && mid->is_constant(c) && rhs->is_constant(c);
}

Typed_AST_Multiary::Typed_AST_Multiary(Typed_AST_Kind kind) {
    this->kind = kind;
    this->type.kind = Value_Type_Kind::None;
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
    Ref<Typed_AST_Multiary> element_nodes)
{
    this->type = type;
    this->kind = kind;
    this->count = count;
    this->array_type = array_type;
    this->element_nodes = element_nodes;
}

bool Typed_AST_Array::is_constant(Compiler &c) {
    return element_nodes->is_constant(c);
}

Typed_AST_Enum_Literal::Typed_AST_Enum_Literal(
    Value_Type enum_type,
    runtime::Int tag,
    Ref<Typed_AST_Multiary> payload)
{
    this->kind = Typed_AST_Kind::Enum;
    this->type = enum_type;
    this->tag = tag;
    this->payload = payload;
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
    Ref<Typed_AST> else_)
{
    kind = Typed_AST_Kind::If;
    this->type = type;
    this->cond = cond;
    this->then = then;
    this->else_ = else_;
}

bool Typed_AST_If::is_constant(Compiler &c) {
    return cond->is_constant(c) && then->is_constant(c) && else_->is_constant(c);
}

Typed_AST_Type_Signature::Typed_AST_Type_Signature(Ref<Value_Type> value_type) {
    this->kind = Typed_AST_Kind::Type_Signature;
    this->value_type = value_type;
}

bool Typed_AST_Type_Signature::is_constant(Compiler &c) {
    return true;
}

Typed_AST_Processed_Pattern::Typed_AST_Processed_Pattern() {
    kind = Typed_AST_Kind::Processed_Pattern;
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

Typed_AST_Match_Pattern::Typed_AST_Match_Pattern() {
    this->kind = Typed_AST_Kind::Match_Pattern;
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

Typed_AST_For::Typed_AST_For(
    Typed_AST_Kind kind,
    Ref<Typed_AST_Processed_Pattern> target,
    String counter,
    Ref<Typed_AST> iterable,
    Ref<Typed_AST_Multiary> body)
{
    this->kind = kind;
    this->target = target;
    this->counter = counter;
    this->iterable = iterable;
    this->body = body;
}

Typed_AST_For::~Typed_AST_For() {
    counter.free();
}

bool Typed_AST_For::is_constant(Compiler &c) {
    return iterable->is_constant(c) && body->is_constant(c);
}

Typed_AST_Match::Typed_AST_Match(
    Ref<Typed_AST> cond,
    Ref<Typed_AST> default_arm,
    Ref<Typed_AST_Multiary> arms)
{
    this->kind = Typed_AST_Kind::Match;
    this->cond = cond;
    this->default_arm = default_arm;
    this->arms = arms;
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
    Ref<Typed_AST> initializer)
{
    kind = Typed_AST_Kind::Let;
    this->is_const = is_const;
    this->target = target;
    this->specified_type = specified_type;
    this->initializer = initializer;
}

bool Typed_AST_Let::is_constant(Compiler &c) {
    return initializer->is_constant(c);
}

Typed_AST_Field_Access::Typed_AST_Field_Access(
    Value_Type type,
    bool deref,
    Ref<Typed_AST> instance,
    Size field_offset)
{
    this->kind = Typed_AST_Kind::Field_Access;
    this->type = type;
    this->deref = deref;
    this->instance = instance;
    this->field_offset = field_offset;
}

bool Typed_AST_Field_Access::is_constant(Compiler &c) {
    return instance->is_constant(c);
}

Typed_AST_Fn_Declaration::Typed_AST_Fn_Declaration(
    Function_Definition *defn,
    Ref<Typed_AST_Multiary> body)
{
    this->kind = Typed_AST_Kind::Fn_Decl;
    this->defn = defn;
    this->body = body;
}

bool Typed_AST_Fn_Declaration::is_constant(Compiler &c) {
    return true;
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

static void print_at_indent(Interpreter *interp, const Ref<Typed_AST> node, size_t indent) {
    switch (node->kind) {
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
        case Typed_AST_Kind::Ident_Func: {
            auto uuid = node.cast<Typed_AST_UUID>();
            auto defn = interp->functions.get_func_by_uuid(uuid->uuid);
            printf("%.*s#%zu :: %s\n", defn->name.size(), defn->name.c_str(), uuid->uuid, defn->type.debug_str());
        } break;
        case Typed_AST_Kind::Int: {
            Ref<Typed_AST_Int> lit = node.cast<Typed_AST_Int>();
            printf("%lld\n", lit->value);
        } break;
        case Typed_AST_Kind::Str: {
            Ref<Typed_AST_Str> lit = node.cast<Typed_AST_Str>();
            printf("\"%.*s\"\n", lit->value.size(), lit->value.c_str());
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
        case Typed_AST_Kind::Return: {
            auto ret = node.cast<Typed_AST_Return>();
            printf("(ret)\n");
            if (ret->sub) {
                print_sub_at_indent(interp, "sub", ret->sub, indent + 1);
            } else {
                printf("%*ssub: nullptr\n", (indent + 1) * INDENT_SIZE, "");
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
        case Typed_AST_Kind::While: {
            print_binary_at_indent(interp, "while", node.cast<Typed_AST_Binary>(), indent);
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
        case Typed_AST_Kind::Function_Call: {
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
            
        default:
            internal_error("Invalid Typed_AST_Kind value: %d\n", node->kind);
            break;
    }
}

void Typed_AST::print(Interpreter *interp) const {
    print_at_indent(interp, Ref<Typed_AST>((Typed_AST *)this), 0);
}

struct Typer_Binding {
    enum {
        Variable,
        Type,
        Function
    } kind;
    Value_Type value_type;
    
    static Typer_Binding variable(Value_Type value_type) {
        return Typer_Binding { Variable, value_type };
    }
    
    static Typer_Binding type(Value_Type type) {
        return Typer_Binding { Type, type };
    }
    
    static Typer_Binding function(Value_Type fn_type) {
        return Typer_Binding { Function, fn_type };
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
    
    Typer(Interpreter *interp, String module_path) {
        this->interp = interp;
        this->module = interp->create_module(module_path);
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
    
    void put_binding(const std::string &id, Typer_Binding binding) {
        auto &cs = current_scope();
        
        auto it = cs.bindings.find(id);
        verify(it == cs.bindings.end() || it->second.kind == Typer_Binding::Variable, "Cannot shadow a type name or name of a function. Reused identifier was '%s'.", id.c_str());
        
        cs.bindings[id] = binding;
    }
    
    void bind_variable(const std::string &id, Value_Type type, bool is_mut) {
        type.is_mut = is_mut;
        return put_binding(id, Typer_Binding::variable(type));
    }
    
    void bind_type(const std::string &id, Value_Type type) {
        internal_verify(type.kind == Value_Type_Kind::Type, "Attempted to bind a type name to something other than a type.");
        return put_binding(id, Typer_Binding::type(type));
    }
    
    void bind_function(const std::string &id, Value_Type fn_type) {
        internal_verify(fn_type.kind == Value_Type_Kind::Function, "Attempted to bind a function name to a non-function Value_Type.");
        return put_binding(id, Typer_Binding::function(fn_type));
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
                bind_variable(ip->id.c_str(), type, ip->is_mut);
            } break;
            case Untyped_AST_Kind::Pattern_Tuple: {
                auto tp = pattern.cast<Untyped_AST_Pattern_Tuple>();
                verify(type.kind == Value_Type_Kind::Tuple &&
                       tp->sub_patterns.size() == type.data.tuple.child_types.size(),
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
                       type.data.struct_.defn->uuid == uuid->uuid, "Cannot match %s struct pattern with %s.", sp->struct_id->display_str(), type.display_str());
                
                auto defn = type.data.struct_.defn;
                verify(defn->fields.size() == sp->sub_patterns.size(), "Incorrect number of sub patterns in struct pattern for struct %s. Expected %zu but was given %zu.", type.display_str(), defn->fields.size(), sp->sub_patterns.size());
                
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
                bind_variable(ip->id.c_str(), id_type, ip->is_mut);
            } break;
            case Untyped_AST_Kind::Pattern_Tuple: {
                auto tp = pattern.cast<Untyped_AST_Pattern_Tuple>();
                verify(type.kind == Value_Type_Kind::Tuple &&
                       tp->sub_patterns.size() == type.data.tuple.child_types.size(),
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
                       type.data.struct_.defn->uuid == uuid->uuid, "Cannot match %s struct pattern with %s.", sp->struct_id->display_str(), type.display_str());
                
                auto defn = type.data.struct_.defn;
                verify(defn->fields.size() == sp->sub_patterns.size(), "Incorrect number of sub patterns in struct pattern for struct %s. Expected %zu but was given %zu.", type.display_str(), defn->fields.size(), sp->sub_patterns.size());
                
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
                verify(lit, "Failed to cast to Enum_Literal* in Typer::bind_match_pattern().");
                
                auto defn = lit->type.data.enum_.defn;
                auto &variant = defn->variants[lit->tag];
                
                verify(variant.payload.size() == ep->sub_patterns.size(), "Incorrect number of sub patterns in enum pattern for enum %s. Expected %zu but was given %zu.", type.display_str(), variant.payload.size(), ep->sub_patterns.size());
                
                auto tag = Mem.make<Typed_AST_Int>(lit->tag);
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
                
                verify(value->type.eq_ignoring_mutability(type), "Type mismatch in pattern. Expected '%s' but was given '%s'.", type.display_str(), value->type.display_str());
                
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
                String id = type.data.unresolved.id;
                Typer_Binding binding;
                verify(find_binding_by_id(id.c_str(), binding), "Unresolved identifier '%.*s'.", id.size(), id.c_str());
                verify(binding.kind == Typer_Binding::Type, "Expected identifier of a type but instead found an identnfier to '%s'.", binding.value_type.display_str());
                resolved = *binding.value_type.data.type.type;
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
    Ref<Untyped_AST_Multiary> node)
{
    auto t = Typer { &interp, "<@TODO MODULE PATH>" };
    
    auto typechecked = Mem.make<Typed_AST_Multiary>(to_typed(node->kind));
    for (auto &n : node->nodes) {
        if (auto tn = n->typecheck(t))
            typechecked->add(tn);
    }
    
    return typechecked;
}

Ref<Typed_AST> Untyped_AST_Bool::typecheck(Typer &t) {
    return Mem.make<Typed_AST_Bool>(value);
}

Ref<Typed_AST> Untyped_AST_Char::typecheck(Typer &t) {
    return Mem.make<Typed_AST_Char>(value);
}

Ref<Typed_AST> Untyped_AST_Float::typecheck(Typer &t) {
    return Mem.make<Typed_AST_Float>(value);
}

Ref<Typed_AST> Untyped_AST_Ident::typecheck(Typer &t) {
    Typer_Binding binding;
    verify(t.find_binding_by_id(id.c_str(), binding), "Unresolved identifier '%s'.", id.c_str());
    
    Ref<Typed_AST> ident;
    switch (binding.kind) {
        case Typer_Binding::Type: {
            switch (binding.value_type.data.type.type->kind) {
                case Value_Type_Kind::Struct:
                    ident = Mem.make<Typed_AST_UUID>(Typed_AST_Kind::Ident_Struct, binding.value_type.data.type.type->data.struct_.defn->uuid, binding.value_type);
                    break;
                case Value_Type_Kind::Enum:
                    ident = Mem.make<Typed_AST_UUID>(Typed_AST_Kind::Ident_Enum, binding.value_type.data.type.type->data.enum_.defn->uuid, binding.value_type);
                    break;
                
                default:
                    internal_error("Invalid Value_Type_Kind for Type type.");
                    break;
            }
        } break;
        case Typer_Binding::Function: {
            ident = Mem.make<Typed_AST_UUID>(Typed_AST_Kind::Ident_Func, binding.value_type.data.func.uuid, binding.value_type);
        } break;
            
        default:
            ident = Mem.make<Typed_AST_Ident>(id.clone(), binding.value_type);
            break;
    }
    
    return ident;
}

static Ref<Typed_AST> typecheck_ident_in_struct_namespace(
    Typer &t,
    Struct_Definition *defn,
    Ref<Untyped_AST_Ident> id)
{
    //
    // @TODO:
    //      Handle things that aren't methods.
    //
    
    Method method;
    verify(defn->find_method(id->id, method), "Struct type '%s' does not have a method called '%s'.", defn->name.c_str(), id->id.c_str());
    
    auto method_defn = t.interp->functions.get_func_by_uuid(method.uuid);
    internal_verify(method_defn, "Failed to retrieve method '%s' from funcbook with id #%zu.", id->id.c_str(), method.uuid);
    
    return Mem.make<Typed_AST_UUID>(
        Typed_AST_Kind::Ident_Func,
        method.uuid,
        method_defn->type
    );
}

static Ref<Typed_AST> typecheck_ident_in_enum_namespace(
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
            nullptr
        );
    } else {
        Method method;
        verify(defn->find_method(variant_id, method), "'%s' does not exist within the '%s' enum type's namespace.", variant_id.c_str(), defn->name.c_str());
        
        auto method_defn = t.interp->functions.get_func_by_uuid(method.uuid);
        internal_verify(method_defn, "Failed to retrieve method defn from funcbook.");
        
        typechecked = Mem.make<Typed_AST_UUID>(
            Typed_AST_Kind::Ident_Func,
            method.uuid,
            method_defn->type
        );
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
            auto id = rhs.cast<Untyped_AST_Ident>();
            typechecked = typecheck_ident_in_struct_namespace(t, defn, id);
        } break;
        case Typed_AST_Kind::Ident_Enum: {
            auto uuid = namespace_.cast<Typed_AST_UUID>();
            auto defn = t.interp->types.get_enum_by_uuid(uuid->uuid);
            auto id = rhs.cast<Untyped_AST_Ident>();
            typechecked = typecheck_ident_in_enum_namespace(t, defn, id);
        } break;
            
        default:
            internal_error("Invalid Typed_AST_Kind in typecheck_path().");
            break;
    }
    
    return typechecked;
}

Ref<Typed_AST> Untyped_AST_Int::typecheck(Typer &t) {
    return Mem.make<Typed_AST_Int>(value);
}

Ref<Typed_AST> Untyped_AST_Str::typecheck(Typer &t) {
    return Mem.make<Typed_AST_Str>(value.clone());
}

Ref<Typed_AST> Untyped_AST_Nullary::typecheck(Typer &t) {
    switch (kind) {
        case Untyped_AST_Kind::Noinit:
            error("'noinit' only allowed as initializer expression of variable declaration.");
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
                   "(-) requires operand to be an 'int' or a 'float' but was given '%s'.", sub->type.display_str());
            return Mem.make<Typed_AST_Unary>(Typed_AST_Kind::Negation, sub->type, sub);
        case Untyped_AST_Kind::Not:
            verify(sub->type.kind == Value_Type_Kind::Bool, "(!) requires operand to be a 'bool' but got a '%s'.", sub->type.display_str());
            return Mem.make<Typed_AST_Unary>(Typed_AST_Kind::Not, value_types::Bool, sub);
        case Untyped_AST_Kind::Address_Of: {
            verify(sub->type.kind != Value_Type_Kind::None, "Cannot take a pointer to something that doesn't return a value.");
            auto pty = value_types::ptr_to(&sub->type);
            pty.data.ptr.child_type->is_mut = false;
            return Mem.make<Typed_AST_Unary>(Typed_AST_Kind::Address_Of, pty, sub);
        }
        case Untyped_AST_Kind::Address_Of_Mut: {
            verify(sub->type.kind != Value_Type_Kind::None, "Cannot take a pointer to something that doesn't return a value.");
            verify(sub->type.is_mut, "Cannot take a mutable pointer to something that isn't itself mutable.");
            auto pty = value_types::ptr_to(&sub->type);
            return Mem.make<Typed_AST_Unary>(Typed_AST_Kind::Address_Of_Mut, pty, sub);
        }
        case Untyped_AST_Kind::Deref:
            verify(sub->type.kind == Value_Type_Kind::Ptr, "Cannot dereference something of type '%s' because it is not a pointer type.", sub->type.display_str());
            return Mem.make<Typed_AST_Unary>(Typed_AST_Kind::Deref, *sub->type.data.ptr.child_type, sub);
            
        default:
            internal_error("Invalid Unary Untyped_AST_Kind value: %d\n", kind);
            return nullptr;
    }
}

Ref<Typed_AST> Untyped_AST_Return::typecheck(Typer &t) {
    verify(t.function, "Return statement outside of function.");

    Ref<Typed_AST> sub = nullptr;
    if (t.function->type.kind == Value_Type_Kind::Void) {
        verify(!this->sub, "Return statement does not match function definition. Expected '%s' but was given '%s'.", t.function->type.data.func.return_type->display_str(), this->sub->typecheck(t)->type.size());
    } else {
        sub = this->sub->typecheck(t);
        verify(t.function->type.data.func.return_type->assignable_from(sub->type), "Return statement does not match function definition. Expected '%s' but was given '%s'.", t.function->type.data.func.return_type->display_str(), sub->type.display_str());
    }
    
    t.has_return = true;
    
    return Mem.make<Typed_AST_Return>(sub);
}

static void typecheck_function_call_arguments(
    Typer &t,
    Function_Definition *defn,
    Ref<Typed_AST_Multiary> args,
    Ref<Untyped_AST_Multiary> rhs,
    bool skip_receiver = false)
{
    args->nodes.reserve(defn->type.data.func.arg_types.size());
    for (size_t i = args->nodes.size(); i < defn->type.data.func.arg_types.size(); i++) {
        args->nodes.push_back(nullptr);
    }
    
    bool began_named_args = false;
    size_t num_positional_args = skip_receiver ? 1 : 0;
    for (auto arg_node : rhs->nodes) {
        Ref<Untyped_AST> arg_expr;
        size_t arg_pos;
        if (arg_node->kind == Untyped_AST_Kind::Binding) {
            began_named_args = true;
            
            auto arg_bin = arg_node.cast<Untyped_AST_Binary>();
            String arg_id = arg_bin->lhs.cast<Untyped_AST_Ident>()->id;
            
            arg_pos = -1;
            for (size_t i = 0; i < defn->param_names.size(); i++) {
                if (arg_id == defn->param_names[i]) {
                    arg_pos = i;
                    break;
                }
            }
            verify(arg_pos != -1, "Unknown parameter '%.*s'.", arg_id.size(), arg_id.c_str());
            
            arg_expr = arg_bin->rhs;
        } else if (began_named_args) {
            error("Cannot have positional argruments after named arguments in function call.");
        } else {
            arg_expr = arg_node;
            arg_pos = num_positional_args++;
        }
        
        auto typecheck_arg_expr = arg_expr->typecheck(t);
        verify(defn->type.data.func.arg_types[arg_pos]
                   .assignable_from(typecheck_arg_expr->type),
               "Argument type mismatch. Expected '%s' but was given '%s'.", defn->type.data.func.arg_types[arg_pos].display_str(), typecheck_arg_expr->type.display_str());
        
        auto &arg = args->nodes[arg_pos];
        verify(!arg, "Argument '%.*s' given more than once.", defn->param_names[arg_pos].size(), defn->param_names[arg_pos].c_str());
        arg = typecheck_arg_expr;
    }
}

static Ref<Typed_AST_Binary> typecheck_function_call(
    Typer &t,
    Ref<Typed_AST> func,
    Ref<Untyped_AST_Multiary> rhs)
{
    // @TODO: Check for function pointer type of stuff
    verify(func->kind == Typed_AST_Kind::Ident_Func, "First operand of function call must be a function.");
    
    auto func_uuid = func.cast<Typed_AST_UUID>();
    auto defn = t.interp->functions.get_func_by_uuid(func_uuid->uuid);
    internal_verify(defn, "Failed to retrieve function with id #%zu.", func_uuid->uuid);
    
    // @TODO: default arguments stuff (if we do default arguments)
    
    verify(rhs->nodes.size() == defn->type.data.func.arg_types.size(), "Incorrect number of arguments for invocation. Expected %zu but was given %zu.", defn->type.data.func.arg_types.size(), rhs->nodes.size());
    
    auto args = Mem.make<Typed_AST_Multiary>(Typed_AST_Kind::Comma);
    
    typecheck_function_call_arguments(t, defn, args, rhs);
    
    return Mem.make<Typed_AST_Binary>(
        Typed_AST_Kind::Function_Call,
        *defn->type.data.func.return_type,
        func,
        args
    );
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

static Ref<Typed_AST> typecheck_invocation(
    Typer &t,
    Untyped_AST_Binary &call)
{
    auto lhs = call.lhs->typecheck(t);
    auto rhs = call.rhs.cast<Untyped_AST_Multiary>();
    internal_verify(rhs, "Failed to cast rhs to Multiary* in typecheck_invocation().");
    
    Ref<Typed_AST> typechecked;
    switch (lhs->type.kind) {
        case Value_Type_Kind::Function:
            typechecked = typecheck_function_call(t, lhs, rhs);
            break;
        case Value_Type_Kind::Enum:
            typechecked = typecheck_enum_literal_with_payload(t, lhs, rhs);
            break;
            
        default:
            error("Type '%s' isn't invocable.", lhs->type.display_str());
            break;
    }
    
    return typechecked;
}

Ref<Typed_AST> Untyped_AST_Binary::typecheck(Typer &t) {
    switch (kind) {
        case Untyped_AST_Kind::Invocation:
            return typecheck_invocation(t, *this);
            
        default:
            break;
    }
    
    auto lhs = this->lhs->typecheck(t);
    auto rhs = this->rhs->typecheck(t);
    switch (kind) {
        case Untyped_AST_Kind::Addition:
            verify(lhs->type.kind == rhs->type.kind, "(+) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int ||
                   lhs->type.kind == Value_Type_Kind::Float, "(+) requires operands to be either 'int' or 'float' but was given '%s'.", lhs->type.display_str());
            verify(rhs->type.kind == Value_Type_Kind::Int ||
                   rhs->type.kind == Value_Type_Kind::Float, "(+) requires operands to be either 'int' or 'float' but was given '%s'.", rhs->type.display_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Addition, lhs->type, lhs, rhs);
        case Untyped_AST_Kind::Subtraction:
            verify(lhs->type.kind == rhs->type.kind, "(-) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int ||
                   lhs->type.kind == Value_Type_Kind::Float, "(-) requires operands to be either 'int' or 'float' but was given '%s'.", lhs->type.display_str());
            verify(rhs->type.kind == Value_Type_Kind::Int ||
                   rhs->type.kind == Value_Type_Kind::Float, "(-) requires operands to be either 'int' or 'float' but was given '%s'.", rhs->type.display_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Subtraction, lhs->type, lhs, rhs);
        case Untyped_AST_Kind::Multiplication:
            verify(lhs->type.kind == rhs->type.kind, "(*) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int ||
                   lhs->type.kind == Value_Type_Kind::Float, "(*) requires operands to be either 'int' or 'float' but was given '%s'.", lhs->type.display_str());
            verify(rhs->type.kind == Value_Type_Kind::Int ||
                   rhs->type.kind == Value_Type_Kind::Float, "(*) requires operands to be either 'int' or 'float' but was given '%s'.", rhs->type.display_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Multiplication, lhs->type, lhs, rhs);
        case Untyped_AST_Kind::Division:
            verify(lhs->type.kind == rhs->type.kind, "(/) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int ||
                   lhs->type.kind == Value_Type_Kind::Float, "(/) requires operands to be either 'int' or 'float' but was given '%s'.", lhs->type.display_str());
            verify(rhs->type.kind == Value_Type_Kind::Int ||
                   rhs->type.kind == Value_Type_Kind::Float, "(/) requires operands to be either 'int' or 'float' but was given '%s'.", rhs->type.display_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Division, lhs->type, lhs, rhs);
        case Untyped_AST_Kind::Mod:
            verify(lhs->type.kind == rhs->type.kind, "(+) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int, "(+) requires operands to be 'int' but was given '%s'.", lhs->type.display_str());
            verify(rhs->type.kind == Value_Type_Kind::Int, "(+) requires operands to be 'int' but was given '%s'.", rhs->type.display_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Mod, value_types::Int, lhs, rhs);
        case Untyped_AST_Kind::Assignment:
            verify(lhs->type.is_mut, "Cannot assign to something of type '%s' because it is immutable.", lhs->type.display_str());
            verify(lhs->type.assignable_from(rhs->type), "(=) requires both operands to be the same type.");
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Assignment, value_types::None, lhs, rhs);
        case Untyped_AST_Kind::Equal:
            verify(lhs->type.eq_ignoring_mutability(rhs->type), "(==) requires both operands to be the same type.");
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Equal, value_types::Bool, lhs, rhs);
        case Untyped_AST_Kind::Not_Equal:
            verify(lhs->type.eq_ignoring_mutability(rhs->type), "(!=) requires both operands to be the same type.");
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Not_Equal, value_types::Bool, lhs, rhs);
        case Untyped_AST_Kind::Less:
            verify(lhs->type.eq_ignoring_mutability(rhs->type), "(<) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int ||
                   lhs->type.kind == Value_Type_Kind::Float,
                   "(<) requires operands to be 'int' or 'float' but was given '%s'.", lhs->type.display_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Less, value_types::Bool, lhs, rhs);
        case Untyped_AST_Kind::Greater:
            verify(lhs->type.eq_ignoring_mutability(rhs->type), "(>) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int ||
                   lhs->type.kind == Value_Type_Kind::Float,
                   "(>) requires operands to be 'int' or 'float' but was given '%s'.", lhs->type.display_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Greater, value_types::Bool, lhs, rhs);
        case Untyped_AST_Kind::Less_Eq:
            verify(lhs->type.eq_ignoring_mutability(rhs->type), "(<=) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int ||
                   lhs->type.kind == Value_Type_Kind::Float,
                   "(<=) requires operands to be 'int' or 'float' but was given '%s'.", lhs->type.display_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Less_Eq, value_types::Bool, lhs, rhs);
        case Untyped_AST_Kind::Greater_Eq:
            verify(lhs->type.eq_ignoring_mutability(rhs->type), "(>=) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int ||
                   lhs->type.kind == Value_Type_Kind::Float,
                   "(>=) requires operands to be 'int' or 'float' but was given '%s'.", lhs->type.display_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Greater_Eq, value_types::Bool, lhs, rhs);
        case Untyped_AST_Kind::And:
            verify(lhs->type.kind == Value_Type_Kind::Bool, "(and) requires first operand to be 'bool' but was given '%s'.", lhs->type.display_str());
            verify(rhs->type.kind == Value_Type_Kind::Bool, "(and) requires second operand to be 'bool' but was given '%s'.", rhs->type.display_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::And, value_types::Bool, lhs, rhs);
        case Untyped_AST_Kind::Or:
            verify(lhs->type.kind == Value_Type_Kind::Bool, "(or) requires first operand to be 'bool' but was given '%s'.", lhs->type.display_str());
            verify(rhs->type.kind == Value_Type_Kind::Bool, "(or) requires second operand to be 'bool' but was given '%s'.", rhs->type.display_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Or, value_types::Bool, lhs, rhs);
        case Untyped_AST_Kind::Field_Access_Tuple: {
            bool needs_deref = lhs->type.kind == Value_Type_Kind::Ptr;
            Value_Type &ty = needs_deref ? *lhs->type.child_type() : lhs->type;
            verify(ty.kind == Value_Type_Kind::Tuple, "(.) requires first operand to be a tuple but was given '%s'.", lhs->type.display_str());
            auto i = rhs.cast<Typed_AST_Int>();
            internal_verify(i, "Dot_Tuple got a rhs that wasn't an int.");
            verify(i->value < ty.data.tuple.child_types.size(), "Cannot access type %lld from a %s.", i->value, lhs->type.display_str());
            Value_Type child_ty = ty.data.tuple.child_types[i->value];
            child_ty.is_mut = ty.is_mut;
            return Mem.make<Typed_AST_Field_Access>(
                child_ty,
                needs_deref,
                lhs,
                lhs->type.data.tuple.offset_of_type(i->value)
            );
        } break;
        case Untyped_AST_Kind::Subscript: {
            verify(lhs->type.kind == Value_Type_Kind::Array ||
                   lhs->type.kind == Value_Type_Kind::Slice,
                   "([]) requires first operand to be an array or slice but was given '%s'.", lhs->type.display_str());
            verify(rhs->type.kind == Value_Type_Kind::Int, "([]) requires second operand to be 'int' but was given '%s'.", rhs->type.display_str());
            
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
            
            return Mem.make<Typed_AST_Binary>(kind, *lhs->type.child_type(), lhs, rhs);
        }
        case Untyped_AST_Kind::Range:
            verify(lhs->type.eq_ignoring_mutability(rhs->type), "(..) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int, "(..) requires operands to be of type 'int' but was given '%s'.", lhs->type.display_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Range, value_types::range_of(false, &lhs->type), lhs, rhs);
        case Untyped_AST_Kind::Inclusive_Range:
            verify(lhs->type.eq_ignoring_mutability(rhs->type), "(...) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int, "(...) requires operands to be of type 'int' but was given '%s'.", lhs->type.display_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Inclusive_Range, value_types::range_of(true, &lhs->type), lhs, rhs);
        case Untyped_AST_Kind::While:
            verify(lhs->type.kind == Value_Type_Kind::Bool, "(while) requires condition to be 'bool' but was given '%s'.", lhs->type.display_str());
            t.has_return = false;
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::While, value_types::None, lhs, rhs);
            
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
    auto multi = Mem.make<Typed_AST_Multiary>(to_typed(kind));
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
        verify(element_nodes->nodes.size() > 0, "Cannot infer element type of empty array literal.");
        bool is_mut = element_type->is_mut;
        *element_type = element_nodes->nodes[0]->type;
        element_type->is_mut = is_mut;
    }
    for (size_t i = 0; i < element_nodes->nodes.size(); i++) {
        verify(element_nodes->nodes[i]->type.eq_ignoring_mutability(*element_type), "Element %zu in array literal does not match the expected type '%s'.", i+1, element_type->display_str());
    }
    return Mem.make<Typed_AST_Array>(*array_type, to_typed(kind), count, array_type, element_nodes);
}

Ref<Typed_AST> Untyped_AST_Struct_Literal::typecheck(Typer &t) {
    auto struct_uuid = this->struct_id->typecheck(t).cast<Typed_AST_UUID>();
    internal_verify(struct_uuid, "Failed to cast struct_id to UUID* in Struct_Literal::typecheck().");
    
    auto defn = t.interp->types.get_struct_by_uuid(struct_uuid->uuid);
    internal_verify(defn, "Failed to retrieve struct-defn #%zu from typebook.", struct_uuid->uuid);
    auto bindings = this->bindings;
    
    verify(defn->fields.size() == bindings->nodes.size(), "Incorrect number of arguments in struct literal. Expected %zu but was given %zu.", defn->fields.size(), bindings->nodes.size());
    
    auto args = Mem.make<Typed_AST_Multiary>(Typed_AST_Kind::Comma);
    args->type = *struct_uuid->type.data.type.type;
    
    for (size_t i = 0; i < defn->fields.size(); i++) {
        auto &field = defn->fields[i];
        auto binding = bindings->nodes[i];
        
        Ref<Typed_AST> arg = nullptr;
        switch (binding->kind) {
            case Untyped_AST_Kind::Ident: {
                auto bid = binding.cast<Untyped_AST_Ident>();
                verify(field.id == bid->id, "Given identifier doesn't match name of field. Expected '%.*s' but was given '%.*s'. Please specify field.", field.id.size(), field.id.c_str(), bid->id.size(), bid->id.c_str());
                arg = bid->typecheck(t);
                verify(field.type.assignable_from(arg->type), "Cannot assign to field '%.*s' because of mismatched types. Expected '%s' but was given '%s'.", field.id.size(), field.id.c_str(), field.type.display_str(), arg->type.display_str());
            } break;
            case Untyped_AST_Kind::Binding: {
                auto b = binding.cast<Untyped_AST_Binary>();
                auto bid = b->lhs.cast<Untyped_AST_Ident>();
                verify(field.id == bid->id, "Given identifier doesn't match name of field. Expected '%.*s' but was given '%.*s'.", field.id.size(), field.id.c_str(), bid->id.size(), bid->id.c_str());
                arg = b->rhs->typecheck(t);
                verify(field.type.assignable_from(arg->type), "Cannot assign to field '%.*s' because of mismatched types. Expected '%s' but was given '%s'.", field.id.size(), field.id.c_str(), field.type.display_str(), arg->type.display_str());
            } break;
                
            default:
                error("Expected either an identifier expression or binding in struct literal.");
                break;
        }
        
        args->add(arg);
    }
    
    return args;
}

Ref<Typed_AST> Untyped_AST_Field_Access::typecheck(Typer &t) {
    auto instance = this->instance->typecheck(t);
    
    bool needs_deref = instance->type.kind == Value_Type_Kind::Ptr;
    Value_Type &ty = needs_deref ? *instance->type.child_type() : instance->type;
    verify(ty.kind == Value_Type_Kind::Struct, "(.) requires first operand to be a struct type but was given '%s'.", instance->type.display_str());
    
    Struct_Field *field = ty.data.struct_.defn->find_field(field_id);
    verify(field, "'%.*s' is not a field of '%.*s'.", field_id.size(), field_id.c_str(), ty.data.struct_.defn->name.size(), ty.data.struct_.defn->name.c_str());
    
    Value_Type field_ty = field->type;
    field_ty.is_mut |= ty.is_mut; // |= because fields can be 'forced mut'
    
    Size field_offset = field->offset;
    
    return Mem.make<Typed_AST_Field_Access>(field_ty, needs_deref, instance, field_offset);
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
    verify(!else_ || then->type.eq(else_->type), "Both branches of (if) must be the same. '%s' vs '%s'.", then->type.display_str(), else_->type.display_str());
    return Mem.make<Typed_AST_If>(then->type, cond, then, else_);
}

Ref<Typed_AST> Untyped_AST_Type_Signature::typecheck(Typer &t) {
    auto resolved_type = Mem.make<Value_Type>(t.resolve_value_type(*value_type));
    return Mem.make<Typed_AST_Type_Signature>(resolved_type);
}

Ref<Typed_AST> Untyped_AST_For::typecheck(Typer &t) {
    auto iterable = this->iterable->typecheck(t);
    switch (iterable->type.kind) {
        case Value_Type_Kind::Array:
        case Value_Type_Kind::Slice:
//        case Value_Type_Kind::Str:
        case Value_Type_Kind::Range:
            break;
            
        default:
            error("Cannot iterate over something of type '%s'.", iterable->type.display_str());
            break;
    }
    
    Value_Type *target_type = iterable->type.child_type();
    
    t.begin_scope();
    
    auto processed_target = Mem.make<Typed_AST_Processed_Pattern>();
    t.bind_pattern(target, *target_type, processed_target);
    
    if (counter != "") {
        t.bind_variable(counter.c_str(), value_types::Int, false);
    }
    
    auto body = this->body->typecheck(t);
    
    t.end_scope();
    
    t.has_return = false;
    
    return Mem.make<Typed_AST_For>(
        iterable->type.kind == Value_Type_Kind::Range ?
            Typed_AST_Kind::For_Range :
            Typed_AST_Kind::For,
        processed_target,
        counter.clone(),
        iterable,
        body.cast<Typed_AST_Multiary>()
    );
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
    
    auto arms = Mem.make<Typed_AST_Multiary>(Typed_AST_Kind::Comma);
    for (auto arm : this->arms->nodes) {
        internal_verify(arm->kind == Untyped_AST_Kind::Match_Arm, "Arm node in match node is not an Untyped_AST_Kind::Match_Arm.");
        auto arm_bin = arm.cast<Untyped_AST_Binary>();
        
        t.begin_scope();
        
        auto pat = arm_bin->lhs.cast<Untyped_AST_Pattern>();
        auto match_pat = Mem.make<Typed_AST_Match_Pattern>();
        t.bind_match_pattern(pat, cond->type, match_pat, 0);
        
        auto body = arm_bin->rhs->typecheck(t);
        
        t.end_scope();
        
        auto typechecked_arm = Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Match_Arm, value_types::None, match_pat, body);
        arms->add(typechecked_arm);
        
        has_return &= t.has_return;
        t.has_return = false;
    }
    
    t.has_return = has_return && default_arm;
    
    return Mem.make<Typed_AST_Match>(
        cond,
        default_arm,
        arms
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
            init = Mem.make<Typed_AST_Nullary>(Typed_AST_Kind::Allocate, ty);
        } else {
            init = initializer->typecheck(t);
            if (sig) {
                verify(specified_type->value_type->assignable_from(init->type), "Specified type '%s' does not match given type '%s'.", sig->value_type->display_str(), init->type.display_str());
            } else {
                ty = init->type;
            }
        }
    }
    
    if (is_const) {
        verify(!(ty.is_mut || ty.is_partially_mutable()), "Constants must be completely immutable.");
    }

    auto processed_target = Mem.make<Typed_AST_Processed_Pattern>();
    t.bind_pattern(target, ty, processed_target);
    
    return Mem.make<Typed_AST_Let>(is_const, processed_target, sig, init);
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
        verify(!defn.has_field(field.id), "Redefinition of field '%.*s'.", field.id.size(), field.id.c_str());
        field.offset = current_offset;
        field.type = t.resolve_value_type(*f.type->value_type);
        defn.fields.push_back(field);
        current_offset += field.type.size();
    }
    
    defn.size = current_offset;
    auto new_defn = t.interp->types.add_struct(defn);
    auto [_, success] = t.module->structs.insert(new_defn->uuid);
    internal_verify(success, "Failed to add struct with id #%zu to module set.", new_defn->uuid);
    
    Value_Type *struct_type = Mem.make<Value_Type>().as_ptr();
    struct_type->kind = Value_Type_Kind::Struct;
    struct_type->data.struct_.defn = new_defn;
    
    Value_Type type;
    type.kind = Value_Type_Kind::Type;
    type.data.type.type = struct_type;
    
    String id = this->id.clone();
    t.bind_type(id.c_str(), type);
    
    return nullptr;
}

Ref<Typed_AST> Untyped_AST_Enum_Declaration::typecheck(Typer &t) {
    //
    // @TODO: Do Payloads
    //
    
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
    auto [_, success] = t.module->enums.insert(new_defn->uuid);
    internal_verify(success, "Failed to add enum with id #%zu to module set.", new_defn->uuid);
    
    Value_Type *enum_type = Mem.make<Value_Type>().as_ptr();
    enum_type->kind = Value_Type_Kind::Enum;
    enum_type->data.enum_.defn = new_defn;
    
    Value_Type type;
    type.kind = Value_Type_Kind::Type;
    type.data.type.type = enum_type;
    
    String id = this->id.clone();
    t.bind_type(id.c_str(), type);
    
    return nullptr;
}

struct Typecheck_Fn_Decl_Result {
    Function_Definition *defn;
    Ref<Typed_AST_Fn_Declaration> typed_decl;
};

static Typecheck_Fn_Decl_Result typecheck_fn_decl(
    Typer &t,
    Untyped_AST_Fn_Declaration &decl)
{
    Function_Definition defn;
    defn.uuid = t.interp->next_uuid();
    defn.module = t.module;
    defn.name = decl.id.clone();

    Value_Type func_type;
    func_type.kind = Value_Type_Kind::Function;
    func_type.data.func.uuid = defn.uuid;

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
        // @TODO: [ ]
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
                error("Expected a parameter.");
                break;
        }

        param_types[i] = param_type;
        defn.param_names.push_back(param_name);
    }

    func_type.data.func.arg_types = param_types;
    defn.type = func_type;

    auto new_defn = t.interp->functions.add_func(defn);
    auto [_, success] = t.module->funcs.insert(new_defn->uuid);
    internal_verify(success, "Failed to add function with id #%zu to module set.", new_defn->uuid);

    // actually typecheck the function
    auto new_t = Typer { t, new_defn };

    new_t.begin_scope();
    for (size_t i = 0; i < new_defn->param_names.size(); i++) {
        String param_name = new_defn->param_names[i];
        Value_Type param_type = new_defn->type.data.func.arg_types[i];
        new_t.bind_variable(param_name.c_str(), param_type, param_type.is_mut);
    }

    auto body = decl.body->typecheck(new_t).cast<Typed_AST_Multiary>();

    verify(new_defn->type.data.func.return_type->kind == Value_Type_Kind::Void ||
           new_t.has_return, "Not all paths return a value in non-void function '%.*s'.", new_defn->name.size(), new_defn->name.c_str());
    
    auto typed_decl = Mem.make<Typed_AST_Fn_Declaration>(new_defn, body);
    return { new_defn, typed_decl };
}

Ref<Typed_AST> Untyped_AST_Fn_Declaration::typecheck(Typer &t) {
    auto [defn, typed_decl] = typecheck_fn_decl(t, *this);
    t.bind_function(id.c_str(), defn->type);
    return typed_decl;
}

static Ref<Typed_AST_Multiary> typecheck_impl_declaration(
    Typer &t,
    const char *type_name,
    std::unordered_map<std::string, Method> &methods,
    Ref<Untyped_AST_Multiary> body)
{
    //
    // @HACK:
    //      Giving this Typed_AST_Kind::Comma might cause problems in the future.
    //      It was made this way to prevent redundant Flush instructions from being
    //      emitted.
    //
    auto typed_body = Mem.make<Typed_AST_Multiary>(Typed_AST_Kind::Comma);
    
    for (auto node : body->nodes) {
        switch (node->kind) {
            case Untyped_AST_Kind::Fn_Decl:
            case Untyped_AST_Kind::Method_Decl: {
                auto decl = node.cast<Untyped_AST_Fn_Declaration>();
                auto [fn_defn, fn_decl] = typecheck_fn_decl(t, *decl);
                verify(methods.find(fn_defn->name.c_str()) == methods.end(), "Cannot have two methods of the same name for one type. Reused name '%s'. Type '%s'.", fn_defn->name.c_str(), type_name);
                
                auto fn_sid = std::string { fn_defn->name.c_str(), fn_defn->name.size() };
                methods[fn_sid] = {
                    node->kind == Untyped_AST_Kind::Fn_Decl,
                    fn_defn->uuid
                };
                
                typed_body->add(fn_decl);
            } break;
                
            default:
                error("Impl declaration bodies can only canotain function declarations, for now.");
                break;
        }
    }
    
    return typed_body;
}

Ref<Typed_AST> Untyped_AST_Impl_Declaration::typecheck(Typer &t) {
    internal_verify(!for_, "trait impl decls not yet implemented.");
    
    auto target = this->target->typecheck(t).cast<Typed_AST_UUID>();
    internal_verify(target, "Failed to cast target to UUID* in Untyped_AST_Impl_Declaration::typecheck().");
    
    Ref<Typed_AST> typechecked = nullptr;
    
    switch (target->type.kind) {
        case Value_Type_Kind::Type:
            switch (target->type.data.type.type->kind) {
                case Value_Type_Kind::Struct: {
                    auto defn = t.interp->types.get_struct_by_uuid(target->uuid);
                    internal_verify(defn, "Failed to retrieve Struct_Definition from typebook.");
                    
                    t.begin_scope();
                    t.bind_type("Self", target->type);
                    typechecked = typecheck_impl_declaration(t, defn->name.c_str(), defn->methods, body);
                    t.end_scope();
                } break;
                case Value_Type_Kind::Enum: {
                    auto defn = t.interp->types.get_enum_by_uuid(target->uuid);
                    internal_verify(defn, "Failed to retrieve Enum_Definition from typebook.");
                    
                    t.begin_scope();
                    t.bind_type("Self", target->type);
                    typechecked = typecheck_impl_declaration(t, defn->name.c_str(), defn->methods, body);
                    t.end_scope();
                } break;
                    
                default:
                    internal_error("Invalid Value_Type_Kind: %d.", target->type.data.type.type->kind);
                    break;
            }
            break;
            
        default:
            error("Cannot implement something that isn't a type.");
            break;
    }
    
    return typechecked;
}

static Ref<Typed_AST> typecheck_dot_call_for_struct(
    Typer &t,
    Ref<Typed_AST> receiver,
    String method_id,
    Ref<Untyped_AST_Multiary> args)
{
    auto defn = receiver->type.data.struct_.defn;
    
    Method method;
    verify(defn->find_method(method_id, method), "Struct type '%s' does not have a method called '%s'.", defn->name.c_str(), method_id.c_str());
    verify(!method.is_static, "Cannot call '%s' with dot call since the method does not take a receiver.", method_id.c_str());
    
    auto method_defn = t.interp->functions.get_func_by_uuid(method.uuid);
    internal_verify(method_defn, "Failed to retreive method from funcbook.");
    
    Value_Type method_type = method_defn->type;
    
    auto method_uuid = Mem.make<Typed_AST_UUID>(Typed_AST_Kind::Ident_Func, method.uuid, method_type);
    
    //
    // @TODO:
    //       Handle default arguments (if we do them)
    //
    verify(args->nodes.size() == method_type.data.func.arg_types.size() - 1, "Incorrect number of arguments passed to '%s'. Expected %zu but was given %zu.", method_id.c_str(), method_type.data.func.arg_types.size() - 1, args->nodes.size());
    
    if (receiver->type.kind != Value_Type_Kind::Ptr) {
        Value_Type ptr_ty;
        ptr_ty.kind = Value_Type_Kind::Ptr;
        ptr_ty.data.ptr.child_type = &receiver->type;
        receiver = Mem.make<Typed_AST_Unary>(Typed_AST_Kind::Address_Of, ptr_ty, receiver);
    }
    
    verify(method_type.data.func.arg_types[0].assignable_from(receiver->type), "Cannot call this method because the receiver's type does not match the parameter's type. Expected '%s' but was given '%s'.", method_type.data.func.arg_types[0].display_str(), receiver->type.display_str());
    
    auto typechecked_args = Mem.make<Typed_AST_Multiary>(Typed_AST_Kind::Comma);
    typechecked_args->add(receiver);
    typecheck_function_call_arguments(t, method_defn, typechecked_args, args, /*skip_receiver*/ true);
    
    return Mem.make<Typed_AST_Binary>(
        Typed_AST_Kind::Function_Call,
        *method_type.data.func.return_type,
        method_uuid,
        typechecked_args
    );
}

static Ref<Typed_AST> typecheck_dot_call_for_enum(
    Typer &t,
    Ref<Typed_AST> receiver,
    String method_id,
    Ref<Untyped_AST_Multiary> args)
{
    auto defn = receiver->type.data.struct_.defn;
    
    Method method;
    verify(defn->find_method(method_id, method), "Enum type '%s' does not have a method called '%s'.", defn->name.c_str(), method_id.c_str());
    verify(!method.is_static, "Cannot call '%s' with dot call since the method does not take a receiver.", method_id.c_str());
    
    auto method_defn = t.interp->functions.get_func_by_uuid(method.uuid);
    internal_verify(method_defn, "Failed to retreive method from funcbook.");
    
    Value_Type method_type = method_defn->type;
    
    auto method_uuid = Mem.make<Typed_AST_UUID>(Typed_AST_Kind::Ident_Func, method.uuid, method_type);
    
    //
    // @TODO:
    //       Handle default arguments (if we do them)
    //
    verify(args->nodes.size() == method_type.data.func.arg_types.size() - 1, "Incorrect number of arguments passed to '%s'. Expected %zu but was given %zu.", method_id.c_str(), method_type.data.func.arg_types.size() - 1, args->nodes.size());
    
    if (receiver->type.kind != Value_Type_Kind::Ptr) {
        Value_Type ptr_ty;
        ptr_ty.kind = Value_Type_Kind::Ptr;
        ptr_ty.data.ptr.child_type = &receiver->type;
        receiver = Mem.make<Typed_AST_Unary>(Typed_AST_Kind::Address_Of, ptr_ty, receiver);
    }
    
    verify(method_type.data.func.arg_types[0].assignable_from(receiver->type), "Cannot call this method because the receiver's type does not match the parameter's type. Expected '%s' but was given '%s'.", method_type.data.func.arg_types[0].display_str(), receiver->type.display_str());
    
    auto typechecked_args = Mem.make<Typed_AST_Multiary>(Typed_AST_Kind::Comma);
    typechecked_args->add(receiver);
    typecheck_function_call_arguments(t, method_defn, typechecked_args, args, /*skip_receiver*/ true);
    
    return Mem.make<Typed_AST_Binary>(
        Typed_AST_Kind::Function_Call,
        *method_type.data.func.return_type,
        method_uuid,
        typechecked_args
    );
}

Ref<Typed_AST> Untyped_AST_Dot_Call::typecheck(Typer &t) {
    auto receiver = this->receiver->typecheck(t);
    
    Ref<Typed_AST> typechecked;
    switch (receiver->type.kind) {
        case Value_Type_Kind::Struct:
            typechecked = typecheck_dot_call_for_struct(t, receiver, method_id, args);
            break;
        case Value_Type_Kind::Enum:
            typechecked = typecheck_dot_call_for_enum(t, receiver, method_id, args);
            break;
            
        default:
            error("Cannot use dot calls with something that isn't a struct or enum type, for now.");
            break;
    }
    
    return typechecked;
}
