//
//  parser.cpp
//  Fox
//
//  Created by Denver Lacey on 7/7/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#include "parser.h"
#include "error.h"

enum class Precedence {
    None,
    Assignment, // = += -= *= /= &= etc.
    Colon,      // :
    Range,      // .. ...
    Or,         // or
    And,        // and
    BitOr,      // |
    Xor,        // ^
    BitAnd,     // &
    Equality,   // == !=
    Comparison, // < > <= >=
    Shift,      // << >>
    Term,       // + -
    Factor,     // * / %
    Unary,      // !
    Call,       // . () [] ::
    Primary,
};

Precedence token_precedence(Token token) {
    switch (token.kind) {
        // internal
        case Token_Kind::Eof: return Precedence::None;
        case Token_Kind::Err: return Precedence::None;
            
        // literals
        case Token_Kind::True:   return Precedence::None;
        case Token_Kind::False:  return Precedence::None;
        case Token_Kind::Int:    return Precedence::None;
        case Token_Kind::Float:  return Precedence::None;
        case Token_Kind::Char:   return Precedence::None;
        case Token_Kind::String: return Precedence::None;
        case Token_Kind::Ident:  return Precedence::None;
            
        // delimeters
        case Token_Kind::Semi: return Precedence::None;
        case Token_Kind::Colon: return Precedence::Colon;
        case Token_Kind::Double_Colon: return Precedence::Call;
        case Token_Kind::Comma: return Precedence::None;
        case Token_Kind::Left_Paren: return Precedence::Call;
        case Token_Kind::Right_Paren: return Precedence::None;
        case Token_Kind::Left_Curly: return Precedence::None;
        case Token_Kind::Right_Curly: return Precedence::None;
        case Token_Kind::Left_Bracket: return Precedence::Call;
        case Token_Kind::Right_Bracket: return Precedence::None;
            
        // arrows
        case Token_Kind::Thin_Right_Arrow: return Precedence::None;
        case Token_Kind::Fat_Right_Arrow: return Precedence::None;
            
        // keywords
        case Token_Kind::Noinit: return Precedence::Primary;
        case Token_Kind::Let: return Precedence::None;
        case Token_Kind::Mut: return Precedence::None;
        case Token_Kind::Const: return Precedence::None;
        case Token_Kind::If: return Precedence::None;
        case Token_Kind::Else: return Precedence::None;
        case Token_Kind::While: return Precedence::None;
        case Token_Kind::For: return Precedence::None;
        case Token_Kind::In: return Precedence::None;
        case Token_Kind::Match: return Precedence::None;
        case Token_Kind::Fn: return Precedence::None;
        case Token_Kind::Struct: return Precedence::None;
        case Token_Kind::Enum: return Precedence::None;
        case Token_Kind::Impl: return Precedence::None;
        case Token_Kind::And: return Precedence::And;
        case Token_Kind::Or: return Precedence::Or;
        case Token_Kind::Underscore: return Precedence::None;
        case Token_Kind::Return: return Precedence::None;
            
        // operators
        case Token_Kind::Plus: return Precedence::Term;
        case Token_Kind::Dash: return Precedence::Term;
        case Token_Kind::Star: return Precedence::Factor;
        case Token_Kind::Slash: return Precedence::Factor;
        case Token_Kind::Percent: return Precedence::Factor;
        case Token_Kind::Bang: return Precedence::Unary;
        case Token_Kind::Double_Eq: return Precedence::Equality;
        case Token_Kind::Bang_Eq: return Precedence::Equality;
        case Token_Kind::Left_Angle: return Precedence::Comparison;
        case Token_Kind::Left_Angle_Eq: return Precedence::Comparison;
        case Token_Kind::Right_Angle: return Precedence::Comparison;
        case Token_Kind::Right_Angle_Eq: return Precedence::Comparison;
        case Token_Kind::Ampersand: return Precedence::BitAnd;
        case Token_Kind::Ampersand_Mut: return Precedence::Unary;
        case Token_Kind::Dot: return Precedence::Call;
        case Token_Kind::Double_Dot: return Precedence::Range;
        case Token_Kind::Triple_Dot: return Precedence::Range;
            
        // assignment operator
        case Token_Kind::Eq: return Precedence::Assignment;
        case Token_Kind::Plus_Eq: return Precedence::Assignment;
        case Token_Kind::Dash_Eq: return Precedence::Assignment;
        case Token_Kind::Star_Eq: return Precedence::Assignment;
        case Token_Kind::Slash_Eq: return Precedence::Assignment;
        case Token_Kind::Percent_Eq: return Precedence::Assignment;
    }
    
    internal_error("Invalid Precednece value: %d\n", token.kind);
}

Precedence operator+(Precedence p, int step) {
    auto q = static_cast<int>(p) + step;
    q = std::clamp(q, static_cast<int>(Precedence::None), static_cast<int>(Precedence::Primary));
    return static_cast<Precedence>(q);
}

struct Parser {
    const std::vector<Token> &tokens;
    size_t current;
    
    Parser(const std::vector<Token> &tokens) : tokens(tokens), current(0) {}
    
    bool has_more() const {
        return current < tokens.size() - 1;
    }
    
    Token peek() {
        return tokens[current];
    }
    
    Token peek(size_t n) {
        if (current + n >= tokens.size()) {
            return tokens.back();
        }
        return tokens[current + n];
    }
    
    Token next() {
        auto t = peek();
        if (current < tokens.size()) {
            current++;
        }
        return t;
    }
    
    bool check(Token_Kind kind) {
        return peek().kind == kind;
    }
    
    bool check(Token_Kind kind, size_t n) {
        return peek(n).kind == kind;
    }
    
    bool check_terminating_delimeter() {
        Token t = peek();
        switch (t.kind) {
            case Token_Kind::Semi:
            case Token_Kind::Right_Paren:
            case Token_Kind::Right_Curly:
            case Token_Kind::Right_Bracket:
                return true;
        }
        return false;
    }
    
    bool check_beginning_of_struct_literal() {
        return (check(Token_Kind::Left_Curly)) &&
               
               (check(Token_Kind::Right_Curly, 1) ||
               (check(Token_Kind::Ident, 1) &&
        
               (check(Token_Kind::Colon, 2) ||
                check(Token_Kind::Comma, 2) ||
                check(Token_Kind::Right_Curly, 2))));
    }
    
    bool match_type_signature() {
        bool is_type_signature = true;
        
        auto token = next();
        switch (token.kind) {
            case Token_Kind::Ident: {
                auto id = token.data.s;
                if (id == "void"  ||
                    id == "bool"  ||
                    id == "char"  ||
                    id == "float" ||
                    id == "int"   ||
                    id == "str")
                {
                    // we're all good :)
                } else {
                    is_type_signature = false;
                }
            } break;
            case Token_Kind::Star: {
                match(Token_Kind::Mut);
                is_type_signature = match_type_signature();
            } break;
            case Token_Kind::Left_Paren: {
                do {
                    if (check(Token_Kind::Right_Paren)) break;
                    is_type_signature = match_type_signature();
                    if (!is_type_signature) break;
                } while (match(Token_Kind::Comma) && has_more());
                
                if (!is_type_signature) break;
                
                is_type_signature = match(Token_Kind::Right_Paren);
                if (!is_type_signature) break;
                
                if (match(Token_Kind::Thin_Right_Arrow)) {
                    is_type_signature = match_type_signature();
                    if (!is_type_signature) break;
                }
            } break;
            case Token_Kind::Left_Bracket: {
                if (!check(Token_Kind::Right_Bracket)) {
                    is_type_signature = match(Token_Kind::Int);
                    if (!is_type_signature) break;
                }
                
                is_type_signature = match(Token_Kind::Right_Bracket);
                if (!is_type_signature) break;
                
                is_type_signature = match_type_signature();
            } break;
            default:
                is_type_signature = false;
                break;
        }
        
        return is_type_signature;
    }
    
    bool check_beginning_of_generic_specification() {
        size_t reset_point = current;
        bool is_beginning_of_generic_spec = true;
        
        if (!match(Token_Kind::Left_Angle)) {
            is_beginning_of_generic_spec = false;
        } else if (!match_type_signature()) {
            is_beginning_of_generic_spec = false;
        } else if (!(check(Token_Kind::Comma) || check(Token_Kind::Right_Angle))) {
            is_beginning_of_generic_spec = false;
        }
        
        current = reset_point;
        return is_beginning_of_generic_spec;
    }
    
    bool check_identifier(const char *id, size_t n = 0) {
        auto tok = tokens[current + n];
        if (tok.kind != Token_Kind::Ident) return false;
        return tok.data.s == id;
    }
    
    bool match(Token_Kind kind) {
        if (check(kind)) {
            next();
            return true;
        }
        return false;
    }
    
    Token expect(Token_Kind kind, const char *err, ...) {
        auto t = next();
        va_list args;
        va_start(args, err);
        verify(t.kind == kind, err, args);
        va_end(args);
        return t;
    }
    
    Ref<Untyped_AST> parse_declaration() {
        if (match(Token_Kind::Fn)) {
            return parse_fn_declaration();
        } else if (match(Token_Kind::Struct)) {
            return parse_struct_declaration();
        } else if (match(Token_Kind::Enum)) {
            return parse_enum_declaration();
        } else if (match(Token_Kind::Impl)) {
            return parse_impl_declaration();
        } else {
            return parse_statement();
        }
    }
    
    Ref<Untyped_AST> parse_fn_declaration() {
        Untyped_AST_Kind kind = Untyped_AST_Kind::Fn_Decl;
        
        String id = expect(Token_Kind::Ident, "Expected identifier after 'fn' keyword").data.s.clone();
        
        if (match(Token_Kind::Left_Angle)) {
            todo("Generic functions not yet implemented.");
            expect(Token_Kind::Right_Angle, "Expected '>' to terminate generic parameter list.");
        }
        
        expect(Token_Kind::Left_Paren, "Expected '(' to begin function parameter list.");
        
        Ref<Untyped_AST_Multiary> params = Mem.make<Untyped_AST_Multiary>(Untyped_AST_Kind::Comma);
        
        if (check_identifier("self") || (
            check(Token_Kind::Mut) &&
            check_identifier("self", 1)))
        {
            kind = Untyped_AST_Kind::Method_Decl;
            
            auto self_type = Mem.make<Value_Type>().as_ptr();
            *self_type = value_types::unresolved("Self");
            auto value_type = Mem.make<Value_Type>();
            *value_type = value_types::ptr_to(self_type);
            value_type->data.ptr.child_type->is_mut = match(Token_Kind::Mut);
            auto sig = Mem.make<Untyped_AST_Type_Signature>(value_type);
            
            auto id = expect(Token_Kind::Ident, "Expected identifier of parameter.").data.s.clone();
            auto target = Mem.make<Untyped_AST_Pattern_Ident>(false, id);
            
            auto param = Mem.make<Untyped_AST_Binary>(
                Untyped_AST_Kind::Binding,
                target,
                sig
            );
            
            params->add(param);
            
            match(Token_Kind::Comma);
        }
        
        auto param_list = parse_parameter_list();
        params->nodes.insert(params->nodes.end(), param_list->nodes.begin(), param_list->nodes.end());
        
        expect(Token_Kind::Right_Paren, "Expected ')' to terminate function parameter list.");
        
        Ref<Untyped_AST_Type_Signature> return_type_signature = nullptr;
        if (match(Token_Kind::Thin_Right_Arrow)) {
            auto type = parse_type_signiture();
            return_type_signature = Mem.make<Untyped_AST_Type_Signature>(type);
        }
        
        auto body = parse_block();
        
        return Mem.make<Untyped_AST_Fn_Declaration>(
            kind,
            id,
            params,
            return_type_signature,
            body
        );
    }
    
    Ref<Untyped_AST> parse_struct_declaration() {
        String id = expect(Token_Kind::Ident, "Expected identifier after 'struct' keyword.").data.s.clone();
        auto decl = Mem.make<Untyped_AST_Struct_Declaration>(id);
        
        expect(Token_Kind::Left_Curly, "Expected '{' in struct declaration.");
        
        do {
            if (check_terminating_delimeter()) break;
            
            bool force_mut = match(Token_Kind::Mut);
            String field_id = expect(Token_Kind::Ident, "Expected identifier of field in struct declaration.").data.s.clone();
            expect(Token_Kind::Colon, "Expected ':' after field identifier.");
            
            auto type = parse_type_signiture();
            type->is_mut = force_mut;
            auto sig = Mem.make<Untyped_AST_Type_Signature>(type);
            
            decl->add_field(field_id, sig);
        } while (match(Token_Kind::Comma) && has_more());
        
        expect(Token_Kind::Right_Curly, "Expected '}' to terminate struct declaration.");
        
        return decl;
    }
    
    Ref<Untyped_AST> parse_enum_declaration() {
        String id = expect(Token_Kind::Ident, "Expected identifier after 'enum' keyword.").data.s.clone();
        
        auto decl = Mem.make<Untyped_AST_Enum_Declaration>(id);
        
        expect(Token_Kind::Left_Curly, "Expected '{' in enum declaration.");
        
        do {
            if (check_terminating_delimeter()) break;
            
            String variant_id = expect(Token_Kind::Ident, "Expected name of enum variant.").data.s.clone();
            
            Ref<Untyped_AST_Multiary> payload = nullptr;
            if (match(Token_Kind::Left_Paren)) {
                payload = Mem.make<Untyped_AST_Multiary>(Untyped_AST_Kind::Comma);
                
                do {
                    if (check(Token_Kind::Right_Paren)) break;
                    auto value_type = parse_type_signiture();
                    auto sig = Mem.make<Untyped_AST_Type_Signature>(value_type);
                    payload->add(sig);
                } while (match(Token_Kind::Comma) && has_more());
                
                expect(Token_Kind::Right_Paren, "Expected ')' to terminate payload of enum variant.");
            } else if (match(Token_Kind::Left_Curly)) {
                todo("Implement struct-like enum variant payloads.");
            }
            
            decl->add_variant(variant_id, payload);
        } while (match(Token_Kind::Comma) && has_more());
        
        expect(Token_Kind::Right_Curly, "Expected '}' to terminate enum declaration.");
        
        return decl;
    }
    
    Ref<Untyped_AST_Impl_Declaration> parse_impl_declaration() {
        auto target_expr = parse_expression();
        verify(target_expr->kind == Untyped_AST_Kind::Ident || target_expr->kind == Untyped_AST_Kind::Path, "Expected a type name after 'impl' keyword.");
        auto target = target_expr.cast<Untyped_AST_Symbol>();
        
        Ref<Untyped_AST_Symbol> for_ = nullptr;
        if (match(Token_Kind::For)) {
            todo("Implement trait impl decls.");
        }
        
        auto body = parse_block();
        
        return Mem.make<Untyped_AST_Impl_Declaration>(target, for_, body);
    }
    
    Ref<Untyped_AST> parse_statement() {
        Ref<Untyped_AST> s;
        if (match(Token_Kind::Let)) {
            s = parse_let_statement();
            expect(Token_Kind::Semi, "Expected ';' after statement.");
        } else if (match(Token_Kind::Const)) {
            s = parse_const_statement();
            expect(Token_Kind::Semi, "Expected ';' after statement.");
        } else if (match(Token_Kind::If)) {
            s = parse_if_statement();
        } else if (match(Token_Kind::While)) {
            s = parse_while_statement();
        } else if (match(Token_Kind::For)) {
            s = parse_for_statement();
        } else if (match(Token_Kind::Match)) {
            s = parse_match_statement();
        } else if (match(Token_Kind::Return)) {
            s = parse_return_statement();
            expect(Token_Kind::Semi, "Expected ';' after statement.");
        } else if (check(Token_Kind::Left_Curly)) {
            s = parse_block();
        } else {
            s = parse_expression_or_assignment();
            expect(Token_Kind::Semi, "Expected ';' after statement.");
        }
        return s;
    }
    
    Ref<Untyped_AST_Let> parse_let_statement() {
        auto target = parse_pattern();
        
        Ref<Untyped_AST_Type_Signature> specified_type = nullptr;
        if (match(Token_Kind::Colon)) {
            auto type = parse_type_signiture();
            specified_type = Mem.make<Untyped_AST_Type_Signature>(type);
        }
        
        Ref<Untyped_AST> initializer = nullptr;
        if (match(Token_Kind::Eq)) {
            initializer = parse_expression();
        }
        
        verify(specified_type || (initializer && initializer->kind != Untyped_AST_Kind::Noinit), "Type signiture required in 'let' statement without an initializer.");
        verify(initializer || target->are_all_variables_mut() || (specified_type ? specified_type->value_type->is_partially_mutable() : false), "'let' statements without an initializer must be marked 'mut'.");
        
        return Mem.make<Untyped_AST_Let>(false, target, specified_type, initializer);
    }
    
    //
    // @NOTE:
    //      Might be better to incorporate this into parse_let_statement()
    //      since they share lots of code.
    //
    Ref<Untyped_AST_Let> parse_const_statement() {
        auto target = parse_pattern();
        verify(target->are_no_variables_mut(), "Cannot mark target of assignment as 'mut' when declaring a constant.");
        
        Ref<Untyped_AST_Type_Signature> specified_type = nullptr;
        if (match(Token_Kind::Colon)) {
            auto type = parse_type_signiture();
            specified_type = Mem.make<Untyped_AST_Type_Signature>(type);
        }
        
        expect(Token_Kind::Eq, "Expected '=' in 'const' statement because it requires an initializer expression.");
        
        auto initilizer = parse_expression();

        return Mem.make<Untyped_AST_Let>(true, target, specified_type, initilizer);
    }
    
    Ref<Untyped_AST_Pattern> parse_pattern(bool allow_value_pattern = false) {
        Ref<Untyped_AST_Pattern> p = nullptr;
        auto n = next();
        
        switch (n.kind) {
            case Token_Kind::Underscore: {
                p = Mem.make<Untyped_AST_Pattern_Underscore>();
            } break;
            case Token_Kind::Ident: {
                auto id = n.data.s.clone();
                if (match(Token_Kind::Left_Curly)) {
                    auto struct_id = Mem.make<Untyped_AST_Ident>(id);
                    auto sp = Mem.make<Untyped_AST_Pattern_Struct>(struct_id);
                    do {
                        if (check(Token_Kind::Right_Curly)) break;
                        sp->add(parse_pattern(allow_value_pattern));
                    } while (match(Token_Kind::Comma) && has_more());
                    expect(Token_Kind::Right_Curly, "Expected '}' to terminate struct pattern.");
                    p = sp;
                } else if (match(Token_Kind::Left_Paren)) {
                    todo("Enum Patterns not yet implemented.");
                } else {
                    p = Mem.make<Untyped_AST_Pattern_Ident>(false, id);
                }
            } break;
            case Token_Kind::Mut: {
                auto id = expect(Token_Kind::Ident, "Expected identifier after 'mut' keyword.").data.s.clone();
                p = Mem.make<Untyped_AST_Pattern_Ident>(true, id);
            } break;
            case Token_Kind::Left_Paren: {
                auto tp = Mem.make<Untyped_AST_Pattern_Tuple>();
                do {
                    if (check(Token_Kind::Right_Paren)) break;
                    tp->add(parse_pattern(allow_value_pattern));
                } while (match(Token_Kind::Comma) && has_more());
                expect(Token_Kind::Right_Paren, "Expected ')' to terminate tuple pattern.");
                p = tp;
            } break;
                
            default: {
                verify(allow_value_pattern, "Invalid pattern.");
                
                //
                // @HACK:
                //      We're doing this because to get 'n' we used 'next()' but now
                //      we need it to parse the expression properly.
                //      This is a hack and can probably be done better.
                //
                current--;
                
                auto value = parse_expression();
                p = Mem.make<Untyped_AST_Pattern_Value>(value);
            } break;
        }
        
        return p;
    }
    
    Ref<Value_Type> parse_type_signiture() {
        Ref<Value_Type> type = Mem.make<Value_Type>();
        auto token = next();
        switch (token.kind) {
            case Token_Kind::Ident: {
                auto id = token.data.s;
                if (id == "void") {
                    type->kind = Value_Type_Kind::Void;
                } else if (id == "bool") {
                    type->kind = Value_Type_Kind::Bool;
                } else if (id == "char") {
                    type->kind = Value_Type_Kind::Char;
                } else if (id == "float") {
                    type->kind = Value_Type_Kind::Float;
                } else if (id == "int") {
                    type->kind = Value_Type_Kind::Int;
                } else if (id == "str") {
                    type->kind = Value_Type_Kind::Str;
                } else {
                    type->kind = Value_Type_Kind::Unresolved_Type;
                    type->data.unresolved.id = id.clone();
                }
            } break;
            case Token_Kind::Star: {
                type->kind = Value_Type_Kind::Ptr;
                if (match(Token_Kind::Mut)) {
                    type->data.ptr.child_type = parse_type_signiture().as_ptr();
                    type->data.ptr.child_type->is_mut = true;
                } else {
                    type->data.ptr.child_type = parse_type_signiture().as_ptr();
                }
            } break;
            case Token_Kind::Left_Paren: {
                std::vector<Ref<Value_Type>> subtypes;
                do {
                    if (check(Token_Kind::Right_Paren)) break;
                    subtypes.push_back(parse_type_signiture());
                } while (match(Token_Kind::Comma) && has_more());
                
                expect(Token_Kind::Right_Paren, "Expected ')' in type signiture.");
                
                if (match(Token_Kind::Thin_Right_Arrow)) {
                    auto return_type = parse_type_signiture().as_ptr();
                    *type = value_types::func(return_type, subtypes.size(), flatten(subtypes));
                } else {
                    *type = value_types::tup_from(subtypes.size(), flatten(subtypes));
                }
            } break;
            case Token_Kind::Left_Bracket: {
                if (check(Token_Kind::Right_Bracket)) {
                    type->kind = Value_Type_Kind::Slice;
                } else {
                    auto count = expect(Token_Kind::Int, "Expected integer literal in array type signature").data.i;
                    type->kind = Value_Type_Kind::Array;
                    type->data.array.count = count;
                }
                expect(Token_Kind::Right_Bracket, "Expected ']' in array literal.");
                bool is_mut = match(Token_Kind::Mut);
                type->data.array.element_type = parse_type_signiture().as_ptr();
                type->data.array.element_type->is_mut = is_mut;
            } break;
            default:
                error("Invalid type signiture.");
                break;
        }
        return type;
    }
    
    Ref<Untyped_AST_If> parse_if_statement() {
        auto cond = parse_expression();
        auto then = parse_block();
        
        Ref<Untyped_AST> else_ = nullptr;
        if (match(Token_Kind::Else)) {
            if (match(Token_Kind::If)) {
                else_ = parse_if_statement();
            } else {
                else_ = parse_block();
            }
        }
        
        return Mem.make<Untyped_AST_If>(cond, then, else_);
    }
    
    Ref<Untyped_AST_Binary> parse_while_statement() {
        auto cond = parse_expression();
        auto body = parse_block();
        return Mem.make<Untyped_AST_Binary>(Untyped_AST_Kind::While, cond, body);
    }
    
    Ref<Untyped_AST_For> parse_for_statement() {
        auto target = parse_pattern();
        
        String counter = "";
        if (match(Token_Kind::Comma)) {
            auto counter_tok = expect(Token_Kind::Ident, "Expected identifier of counter variable of for-loop.");
            counter = counter_tok.data.s.clone();
        }
        
        expect(Token_Kind::In, "Expected 'in' keyword in for-loop.");
        
        auto iterable = parse_expression();
        auto body = parse_block();
        
        return Mem.make<Untyped_AST_For>(target, counter, iterable, body);
    }
    
    Ref<Untyped_AST_Match> parse_match_statement() {
        auto cond = parse_expression();
        
        expect(Token_Kind::Left_Curly, "Expected '{' in 'match' statement.");
        
        auto arms = Mem.make<Untyped_AST_Multiary>(Untyped_AST_Kind::Comma);
        Ref<Untyped_AST> default_arm = nullptr;
        while (!check(Token_Kind::Right_Curly) && has_more()) {
            auto pat = parse_pattern(/*allow_value_pattern*/ true);
            expect(Token_Kind::Fat_Right_Arrow, "Expected '=>' while parsing arm of 'match' statement.");
            auto body = parse_statement();
            if (pat->kind == Untyped_AST_Kind::Pattern_Underscore) {
                verify(!default_arm, "There can only be one default arm in 'match' statement.");
                default_arm = body;
            } else {
                auto arm = Mem.make<Untyped_AST_Binary>(Untyped_AST_Kind::Match_Arm, pat, body);
                arms->add(arm);
            }
        }
        
        expect(Token_Kind::Right_Curly, "Expected '}' to terminate 'match' statement.");
        
        return Mem.make<Untyped_AST_Match>(
            cond,
            default_arm,
            arms
        );
    }
    
    Ref<Untyped_AST_Return> parse_return_statement() {
        Ref<Untyped_AST> sub = nullptr;
        if (!check(Token_Kind::Semi)) {
            sub = parse_expression();
        }
        return Mem.make<Untyped_AST_Return>(sub);
    }
    
    Ref<Untyped_AST> parse_expression_or_assignment() {
        return parse_precedence(Precedence::Assignment);
    }
    
    Ref<Untyped_AST> parse_expression() {
        auto expr = parse_expression_or_assignment();
        verify(expr->kind != Untyped_AST_Kind::Assignment, "Cannot assign in expression context.");
        return expr;
    }
    
    Ref<Untyped_AST> parse_precedence(Precedence prec) {
        auto token = next();
        verify(token.kind != Token_Kind::Eof, "Unexpected end of input.");
        
        auto prev = parse_prefix(token);
        verify(prev, "Expected expression.");
        
        while (prec <= token_precedence(peek())) {
            auto token = next();
            prev = parse_infix(token, prev);
            verify(prev, "Unexpected token in middle of expression.");
        }
        
        return prev;
    }
    
    Ref<Untyped_AST> parse_prefix(Token token) {
        Ref<Untyped_AST> a = nullptr;
        switch (token.kind) {
            // delimeters
            case Token_Kind::Left_Paren:
                a = parse_expression();
                if (match(Token_Kind::Comma)) {
                    a = parse_comma_separated_expressions(a);
                    a->kind = Untyped_AST_Kind::Tuple;
                }
                expect(Token_Kind::Right_Paren, "Expected ')' to terminate parenthesized expression.");
                break;
            case Token_Kind::Left_Bracket:
                a = parse_array_literal();
                break;
                
            // literals
            case Token_Kind::Ident:
                a = Mem.make<Untyped_AST_Ident>(token.data.s.clone());
                if (check_beginning_of_struct_literal()) {
                    a = parse_struct_literal(a);
                } else if (check_beginning_of_generic_specification()) {
                    a = parse_generic_specification(a);
                }
                break;
            case Token_Kind::True:
                a = Mem.make<Untyped_AST_Bool>(true);
                break;
            case Token_Kind::False:
                a = Mem.make<Untyped_AST_Bool>(false);
                break;
            case Token_Kind::Int:
                a = Mem.make<Untyped_AST_Int>(token.data.i);
                break;
            case Token_Kind::Float:
                a = Mem.make<Untyped_AST_Float>(token.data.f);
                break;
            case Token_Kind::Char:
                a = Mem.make<Untyped_AST_Char>(token.data.c);
                break;
            case Token_Kind::String:
                a = Mem.make<Untyped_AST_Str>(token.data.s);
                break;
                
            // keywords
            case Token_Kind::Noinit:
                a = Mem.make<Untyped_AST_Nullary>(Untyped_AST_Kind::Noinit);
                break;
                
            // operators
            case Token_Kind::Dash: {
                auto sub = parse_precedence(Precedence::Unary);
                if (sub->kind == Untyped_AST_Kind::Int) {
                    auto casted = sub.cast<Untyped_AST_Int>();
                    casted->value = -casted->value;
                    a = casted;
                } else if (sub->kind == Untyped_AST_Kind::Float) {
                    auto casted = sub.cast<Untyped_AST_Float>();
                    casted->value = -casted->value;
                    a = casted;
                } else {
                    a = Mem.make<Untyped_AST_Unary>(Untyped_AST_Kind::Negation, sub);
                }
            } break;
            case Token_Kind::Bang:
                a = parse_unary(Untyped_AST_Kind::Not);
                break;
            case Token_Kind::Ampersand:
                a = parse_unary(Untyped_AST_Kind::Address_Of);
                break;
            case Token_Kind::Ampersand_Mut:
                a = parse_unary(Untyped_AST_Kind::Address_Of_Mut);
                break;
            case Token_Kind::Star:
                a = parse_unary(Untyped_AST_Kind::Deref);
                break;
                
            default:
                break;
        }
        return a;
    }
    
    Ref<Untyped_AST> parse_infix(Token token, Ref<Untyped_AST> prev) {
        Ref<Untyped_AST> a = nullptr;
        auto prec = token_precedence(token);
        switch (token.kind) {
            // delimeters
            case Token_Kind::Left_Paren:
                a = parse_invocation(prev);
                break;
            case Token_Kind::Comma:
                a = parse_comma_separated_expressions(prev);
                break;
            case Token_Kind::Double_Colon: {
                auto lhs = prev.cast<Untyped_AST_Ident>();
                verify(lhs, "Symbol paths can only consist of identifiers.");
                
                auto rhs = parse_precedence(prec + 1).cast<Untyped_AST_Symbol>();
                verify(rhs, "Symbol paths can only consist of identifiers.");
                
                a = Mem.make<Untyped_AST_Path>(lhs, rhs);
                
                if (check_beginning_of_struct_literal()) {
                    a = parse_struct_literal(a);
                } else if (check_beginning_of_generic_specification()) {
                    a = parse_generic_specification(a);
                }
            } break;
            case Token_Kind::Colon:
                a = parse_binary(Untyped_AST_Kind::Binding, prec, prev);
                break;
            case Token_Kind::Left_Bracket:
                a = parse_binary(Untyped_AST_Kind::Subscript, Precedence::Assignment + 1, prev);
                expect(Token_Kind::Right_Bracket, "Expected ']' in subscript expression.");
                break;
                
            // operators
            case Token_Kind::Plus:
                a = parse_binary(Untyped_AST_Kind::Addition, prec, prev);
                break;
            case Token_Kind::Plus_Eq:
                a = parse_operator_assignment(Untyped_AST_Kind::Addition, prev);
                break;
            case Token_Kind::Dash:
                a = parse_binary(Untyped_AST_Kind::Subtraction, prec, prev);
                break;
            case Token_Kind::Dash_Eq:
                a = parse_operator_assignment(Untyped_AST_Kind::Subtraction, prev);
                break;
            case Token_Kind::Star:
                a = parse_binary(Untyped_AST_Kind::Multiplication, prec, prev);
                break;
            case Token_Kind::Star_Eq:
                a = parse_operator_assignment(Untyped_AST_Kind::Multiplication, prev);
                break;
            case Token_Kind::Slash:
                a = parse_binary(Untyped_AST_Kind::Division, prec, prev);
                break;
            case Token_Kind::Slash_Eq:
                a = parse_operator_assignment(Untyped_AST_Kind::Division, prev);
                break;
            case Token_Kind::Percent:
                a = parse_binary(Untyped_AST_Kind::Mod, prec, prev);
                break;
            case Token_Kind::Percent_Eq:
                a = parse_operator_assignment(Untyped_AST_Kind::Mod, prev);
                break;
            case Token_Kind::Eq:
                a = parse_binary(Untyped_AST_Kind::Assignment, prec, prev);
                break;
            case Token_Kind::Double_Eq:
                a = parse_binary(Untyped_AST_Kind::Equal, prec, prev);
                break;
            case Token_Kind::Bang_Eq:
                a = parse_binary(Untyped_AST_Kind::Not_Equal, prec, prev);
                break;
            case Token_Kind::Left_Angle:
                a = parse_binary(Untyped_AST_Kind::Less, prec, prev);
                break;
            case Token_Kind::Left_Angle_Eq:
                a = parse_binary(Untyped_AST_Kind::Less_Eq, prec, prev);
                break;
            case Token_Kind::Right_Angle:
                a = parse_binary(Untyped_AST_Kind::Greater, prec, prev);
                break;
            case Token_Kind::Right_Angle_Eq:
                a = parse_binary(Untyped_AST_Kind::Greater_Eq, prec, prev);
                break;
            case Token_Kind::And:
                a = parse_binary(Untyped_AST_Kind::And, prec, prev);
                break;
            case Token_Kind::Or:
                a = parse_binary(Untyped_AST_Kind::Or, prec, prev);
                break;
            case Token_Kind::Dot:
                a = parse_dot_operator(prev);
                break;
            case Token_Kind::Double_Dot:
                a = parse_binary(Untyped_AST_Kind::Range, prec, prev);
                break;
            case Token_Kind::Triple_Dot:
                a = parse_binary(Untyped_AST_Kind::Inclusive_Range, prec, prev);
                break;
                
            default:
                break;
        }
        return a;
    }
    
    Ref<Untyped_AST_Unary> parse_unary(Untyped_AST_Kind kind) {
        auto sub = parse_precedence(Precedence::Unary);
        return Mem.make<Untyped_AST_Unary>(kind, sub);
    }
    
    Ref<Untyped_AST_Binary> parse_binary(Untyped_AST_Kind kind, Precedence prec, Ref<Untyped_AST> lhs) {
        auto rhs = parse_precedence(prec + 1);
        return Mem.make<Untyped_AST_Binary>(kind, lhs, rhs);
    }
    
    Ref<Untyped_AST_Binary> parse_operator_assignment(Untyped_AST_Kind kind, Ref<Untyped_AST> lhs) {
        auto rhs_lhs = lhs->clone();
        auto rhs_rhs = parse_expression();
        auto rhs = Mem.make<Untyped_AST_Binary>(kind, rhs_lhs, rhs_rhs);
        return Mem.make<Untyped_AST_Binary>(Untyped_AST_Kind::Assignment, lhs, rhs);
    }
    
    Ref<Untyped_AST_Multiary> parse_block() {
        auto block = Mem.make<Untyped_AST_Multiary>(Untyped_AST_Kind::Block);
        expect(Token_Kind::Left_Curly, "Expected '{' to begin block.");
        while (!check(Token_Kind::Right_Curly) && has_more()) {
            block->add(parse_declaration());
        }
        expect(Token_Kind::Right_Curly, "Expected '}' to end block.");
        return block;
    }
    
    Ref<Untyped_AST_Binary> parse_invocation(Ref<Untyped_AST> lhs) {
        auto args = parse_comma_separated_expressions();
        expect(Token_Kind::Right_Paren, "Expected ')' to terminate function call.");
        return Mem.make<Untyped_AST_Binary>(Untyped_AST_Kind::Invocation, lhs, args);
    }
    
    Ref<Untyped_AST_Multiary> parse_comma_separated_expressions(
        Ref<Untyped_AST> prev = nullptr)
    {
        auto comma = Mem.make<Untyped_AST_Multiary>(Untyped_AST_Kind::Comma);
        if (prev) comma->add(prev);
        do {
            if (check_terminating_delimeter()) break;
            comma->add(parse_expression());
        } while (match(Token_Kind::Comma) && has_more());
        return comma;
    }
    
    Ref<Untyped_AST_Multiary> parse_parameter_list() {
        auto params = Mem.make<Untyped_AST_Multiary>(Untyped_AST_Kind::Comma);
        do {
            if (check_terminating_delimeter()) break;
            bool is_mut = match(Token_Kind::Mut);
            auto id = expect(Token_Kind::Ident, "Expected parameter name.").data.s.clone();
            auto target = Mem.make<Untyped_AST_Pattern_Ident>(is_mut, id);
            expect(Token_Kind::Colon, "Expected ':' before parameters type.");
            auto value_type = parse_type_signiture();
            auto sig = Mem.make<Untyped_AST_Type_Signature>(value_type);
            auto binding = Mem.make<Untyped_AST_Binary>(Untyped_AST_Kind::Binding, target, sig);
            params->add(binding);
        } while (match(Token_Kind::Comma) && has_more());
        return params;
    }
    
    Ref<Untyped_AST> parse_dot_operator(Ref<Untyped_AST> lhs) {
        Ref<Untyped_AST> dot;
        if (check(Token_Kind::Int)) {
            auto i = next().data.i;
            auto rhs = Mem.make<Untyped_AST_Int>(i);
            Untyped_AST_Kind kind = Untyped_AST_Kind::Field_Access_Tuple;
            dot = Mem.make<Untyped_AST_Binary>(kind, lhs, rhs);
        } else {
            verify(check(Token_Kind::Ident), "Expected an identifier after '.'.");
            auto id_str = next().data.s.clone();
            if (match(Token_Kind::Left_Paren)) {
                dot = parse_dot_call_operator(lhs, id_str);
            } else {
                dot = Mem.make<Untyped_AST_Field_Access>(lhs, id_str);
            }
        }
        return dot;
    }
    
    Ref<Untyped_AST> parse_dot_call_operator(
        Ref<Untyped_AST> receiver,
        String method_id)
    {
        auto args = parse_comma_separated_expressions();
        expect(Token_Kind::Right_Paren, "Expected ')' to terminate method call.");
        
        return Mem.make<Untyped_AST_Dot_Call>(receiver, method_id, args);
    }
    
    Ref<Untyped_AST_Array> parse_array_literal() {
        Value_Type_Kind array_kind = Value_Type_Kind::Array;
        bool infer_count = false;
        size_t count = 0;
        
        if (match(Token_Kind::Right_Bracket)) {
            array_kind = Value_Type_Kind::Slice;
            infer_count = true;
        } else {
            if (match(Token_Kind::Underscore)) {
                infer_count = true;
            } else {
                auto count_tok = next();
                verify(count_tok.kind == Token_Kind::Int, "Expected an int to specify count for array literal.");
                count = count_tok.data.i;
            }
            expect(Token_Kind::Right_Bracket, "Expected ']' in array literal.");
        }
        
        Ref<Value_Type> element_type = nullptr;
        if (check(Token_Kind::Left_Curly)) {
            element_type = Mem.make<Value_Type>();
            element_type->kind = Value_Type_Kind::None;
        } else if (match(Token_Kind::Mut)) {
            if (check(Token_Kind::Left_Curly)) {
                element_type = Mem.make<Value_Type>();
                element_type->kind = Value_Type_Kind::None;
            } else {
                element_type = parse_type_signiture();
            }
            element_type->is_mut = true;
        } else {
            element_type = parse_type_signiture();
        }
        
        expect(Token_Kind::Left_Curly, "Expected '{' in array literal.");
        auto element_nodes = parse_comma_separated_expressions();
        expect(Token_Kind::Right_Curly, "Expected '}' to terminate array literal.");
        
        if (!infer_count) {
            verify(count == element_nodes->nodes.size(), "Incorrect number of elements in array literal. Expected %zu but was given %zu.", count, element_nodes->nodes.size());
        }
        
        count = element_nodes->nodes.size();
        
        Ref<Value_Type> array_type = nullptr;
        if (array_kind == Value_Type_Kind::Array) {
            array_type = Mem.make<Value_Type>(value_types::array_of(count, element_type.as_ptr()));
        } else {
            array_type = Mem.make<Value_Type>(value_types::slice_of(element_type.as_ptr()));
        }
        
        return Mem.make<Untyped_AST_Array>(
            array_kind == Value_Type_Kind::Array ? Untyped_AST_Kind::Array : Untyped_AST_Kind::Slice,
            count,
            array_type,
            element_nodes
        );
    }
    
    Ref<Untyped_AST_Struct_Literal> parse_struct_literal(Ref<Untyped_AST> id) {
        expect(Token_Kind::Left_Curly, "Expected '{' in struct literal.");
        auto bindings = parse_comma_separated_expressions();
        expect(Token_Kind::Right_Curly, "Expected '}' to terminate struct literal.");
        return Mem.make<Untyped_AST_Struct_Literal>(
            id.cast<Untyped_AST_Ident>(),
            bindings.cast<Untyped_AST_Multiary>()
        );
    }
    
    Ref<Untyped_AST_Generic_Specification> parse_generic_specification(
        Ref<Untyped_AST> id)
    {
        expect(Token_Kind::Left_Angle, "Expected '<' to begin generic specification.");
        
        auto type_params = Mem.make<Untyped_AST_Multiary>(Untyped_AST_Kind::Comma);
        do {
            if (check(Token_Kind::Right_Curly)) break;
            auto value_type = parse_type_signiture();
            auto type_sig = Mem.make<Untyped_AST_Type_Signature>(value_type);
            type_params->add(type_sig);
        } while (match(Token_Kind::Comma) && has_more());
        
        expect(Token_Kind::Right_Angle, "Expected '>' to terminate generic specification.");
        
        return Mem.make<Untyped_AST_Generic_Specification>(
            id.cast<Untyped_AST_Ident>(),
            type_params
        );
    }
};

Ref<Untyped_AST_Multiary> parse(const std::vector<Token> &tokens) {
    auto p = Parser { tokens };
    auto nodes = Mem.make<Untyped_AST_Multiary>(Untyped_AST_Kind::Block);
    
    while (p.has_more()) {
        nodes->add(p.parse_declaration());
    }
    
    return nodes;
}
