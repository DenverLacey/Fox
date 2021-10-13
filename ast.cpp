//
//  ast.cpp
//  Fox
//
//  Created by Denver Lacey on 7/7/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#include "ast.h"
#include "error.h"
#include "typer.h"

#include <sstream>

Untyped_AST_Bool::Untyped_AST_Bool(bool value) {
    kind = Untyped_AST_Kind::Bool;
    this->value = value;
}

Ref<Untyped_AST> Untyped_AST_Bool::clone() {
    return Mem.make<Untyped_AST_Bool>(value);
}

Untyped_AST_Char::Untyped_AST_Char(char32_t value) {
    kind = Untyped_AST_Kind::Char;
    this->value = value;
}

Ref<Untyped_AST> Untyped_AST_Char::clone() {
    return Mem.make<Untyped_AST_Char>(value);
}

Untyped_AST_Float::Untyped_AST_Float(double value) {
    kind = Untyped_AST_Kind::Float;
    this->value = value;
}

Ref<Untyped_AST> Untyped_AST_Float::clone() {
    return Mem.make<Untyped_AST_Float>(value);
}

const char *Untyped_AST_Symbol::display_str() const {
    const char *str;
    switch (kind) {
        case Untyped_AST_Kind::Ident: {
            auto id = dynamic_cast<const Untyped_AST_Ident *>(this);
            internal_verify(id, "Failed to cast to Ident* in Untyped_AST_Symbol::debug_str().");
            
            str = id->id.c_str();
        } break;
        case Untyped_AST_Kind::Path: {
            auto path = dynamic_cast<const Untyped_AST_Path *>(this);
            internal_verify(path, "Failed to cast to Path* in Untyped_AST_Symbol::debug_str().");
            
            std::ostringstream s;
            s << path->lhs->display_str() << "::" << path->rhs->display_str();
            
            std::string cpp_str = s.str();
            str = SMem.duplicate(cpp_str.c_str(), cpp_str.size());
        } break;
            
        default:
            internal_error("Invalid symbol kind: %d.", kind);
            break;
    }
    
    return str;
}

Untyped_AST_Ident::Untyped_AST_Ident(String id) {
    kind = Untyped_AST_Kind::Ident;
    this->id = id;
}

Untyped_AST_Ident::~Untyped_AST_Ident() {
    id.free();
}

Ref<Untyped_AST> Untyped_AST_Ident::clone() {
    return Mem.make<Untyped_AST_Ident>(id.clone());
}

Untyped_AST_Path::Untyped_AST_Path(
    Ref<Untyped_AST_Ident> lhs,
    Ref<Untyped_AST_Symbol> rhs)
{
    this->kind = Untyped_AST_Kind::Path;
    this->lhs = lhs;
    this->rhs = rhs;
}

Ref<Untyped_AST> Untyped_AST_Path::clone() {
    return Mem.make<Untyped_AST_Path>(
        lhs->clone().cast<Untyped_AST_Ident>(),
        rhs->clone().cast<Untyped_AST_Symbol>()
    );
}

Untyped_AST_Int::Untyped_AST_Int(int64_t value) {
    kind = Untyped_AST_Kind::Int;
    this->value = value;
}

Ref<Untyped_AST> Untyped_AST_Int::clone() {
    return Mem.make<Untyped_AST_Int>(value);
}

Untyped_AST_Str::Untyped_AST_Str(String value) {
    kind = Untyped_AST_Kind::Str;
    this->value = value;
}

Untyped_AST_Str::~Untyped_AST_Str() {
    value.free();
}

Ref<Untyped_AST> Untyped_AST_Str::clone() {
    return Mem.make<Untyped_AST_Str>(value.clone());
}

Untyped_AST_Nullary::Untyped_AST_Nullary(Untyped_AST_Kind kind) {
    this->kind = kind;
}

Ref<Untyped_AST> Untyped_AST_Nullary::clone() {
    return Mem.make<Untyped_AST_Nullary>(kind);
}

Untyped_AST_Unary::Untyped_AST_Unary(Untyped_AST_Kind kind, Ref<Untyped_AST> sub) {
    this->kind = kind;
    this->sub = sub;
}

Ref<Untyped_AST> Untyped_AST_Unary::clone() {
    return Mem.make<Untyped_AST_Unary>(kind, sub->clone());
}

Untyped_AST_Return::Untyped_AST_Return(Ref<Untyped_AST> sub)
    : Untyped_AST_Unary(Untyped_AST_Kind::Return, sub)
{
}

Ref<Untyped_AST> Untyped_AST_Return::clone() {
    return Mem.make<Untyped_AST_Return>(sub ? sub->clone() : nullptr);
}

Untyped_AST_Binary::Untyped_AST_Binary(Untyped_AST_Kind kind, Ref<Untyped_AST> lhs, Ref<Untyped_AST> rhs) {
    this->kind = kind;
    this->lhs = lhs;
    this->rhs = rhs;
}

Ref<Untyped_AST> Untyped_AST_Binary::clone() {
    return Mem.make<Untyped_AST_Binary>(kind, lhs->clone(), rhs->clone());
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
    return Mem.make<Untyped_AST_Ternary>(kind, lhs->clone(), mid->clone(), rhs->clone());
}

Untyped_AST_Multiary::Untyped_AST_Multiary(Untyped_AST_Kind kind) {
    this->kind = kind;
}

void Untyped_AST_Multiary::add(Ref<Untyped_AST> node) {
    nodes.push_back(node);
}

Ref<Untyped_AST> Untyped_AST_Multiary::clone() {
    auto block = Mem.make<Untyped_AST_Multiary>(kind);
    for (auto &n : nodes) {
        block->add(n->clone());
    }
    return block;
}

Untyped_AST_Type_Signature::Untyped_AST_Type_Signature(Ref<Value_Type> value_type) {
    this->kind = Untyped_AST_Kind::Type_Signature;
    this->value_type = value_type;
}

Ref<Untyped_AST> Untyped_AST_Type_Signature::clone() {
    Ref<Value_Type> type = Mem.make<Value_Type>();
    *type = value_type->clone();
    return Mem.make<Untyped_AST_Type_Signature>(type);
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
    Ref<Value_Type> type = Mem.make<Value_Type>();
    *type = array_type->clone();
    return Mem.make<Untyped_AST_Array>(
        kind,
        count,
        type,
        element_nodes->clone().cast<Untyped_AST_Multiary>()
    );
}

Untyped_AST_Struct_Literal::Untyped_AST_Struct_Literal(
    Ref<Untyped_AST_Symbol> struct_id,
    Ref<Untyped_AST_Multiary> bindings)
{
    this->kind = Untyped_AST_Kind::Struct;
    this->struct_id = struct_id;
    this->bindings = bindings;
}

Ref<Untyped_AST> Untyped_AST_Struct_Literal::clone() {
    return Mem.make<Untyped_AST_Struct_Literal>(
        struct_id->clone().cast<Untyped_AST_Symbol>(),
        bindings->clone().cast<Untyped_AST_Multiary>()
    );
}

Untyped_AST_Field_Access::Untyped_AST_Field_Access(
    Ref<Untyped_AST> instance,
    String field_id)
{
    this->kind = Untyped_AST_Kind::Field_Access;
    this->instance = instance;
    this->field_id = field_id;
}

Untyped_AST_Field_Access::~Untyped_AST_Field_Access() {
    field_id.free();
}

Ref<Untyped_AST> Untyped_AST_Field_Access::clone() {
    return Mem.make<Untyped_AST_Field_Access>(instance->clone(), field_id.clone());
}

bool Untyped_AST_Pattern::are_all_variables_mut() {
    bool is_mut;
    switch (kind) {
        case Untyped_AST_Kind::Pattern_Underscore:
            is_mut = true;
            break;
        case Untyped_AST_Kind::Pattern_Ident: {
            auto ip = dynamic_cast<Untyped_AST_Pattern_Ident *>(this);
            internal_verify(ip, "Failed to cast pattern to Pattern_Ident*");
            is_mut = ip->is_mut;
        } break;
        case Untyped_AST_Kind::Pattern_Tuple: {
            auto tp = dynamic_cast<Untyped_AST_Pattern_Tuple *>(this);
            internal_verify(tp, "Failed to cast pattern to Pattern_Tuple*");
            is_mut = true;
            for (auto sub : tp->sub_patterns) {
                is_mut = sub->are_all_variables_mut();
                if (!is_mut) break;
            }
        } break;
            
        default:
            internal_error("Invalid pattern kind: %d.", kind);
            break;
    }
    return is_mut;
}

bool Untyped_AST_Pattern::are_no_variables_mut() {
    bool not_mut;
    switch (kind) {
        case Untyped_AST_Kind::Pattern_Underscore:
            not_mut = true;
            break;
        case Untyped_AST_Kind::Pattern_Ident: {
            auto ip = dynamic_cast<Untyped_AST_Pattern_Ident *>(this);
            internal_verify(ip, "Failed to cast pattern to Pattern_Ident*");
            not_mut = !ip->is_mut;
        } break;
        case Untyped_AST_Kind::Pattern_Tuple: {
            auto tp = dynamic_cast<Untyped_AST_Pattern_Tuple *>(this);
            internal_verify(tp, "Failed to cast pattern to Pattern_Tuple*");
            not_mut = true;
            for (auto sub : tp->sub_patterns) {
                not_mut = sub->are_no_variables_mut();
                if (!not_mut) break;
            }
        } break;
            
        default:
            internal_error("Invalid pattern kind: %d.", kind);
            break;
    }
    return not_mut;
}

Untyped_AST_Pattern_Underscore::Untyped_AST_Pattern_Underscore() {
    kind = Untyped_AST_Kind::Pattern_Underscore;
}

Ref<Untyped_AST> Untyped_AST_Pattern_Underscore::clone() {
    return Mem.make<Untyped_AST_Pattern_Underscore>();
}

Untyped_AST_Pattern_Ident::Untyped_AST_Pattern_Ident(bool is_mut, String id) {
    kind = Untyped_AST_Kind::Pattern_Ident;
    this->is_mut = is_mut;
    this->id = id;
}

Untyped_AST_Pattern_Ident::~Untyped_AST_Pattern_Ident() {
    id.free();
}

Ref<Untyped_AST> Untyped_AST_Pattern_Ident::clone() {
    return Mem.make<Untyped_AST_Pattern_Ident>(is_mut, id.clone());
}

Untyped_AST_Pattern_Tuple::Untyped_AST_Pattern_Tuple() {
    kind = Untyped_AST_Kind::Pattern_Tuple;
}

Ref<Untyped_AST> Untyped_AST_Pattern_Tuple::clone() {
    auto copy = Mem.make<Untyped_AST_Pattern_Tuple>();
    for (auto sub : sub_patterns) {
        copy->add(sub->clone().cast<Untyped_AST_Pattern>());
    }
    return copy;
}

void Untyped_AST_Pattern_Tuple::add(Ref<Untyped_AST_Pattern> sub) {
    sub_patterns.push_back(sub);
}

Untyped_AST_Pattern_Struct::Untyped_AST_Pattern_Struct(
    Ref<Untyped_AST_Symbol> struct_id)
{
    this->kind = Untyped_AST_Kind::Pattern_Struct;
    this->struct_id = struct_id;
}

Ref<Untyped_AST> Untyped_AST_Pattern_Struct::clone() {
    auto copy = Mem.make<Untyped_AST_Pattern_Struct>(struct_id->clone().cast<Untyped_AST_Ident>());
    for (auto s : sub_patterns) {
        copy->add(s->clone().cast<Untyped_AST_Pattern>());
    }
    return copy;
}

Untyped_AST_Pattern_Enum::Untyped_AST_Pattern_Enum(Ref<Untyped_AST_Symbol> enum_id) {
    this->kind = Untyped_AST_Kind::Pattern_Enum;
    this->enum_id = enum_id;
}

Ref<Untyped_AST> Untyped_AST_Pattern_Enum::clone() {
    auto copy = Mem.make<Untyped_AST_Pattern_Enum>(enum_id->clone().cast<Untyped_AST_Ident>());
    for (auto s : sub_patterns) {
        copy->add(s->clone().cast<Untyped_AST_Pattern>());
    }
    return copy;
}

Untyped_AST_Pattern_Value::Untyped_AST_Pattern_Value(Ref<Untyped_AST> value) {
    this->kind = Untyped_AST_Kind::Pattern_Value;
    this->value = value;
}

Ref<Untyped_AST> Untyped_AST_Pattern_Value::clone() {
    return Mem.make<Untyped_AST_Pattern_Value>(value->clone());
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
    return Mem.make<Untyped_AST_If>(cond, then, else_);
}

Untyped_AST_For::Untyped_AST_For(
    Ref<Untyped_AST_Pattern> target,
    String counter,
    Ref<Untyped_AST> iterable,
    Ref<Untyped_AST_Multiary> body)
{
    this->kind = Untyped_AST_Kind::For;
    this->target = target;
    this->counter = counter;
    this->iterable = iterable;
    this->body = body;
}

Untyped_AST_For::~Untyped_AST_For() {
    counter.free();
}

Ref<Untyped_AST> Untyped_AST_For::clone() {
    return Mem.make<Untyped_AST_For>(
        target->clone().cast<Untyped_AST_Pattern>(),
        counter.clone(),
        iterable->clone(),
        body->clone().cast<Untyped_AST_Multiary>()
    );
}

Untyped_AST_Match::Untyped_AST_Match(
    Ref<Untyped_AST> cond,
    Ref<Untyped_AST> default_arm,
    Ref<Untyped_AST_Multiary> arms)
{
    this->kind = Untyped_AST_Kind::Match;
    this->cond = cond;
    this->default_arm = default_arm;
    this->arms = arms;
}

Ref<Untyped_AST> Untyped_AST_Match::clone() {
    return Mem.make<Untyped_AST_Match>(
        cond->clone(),
        default_arm->clone(),
        arms->clone().cast<Untyped_AST_Multiary>()
    );
}

Untyped_AST_Let::Untyped_AST_Let(
    bool is_const,
    Ref<Untyped_AST_Pattern> target,
    Ref<Untyped_AST_Type_Signature> specified_type,
    Ref<Untyped_AST> initializer)
{
    kind = Untyped_AST_Kind::Let;
    this->is_const = is_const;
    this->target = target;
    this->specified_type = specified_type;
    this->initializer = initializer;
}

Ref<Untyped_AST> Untyped_AST_Let::clone() {
    auto sig = specified_type ? specified_type->clone().cast<Untyped_AST_Type_Signature>() : nullptr;
    return Mem.make<Untyped_AST_Let>(is_const, target->clone().cast<Untyped_AST_Pattern>(), sig, initializer->clone());
}

Untyped_AST_Generic_Specification::Untyped_AST_Generic_Specification(
    Ref<Untyped_AST_Symbol> id,
    Ref<Untyped_AST_Multiary> type_params)
{
    this->kind = Untyped_AST_Kind::Generic_Specification;
    this->id = id;
    this->type_params = type_params;
}

Ref<Untyped_AST> Untyped_AST_Generic_Specification::clone() {
    return Mem.make<Untyped_AST_Generic_Specification>(
        id->clone().cast<Untyped_AST_Symbol>(),
        type_params->clone().cast<Untyped_AST_Multiary>()
    );
}

Untyped_AST_Struct_Declaration::Untyped_AST_Struct_Declaration(String id) {
    kind = Untyped_AST_Kind::Struct_Decl;
    this->id = id;
}

Untyped_AST_Struct_Declaration::~Untyped_AST_Struct_Declaration() {
    id.free();
}

void Untyped_AST_Struct_Declaration::add_field(
    String id,
    Ref<Untyped_AST_Type_Signature> type)
{
    fields.push_back({ id, type });
}

Ref<Untyped_AST> Untyped_AST_Struct_Declaration::clone() {
    auto copy = Mem.make<Untyped_AST_Struct_Declaration>(id.clone());
    for (auto &f : fields) {
        copy->add_field(f.id.clone(), f.type->clone().cast<Untyped_AST_Type_Signature>());
    }
    return copy;
}

Untyped_AST_Enum_Declaration::Untyped_AST_Enum_Declaration(String id) {
    this->kind = Untyped_AST_Kind::Enum_Decl;
    this->id = id;
}

Untyped_AST_Enum_Declaration::~Untyped_AST_Enum_Declaration() {
    id.free();
}

void Untyped_AST_Enum_Declaration::add_variant(
    String id,
    Ref<Untyped_AST_Multiary> payload)
{
    variants.push_back({ id, payload });
}

Ref<Untyped_AST> Untyped_AST_Enum_Declaration::clone() {
    auto copy = Mem.make<Untyped_AST_Enum_Declaration>(id.clone());
    for (auto &v : variants) {
        copy->add_variant(v.id.clone(), v.payload->clone().cast<Untyped_AST_Multiary>());
    }
    return copy;
}

Untyped_AST_Fn_Declaration::Untyped_AST_Fn_Declaration(
    Untyped_AST_Kind kind,
    String id,
    Ref<Untyped_AST_Multiary> params,
    Ref<Untyped_AST_Type_Signature> return_type_signature,
    Ref<Untyped_AST_Multiary> body)
{
    this->kind = kind;
    this->id = id;
    this->params = params;
    this->return_type_signature = return_type_signature;
    this->body = body;
}

Untyped_AST_Fn_Declaration::~Untyped_AST_Fn_Declaration() {
    id.free();
}

Ref<Untyped_AST> Untyped_AST_Fn_Declaration::clone() {
    return Mem.make<Untyped_AST_Fn_Declaration>(
        kind,
        id.clone(),
        params->clone().cast<Untyped_AST_Multiary>(),
        return_type_signature->clone().cast<Untyped_AST_Type_Signature>(),
        body->clone().cast<Untyped_AST_Multiary>()
    );
}

Untyped_AST_Impl_Declaration::Untyped_AST_Impl_Declaration(
    Ref<Untyped_AST_Symbol> target,
    Ref<Untyped_AST_Symbol> for_,
    Ref<Untyped_AST_Multiary> body)
{
    this->kind = Untyped_AST_Kind::Impl_Decl;
    this->target = target;
    this->for_ = for_;
    this->body = body;
}

Ref<Untyped_AST> Untyped_AST_Impl_Declaration::clone() {
    return Mem.make<Untyped_AST_Impl_Declaration>(
        target->clone().cast<Untyped_AST_Symbol>(),
        for_ ? for_->clone().cast<Untyped_AST_Symbol>() : nullptr,
        body->clone().cast<Untyped_AST_Multiary>()
    );
}

Untyped_AST_Dot_Call::Untyped_AST_Dot_Call(
    Ref<Untyped_AST> receiver,
    String method_id,
    Ref<Untyped_AST_Multiary> args)
{
    this->kind = Untyped_AST_Kind::Dot_Call;
    this->receiver = receiver;
    this->method_id = method_id;
    this->args = args;
}

Untyped_AST_Dot_Call::~Untyped_AST_Dot_Call() {
    method_id.free();
}

Ref<Untyped_AST> Untyped_AST_Dot_Call::clone() {
    return Mem.make<Untyped_AST_Dot_Call>(
        receiver->clone(),
        method_id.clone(),
        args->clone().cast<Untyped_AST_Multiary>()
    );
}

constexpr size_t INDENT_SIZE = 2;
static void print_at_indent(const Ref<Untyped_AST> node, size_t indent);

static void print_sub_at_indent(const char *name, const Ref<Untyped_AST> sub, size_t indent) {
    printf("%*s%s: ", indent * INDENT_SIZE, "", name);
    print_at_indent(sub, indent);
}

static void print_nullary(const char *id) {
    printf("(%s)\n", id);
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

static void print_pattern(Ref<Untyped_AST_Pattern> p) {
    switch (p->kind) {
        case Untyped_AST_Kind::Pattern_Underscore:
            printf("_");
            break;
        case Untyped_AST_Kind::Pattern_Ident: {
            auto ip = p.cast<Untyped_AST_Pattern_Ident>();
            if (ip->is_mut) {
                printf("mut %.*s", ip->id.size(), ip->id.c_str());
            } else {
                printf("%.*s", ip->id.size(), ip->id.c_str());
            }
        } break;
        case Untyped_AST_Kind::Pattern_Tuple: {
            auto t = p.cast<Untyped_AST_Pattern_Tuple>();
            printf("(");
            for (size_t i = 0; i < t->sub_patterns.size(); i++) {
                auto s = t->sub_patterns[i];
                print_pattern(s);
                if (i + 1 < t->sub_patterns.size()) printf(", ");
            }
            printf(")");
        } break;
        case Untyped_AST_Kind::Pattern_Struct: {
            auto s = p.cast<Untyped_AST_Pattern_Struct>();
            printf("%s {", s->struct_id->display_str());
            for (size_t i = 0; i < s->sub_patterns.size(); i++) {
                auto sub = s->sub_patterns[i];
                printf(" ");
                print_pattern(sub);
                if (i + 1 < s->sub_patterns.size()) printf(",");
            }
            printf(" }");
        } break;
        case Untyped_AST_Kind::Pattern_Enum: {
            auto e = p.cast<Untyped_AST_Pattern_Enum>();
            printf("%s(", e->enum_id->display_str());
            for (size_t i = 0; i < e->sub_patterns.size(); i++) {
                auto sub = e->sub_patterns[i];
                print_pattern(sub);
                if (i + 1 < e->sub_patterns.size()) printf(", ");
            }
            printf(")");
        } break;
        case Untyped_AST_Kind::Pattern_Value:
            print_at_indent(p, 0);
            break;
            
        default:
            internal_error("Invalid Kind for Pattern: %d.", p->kind);
            break;
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
        case Untyped_AST_Kind::Path: {
            auto path = node.cast<Untyped_AST_Path>();
            printf("(path)\n");
            print_sub_at_indent("lhs", path->lhs, indent + 1);
            print_sub_at_indent("rhs", path->rhs, indent + 1);
        } break;
        case Untyped_AST_Kind::Int: {
            Ref<Untyped_AST_Int> lit = node.cast<Untyped_AST_Int>();
            printf("%lld\n", lit->value);
        } break;
        case Untyped_AST_Kind::Str: {
            Ref<Untyped_AST_Str> lit = node.cast<Untyped_AST_Str>();
            printf("\"%.*s\"\n", lit->value.size(), lit->value.c_str());
        } break;
        case Untyped_AST_Kind::Struct: {
            auto lit = node.cast<Untyped_AST_Struct_Literal>();
            printf("(struct)\n");
            print_sub_at_indent("struct_id", lit->struct_id, indent + 1);
            print_sub_at_indent("bindings", lit->bindings, indent + 1);
        } break;
        case Untyped_AST_Kind::Noinit: {
            print_nullary("noinit");
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
        case Untyped_AST_Kind::Return: {
            auto ret = node.cast<Untyped_AST_Return>();
            printf("(ret)\n");
            if (ret->sub) {
                print_sub_at_indent("sub", ret->sub, indent + 1);
            } else {
                printf("%*ssub: nullptr\n", (indent + 1) * INDENT_SIZE, "");
            }
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
        case Untyped_AST_Kind::Field_Access: {
            auto dot = node.cast<Untyped_AST_Field_Access>();
            printf("(.)\n");
            print_sub_at_indent("instance", dot->instance, indent + 1);
            printf("%*sfield: %.*s\n", (indent + 1) * INDENT_SIZE, "", dot->field_id.size(), dot->field_id.c_str());
        } break;
        case Untyped_AST_Kind::Field_Access_Tuple: {
            print_binary_at_indent(".", node.cast<Untyped_AST_Binary>(), indent);
        } break;
        case Untyped_AST_Kind::Subscript: {
            print_binary_at_indent("[]", node.cast<Untyped_AST_Binary>(), indent);
        } break;
        case Untyped_AST_Kind::Range: {
            print_binary_at_indent("..", node.cast<Untyped_AST_Binary>(), indent);
        } break;
        case Untyped_AST_Kind::Inclusive_Range: {
            print_binary_at_indent("...", node.cast<Untyped_AST_Binary>(), indent);
        } break;
        case Untyped_AST_Kind::Binding: {
            print_binary_at_indent(":", node.cast<Untyped_AST_Binary>(), indent);
        } break;
        case Untyped_AST_Kind::Invocation: {
            print_binary_at_indent("call", node.cast<Untyped_AST_Binary>(), indent);
        } break;
        case Untyped_AST_Kind::Match_Arm: {
            print_binary_at_indent("arm", node.cast<Untyped_AST_Binary>(), indent);
        } break;
        case Untyped_AST_Kind::Pattern_Underscore:
        case Untyped_AST_Kind::Pattern_Ident:
        case Untyped_AST_Kind::Pattern_Tuple:
        case Untyped_AST_Kind::Pattern_Struct:
        case Untyped_AST_Kind::Pattern_Enum: {
            auto p = node.cast<Untyped_AST_Pattern>();
            print_pattern(p);
            printf("\n");
        } break;
        case Untyped_AST_Kind::Pattern_Value: {
            auto vp = node.cast<Untyped_AST_Pattern_Value>();
            print_at_indent(vp->value, indent);
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
        case Untyped_AST_Kind::For: {
            auto f = node.cast<Untyped_AST_For>();
            printf("(for)\n");
            print_sub_at_indent("target", f->target, indent + 1);
            if (f->counter != "") {
                printf("%*scounter: %s\n", (indent + 1) * INDENT_SIZE, "", f->counter.c_str());
            }
            print_sub_at_indent("iterable", f->iterable, indent + 1);
            print_sub_at_indent("body", f->body, indent + 1);
        } break;
        case Untyped_AST_Kind::Match: {
            auto m = node.cast<Untyped_AST_Match>();
            printf("(match)\n");
            print_sub_at_indent("cond", m->cond, indent + 1);
            if (m->default_arm) {
                print_sub_at_indent("default", m->default_arm, indent + 1);
            }
            print_sub_at_indent("arms", m->arms, indent + 1);
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
            printf("(%s)\n", let->is_const ? "const" : "let");
            
            print_sub_at_indent("target", let->target, indent + 1);
            
            if (let->specified_type) {
                print_sub_at_indent("type", let->specified_type, indent + 1);
            }
            if (let->initializer) {
                print_sub_at_indent("init", let->initializer, indent + 1);
            }
        } break;
        case Untyped_AST_Kind::Type_Signature: {
            Ref<Untyped_AST_Type_Signature> sig = node.cast<Untyped_AST_Type_Signature>();
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
        case Untyped_AST_Kind::Struct_Decl: {
            auto decl = node.cast<Untyped_AST_Struct_Declaration>();
            printf("(struct-decl)\n");
            printf("%*sid: %.*s\n", (indent + 1) * INDENT_SIZE, "", decl->id.size(), decl->id.c_str());
            printf("%*sfields:\n", (indent + 1) * INDENT_SIZE, "");
            for (auto &f : decl->fields) {
                printf("%*s%s: %s\n", (indent + 2) * INDENT_SIZE, "", f.id.c_str(), f.type->value_type->debug_str());
            }
        } break;
        case Untyped_AST_Kind::Enum_Decl: {
            auto decl = node.cast<Untyped_AST_Enum_Declaration>();
            printf("(enum-decl)\n");
            printf("%*sid: %.*s\n", (indent + 1) * INDENT_SIZE, "", decl->id.size(), decl->id.c_str());
            printf("%*svariants:\n", (indent + 1) * INDENT_SIZE, "");
            for (auto &v : decl->variants) {
                if (v.payload) {
                    print_sub_at_indent(v.id.c_str(), v.payload, indent + 2);
                } else {
                    printf("%*s%.*s\n", (indent + 2) * INDENT_SIZE, "", v.id.size(), v.id.c_str());
                }
            }
        } break;
        case Untyped_AST_Kind::Method_Decl:
        case Untyped_AST_Kind::Fn_Decl: {
            auto decl = node.cast<Untyped_AST_Fn_Declaration>();
            printf("(fn-decl)\n");
            printf("%*sid: %.*s\n", (indent + 1) * INDENT_SIZE, "", decl->id.size(), decl->id.c_str());
            print_sub_at_indent("params", decl->params, indent + 1);
            if (decl->return_type_signature) {
                print_sub_at_indent("return", decl->return_type_signature, indent + 1);
            }
            print_sub_at_indent("body", decl->body, indent + 1);
        } break;
        case Untyped_AST_Kind::Impl_Decl: {
            auto decl = node.cast<Untyped_AST_Impl_Declaration>();
            printf("(impl)\n");
            print_sub_at_indent("target", decl->target, indent + 1);
            if (decl->for_) {
                print_sub_at_indent("for", decl->for_, indent + 1);
            }
            print_sub_at_indent("body", decl->body, indent + 1);
        } break;
        case Untyped_AST_Kind::Dot_Call: {
            auto dot = node.cast<Untyped_AST_Dot_Call>();
            printf("(dot-call)\n");
            print_sub_at_indent("receiver", dot->receiver, indent + 1);
            printf("%*smethod: %s\n", (indent + 1) * INDENT_SIZE, "", dot->method_id.c_str());
            print_sub_at_indent("args", dot->args, indent + 1);
        } break;
            
        case Untyped_AST_Kind::Generic_Specification: {
            auto spec = node.cast<Untyped_AST_Generic_Specification>();
            printf("(<>)\n");
            print_sub_at_indent("id", spec->id, indent + 1);
            print_sub_at_indent("type params", spec->type_params, indent + 1);
        } break;
            
        default:
            internal_error("Invalid Untyped_AST_Kind: %d.", node->kind);
            break;
    }
}

void Untyped_AST::print() const {
    print_at_indent(Ref<Untyped_AST>(const_cast<Untyped_AST *>(this)), 0);
}
