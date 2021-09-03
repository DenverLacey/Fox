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

Ref<Untyped_AST> Untyped_AST_Bool::clone() {
    return make<Untyped_AST_Bool>(value);
}

Untyped_AST_Char::Untyped_AST_Char(char32_t value) {
    kind = Untyped_AST_Kind::Char;
    this->value = value;
}

Ref<Untyped_AST> Untyped_AST_Char::clone() {
    return make<Untyped_AST_Char>(value);
}

Untyped_AST_Float::Untyped_AST_Float(double value) {
    kind = Untyped_AST_Kind::Float;
    this->value = value;
}

Ref<Untyped_AST> Untyped_AST_Float::clone() {
    return make<Untyped_AST_Float>(value);
}

Untyped_AST_Ident::Untyped_AST_Ident(String id) {
    kind = Untyped_AST_Kind::Ident;
    this->id = id;
}

Untyped_AST_Ident::~Untyped_AST_Ident() {
    id.free();
}

Ref<Untyped_AST> Untyped_AST_Ident::clone() {
    return make<Untyped_AST_Ident>(id.clone());
}

Untyped_AST_Int::Untyped_AST_Int(int64_t value) {
    kind = Untyped_AST_Kind::Int;
    this->value = value;
}

Ref<Untyped_AST> Untyped_AST_Int::clone() {
    return make<Untyped_AST_Int>(value);
}

Untyped_AST_Str::Untyped_AST_Str(String value) {
    kind = Untyped_AST_Kind::Str;
    this->value = value;
}

Untyped_AST_Str::~Untyped_AST_Str() {
    value.free();
}

Ref<Untyped_AST> Untyped_AST_Str::clone() {
    return make<Untyped_AST_Str>(value.clone());
}

Untyped_AST_Unary::Untyped_AST_Unary(Untyped_AST_Kind kind, Ref<Untyped_AST> sub) {
    this->kind = kind;
    this->sub = sub;
}

Ref<Untyped_AST> Untyped_AST_Unary::clone() {
    return make<Untyped_AST_Unary>(kind, sub->clone());
}

Untyped_AST_Binary::Untyped_AST_Binary(Untyped_AST_Kind kind, Ref<Untyped_AST> lhs, Ref<Untyped_AST> rhs) {
    this->kind = kind;
    this->lhs = lhs;
    this->rhs = rhs;
}

Ref<Untyped_AST> Untyped_AST_Binary::clone() {
    return make<Untyped_AST_Binary>(kind, lhs->clone(), rhs->clone());
}

Untyped_AST_Ternary::Untyped_AST_Ternary(
    Untyped_AST_Kind kind,
    Ref<Untyped_AST> lhs,
    Ref<Untyped_AST> mid,
    Ref<Untyped_AST> rhs)
{
    this->kind = kind;
    this->lhs = lhs;
    this->mid = mid;
    this->rhs = rhs;
}

Ref<Untyped_AST> Untyped_AST_Ternary::clone() {
    return make<Untyped_AST_Ternary>(kind, lhs->clone(), mid->clone(), rhs->clone());
}

Untyped_AST_Multiary::Untyped_AST_Multiary(Untyped_AST_Kind kind) {
    this->kind = kind;
}

void Untyped_AST_Multiary::add(Ref<Untyped_AST> node) {
    nodes.push_back(node);
}

Ref<Untyped_AST> Untyped_AST_Multiary::clone() {
    auto block = make<Untyped_AST_Multiary>(kind);
    for (auto &n : nodes) {
        block->add(n->clone());
    }
    return block;
}

Untyped_AST_Type_Signiture::Untyped_AST_Type_Signiture(Ref<Value_Type> value_type) {
    this->kind = Untyped_AST_Kind::Type_Signiture;
    this->value_type = value_type;
}

Ref<Untyped_AST> Untyped_AST_Type_Signiture::clone() {
    Ref<Value_Type> type = make<Value_Type>();
    *type = value_type->clone();
    return make<Untyped_AST_Type_Signiture>(type);
}

Untyped_AST_Array::Untyped_AST_Array(
    Untyped_AST_Kind kind,
    size_t count,
    Ref<Value_Type> array_type,
    Ref<Untyped_AST_Multiary> element_nodes)
{
    this->kind = kind;
    this->count = count;
    this->array_type = array_type;
    this->element_nodes = element_nodes;
}

Ref<Untyped_AST> Untyped_AST_Array::clone() {
    Ref<Value_Type> type = make<Value_Type>();
    *type = array_type->clone();
    return make<Untyped_AST_Array>(
        kind,
        count,
        type,
        element_nodes->clone().cast<Untyped_AST_Multiary>()
    );
}

Untyped_AST_If::Untyped_AST_If(
    Ref<Untyped_AST> cond,
    Ref<Untyped_AST> then,
    Ref<Untyped_AST> else_)
{
    this->kind = Untyped_AST_Kind::If;
    this->cond = cond;
    this->then = then;
    this->else_ = else_;
}

Ref<Untyped_AST> Untyped_AST_If::clone() {
    auto cond = this->cond->clone();
    auto then = this->then->clone();
    auto else_ = this->else_ ? this->else_->clone() : nullptr;
    return make<Untyped_AST_If>(cond, then, else_);
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
    this->specified_type = specified_type;
    this->initializer = initializer;
}

Untyped_AST_Let::~Untyped_AST_Let() {
    id.free();
}

Ref<Untyped_AST> Untyped_AST_Let::clone() {
    auto sig = specified_type ? specified_type->clone().cast<Untyped_AST_Type_Signiture>() : nullptr;
    return make<Untyped_AST_Let>(id.clone(), is_mut, sig, initializer->clone());
}

constexpr size_t INDENT_SIZE = 2;
static void print_at_indent(const Ref<Untyped_AST> node, size_t indent);

static void print_sub_at_indent(const char *name, const Ref<Untyped_AST> sub, size_t indent) {
    printf("%*s%s: ", indent * INDENT_SIZE, "", name);
    print_at_indent(sub, indent);
}

static void print_unary_at_indent(const char *id, const Ref<Untyped_AST_Unary> u, size_t indent) {
    printf("(%s)\n", id);
    print_sub_at_indent("sub", u->sub, indent + 1);
}

static void print_binary_at_indent(const char *id, const Ref<Untyped_AST_Binary> b, size_t indent) {
    printf("(%s)\n", id);
    print_sub_at_indent("lhs", b->lhs, indent + 1);
    print_sub_at_indent("rhs", b->rhs, indent + 1);
}

static void print_ternary_at_indent(const char *id, const Ref<Untyped_AST_Ternary> t, size_t indent) {
    printf("(%s)\n", id);
    print_sub_at_indent("lhs", t->lhs, indent + 1);
    print_sub_at_indent("mid", t->mid, indent + 1);
    print_sub_at_indent("rhs", t->rhs, indent + 1);
}

static void print_multiary_at_indent(const char *id, const Ref<Untyped_AST_Multiary> b, size_t indent) {
    printf("(%s)\n", id);
    for (size_t i = 0; i < b->nodes.size(); i++) {
        const Ref<Untyped_AST> node = b->nodes[i];
        printf("%*s%zu: ", (indent + 1) * INDENT_SIZE, "", i);
        print_at_indent(node, indent + 1);
    }
}

static void print_at_indent(const Ref<Untyped_AST> node, size_t indent) {
    switch (node->kind) {
        case Untyped_AST_Kind::Bool: {
            Ref<Untyped_AST_Bool> lit = node.cast<Untyped_AST_Bool>();
            printf("%s\n", lit->value ? "true" : "false");
        } break;
        case Untyped_AST_Kind::Char: {
            Ref<Untyped_AST_Char> lit = node.cast<Untyped_AST_Char>();
            printf("'%s'\n", utf8char_t::from_char32(lit->value).buf);
        } break;
        case Untyped_AST_Kind::Float: {
            Ref<Untyped_AST_Float> lit = node.cast<Untyped_AST_Float>();
            printf("%f\n", lit->value);
        } break;
        case Untyped_AST_Kind::Ident: {
            Ref<Untyped_AST_Ident> id = node.cast<Untyped_AST_Ident>();
            printf("%.*s\n", id->id.size(), id->id.c_str());
        } break;
        case Untyped_AST_Kind::Int: {
            Ref<Untyped_AST_Int> lit = node.cast<Untyped_AST_Int>();
            printf("%lld\n", lit->value);
        } break;
        case Untyped_AST_Kind::Str: {
            Ref<Untyped_AST_Str> lit = node.cast<Untyped_AST_Str>();
            printf("\"%.*s\"\n", lit->value.size(), lit->value.c_str());
        } break;
        case Untyped_AST_Kind::Negation: {
            print_unary_at_indent("-", node.cast<Untyped_AST_Unary>(), indent);
        } break;
        case Untyped_AST_Kind::Not: {
            print_unary_at_indent("!", node.cast<Untyped_AST_Unary>(), indent);
        } break;
        case Untyped_AST_Kind::Address_Of: {
            print_unary_at_indent("&", node.cast<Untyped_AST_Unary>(), indent);
        } break;
        case Untyped_AST_Kind::Address_Of_Mut: {
            print_unary_at_indent("&mut", node.cast<Untyped_AST_Unary>(), indent);
        } break;
        case Untyped_AST_Kind::Deref: {
            print_unary_at_indent("*", node.cast<Untyped_AST_Unary>(), indent);
        } break;
        case Untyped_AST_Kind::Addition: {
            print_binary_at_indent("+", node.cast<Untyped_AST_Binary>(), indent);
        } break;
        case Untyped_AST_Kind::Subtraction: {
            print_binary_at_indent("-", node.cast<Untyped_AST_Binary>(), indent);
        } break;
        case Untyped_AST_Kind::Multiplication: {
            print_binary_at_indent("*", node.cast<Untyped_AST_Binary>(), indent);
        } break;
        case Untyped_AST_Kind::Division: {
            print_binary_at_indent("/", node.cast<Untyped_AST_Binary>(), indent);
        } break;
        case Untyped_AST_Kind::Mod: {
            print_binary_at_indent("%", node.cast<Untyped_AST_Binary>(), indent);
        } break;
        case Untyped_AST_Kind::Assignment: {
            print_binary_at_indent("=", node.cast<Untyped_AST_Binary>(), indent);
        } break;
        case Untyped_AST_Kind::Equal: {
            print_binary_at_indent("==", node.cast<Untyped_AST_Binary>(), indent);
        } break;
        case Untyped_AST_Kind::Not_Equal: {
            print_binary_at_indent("!=", node.cast<Untyped_AST_Binary>(), indent);
        } break;
        case Untyped_AST_Kind::Less: {
            print_binary_at_indent("<", node.cast<Untyped_AST_Binary>(), indent);
        } break;
        case Untyped_AST_Kind::Less_Eq: {
            print_binary_at_indent("<=", node.cast<Untyped_AST_Binary>(), indent);
        } break;
        case Untyped_AST_Kind::Greater: {
            print_binary_at_indent(">", node.cast<Untyped_AST_Binary>(), indent);
        } break;
        case Untyped_AST_Kind::Greater_Eq: {
            print_binary_at_indent(">=", node.cast<Untyped_AST_Binary>(), indent);
        } break;
        case Untyped_AST_Kind::While: {
            print_binary_at_indent("while", node.cast<Untyped_AST_Binary>(), indent);
        } break;
        case Untyped_AST_Kind::And: {
            print_binary_at_indent("and", node.cast<Untyped_AST_Binary>(), indent);
        } break;
        case Untyped_AST_Kind::Or: {
            print_binary_at_indent("or", node.cast<Untyped_AST_Binary>(), indent);
        } break;
        case Untyped_AST_Kind::Dot:
        case Untyped_AST_Kind::Dot_Tuple: {
            print_binary_at_indent(".", node.cast<Untyped_AST_Binary>(), indent);
        } break;
        case Untyped_AST_Kind::If: {
            Ref<Untyped_AST_If> t = node.cast<Untyped_AST_If>();
            printf("(if)\n");
            print_sub_at_indent("cond", t->cond, indent + 1);
            print_sub_at_indent("then", t->then, indent + 1);
            if (t->else_) {
                print_sub_at_indent("else", t->else_, indent + 1);
            }
        } break;
        case Untyped_AST_Kind::Block: {
            print_multiary_at_indent("block", node.cast<Untyped_AST_Multiary>(), indent);
        } break;
        case Untyped_AST_Kind::Comma: {
            print_multiary_at_indent(",", node.cast<Untyped_AST_Multiary>(), indent);
        } break;
        case Untyped_AST_Kind::Tuple: {
            print_multiary_at_indent("tuple", node.cast<Untyped_AST_Multiary>(), indent);
        } break;
        case Untyped_AST_Kind::Let: {
            Ref<Untyped_AST_Let> let = node.cast<Untyped_AST_Let>();
            if (let->is_mut) {
                printf("(let mut)\n");
            } else {
                printf("(let)\n");
            }
            printf("%*sid: `%.*s`\n", (indent + 1) * INDENT_SIZE, "", let->id.size(), let->id.c_str());
            if (let->specified_type) {
                print_sub_at_indent("type", let->specified_type, indent + 1);
            }
            if (let->initializer) {
                print_sub_at_indent("init", let->initializer, indent + 1);
            }
        } break;
        case Untyped_AST_Kind::Type_Signiture: {
            Ref<Untyped_AST_Type_Signiture> sig = node.cast<Untyped_AST_Type_Signiture>();
            printf("%s\n", sig->value_type->debug_str());
        } break;
        case Untyped_AST_Kind::Array:
        case Untyped_AST_Kind::Slice: {
            Ref<Untyped_AST_Array> array = node.cast<Untyped_AST_Array>();
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

void Untyped_AST::print() const {
    print_at_indent(Ref<Untyped_AST>((Untyped_AST *)this), 0);
}
