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

#include <list>
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

Typed_AST_UUID::Typed_AST_UUID(Typed_AST_Kind kind, UUID id, Value_Type type) {
    this->kind = kind;
    this->id = id;
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

Typed_AST_Unary::Typed_AST_Unary(Typed_AST_Kind kind, Value_Type type, Ref<Typed_AST> sub) {
    this->kind = kind;
    this->type = type;
    this->sub = sub;
}

bool Typed_AST_Unary::is_constant(Compiler &c) {
    return sub->is_constant(c);
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

Typed_AST_Type_Signiture::Typed_AST_Type_Signiture(Ref<Value_Type> value_type) {
    this->kind = Typed_AST_Kind::Type_Signiture;
    this->value_type = value_type;
}

bool Typed_AST_Type_Signiture::is_constant(Compiler &c) {
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

Typed_AST_Let::Typed_AST_Let(
    bool is_const,
    Ref<Typed_AST_Processed_Pattern> target,
    Ref<Typed_AST_Type_Signiture> specified_type,
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

Typed_AST_Field_Access_Tuple::Typed_AST_Field_Access_Tuple(
    Typed_AST_Kind kind,
    Value_Type type,
    bool deref,
    Ref<Typed_AST> lhs,
    Ref<Typed_AST> rhs)
        : Typed_AST_Binary(kind, type, lhs, rhs)
{
    this->deref = deref;
}

bool Typed_AST_Field_Access_Tuple::is_constant(Compiler &c) {
    return lhs->is_constant(c);
}

//Typed_AST_Struct_Declaration::Typed_AST_Struct_Declaration(
//    String id,
//    Ref<Struct_Definition> defn)
//{
//    this->kind = Typed_AST_Kind::Struct;
//    this->id = id;
//    this->defn = defn;
//}
//
//bool Typed_AST_Struct_Declaration::is_constant(Compiler &c) {
//    return true;
//}

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
        case Untyped_AST_Kind::Field_Access:             return Typed_AST_Kind::Field_Access;
        case Untyped_AST_Kind::Field_Access_Tuple:       return Typed_AST_Kind::Field_Access_Tuple;
        case Untyped_AST_Kind::Subscript:       return Typed_AST_Kind::Subscript;
        case Untyped_AST_Kind::Block:           return Typed_AST_Kind::Block;
        case Untyped_AST_Kind::Comma:           return Typed_AST_Kind::Comma;
        case Untyped_AST_Kind::Tuple:           return Typed_AST_Kind::Tuple;
        case Untyped_AST_Kind::If:              return Typed_AST_Kind::If;
        case Untyped_AST_Kind::For:             return Typed_AST_Kind::For;
        case Untyped_AST_Kind::Let:             return Typed_AST_Kind::Let;
        case Untyped_AST_Kind::Type_Signiture:  return Typed_AST_Kind::Type_Signiture;
//        case Untyped_AST_Kind::Struct:          return Typed_AST_Kind::Struct;
            
        case Untyped_AST_Kind::Pattern_Underscore:
        case Untyped_AST_Kind::Pattern_Ident:
        case Untyped_AST_Kind::Pattern_Tuple:
            return Typed_AST_Kind::Processed_Pattern;
            
        case Untyped_AST_Kind::Generic_Specialization:
            break;
    }
    assert(false);
}

constexpr size_t INDENT_SIZE = 2;
static void print_at_indent(const Ref<Typed_AST> node, size_t indent);

static void print_sub_at_indent(const char *name, const Ref<Typed_AST> sub, size_t indent) {
    printf("%*s%s: ", indent * INDENT_SIZE, "", name);
    print_at_indent(sub, indent);
}

static void print_unary_at_indent(const char *id, const Ref<Typed_AST_Unary> u, size_t indent) {
    printf("(%s) %s\n", id, u->type.debug_str());
    print_sub_at_indent("sub", u->sub, indent + 1);
}

static void print_binary_at_indent(const char *id, const Ref<Typed_AST_Binary> b, size_t indent) {
    printf("(%s) %s\n", id, b->type.debug_str());
    print_sub_at_indent("lhs", b->lhs, indent + 1);
    print_sub_at_indent("rhs", b->rhs, indent + 1);
}

static void print_ternary_at_indent(const char *id, const Ref<Typed_AST_Ternary> t, size_t indent) {
    printf("(%s) %s\n", id, t->type.debug_str());
    print_sub_at_indent("lhs", t->lhs, indent + 1);
    print_sub_at_indent("mid", t->mid, indent + 1);
    print_sub_at_indent("rhs", t->rhs, indent + 1);
}

static void print_multiary_at_indent(const char *id, const Ref<Typed_AST_Multiary> m, size_t indent) {
    printf("(%s) %s\n", id, m->type.debug_str());
    for (size_t i = 0; i < m->nodes.size(); i++) {
        const Ref<Typed_AST> node = m->nodes[i];
        printf("%*s%zu: ", (indent + 1) * INDENT_SIZE, "", i);
        print_at_indent(node, indent + 1);
    }
}

static void print_at_indent(const Ref<Typed_AST> node, size_t indent) {
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
        case Typed_AST_Kind::Int: {
            Ref<Typed_AST_Int> lit = node.cast<Typed_AST_Int>();
            printf("%lld\n", lit->value);
        } break;
        case Typed_AST_Kind::Str: {
            Ref<Typed_AST_Str> lit = node.cast<Typed_AST_Str>();
            printf("\"%.*s\"\n", lit->value.size(), lit->value.c_str());
        } break;
        case Typed_AST_Kind::Negation: {
            print_unary_at_indent("-", node.cast<Typed_AST_Unary>(), indent);
        } break;
        case Typed_AST_Kind::Not: {
            print_unary_at_indent("!", node.cast<Typed_AST_Unary>(), indent);
        } break;
        case Typed_AST_Kind::Address_Of: {
            print_unary_at_indent("&", node.cast<Typed_AST_Unary>(), indent);
        } break;
        case Typed_AST_Kind::Address_Of_Mut: {
            print_unary_at_indent("&mut", node.cast<Typed_AST_Unary>(), indent);
        } break;
        case Typed_AST_Kind::Deref: {
            print_unary_at_indent("*", node.cast<Typed_AST_Unary>(), indent);
        } break;
        case Typed_AST_Kind::Addition: {
            print_binary_at_indent("+", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::Subtraction: {
            print_binary_at_indent("-", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::Multiplication: {
            print_binary_at_indent("*", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::Division: {
            print_binary_at_indent("/", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::Mod: {
            print_binary_at_indent("%", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::Assignment: {
            print_binary_at_indent("=", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::Equal: {
            print_binary_at_indent("==", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::Not_Equal: {
            print_binary_at_indent("!=", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::Less: {
            print_binary_at_indent("<", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::Less_Eq: {
            print_binary_at_indent("<=", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::Greater: {
            print_binary_at_indent(">", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::Greater_Eq: {
            print_binary_at_indent(">=", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::While: {
            print_binary_at_indent("while", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::And: {
            print_binary_at_indent("and", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::Or: {
            print_binary_at_indent("or", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::Field_Access: {
            auto dot = node.cast<Typed_AST_Field_Access>();
            printf("(.) %s\n", dot->type.debug_str());
            print_sub_at_indent("instance", dot->instance, indent + 1);
            printf("%*soffset: %d\n", (indent + 1) * INDENT_SIZE, "", dot->field_offset);
        } break;
        case Typed_AST_Kind::Field_Access_Tuple: {
            print_binary_at_indent(".", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::Subscript:
        case Typed_AST_Kind::Negative_Subscript: {
            print_binary_at_indent("[]", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::Range: {
            print_binary_at_indent("..", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::Inclusive_Range: {
            print_binary_at_indent("...", node.cast<Typed_AST_Binary>(), indent);
        } break;
        case Typed_AST_Kind::If: {
            Ref<Typed_AST_If> t = node.cast<Typed_AST_If>();
            printf("(if)\n");
            print_sub_at_indent("cond", t->cond, indent + 1);
            print_sub_at_indent("then", t->then, indent + 1);
            if (t->else_) {
                print_sub_at_indent("else", t->else_, indent + 1);
            }
        } break;
        case Typed_AST_Kind::For:
        case Typed_AST_Kind::For_Range: {
            auto f = node.cast<Typed_AST_For>();
            printf("(for)\n");
            print_sub_at_indent("target", f->target, indent + 1);
            if (f->counter != "") {
                printf("%*scounter: %s\n", (indent + 1) * INDENT_SIZE, "", f->counter.c_str());
            }
            print_sub_at_indent("iterable", f->iterable, indent + 1);
            print_sub_at_indent("body", f->body, indent + 1);
        } break;
        case Typed_AST_Kind::Block: {
            print_multiary_at_indent("block", node.cast<Typed_AST_Multiary>(), indent);
        } break;
        case Typed_AST_Kind::Comma: {
            print_multiary_at_indent(",", node.cast<Typed_AST_Multiary>(), indent);
        } break;
        case Typed_AST_Kind::Tuple: {
            print_multiary_at_indent("tuple", node.cast<Typed_AST_Multiary>(), indent);
        } break;
        case Typed_AST_Kind::Let: {
            Ref<Typed_AST_Let> let = node.cast<Typed_AST_Let>();
            printf("(%s)\n", let->is_const ? "const" : "let");
            
            print_sub_at_indent("target", let->target, indent + 1);
            if (let->specified_type) {
                print_sub_at_indent("type", let->specified_type, indent + 1);
            }
            if (let->initializer) {
                print_sub_at_indent("init", let->initializer, indent + 1);
            }
        } break;
        case Typed_AST_Kind::Type_Signiture: {
            Ref<Typed_AST_Type_Signiture> sig = node.cast<Typed_AST_Type_Signiture>();
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
        case Typed_AST_Kind::Array:
        case Typed_AST_Kind::Slice: {
            Ref<Typed_AST_Array> array = node.cast<Typed_AST_Array>();
            if (array->array_type->kind == Value_Type_Kind::Array) {
                printf("(array)\n");
            } else {
                printf("(slice)\n");
            }
            printf("%*scount: %zu\n", (indent + 1) * INDENT_SIZE, "", array->count);
            printf("%*stype: %s\n", (indent + 1) * INDENT_SIZE, "", array->array_type->debug_str());
            print_sub_at_indent("elems", array->element_nodes, indent + 1);
        } break;
            
        default:
            assert(false);
            break;
    }
}

void Typed_AST::print() const {
    print_at_indent(Ref<Typed_AST>((Typed_AST *)this), 0);
}

struct Typer_Scope {
    std::unordered_map<std::string, Value_Type> variables;
};

struct Typer {
    Interpreter *interp;
    Module *module;
    std::list<Typer_Scope> scopes;
    
    Typer_Scope &current_scope() {
        return scopes.front();
    }
    
    void begin_scope() {
        scopes.push_front(Typer_Scope());
    }
    
    void end_scope() {
        scopes.pop_front();
    }
    
    bool type_of_variable(const std::string &id, Value_Type &out_type) {
        for (auto &s : scopes) {
            auto it = s.variables.find(id);
            if (it == s.variables.end()) continue;
            out_type = it->second;
            return true;
        }
        return false;
    }
    
    void put_variable(const std::string &id, Value_Type type, bool is_mut) {
        type.is_mut = is_mut;
        current_scope().variables[id] = type;
    }
    
    void put_pattern(
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
                put_variable(ip->id.c_str(), type, ip->is_mut);
            } break;
            case Untyped_AST_Kind::Pattern_Tuple: {
                auto tp = pattern.cast<Untyped_AST_Pattern_Tuple>();
                verify(type.kind == Value_Type_Kind::Tuple &&
                       tp->sub_patterns.size() == type.data.tuple.child_types.size(),
                       "Cannot match tuple pattern with %s.", type.debug_str());
                for (size_t i = 0; i < tp->sub_patterns.size(); i++) {
                    auto sub_pattern = tp->sub_patterns[i];
                    auto sub_type    = type.data.tuple.child_types[i];
                    put_pattern(sub_pattern, sub_type, out_pp);
                }
            } break;
            case Untyped_AST_Kind::Pattern_Struct: {
                auto sp = pattern.cast<Untyped_AST_Pattern_Struct>();
                auto uuid = sp->struct_id->typecheck(*this).cast<Typed_AST_UUID>();
                internal_verify(uuid, "Failed to cast to UUID* in Typer::put_pattern().");
                
                verify(type.kind == Value_Type_Kind::Struct &&
                       type.data.struct_.defn->id == uuid->id, "Cannot match %.*s struct pattern with %s.", sp->struct_id->id.size(), sp->struct_id->id.c_str(), type.debug_str());
                
                auto defn = type.data.struct_.defn;
                verify(defn->fields.size() == sp->sub_patterns.size(), "Incorrect number of sub patterns in struct pattern for struct %s. Expected %zu but was given %zu.", type.debug_str(), defn->fields.size(), sp->sub_patterns.size());
                
                for (size_t i = 0; i < sp->sub_patterns.size(); i++) {
                    auto sub_pattern = sp->sub_patterns[i];
                    auto field_type  = defn->fields[i].type;
                    put_pattern(sub_pattern, field_type, out_pp);
                }
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
            case Value_Type_Kind::Unresolved_Type:
                internal_error("Resolving an Unresolved type not yet implemented.");
                break;
                
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

            case Value_Type_Kind::Struct:
                internal_error("Canont resolve struct types yet.");
                break;
            case Value_Type_Kind::Enum:
                internal_error("Cannot resolve enum types yet.");
                break;
                
            default:
                break;
        }
        
        return resolved;
    }
};

Ref<Typed_AST_Multiary> typecheck(
    Interpreter &interp,
    Ref<Untyped_AST_Multiary> node)
{
    Typer t;
    t.interp = &interp;
    t.module = interp.create_module("<@TODO MODULE PATH>");
    
    return node->typecheck(t).cast<Typed_AST_Multiary>();
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
    Value_Type ty;
    verify(t.type_of_variable(id.c_str(), ty), "Unresolved identifier '%s'.", id.c_str());
    
    Ref<Typed_AST> ident;
    if (ty.kind == Value_Type_Kind::Type) {
        switch (ty.data.type.type->kind) {
            case Value_Type_Kind::Struct:
                ident = Mem.make<Typed_AST_UUID>(Typed_AST_Kind::Ident_Struct, ty.data.type.type->data.struct_.defn->id, ty);
                break;
            case Value_Type_Kind::Enum:
                internal_error("Enum idents not yet implemented.");
                break;
                
            default:
                internal_error("Invalid Value_Type_Kind for Type type.");
                break;
        }
    } else {
        ident = Mem.make<Typed_AST_Ident>(id.clone(), ty);
    }
    
    return ident;
}

Ref<Typed_AST> Untyped_AST_Int::typecheck(Typer &t) {
    return Mem.make<Typed_AST_Int>(value);
}

Ref<Typed_AST> Untyped_AST_Str::typecheck(Typer &t) {
    return Mem.make<Typed_AST_Str>(value.clone());
}

Ref<Typed_AST> Untyped_AST_Unary::typecheck(Typer &t) {
    auto sub = this->sub->typecheck(t);
    switch (kind) {
        case Untyped_AST_Kind::Negation:
            verify(sub->type.kind == Value_Type_Kind::Int ||
                   sub->type.kind == Value_Type_Kind::Float,
                   "(-) requires operand to be an (int) or a (float) but was given (%s).", sub->type.debug_str());
            return Mem.make<Typed_AST_Unary>(Typed_AST_Kind::Negation, sub->type, sub);
        case Untyped_AST_Kind::Not:
            verify(sub->type.kind == Value_Type_Kind::Bool, "(!) requires operand to be a (bool) but got a (%s).", sub->type.debug_str());
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
            verify(sub->type.kind == Value_Type_Kind::Ptr, "Cannot dereference something of type (%s) because it is not a pointer type.", sub->type.debug_str());
            return Mem.make<Typed_AST_Unary>(Typed_AST_Kind::Deref, *sub->type.data.ptr.child_type, sub);
            
        default:
            assert(false);
            return nullptr;
    }
}

Ref<Typed_AST> Untyped_AST_Binary::typecheck(Typer &t) {
    auto lhs = this->lhs->typecheck(t);
    auto rhs = this->rhs->typecheck(t);
    switch (kind) {
        case Untyped_AST_Kind::Addition:
            verify(lhs->type.kind == rhs->type.kind, "(+) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int ||
                   lhs->type.kind == Value_Type_Kind::Float, "(+) requires operands to be either (int) or (float) but was given (%s).", lhs->type.debug_str());
            verify(rhs->type.kind == Value_Type_Kind::Int ||
                   rhs->type.kind == Value_Type_Kind::Float, "(+) requires operands to be either (int) or (float) but was given (%s).", rhs->type.debug_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Addition, lhs->type, lhs, rhs);
        case Untyped_AST_Kind::Subtraction:
            verify(lhs->type.kind == rhs->type.kind, "(-) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int ||
                   lhs->type.kind == Value_Type_Kind::Float, "(-) requires operands to be either (int) or (float) but was given (%s).", lhs->type.debug_str());
            verify(rhs->type.kind == Value_Type_Kind::Int ||
                   rhs->type.kind == Value_Type_Kind::Float, "(-) requires operands to be either (int) or (float) but was given (%s).", rhs->type.debug_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Subtraction, lhs->type, lhs, rhs);
        case Untyped_AST_Kind::Multiplication:
            verify(lhs->type.kind == rhs->type.kind, "(*) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int ||
                   lhs->type.kind == Value_Type_Kind::Float, "(*) requires operands to be either (int) or (float) but was given (%s).", lhs->type.debug_str());
            verify(rhs->type.kind == Value_Type_Kind::Int ||
                   rhs->type.kind == Value_Type_Kind::Float, "(*) requires operands to be either (int) or (float) but was given (%s).", rhs->type.debug_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Multiplication, lhs->type, lhs, rhs);
        case Untyped_AST_Kind::Division:
            verify(lhs->type.kind == rhs->type.kind, "(/) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int ||
                   lhs->type.kind == Value_Type_Kind::Float, "(/) requires operands to be either (int) or (float) but was given (%s).", lhs->type.debug_str());
            verify(rhs->type.kind == Value_Type_Kind::Int ||
                   rhs->type.kind == Value_Type_Kind::Float, "(/) requires operands to be either (int) or (float) but was given (%s).", rhs->type.debug_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Division, lhs->type, lhs, rhs);
        case Untyped_AST_Kind::Mod:
            verify(lhs->type.kind == rhs->type.kind, "(+) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int, "(+) requires operands to be (int) but was given (%s).", lhs->type.debug_str());
            verify(rhs->type.kind == Value_Type_Kind::Int, "(+) requires operands to be (int) but was given (%s).", rhs->type.debug_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Mod, value_types::Int, lhs, rhs);
        case Untyped_AST_Kind::Assignment:
            verify(lhs->type.is_mut, "Cannot assign to something of type (%s) because it is immutable.", lhs->type.debug_str());
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
                   "(<) requires operands to be (int) or (float) but was given (%s).", lhs->type.debug_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Less, value_types::Bool, lhs, rhs);
        case Untyped_AST_Kind::Greater:
            verify(lhs->type.eq_ignoring_mutability(rhs->type), "(>) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int ||
                   lhs->type.kind == Value_Type_Kind::Float,
                   "(>) requires operands to be (int) or (float) but was given (%s).", lhs->type.debug_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Greater, value_types::Bool, lhs, rhs);
        case Untyped_AST_Kind::Less_Eq:
            verify(lhs->type.eq_ignoring_mutability(rhs->type), "(<=) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int ||
                   lhs->type.kind == Value_Type_Kind::Float,
                   "(<=) requires operands to be (int) or (float) but was given (%s).", lhs->type.debug_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Less_Eq, value_types::Bool, lhs, rhs);
        case Untyped_AST_Kind::Greater_Eq:
            verify(lhs->type.eq_ignoring_mutability(rhs->type), "(>=) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int ||
                   lhs->type.kind == Value_Type_Kind::Float,
                   "(>=) requires operands to be (int) or (float) but was given (%s).", lhs->type.debug_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Greater_Eq, value_types::Bool, lhs, rhs);
        case Untyped_AST_Kind::And:
            verify(lhs->type.kind == Value_Type_Kind::Bool, "(and) requires first operand to be (bool) but was given (%s).", lhs->type.debug_str());
            verify(rhs->type.kind == Value_Type_Kind::Bool, "(and) requires second operand to be (bool) but was given (%s).", rhs->type.debug_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::And, value_types::Bool, lhs, rhs);
        case Untyped_AST_Kind::Or:
            verify(lhs->type.kind == Value_Type_Kind::Bool, "(or) requires first operand to be (bool) but was given (%s).", lhs->type.debug_str());
            verify(rhs->type.kind == Value_Type_Kind::Bool, "(or) requires second operand to be (bool) but was given (%s).", rhs->type.debug_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Or, value_types::Bool, lhs, rhs);
//        case Untyped_AST_Kind::Field_Access: {
//            bool needs_deref = lhs->type.kind == Value_Type_Kind::Ptr;
//            Value_Type &ty = needs_deref ? *lhs->type.child_type() : lhs->type;
//            verify(ty.kind == Value_Type_Kind::Struct, "(.) requires first operand to be a struct type but was given (%s).", lhs->type.debug_str());
//            String mem_id = rhs.cast<Untyped_AST_Ident>()->id;
//            Value_Type *mem_ty = ty.data.struct_.defn->type_of_field(mem_id);
//            verify(mem_ty, "'%.*s' is not a field of '%.*s'.", mem_id.size(), mem_id.c_str(), ty.data.struct_.defn->name.size(), ty.data.struct_.defn->name.c_str());
//            return Mem.make<Typed_AST_Field_Access>(*mem_ty, needs_deref, lhs)
//        } break;
        case Untyped_AST_Kind::Field_Access_Tuple: {
            bool needs_deref = lhs->type.kind == Value_Type_Kind::Ptr;
            Value_Type &ty = needs_deref ? *lhs->type.child_type() : lhs->type;
            verify(ty.kind == Value_Type_Kind::Tuple, "(.) requires first operand to be a tuple but was given (%s).", lhs->type.debug_str());
            auto i = rhs.cast<Typed_AST_Int>();
            internal_verify(i, "Dot_Tuple got a rhs that wasn't an int.");
            verify(i->value < ty.data.tuple.child_types.size(), "Cannot access type %lld from a %s.", i->value, lhs->type.debug_str());
            Value_Type child_ty = ty.data.tuple.child_types[i->value];
            child_ty.is_mut = ty.is_mut;
            return Mem.make<Typed_AST_Field_Access_Tuple>(
                Typed_AST_Kind::Field_Access_Tuple,
                child_ty,
                needs_deref,
                lhs,
                i
            );
        } break;
        case Untyped_AST_Kind::Subscript: {
            verify(lhs->type.kind == Value_Type_Kind::Array ||
                   lhs->type.kind == Value_Type_Kind::Slice,
                   "([]) requires first operand to be an array or slice but was given (%s).", lhs->type.debug_str());
            verify(rhs->type.kind == Value_Type_Kind::Int, "([]) requires second operand to be (int) but was given (%s).", rhs->type.debug_str());
            
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
            verify(lhs->type.kind == Value_Type_Kind::Int, "(..) requires operands to be of type (int) but was given (%s).", lhs->type.debug_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Range, value_types::range_of(false, &lhs->type), lhs, rhs);
        case Untyped_AST_Kind::Inclusive_Range:
            verify(lhs->type.eq_ignoring_mutability(rhs->type), "(...) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int, "(...) requires operands to be of type (int) but was given (%s).", lhs->type.debug_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::Inclusive_Range, value_types::range_of(true, &lhs->type), lhs, rhs);
            
        case Untyped_AST_Kind::While:
            verify(lhs->type.kind == Value_Type_Kind::Bool, "(while) requires condition to be (bool) but was given (%s).", lhs->type.debug_str());
            return Mem.make<Typed_AST_Binary>(Typed_AST_Kind::While, value_types::None, lhs, rhs);
            
        default:
            assert(false);
    }
    return nullptr;
}

Ref<Typed_AST> Untyped_AST_Ternary::typecheck(Typer &t) {
    auto lhs = this->lhs->typecheck(t);
    auto mid = this->mid->typecheck(t);
    auto rhs = this->rhs->typecheck(t);
    switch (kind) {
            
        default:
            assert(false);
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
        verify(element_nodes->nodes[i]->type.eq_ignoring_mutability(*element_type), "Element %zu in array literal does not match the expected type (%s).", i+1, element_type->debug_str());
    }
    return Mem.make<Typed_AST_Array>(*array_type, to_typed(kind), count, array_type, element_nodes);
}

Ref<Typed_AST> Untyped_AST_Struct_Literal::typecheck(Typer &t) {
    auto struct_id = this->struct_id->typecheck(t).cast<Typed_AST_UUID>();
    internal_verify(struct_id, "Failed to cast struct_id to UUID* in Struct_Literal::typecheck().");
    
    auto defn = &t.interp->typebook.structs[t.module->structs[struct_id->id]];
    auto bindings = this->bindings;
    
    verify(defn->fields.size() == bindings->nodes.size(), "Incorrect number of arguments in struct literal. Expected %zu but was given %zu.", defn->fields.size(), bindings->nodes.size());
    
    auto args = Mem.make<Typed_AST_Multiary>(Typed_AST_Kind::Comma);
    args->type = *struct_id->type.data.type.type;
    
    for (size_t i = 0; i < defn->fields.size(); i++) {
        auto &field = defn->fields[i];
        auto binding = bindings->nodes[i];
        
        Ref<Typed_AST> arg = nullptr;
        switch (binding->kind) {
            case Untyped_AST_Kind::Ident: {
                auto bid = binding.cast<Untyped_AST_Ident>();
                verify(field.id == bid->id, "Given identifier doesn't match name of field. Expected '%.*s' but was given '%.*s'. Please specify field.", field.id.size(), field.id.c_str(), bid->id.size(), bid->id.c_str());
                arg = bid->typecheck(t);
                verify(field.type.assignable_from(arg->type), "Cannot assign to field '%.*s' because of mismatched types. (%s) vs. (%s).", field.id.size(), field.id.c_str(), field.type.debug_str(), arg->type.debug_str());
            } break;
            case Untyped_AST_Kind::Binding: {
                auto b = binding.cast<Untyped_AST_Binary>();
                auto bid = b->lhs.cast<Untyped_AST_Ident>();
                verify(field.id == bid->id, "Given identifier doesn't match name of field. Expected '%.*s' but was given '%.*s'.", field.id.size(), field.id.c_str(), bid->id.size(), bid->id.c_str());
                arg = b->rhs->typecheck(t);
                verify(field.type.assignable_from(arg->type), "Cannot assign to field '%.*s' because of mismatched types. (%s) vs (%s).", field.id.size(), field.id.c_str(), field.type.debug_str(), arg->type.debug_str());
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
    verify(ty.kind == Value_Type_Kind::Struct, "(.) requires first operand to be a struct type but was given (%s).", instance->type.debug_str());
    
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

Ref<Typed_AST> Untyped_AST_If::typecheck(Typer &t) {
    auto cond = this->cond->typecheck(t);
    auto then = this->then->typecheck(t);
    auto else_ = this->else_ ? this->else_->typecheck(t) : nullptr;
    verify(!else_ || then->type.eq(else_->type), "Both branches of (if) must be the same. (%s) vs (%s).", then->type.debug_str(), else_->type.debug_str());
    return Mem.make<Typed_AST_If>(then->type, cond, then, else_);
}

Ref<Typed_AST> Untyped_AST_Type_Signiture::typecheck(Typer &t) {
    auto resolved_type = Mem.make<Value_Type>(t.resolve_value_type(*value_type));
    return Mem.make<Typed_AST_Type_Signiture>(resolved_type);
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
            error("Cannot iterate over something of type (%s).", iterable->type.debug_str());
            break;
    }
    
    Value_Type *target_type = iterable->type.child_type();
    
    t.begin_scope();
    
    auto processed_target = Mem.make<Typed_AST_Processed_Pattern>();
    t.put_pattern(target, *target_type, processed_target);
    
    if (counter != "") {
        t.put_variable(counter.c_str(), value_types::Int, false);
    }
    
    auto body = this->body->typecheck(t);
    
    t.end_scope();
    
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

Ref<Typed_AST> Untyped_AST_Let::typecheck(Typer &t) {
    Value_Type ty = value_types::None;
    Ref<Typed_AST> init = nullptr;
    if (initializer) {
        init = initializer->typecheck(t);
        ty = init->type;
    }
    
    Ref<Typed_AST_Type_Signiture> sig = nullptr;
    if (specified_type) {
        if (init) {
            verify(specified_type->value_type->assignable_from(init->type), "Specified type (%s) does not match given type (%s).", specified_type->value_type->debug_str(), init->type.debug_str());
        }
        sig = specified_type->typecheck(t).cast<Typed_AST_Type_Signiture>();
        internal_verify(sig, "Failed to cast to Type_Sig in Untyped_AST_Let::typecheck().");
        ty = *sig->value_type;
    }

    auto processed_target = Mem.make<Typed_AST_Processed_Pattern>();
    t.put_pattern(target, ty, processed_target);
    
    return Mem.make<Typed_AST_Let>(is_const, processed_target, sig, init);
}

Ref<Typed_AST> Untyped_AST_Generic_Specialization::typecheck(Typer &t) {
    internal_error("Generic_Specialization::typecheck() not yet implemented.");
}

Ref<Typed_AST> Untyped_AST_Struct_Declaration::typecheck(Typer &t) {
    Struct_Definition defn;
    defn.module = t.module;
    defn.id = t.interp->next_uuid();
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
    size_t type_id = t.interp->typebook.add_struct(defn);
    t.module->structs[defn.id] = type_id;
    
    String id = this->id.clone();
    Value_Type *struct_type = Mem.make<Value_Type>().as_ptr();
    struct_type->kind = Value_Type_Kind::Struct;
    struct_type->data.struct_.defn = &t.interp->typebook.structs[type_id];
    
    Value_Type type;
    type.kind = Value_Type_Kind::Type;
    type.data.type.type = struct_type;
    
    t.put_variable(id.c_str(), type, false);
    
    return nullptr;
}
