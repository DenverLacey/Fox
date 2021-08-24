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

Typed_AST_Block::Typed_AST_Block(Typed_AST_Kind kind) {
    this->kind = kind;
    this->type.kind = Value_Type_Kind::None;
}

void Typed_AST_Block::add(Ref<Typed_AST> node) {
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

static void print_block_at_indent(const char *id, const Typed_AST_Block *b, size_t indent) {
    printf("(%s) %s\n", id, b->type.debug_str());
    for (size_t i = 0; i < b->nodes.size(); i++) {
        const Typed_AST *node = b->nodes[i].get();
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
        case Typed_AST_Kind::Not: {
            print_unary_at_indent("!", (Typed_AST_Unary *)node, indent);
        } break;
        case Typed_AST_Kind::Addition: {
            print_binary_at_indent("+", (Typed_AST_Binary *)node, indent);
        } break;
        case Typed_AST_Kind::Assignment: {
            print_binary_at_indent("=", (Typed_AST_Binary *)node, indent);
        } break;
        case Typed_AST_Kind::Division: {
            print_binary_at_indent("/", (Typed_AST_Binary *)node, indent);
        } break;
        case Typed_AST_Kind::Equal: {
            print_binary_at_indent("==", (Typed_AST_Binary *)node, indent);
        } break;
        case Typed_AST_Kind::Mod: {
            print_binary_at_indent("%", (Typed_AST_Binary *)node, indent);
        } break;
        case Typed_AST_Kind::Multiplication: {
            print_binary_at_indent("*", (Typed_AST_Binary *)node, indent);
        } break;
        case Typed_AST_Kind::Subtraction: {
            print_binary_at_indent("-", (Typed_AST_Binary *)node, indent);
        } break;
        case Typed_AST_Kind::While: {
            print_binary_at_indent("while", (Typed_AST_Binary *)node, indent);
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
            print_block_at_indent("block", (Typed_AST_Block *)node, indent);
        } break;
        case Typed_AST_Kind::Comma: {
            print_block_at_indent(",", (Typed_AST_Block *)node, indent);
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
            print_sub_at_indent("init", let->initializer.get(), indent + 1);
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

struct Scope {
    std::unordered_map<std::string, Value_Type> variables;
};

struct Typer {
    std::list<Scope> scopes;
    
    Scope &current_scope() {
        return scopes.front();
    }
    
    void begin_scope() {
        scopes.push_front({});
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

Ref<Typed_AST> typecheck(Untyped_AST *node) {
    Typer t;
    t.begin_scope();
    return node->typecheck(t);
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
        case Untyped_AST_Kind::Not:
            verify(sub->type.kind == Value_Type_Kind::Bool, "(!) requires operand to be a (bool) but got a (%s).", sub->type.debug_str());
            return make<Typed_AST_Unary>(Typed_AST_Kind::Not, value_types::Bool, std::move(sub));
            
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
        case Untyped_AST_Kind::Assignment:
            verify(lhs->type == rhs->type, "(=) requires both operands to be the same type.");
            verify(lhs->type.is_mut, "Cannot assign to something of type (%s) because it isn't mutable.", lhs->type.debug_str());
            return make<Typed_AST_Binary>(Typed_AST_Kind::Assignment, value_types::None, std::move(lhs), std::move(rhs));
        case Untyped_AST_Kind::Division:
            verify(lhs->type.kind == rhs->type.kind, "(/) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int ||
                   lhs->type.kind == Value_Type_Kind::Float, "(/) requires operands to be either (int) or (float) but was given (%s).", lhs->type.debug_str());
            verify(rhs->type.kind == Value_Type_Kind::Int ||
                   rhs->type.kind == Value_Type_Kind::Float, "(/) requires operands to be either (int) or (float) but was given (%s).", rhs->type.debug_str());
            return make<Typed_AST_Binary>(Typed_AST_Kind::Division, lhs->type, std::move(lhs), std::move(rhs));
        case Untyped_AST_Kind::Equal:
            verify(lhs->type == rhs->type, "(==) requires both operands to be the same type.");
            return make<Typed_AST_Binary>(Typed_AST_Kind::Equal, value_types::Bool, std::move(lhs), std::move(rhs));
        case Untyped_AST_Kind::Mod:
            verify(lhs->type.kind == rhs->type.kind, "(+) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int, "(+) requires operands to be (int) but was given (%s).", lhs->type.debug_str());
            verify(rhs->type.kind == Value_Type_Kind::Int, "(+) requires operands to be (int) but was given (%s).", rhs->type.debug_str());
            return make<Typed_AST_Binary>(Typed_AST_Kind::Mod, value_types::Int, std::move(lhs), std::move(rhs));
        case Untyped_AST_Kind::Multiplication:
            verify(lhs->type.kind == rhs->type.kind, "(*) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int ||
                   lhs->type.kind == Value_Type_Kind::Float, "(*) requires operands to be either (int) or (float) but was given (%s).", lhs->type.debug_str());
            verify(rhs->type.kind == Value_Type_Kind::Int ||
                   rhs->type.kind == Value_Type_Kind::Float, "(*) requires operands to be either (int) or (float) but was given (%s).", rhs->type.debug_str());
            return make<Typed_AST_Binary>(Typed_AST_Kind::Multiplication, lhs->type, std::move(lhs), std::move(rhs));
        case Untyped_AST_Kind::Subtraction:
            verify(lhs->type.kind == rhs->type.kind, "(-) requires both operands to be the same type.");
            verify(lhs->type.kind == Value_Type_Kind::Int ||
                   lhs->type.kind == Value_Type_Kind::Float, "(-) requires operands to be either (int) or (float) but was given (%s).", lhs->type.debug_str());
            verify(rhs->type.kind == Value_Type_Kind::Int ||
                   rhs->type.kind == Value_Type_Kind::Float, "(-) requires operands to be either (int) or (float) but was given (%s).", rhs->type.debug_str());
            return make<Typed_AST_Binary>(Typed_AST_Kind::Subtraction, lhs->type, std::move(lhs), std::move(rhs));
        case Untyped_AST_Kind::While:
            verify(lhs->type.kind == Value_Type_Kind::Bool, "(while) requires condition to be (bool) but was given (%s).", lhs->type.debug_str());
            return make<Typed_AST_Binary>(Typed_AST_Kind::While, value_types::None, std::move(lhs), std::move(rhs));
            
        default:
            assert(false);
            return nullptr;
    }
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

Ref<Typed_AST> Untyped_AST_Block::typecheck(Typer &t) {
    auto block = make<Typed_AST_Block>(Typed_AST_Kind::Block);
    for (auto &node : nodes) {
        block->add(node->typecheck(t));
    }
    return block;
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
    auto init = initializer->typecheck(t);
    
    Ref<Typed_AST_Type_Signiture> ty = nullptr;
    if (specified_type) {
        verify(*specified_type->value_type == init->type, "Specified type (%s) does not match given type (%s).", ty->value_type->debug_str(), init->type.debug_str());
        ty = cast<Typed_AST_Type_Signiture>(specified_type->typecheck(t));
        assert(ty);
    }

    t.put_variable(id.c_str(), init->type, is_mut);
    
    return make<Typed_AST_Let>(id.clone(), is_mut, std::move(ty), std::move(init));
}
