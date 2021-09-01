//
//  typer.cpp
//  Fox
//
//  Created by Denver Lacey on 23/8/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#include "typer.h"
#include "ast.h"
#include "error.h"
#include <list>
#include <unordered_map>
#include <string>

Typed_AST_Bool::Typed_AST_Bool(bool value) {
    kind = Typed_AST_Kind::Bool;
    type.kind = Value_Type_Kind::Bool;
    this->value = value;
}

Typed_AST_Char::Typed_AST_Char(char32_t value) {
    kind = Typed_AST_Kind::Char;
    type.kind = Value_Type_Kind::Char;
    this->value = value;
}

Typed_AST_Float::Typed_AST_Float(double value) {
    kind = Typed_AST_Kind::Float;
    type.kind = Value_Type_Kind::Float;
    this->value = value;
}

Typed_AST_Ident::Typed_AST_Ident(String id, Value_Type type) {
    kind = Typed_AST_Kind::Ident;
    this->type = type;
    this->id = id;
}

Typed_AST_Ident::~Typed_AST_Ident() {
    id.free();
}

Typed_AST_Int::Typed_AST_Int(int64_t value) {
    kind = Typed_AST_Kind::Int;
    type.kind = Value_Type_Kind::Int;
    this->value = value;
}

Typed_AST_Str::Typed_AST_Str(String value) {
    kind = Typed_AST_Kind::Str;
    type.kind = Value_Type_Kind::Str;
    this->value = value;
}

Typed_AST_Str::~Typed_AST_Str() {
    value.free();
}

Typed_AST_Unary::Typed_AST_Unary(Typed_AST_Kind kind, Value_Type type, Ref<Typed_AST> sub) {
    this->kind = kind;
    this->type = type;
    this->sub = std::move(sub);
}

Typed_AST_Binary::Typed_AST_Binary(Typed_AST_Kind kind, Value_Type type, Ref<Typed_AST> lhs, Ref<Typed_AST> rhs) {
    this->kind = kind;
    this->type = type;
    this->lhs = std::move(lhs);
    this->rhs = std::move(rhs);
}

Typed_AST_Ternary::Typed_AST_Ternary(Typed_AST_Kind kind, Value_Type type, Ref<Typed_AST> lhs, Ref<Typed_AST> mid, Ref<Typed_AST> rhs) {
    this->kind = kind;
    this->type = type;
    this->lhs = std::move(lhs);
    this->mid = std::move(mid);
    this->rhs = std::move(rhs);
}

Typed_AST_Multiary::Typed_AST_Multiary(Typed_AST_Kind kind) {
    this->kind = kind;
    this->type.kind = Value_Type_Kind::None;
}

void Typed_AST_Multiary::add(Ref<Typed_AST> node) {
    nodes.push_back(std::move(node));
}

Typed_AST_If::Typed_AST_If(
    Value_Type type,
    Ref<Typed_AST> cond,
    Ref<Typed_AST> then,
    Ref<Typed_AST> else_)
{
    kind = Typed_AST_Kind::If;
    this->type = type;
    this->cond = std::move(cond);
    this->then = std::move(then);
    this->else_ = std::move(else_);
}

Typed_AST_Type_Signiture::Typed_AST_Type_Signiture(Ref<Value_Type> value_type) {
    this->kind = Typed_AST_Kind::Type_Signiture;
    this->value_type = std::move(value_type);
}

Typed_AST_Let::Typed_AST_Let(
    String id,
    bool is_mut,
    Ref<Typed_AST_Type_Signiture> specified_type,
    Ref<Typed_AST> initializer)
{
    kind = Typed_AST_Kind::Let;
    this->id = id;
    this->is_mut = is_mut;
    this->specified_type = std::move(specified_type);
    this->initializer = std::move(initializer);
}

Typed_AST_Let::~Typed_AST_Let() {
    id.free();
}

static Typed_AST_Kind to_typed(Untyped_AST_Kind kind) {
    switch (kind) {
        case Untyped_AST_Kind::Bool:            return Typed_AST_Kind::Bool;
        case Untyped_AST_Kind::Char:            return Typed_AST_Kind::Char;
        case Untyped_AST_Kind::Float:           return Typed_AST_Kind::Float;
        case Untyped_AST_Kind::Ident:           return Typed_AST_Kind::Ident;
        case Untyped_AST_Kind::Int:             return Typed_AST_Kind::Int;
        case Untyped_AST_Kind::Str:             return Typed_AST_Kind::Str;
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
        case Untyped_AST_Kind::Dot:             return Typed_AST_Kind::Dot;
        case Untyped_AST_Kind::Dot_Tuple:       return Typed_AST_Kind::Dot_Tuple;
        case Untyped_AST_Kind::Block:           return Typed_AST_Kind::Block;
        case Untyped_AST_Kind::Comma:           return Typed_AST_Kind::Comma;
        case Untyped_AST_Kind::Tuple:           return Typed_AST_Kind::Tuple;
        case Untyped_AST_Kind::If:              return Typed_AST_Kind::If;
        case Untyped_AST_Kind::Let:             return Typed_AST_Kind::Let;
        case Untyped_AST_Kind::Type_Signiture:  return Typed_AST_Kind::Type_Signiture;
    }
    assert(false);
}

constexpr size_t INDENT_SIZE = 2;
static void print_at_indent(const Typed_AST *node, size_t indent);

static void print_sub_at_indent(const char *name, const Typed_AST *sub, size_t indent) {
    printf("%*s%s: ", indent * INDENT_SIZE, "", name);
    print_at_indent(sub, indent);
}

static void print_unary_at_indent(const char *id, const Typed_AST_Unary *u, size_t indent) {
    printf("(%s) %s\n", id, u->type.debug_str());
    print_sub_at_indent("sub", u->sub.get(), indent + 1);
}

static void print_binary_at_indent(const char *id, const Typed_AST_Binary *b, size_t indent) {
    printf("(%s) %s\n", id, b->type.debug_str());
    print_sub_at_indent("lhs", b->lhs.get(), indent + 1);
    print_sub_at_indent("rhs", b->rhs.get(), indent + 1);
}

static void print_ternary_at_indent(const char *id, const Typed_AST_Ternary *t, size_t indent) {
    printf("(%s) %s\n", id, t->type.debug_str());
    print_sub_at_indent("lhs", t->lhs.get(), indent + 1);
    print_sub_at_indent("mid", t->mid.get(), indent + 1);
    print_sub_at_indent("rhs", t->rhs.get(), indent + 1);
}

static void print_multiary_at_indent(const char *id, const Typed_AST_Multiary *m, size_t indent) {
    printf("(%s) %s\n", id, m->type.debug_str());
    for (size_t i = 0; i < m->nodes.size(); i++) {
        const Typed_AST *node = m->nodes[i].get();
        printf("%*s%zu: ", (indent + 1) * INDENT_SIZE, "", i);
        print_at_indent(node, indent + 1);
    }
}

static void print_at_indent(const Typed_AST *node, size_t indent) {
    switch (node->kind) {
        case Typed_AST_Kind::Bool: {
            Typed_AST_Bool *lit = (Typed_AST_Bool *)node;
            printf("%s\n", lit->value ? "true" : "false");
        } break;
        case Typed_AST_Kind::Char: {
            Typed_AST_Char *lit = (Typed_AST_Char *)node;
            printf("%s\n", utf8char_t::from_char32(lit->value).buf);
        } break;
        case Typed_AST_Kind::Float: {
            Typed_AST_Float *lit = (Typed_AST_Float *)node;
            printf("%f\n", lit->value);
        } break;
        case Typed_AST_Kind::Ident: {
            Typed_AST_Ident *id = (Typed_AST_Ident *)node;
            printf("%.*s :: %s\n", id->id.size(), id->id.c_str(), id->type.debug_str());
        } break;
        case Typed_AST_Kind::Int: {
            Typed_AST_Int *lit = (Typed_AST_Int *)node;
            printf("%lld\n", lit->value);
        } break;
        case Typed_AST_Kind::Str: {
            Typed_AST_Str *lit = (Typed_AST_Str *)node;
            printf("%.*s\n", lit->value.size(), lit->value.c_str());
        } break;
        case Typed_AST_Kind::Negation: {
            print_unary_at_indent("-", (Typed_AST_Unary *)node, indent);
        } break;
        case Typed_AST_Kind::Not: {
            print_unary_at_indent("!", (Typed_AST_Unary *)node, indent);
        } break;
        case Typed_AST_Kind::Address_Of: {
            print_unary_at_indent("&", (Typed_AST_Unary *)node, indent);
        } break;
        case Typed_AST_Kind::Address_Of_Mut: {
            print_unary_at_indent("&mut", (Typed_AST_Unary *)node, indent);
        } break;
        case Typed_AST_Kind::Deref: {
            print_unary_at_indent("*", (Typed_AST_Unary *)node, indent);
        } break;
        case Typed_AST_Kind::Addition: {
            print_binary_at_indent("+", (Typed_AST_Binary *)node, indent);
        } break;
        case Typed_AST_Kind::Subtraction: {
            print_binary_at_indent("-", (Typed_AST_Binary *)node, indent);
        } break;
        case Typed_AST_Kind::Multiplication: {
            print_binary_at_indent("*", (Typed_AST_Binary *)node, indent);
        } break;
        case Typed_AST_Kind::Division: {
            print_binary_at_indent("/", (Typed_AST_Binary *)node, indent);
        } break;
        case Typed_AST_Kind::Mod: {
            print_binary_at_indent("%", (Typed_AST_Binary *)node, indent);
        } break;
        case Typed_AST_Kind::Assignment: {
            print_binary_at_indent("=", (Typed_AST_Binary *)node, indent);
        } break;
        case Typed_AST_Kind::Equal: {
            print_binary_at_indent("==", (Typed_AST_Binary *)node, indent);
        } break;
        case Typed_AST_Kind::Not_Equal: {
            print_binary_at_indent("!=", (Typed_AST_Binary *)node, indent);
        } break;
        case Typed_AST_Kind::Less: {
            print_binary_at_indent("<", (Typed_AST_Binary *)node, indent);
        } break;
        case Typed_AST_Kind::Less_Eq: {
            print_binary_at_indent("<=", (Typed_AST_Binary *)node, indent);
        } break;
        case Typed_AST_Kind::Greater: {
            print_binary_at_indent(">", (Typed_AST_Binary *)node, indent);
        } break;
        case Typed_AST_Kind::Greater_Eq: {
            print_binary_at_indent(">=", (Typed_AST_Binary *)node, indent);
        } break;
        case Typed_AST_Kind::While: {
            print_binary_at_indent("while", (Typed_AST_Binary *)node, indent);
        } break;
        case Typed_AST_Kind::And: {
            print_binary_at_indent("and", (Typed_AST_Binary *)node, indent);
        } break;
        case Typed_AST_Kind::Or: {
            print_binary_at_indent("or", (Typed_AST_Binary *)node, indent);
        } break;
        case Typed_AST_Kind::Dot:
        case Typed_AST_Kind::Dot_Tuple: {
            print_binary_at_indent(".", (Typed_AST_Binary *)node, indent);
        } break;
        case Typed_AST_Kind::If: {
            Typed_AST_If *t = (Typed_AST_If *)node;
            printf("(if)\n");
            print_sub_at_indent("cond", t->cond.get(), indent + 1);
            print_sub_at_indent("then", t->then.get(), indent + 1);
            if (t->else_) {
                print_sub_at_indent("else", t->else_.get(), indent + 1);
            }
        } break;
        case Typed_AST_Kind::Block: {
            print_multiary_at_indent("block", (Typed_AST_Multiary *)node, indent);
        } break;
        case Typed_AST_Kind::Comma: {
            print_multiary_at_indent(",", (Typed_AST_Multiary *)node, indent);
        } break;
        case Typed_AST_Kind::Tuple: {
            print_multiary_at_indent("tuple", (Typed_AST_Multiary *)node, indent);
        } break;
        case Typed_AST_Kind::Let: {
            Typed_AST_Let *let = (Typed_AST_Let *)node;
            if (let->is_mut) {
                printf("(let mut)\n");
            } else {
                printf("(let)\n");
            }
            printf("%*sid: `%.*s`\n", (indent + 1) * INDENT_SIZE, "", let->id.size(), let->id.c_str());
            if (let->specified_type) {
                print_sub_at_indent("type", let->specified_type.get(), indent + 1);
            }
            if (let->initializer) {
                print_sub_at_indent("init", let->initializer.get(), indent + 1);
            }
        } break;
        case Typed_AST_Kind::Type_Signiture: {
            Typed_AST_Type_Signiture *sig = (Typed_AST_Type_Signiture *)node;
            printf("%s\n", sig->value_type->debug_str());
        } break;
            
        default:
            assert(false);
            break;
    }
}

void Typed_AST::print() const {
    print_at_indent(this, 0);
}

struct Typer_Scope {
    std::unordered_map<std::string, Value_Type> variables;
};

struct Typer {
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
};

Ref<Typed_AST_Multiary> typecheck(Untyped_AST_Multiary *node) {
    Typer t;
    return cast<Typed_AST_Multiary>(node->typecheck(t));
}

Ref<Typed_AST> Untyped_AST_Bool::typecheck(Typer &t) {
    return make<Typed_AST_Bool>(value);
}

Ref<Typed_AST> Untyped_AST_Char::typecheck(Typer &t) {
    return make<Typed_AST_Char>(value);
}

Ref<Typed_AST> Untyped_AST_Float::typecheck(Typer &t) {
    return make<Typed_AST_Float>(value);
}

Ref<Typed_AST> Untyped_AST_Ident::typecheck(Typer &t) {
    Value_Type ty;
    verify(t.type_of_variable(id.c_str(), ty), "Unresolved identifier '%s'.", id.c_str());
    return make<Typed_AST_Ident>(id.clone(), ty);
}

Ref<Typed_AST> Untyped_AST_Int::typecheck(Typer &t) {
    return make<Typed_AST_Int>(value);
}

Ref<Typed_AST> Untyped_AST_Str::typecheck(Typer &t) {
    return make<Typed_AST_Str>(value.clone());
}

Ref<Typed_AST> Untyped_AST_Unary::typecheck(Typer &t) {
    auto sub = this->sub->typecheck(t);
    switch (kind) {
        case Untyped_AST_Kind::Negation:
            verify(sub->type.kind == Value_Type_Kind::Int ||
                   sub->type.kind == Value_Type_Kind::Float,
                   "(-) requires operand to be an (int) or a (float) but was given (%s).", sub->type.debug_str());
            return make<Typed_AST_Unary>(Typed_AST_Kind::Negation, sub->type, std::move(sub));
        case Untyped_AST_Kind::Not:
            verify(sub->type.kind == Value_Type_Kind::Bool, "(!) requires operand to be a (bool) but got a (%s).", sub->type.debug_str());
            return make<Typed_AST_Unary>(Typed_AST_Kind::Not, value_types::Bool, std::move(sub));
        case Untyped_AST_Kind::Address_Of: {
            verify(sub->type.kind != Value_Type_Kind::None, "Cannot take a pointer to something that doesn't return a value.");
            auto pty = value_types::ptr_to(&sub->type);
            pty.data.ptr.subtype->is_mut = false;
            return make<Typed_AST_Unary>(Typed_AST_Kind::Address_Of, pty, std::move(sub));
        }
        case Untyped_AST_Kind::Address_Of_Mut: {
            verify(sub->type.kind != Value_Type_Kind::None, "Cannot take a pointer to something that doesn't return a value.");
            verify(sub->type.is_mut, "Cannot take a mutable pointer to something that isn't itself mutable.");
            auto pty = value_types::ptr_to(&sub->type);
            return make<Typed_AST_Unary>(Typed_AST_Kind::Address_Of_Mut, pty, std::move(sub));
        }
        case Untyped_AST_Kind::Deref:
            verify(sub->type.kind == Value_Type_Kind::Ptr, "Cannot dereference something of type (%s) because it is not a pointer type.", sub->type.debug_str());
            return make<Typed_AST_Unary>(Typed_AST_Kind::Deref, *sub->type.data.ptr.subtype, std::move(sub));
            
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
            return make<Typed_AST_Binary>(Typed_AST_Kind::Addition, lhs->type, std::move(lhs), std::move(rhs));
        case Untyped_AST_Kind::Subtraction:
            verify(lhs->type.kind == rhs->type.kind, "(-) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int ||
                   lhs->type.kind == Value_Type_Kind::Float, "(-) requires operands to be either (int) or (float) but was given (%s).", lhs->type.debug_str());
            verify(rhs->type.kind == Value_Type_Kind::Int ||
                   rhs->type.kind == Value_Type_Kind::Float, "(-) requires operands to be either (int) or (float) but was given (%s).", rhs->type.debug_str());
            return make<Typed_AST_Binary>(Typed_AST_Kind::Subtraction, lhs->type, std::move(lhs), std::move(rhs));
        case Untyped_AST_Kind::Multiplication:
            verify(lhs->type.kind == rhs->type.kind, "(*) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int ||
                   lhs->type.kind == Value_Type_Kind::Float, "(*) requires operands to be either (int) or (float) but was given (%s).", lhs->type.debug_str());
            verify(rhs->type.kind == Value_Type_Kind::Int ||
                   rhs->type.kind == Value_Type_Kind::Float, "(*) requires operands to be either (int) or (float) but was given (%s).", rhs->type.debug_str());
            return make<Typed_AST_Binary>(Typed_AST_Kind::Multiplication, lhs->type, std::move(lhs), std::move(rhs));
        case Untyped_AST_Kind::Division:
            verify(lhs->type.kind == rhs->type.kind, "(/) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int ||
                   lhs->type.kind == Value_Type_Kind::Float, "(/) requires operands to be either (int) or (float) but was given (%s).", lhs->type.debug_str());
            verify(rhs->type.kind == Value_Type_Kind::Int ||
                   rhs->type.kind == Value_Type_Kind::Float, "(/) requires operands to be either (int) or (float) but was given (%s).", rhs->type.debug_str());
            return make<Typed_AST_Binary>(Typed_AST_Kind::Division, lhs->type, std::move(lhs), std::move(rhs));
        case Untyped_AST_Kind::Mod:
            verify(lhs->type.kind == rhs->type.kind, "(+) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int, "(+) requires operands to be (int) but was given (%s).", lhs->type.debug_str());
            verify(rhs->type.kind == Value_Type_Kind::Int, "(+) requires operands to be (int) but was given (%s).", rhs->type.debug_str());
            return make<Typed_AST_Binary>(Typed_AST_Kind::Mod, value_types::Int, std::move(lhs), std::move(rhs));
        case Untyped_AST_Kind::Assignment:
            verify(lhs->type == rhs->type, "(=) requires both operands to be the same type.");
            verify(lhs->type.is_mut, "Cannot assign to something of type (%s) because it isn't mutable.", lhs->type.debug_str());
            return make<Typed_AST_Binary>(Typed_AST_Kind::Assignment, value_types::None, std::move(lhs), std::move(rhs));
        case Untyped_AST_Kind::Equal:
            verify(lhs->type == rhs->type, "(==) requires both operands to be the same type.");
            return make<Typed_AST_Binary>(Typed_AST_Kind::Equal, value_types::Bool, std::move(lhs), std::move(rhs));
        case Untyped_AST_Kind::Not_Equal:
            verify(lhs->type == rhs->type, "(!=) requires both operands to be the same type.");
            return make<Typed_AST_Binary>(Typed_AST_Kind::Not_Equal, value_types::Bool, std::move(lhs), std::move(rhs));
        case Untyped_AST_Kind::Less:
            verify(lhs->type == rhs->type, "(<) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int ||
                   lhs->type.kind == Value_Type_Kind::Float,
                   "(<) requires operands to be (int) or (float) but was given (%s).", lhs->type.debug_str());
            return make<Typed_AST_Binary>(Typed_AST_Kind::Less, value_types::Bool, std::move(lhs), std::move(rhs));
        case Untyped_AST_Kind::Greater:
            verify(lhs->type == rhs->type, "(>) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int ||
                   lhs->type.kind == Value_Type_Kind::Float,
                   "(>) requires operands to be (int) or (float) but was given (%s).", lhs->type.debug_str());
            return make<Typed_AST_Binary>(Typed_AST_Kind::Greater, value_types::Bool, std::move(lhs), std::move(rhs));
        case Untyped_AST_Kind::Less_Eq:
            verify(lhs->type == rhs->type, "(<=) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int ||
                   lhs->type.kind == Value_Type_Kind::Float,
                   "(<=) requires operands to be (int) or (float) but was given (%s).", lhs->type.debug_str());
            return make<Typed_AST_Binary>(Typed_AST_Kind::Less_Eq, value_types::Bool, std::move(lhs), std::move(rhs));
        case Untyped_AST_Kind::Greater_Eq:
            verify(lhs->type == rhs->type, "(>=) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int ||
                   lhs->type.kind == Value_Type_Kind::Float,
                   "(>=) requires operands to be (int) or (float) but was given (%s).", lhs->type.debug_str());
            return make<Typed_AST_Binary>(Typed_AST_Kind::Greater_Eq, value_types::Bool, std::move(lhs), std::move(rhs));
        case Untyped_AST_Kind::And:
            verify(lhs->type.kind == Value_Type_Kind::Bool, "(and) requires first operand to be (bool) but was given (%s).", lhs->type.debug_str());
            verify(rhs->type.kind == Value_Type_Kind::Bool, "(and) requires second operand to be (bool) but was given (%s).", rhs->type.debug_str());
            return make<Typed_AST_Binary>(Typed_AST_Kind::And, value_types::Bool, std::move(lhs), std::move(rhs));
        case Untyped_AST_Kind::Or:
            verify(lhs->type.kind == Value_Type_Kind::Bool, "(or) requires first operand to be (bool) but was given (%s).", lhs->type.debug_str());
            verify(rhs->type.kind == Value_Type_Kind::Bool, "(or) requires second operand to be (bool) but was given (%s).", rhs->type.debug_str());
            return make<Typed_AST_Binary>(Typed_AST_Kind::Or, value_types::Bool, std::move(lhs), std::move(rhs));
        case Untyped_AST_Kind::Dot:
            assert(false);
            break;
        case Untyped_AST_Kind::Dot_Tuple: {
            verify(lhs->type.kind == Value_Type_Kind::Tuple, "(.) requires first operand to be a tuple but was given (%s).", lhs->type.debug_str());
            auto i = cast<Typed_AST_Int>(rhs);
            internal_verify(i, "Dot_Tuple got a rhs that wasn't an int.");
            verify(i->value < lhs->type.data.tuple.subtypes.size(), "Cannot access type %lld from a %s.", i->value, lhs->type.debug_str());
            return make<Typed_AST_Binary>(Typed_AST_Kind::Dot_Tuple, lhs->type.data.tuple.subtypes[i->value], std::move(lhs), std::move(i));
        } break;
            
        case Untyped_AST_Kind::While:
            verify(lhs->type.kind == Value_Type_Kind::Bool, "(while) requires condition to be (bool) but was given (%s).", lhs->type.debug_str());
            return make<Typed_AST_Binary>(Typed_AST_Kind::While, value_types::None, std::move(lhs), std::move(rhs));
            
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
    auto multi = make<Typed_AST_Multiary>(to_typed(kind));
    for (auto &node : nodes) {
        multi->add(node->typecheck(t));
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

Ref<Typed_AST> Untyped_AST_If::typecheck(Typer &t) {
    auto cond = this->cond->typecheck(t);
    auto then = this->then->typecheck(t);
    auto else_ = this->else_ ? this->else_->typecheck(t) : nullptr;
    verify(!else_ || then->type == else_->type, "Both branches of (if) must be the same. (%s) vs (%s).", then->type.debug_str(), else_->type.debug_str());
    return make<Typed_AST_If>(then->type, std::move(cond), std::move(then), std::move(else_));
}

Ref<Typed_AST> Untyped_AST_Type_Signiture::typecheck(Typer &t) {
    return make<Typed_AST_Type_Signiture>(std::move(value_type));
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
            verify(*specified_type->value_type == init->type, "Specified type (%s) does not match given type (%s).", specified_type->value_type->debug_str(), init->type.debug_str());
        }
        sig = cast<Typed_AST_Type_Signiture>(specified_type->typecheck(t));
        internal_verify(sig, "Failed to cast to Type_Sig in Untyped_AST_Let::typecheck().");
        ty = *sig->value_type;
    }

    t.put_variable(id.c_str(), ty, is_mut);
    
    return make<Typed_AST_Let>(id.clone(), is_mut, std::move(sig), std::move(init));
}
