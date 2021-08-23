//
//  ast.cpp
//  Fox
//
//  Created by Denver Lacey on 7/7/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#include "ast.h"
#include "typer.h"

Untyped_AST_Bool::Untyped_AST_Bool(bool value) {
    kind = Untyped_AST_Kind::Bool;
    this->value = value;
}

Untyped_AST_Char::Untyped_AST_Char(char32_t value) {
    kind = Untyped_AST_Kind::Char;
    this->value = value;
}

Untyped_AST_Float::Untyped_AST_Float(double value) {
    kind = Untyped_AST_Kind::Float;
    this->value = value;
}

Untyped_AST_Ident::Untyped_AST_Ident(String id) {
    kind = Untyped_AST_Kind::Ident;
    this->id = id;
}

Untyped_AST_Ident::~Untyped_AST_Ident() {
    id.free();
}

Untyped_AST_Int::Untyped_AST_Int(int64_t value) {
    kind = Untyped_AST_Kind::Int;
    this->value = value;
}

Untyped_AST_Str::Untyped_AST_Str(String value) {
    kind = Untyped_AST_Kind::Str;
    this->value = value;
}

Untyped_AST_Str::~Untyped_AST_Str() {
    value.free();
}

Untyped_AST_Unary::Untyped_AST_Unary(Untyped_AST_Kind kind, Ref<Untyped_AST> sub) {
    this->kind = kind;
    this->sub = std::move(sub);
}

Untyped_AST_Binary::Untyped_AST_Binary(Untyped_AST_Kind kind, Ref<Untyped_AST> lhs, Ref<Untyped_AST> rhs) {
    this->kind = kind;
    this->lhs = std::move(lhs);
    this->rhs = std::move(rhs);
}

Untyped_AST_Ternary::Untyped_AST_Ternary(Untyped_AST_Kind kind, Ref<Untyped_AST> lhs, Ref<Untyped_AST> mid, Ref<Untyped_AST> rhs) {
    this->kind = kind;
    this->lhs = std::move(lhs);
    this->mid = std::move(mid);
    this->rhs = std::move(rhs);
}

Untyped_AST_Block::Untyped_AST_Block(Untyped_AST_Kind kind) {
    this->kind = kind;
}

void Untyped_AST_Block::add(Ref<Untyped_AST> node) {
    nodes.push_back(std::move(node));
}

Untyped_AST_Type_Signiture::Untyped_AST_Type_Signiture(Ref<Value_Type> value_type) {
    this->kind = Untyped_AST_Kind::Type_Signiture;
    this->value_type = std::move(value_type);
}

Untyped_AST_Let::Untyped_AST_Let(
                     String id,
                     bool is_mut,
                     Ref<Untyped_AST_Type_Signiture> specified_type,
                     Ref<Untyped_AST> initializer)
{
    kind = Untyped_AST_Kind::Let;
    this->id = id;
    this->is_mut = is_mut;
    this->specified_type = std::move(specified_type);
    this->initializer = std::move(initializer);
}

Untyped_AST_Let::~Untyped_AST_Let() {
    id.free();
}

constexpr size_t INDENT_SIZE = 2;
static void print_at_indent(const Untyped_AST *node, size_t indent);

static void print_sub_at_indent(const char *name, const Untyped_AST *sub, size_t indent) {
    printf("%*s%s: ", indent * INDENT_SIZE, "", name);
    print_at_indent(sub, indent);
}

static void print_unary_at_indent(const char *id, const Untyped_AST_Unary *u, size_t indent) {
    printf("(%s)\n", id);
    print_sub_at_indent("sub", u->sub.get(), indent + 1);
}

static void print_binary_at_indent(const char *id, const Untyped_AST_Binary *b, size_t indent) {
    printf("(%s)\n", id);
    print_sub_at_indent("lhs", b->lhs.get(), indent + 1);
    print_sub_at_indent("rhs", b->rhs.get(), indent + 1);
}

static void print_ternary_at_indent(const char *id, const Untyped_AST_Ternary *t, size_t indent) {
    printf("(%s)\n", id);
    print_sub_at_indent("lhs", t->lhs.get(), indent + 1);
    print_sub_at_indent("mid", t->mid.get(), indent + 1);
    print_sub_at_indent("rhs", t->rhs.get(), indent + 1);
}

static void print_block_at_indent(const char *id, const Untyped_AST_Block *b, size_t indent) {
    printf("(%s)\n", id);
    for (size_t i = 0; i < b->nodes.size(); i++) {
        const Untyped_AST *node = b->nodes[i].get();
        printf("%*s%zu: ", (indent + 1) * INDENT_SIZE, "", i);
        print_at_indent(node, indent + 1);
    }
}

static void print_at_indent(const Untyped_AST *node, size_t indent) {
    switch (node->kind) {
        case Untyped_AST_Kind::Bool: {
            Untyped_AST_Bool *lit = (Untyped_AST_Bool *)node;
            printf("%s\n", lit->value ? "true" : "false");
        } break;
        case Untyped_AST_Kind::Char: {
            Untyped_AST_Char *lit = (Untyped_AST_Char *)node;
            printf("%s\n", utf8char_t::from_char32(lit->value).buf);
        } break;
        case Untyped_AST_Kind::Float: {
            Untyped_AST_Float *lit = (Untyped_AST_Float *)node;
            printf("%f\n", lit->value);
        } break;
        case Untyped_AST_Kind::Ident: {
            Untyped_AST_Ident *id = (Untyped_AST_Ident *)node;
            printf("%.*s\n", id->id.size(), id->id.c_str());
        } break;
        case Untyped_AST_Kind::Int: {
            Untyped_AST_Int *lit = (Untyped_AST_Int *)node;
            printf("%lld\n", lit->value);
        } break;
        case Untyped_AST_Kind::Str: {
            Untyped_AST_Str *lit = (Untyped_AST_Str *)node;
            printf("%.*s\n", lit->value.size(), lit->value.c_str());
        } break;
        case Untyped_AST_Kind::Not: {
            print_unary_at_indent("!", (Untyped_AST_Unary *)node, indent);
        } break;
        case Untyped_AST_Kind::Addition: {
            print_binary_at_indent("+", (Untyped_AST_Binary *)node, indent);
        } break;
        case Untyped_AST_Kind::Assignment: {
            print_binary_at_indent("=", (Untyped_AST_Binary *)node, indent);
        } break;
        case Untyped_AST_Kind::Division: {
            print_binary_at_indent("/", (Untyped_AST_Binary *)node, indent);
        } break;
        case Untyped_AST_Kind::Equal: {
            print_binary_at_indent("==", (Untyped_AST_Binary *)node, indent);
        } break;
        case Untyped_AST_Kind::Mod: {
            print_binary_at_indent("%", (Untyped_AST_Binary *)node, indent);
        } break;
        case Untyped_AST_Kind::Multiplication: {
            print_binary_at_indent("*", (Untyped_AST_Binary *)node, indent);
        } break;
        case Untyped_AST_Kind::Subtraction: {
            print_binary_at_indent("-", (Untyped_AST_Binary *)node, indent);
        } break;
        case Untyped_AST_Kind::While: {
            print_binary_at_indent("while", (Untyped_AST_Binary *)node, indent);
        } break;
        case Untyped_AST_Kind::If: {
            Untyped_AST_Ternary *t = (Untyped_AST_Ternary *)node;
            printf("(if)\n");
            print_sub_at_indent("cond", t->lhs.get(), indent + 1);
            print_sub_at_indent("then", t->mid.get(), indent + 1);
            if (t->rhs) {
                print_sub_at_indent("else", t->rhs.get(), indent + 1);
            }
        } break;
        case Untyped_AST_Kind::Block: {
            print_block_at_indent("block", (Untyped_AST_Block *)node, indent);
        } break;
        case Untyped_AST_Kind::Comma: {
            print_block_at_indent(",", (Untyped_AST_Block *)node, indent);
        } break;
        case Untyped_AST_Kind::Let: {
            Untyped_AST_Let *let = (Untyped_AST_Let *)node;
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
        case Untyped_AST_Kind::Type_Signiture: {
            Untyped_AST_Type_Signiture *sig = (Untyped_AST_Type_Signiture *)node;
            printf("%s\n", sig->value_type->debug_str());
        } break;
            
        default:
            assert(false);
            break;
    }
}

void Untyped_AST::print() const {
    print_at_indent(this, 0);
}
