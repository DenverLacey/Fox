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
    Cast,       // as
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
    Call,       // . () []
    Path,       // ::
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
        case Token_Kind::Double_Colon: return Precedence::Path;
        case Token_Kind::Comma: return Precedence::None;
        case Token_Kind::Left_Paren: return Precedence::Call;
        case Token_Kind::Right_Paren: return Precedence::None;
        case Token_Kind::Left_Curly: return Precedence::None;
        case Token_Kind::Right_Curly: return Precedence::None;
        case Token_Kind::Left_Bracket: return Precedence::Call;
        case Token_Kind::Right_Bracket: return Precedence::None;
        case Token_Kind::At: return Precedence::None;
            
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
        case Token_Kind::Break: return Precedence::None;
        case Token_Kind::Continue: return Precedence::None;
        case Token_Kind::Import: return Precedence::None;
        case Token_Kind::As: return Precedence::Cast;
        case Token_Kind::Vararg: return Precedence::None;
            
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

    Code_Location current_location() {
        return peek().location;
    }

    Code_Location previous_location() {
        return tokens[current - 1].location;
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
        vverify(t.kind == kind, t.location, err, args);
        return t;
    }
    
    Ref<Untyped_AST> parse_declaration() {
        Ref<Untyped_AST> d;
        if (check(Token_Kind::Fn)) {
            d = parse_fn_declaration(next());
        } else if (check(Token_Kind::Struct)) {
            d = parse_struct_declaration(next());
        } else if (check(Token_Kind::Enum)) {
            d = parse_enum_declaration(next());
        } else if (check(Token_Kind::Trait)) {
            d = parse_trait_declaration(next());
        } else if (check(Token_Kind::Impl)) {
            d = parse_impl_declaration(next());
        } else if (check(Token_Kind::Import)) {
            d = parse_import_declaration(next());
            expect(Token_Kind::Semi, "Expected ';' after import declaration.");
        } else {
            d = parse_statement();
        }
        return d;
    }
    
    Ref<Untyped_AST> parse_fn_declaration(Token token) {
        // Untyped_AST_Kind kind = Untyped_AST_Kind::Fn_Decl;
        bool is_method = false;
        
        String id = expect(Token_Kind::Ident, "Expected identifier after 'fn' keyword").data.s.clone();
        
        if (match(Token_Kind::Left_Angle)) {
            todo("Generic functions not yet implemented.");
            expect(Token_Kind::Right_Angle, "Expected '>' to terminate generic parameter list.");
        }
        
        auto param_tok = expect(Token_Kind::Left_Paren, "Expected '(' to begin function parameter list.");
        Ref<Untyped_AST_Multiary> params = Mem.make<Untyped_AST_Multiary>(Untyped_AST_Kind::Comma, param_tok.location);
        
        if (check_identifier("self") || (
            check(Token_Kind::Mut) &&
            check_identifier("self", 1)))
        {
            // kind = Untyped_AST_Kind::Method_Decl;
            is_method = true;

            bool is_mut = match(Token_Kind::Mut);
            
            auto id_tok = expect(Token_Kind::Ident, "Expected identifier of parameter.");
            
            auto self_type = Mem.make<Value_Type>().as_ptr();
            *self_type = value_types::unresolved("Self", id_tok.location);
            auto value_type = Mem.make<Value_Type>();
            *value_type = value_types::ptr_to(self_type);
            value_type->data.ptr.child_type->is_mut = is_mut;
            auto sig = Mem.make<Untyped_AST_Type_Signature>(value_type, id_tok.location);
            
            auto target = Mem.make<Untyped_AST_Pattern_Ident>(false, id_tok.data.s.clone(), id_tok.location);
            
            auto param = Mem.make<Untyped_AST_Binary>(
                Untyped_AST_Kind::Binding,
                target,
                sig,
                id_tok.location
            );
            
            params->add(param);
            
            match(Token_Kind::Comma);
        }
    
        // parse parameters
        bool varargs = false;
        do {
            if (check_terminating_delimeter()) break;
            
            verify(!varargs, current_location(), "Variadic parameter must be the last parameter of a function. '%s' has parameters after the variadic parameter.", id.c_str());
            
            varargs = match(Token_Kind::Vararg);
            
            bool is_mut = match(Token_Kind::Mut);
            auto id_tok = expect(Token_Kind::Ident, "Expected parameter name.");
            auto target = Mem.make<Untyped_AST_Pattern_Ident>(is_mut, id_tok.data.s.clone(), id_tok.location);
            
            auto colon_tok = expect(Token_Kind::Colon, "Expected ':' before parameters type.");
            
            auto value_type = parse_type_signature();
            auto sig = Mem.make<Untyped_AST_Type_Signature>(value_type, colon_tok.location); // @TODO: Actual loc
            auto binding = Mem.make<Untyped_AST_Binary>(Untyped_AST_Kind::Binding, target, sig, colon_tok.location);
            
            params->add(binding);
        } while (match(Token_Kind::Comma) && has_more());
        
        expect(Token_Kind::Right_Paren, "Expected ')' to terminate function parameter list.");
        
        Ref<Untyped_AST_Type_Signature> return_type_signature = nullptr;
        if (match(Token_Kind::Thin_Right_Arrow)) {
            auto type = parse_type_signature();
            return_type_signature = Mem.make<Untyped_AST_Type_Signature>(type, Code_Location{ 0, 0, "<return-type>" });
        }
        
        if (match(Token_Kind::Semi)) {
            return Mem.make<Untyped_AST_Fn_Declaration_Header>(
                is_method ? Untyped_AST_Kind::Method_Decl_Header : Untyped_AST_Kind::Fn_Decl_Header,
                id,
                params,
                varargs,
                return_type_signature,
                token.location
            );
        } else {
            auto body = parse_block();
        
            return Mem.make<Untyped_AST_Fn_Declaration>(
                is_method ? Untyped_AST_Kind::Method_Decl : Untyped_AST_Kind::Fn_Decl,
                id,
                params,
                varargs,
                return_type_signature,
                body,
                token.location
            );
        }
    }
    
    Ref<Untyped_AST> parse_struct_declaration(Token token) {
        String id = expect(Token_Kind::Ident, "Expected identifier after 'struct' keyword.").data.s.clone();
        auto decl = Mem.make<Untyped_AST_Struct_Declaration>(id, token.location);
        
        expect(Token_Kind::Left_Curly, "Expected '{' in struct declaration.");
        
        do {
            if (check_terminating_delimeter()) break;
            
            bool force_mut = match(Token_Kind::Mut);
            String field_id = expect(Token_Kind::Ident, "Expected identifier of field in struct declaration.").data.s.clone();
            expect(Token_Kind::Colon, "Expected ':' after field identifier.");
            
            auto type = parse_type_signature();
            type->is_mut = force_mut;
            auto sig = Mem.make<Untyped_AST_Type_Signature>(type, Code_Location{ 0, 0, "<value-type-loc>" });
            
            decl->add_field(field_id, sig);
        } while (match(Token_Kind::Comma) && has_more());
        
        expect(Token_Kind::Right_Curly, "Expected '}' to terminate struct declaration.");
        
        return decl;
    }
    
    Ref<Untyped_AST> parse_enum_declaration(Token token) {
        String id = expect(Token_Kind::Ident, "Expected identifier after 'enum' keyword.").data.s.clone();
        
        auto decl = Mem.make<Untyped_AST_Enum_Declaration>(id, token.location);
        
        expect(Token_Kind::Left_Curly, "Expected '{' in enum declaration.");
        
        do {
            if (check_terminating_delimeter()) break;
            
            String variant_id = expect(Token_Kind::Ident, "Expected name of enum variant.").data.s.clone();
            
            Ref<Untyped_AST_Multiary> payload = nullptr;
            if (match(Token_Kind::Left_Paren)) {
                payload = Mem.make<Untyped_AST_Multiary>(Untyped_AST_Kind::Comma, previous_location());
                
                do {
                    if (check(Token_Kind::Right_Paren)) break;
                    auto value_type = parse_type_signature();
                    auto sig = Mem.make<Untyped_AST_Type_Signature>(value_type, Code_Location{ 0,0,"<value-type-loc>" });
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

    Ref<Untyped_AST_Trait_Declaration> parse_trait_declaration(Token token) {
        String id = expect(Token_Kind::Ident, "Expected identifier after 'trait' keyword.").data.s.clone();
        auto body = parse_block();

        return Mem.make<Untyped_AST_Trait_Declaration>(id, body, token.location);
    }
    
    Ref<Untyped_AST_Impl_Declaration> parse_impl_declaration(Token token) {
        auto target_expr = parse_expression();
        verify(target_expr->kind == Untyped_AST_Kind::Ident || target_expr->kind == Untyped_AST_Kind::Path, target_expr->location, "Expected a type name after 'impl' keyword.");
        auto target = target_expr.cast<Untyped_AST_Symbol>();
        
        Ref<Untyped_AST_Symbol> for_ = nullptr;
        if (match(Token_Kind::For)) {
            for_ = parse_symbol();
        }
        
        auto body = parse_block();
        
        return Mem.make<Untyped_AST_Impl_Declaration>(target, for_, body, token.location);
    }
    
    Ref<Untyped_AST_Import_Declaration> parse_import_declaration(Token token) {
        auto path = parse_symbol();
        
        Ref<Untyped_AST_Ident> rename_id = nullptr;
        if (match(Token_Kind::As)) {
            if (match(Token_Kind::Star)) {
                rename_id = Mem.make<Untyped_AST_Ident>(String {"*"}, previous_location());
            } else {
                auto expr = parse_expression();
                verify(expr->kind == Untyped_AST_Kind::Ident, expr->location, "Expected identifier after 'as' keyword.");

                rename_id = expr.cast<Untyped_AST_Ident>();
            }
        }
        
        return Mem.make<Untyped_AST_Import_Declaration>(path, rename_id, token.location);
    }
    
    Ref<Untyped_AST> parse_statement() {
        Ref<Untyped_AST> s;
        if (check(Token_Kind::Let)) {
            s = parse_let_statement(next());
            expect(Token_Kind::Semi, "Expected ';' after statement.");
        } else if (check(Token_Kind::Const)) {
            s = parse_const_statement(next());
            expect(Token_Kind::Semi, "Expected ';' after statement.");
        } else if (check(Token_Kind::If)) {
            s = parse_if_statement(next());
        } else if (check(Token_Kind::While)) {
            s = parse_while_statement(next());
        } else if (check(Token_Kind::For)) {
            s = parse_for_statement(next());
        } else if (check(Token_Kind::Match)) {
            s = parse_match_statement(next());
        } else if (check(Token_Kind::Defer)) {
            s = parse_defer_statement(next());
        } else if (check(Token_Kind::Return)) {
            s = parse_return_statement(next());
            expect(Token_Kind::Semi, "Expected ';' after statement.");
        } else if (check(Token_Kind::Break) || check(Token_Kind::Continue)) {
            s = parse_loop_control(next());
            expect(Token_Kind::Semi, "Expected ';' after statement.");
        } else if (check(Token_Kind::Left_Curly)) {
            s = parse_block();
        } else {
            s = parse_expression_or_assignment();
            expect(Token_Kind::Semi, "Expected ';' after statement.");
        }
        return s;
    }
    
    Ref<Untyped_AST_Let> parse_let_statement(Token token) {
        auto target = parse_pattern();
        
        Ref<Untyped_AST_Type_Signature> specified_type = nullptr;
        if (match(Token_Kind::Colon)) {
            auto type = parse_type_signature();
            specified_type = Mem.make<Untyped_AST_Type_Signature>(type, Code_Location{ 0,0,"<value-type-loc>" });
        }
        
        Ref<Untyped_AST> initializer = nullptr;
        if (match(Token_Kind::Eq)) {
            initializer = parse_expression();
        }
        
        verify(specified_type || (initializer && initializer->kind != Untyped_AST_Kind::Noinit), current_location(), "Type signiture required in 'let' statement without an initializer.");
        verify(initializer || target->are_all_variables_mut() || (specified_type ? specified_type->value_type->is_partially_mutable() : false), current_location(), "'let' statements without an initializer must be marked 'mut'.");
        
        return Mem.make<Untyped_AST_Let>(false, target, specified_type, initializer, token.location);
    }
    
    //
    // @NOTE:
    //      Might be better to incorporate this into parse_let_statement()
    //      since they share lots of code.
    //
    Ref<Untyped_AST_Let> parse_const_statement(Token token) {
        auto target = parse_pattern();
        verify(target->are_no_variables_mut(), target->location, "Cannot mark target of assignment as 'mut' when declaring a constant.");
        
        Ref<Untyped_AST_Type_Signature> specified_type = nullptr;
        if (match(Token_Kind::Colon)) {
            auto type = parse_type_signature();
            specified_type = Mem.make<Untyped_AST_Type_Signature>(type, Code_Location{ 0,0,"<value-type-loc>" });
        }
        
        expect(Token_Kind::Eq, "Expected '=' in 'const' statement because it requires an initializer expression.");
        
        auto initilizer = parse_expression();

        return Mem.make<Untyped_AST_Let>(true, target, specified_type, initilizer, token.location);
    }
    
    Ref<Untyped_AST_Pattern> parse_pattern(bool allow_value_pattern = false) {
        Ref<Untyped_AST_Pattern> p = nullptr;
        auto n = next();
        
        switch (n.kind) {
            case Token_Kind::Underscore: {
                p = Mem.make<Untyped_AST_Pattern_Underscore>(n.location);
            } break;
            case Token_Kind::Ident: {
                auto id_str = n.data.s.clone();
                
                if (!(check(Token_Kind::Left_Curly) ||
                      check(Token_Kind::Left_Paren) ||
                      check(Token_Kind::Double_Colon)))
                {
                    p = Mem.make<Untyped_AST_Pattern_Ident>(false, id_str, n.location);
                } else {
                    auto id = Mem.make<Untyped_AST_Ident>(id_str, n.location);
                    
                    Ref<Untyped_AST_Symbol> sym;
                    if (check(Token_Kind::Double_Colon)) {
                        sym = parse_symbol(id);
                    } else {
                        sym = id;
                    }
                    
                    if (match(Token_Kind::Left_Curly)) {
                        auto sp = Mem.make<Untyped_AST_Pattern_Struct>(sym, previous_location());
                        do {
                            if (check(Token_Kind::Right_Curly)) break;
                            sp->add(parse_pattern(allow_value_pattern));
                        } while (match(Token_Kind::Comma) && has_more());
                        expect(Token_Kind::Right_Curly, "Expected '}' to terminate struct pattern.");
                        p = sp;
                    } else if (match(Token_Kind::Left_Paren)) {
                        auto ep = Mem.make<Untyped_AST_Pattern_Enum>(sym, previous_location());
                        do {
                            if (check(Token_Kind::Right_Paren)) break;
                            ep->add(parse_pattern(allow_value_pattern));
                        } while (match(Token_Kind::Comma) && has_more());
                        expect(Token_Kind::Right_Paren, "Expected ')' to terminate enum pattern.");
                        p = ep;
                    } else {
                        verify(allow_value_pattern, sym->location, "Invalid pattern.");
                        p = Mem.make<Untyped_AST_Pattern_Value>(sym, sym->location);
                    }
                }
            } break;
            case Token_Kind::Mut: {
                auto id = expect(Token_Kind::Ident, "Expected identifier after 'mut' keyword.").data.s.clone();
                p = Mem.make<Untyped_AST_Pattern_Ident>(true, id, n.location);
            } break;
            case Token_Kind::Left_Paren: {
                auto tp = Mem.make<Untyped_AST_Pattern_Tuple>(n.location);
                do {
                    if (check(Token_Kind::Right_Paren)) break;
                    tp->add(parse_pattern(allow_value_pattern));
                } while (match(Token_Kind::Comma) && has_more());
                expect(Token_Kind::Right_Paren, "Expected ')' to terminate tuple pattern.");
                p = tp;
            } break;
                
            default: {
                verify(allow_value_pattern, n.location, "Invalid pattern.");
                
                //
                // @HACK:
                //      We're doing this because to get 'n' we used 'next()' but now
                //      we need it to parse the expression properly.
                //      This is a hack and can probably be done better.
                //
                current--;
                
                auto value = parse_expression();
                p = Mem.make<Untyped_AST_Pattern_Value>(value, value->location);
            } break;
        }
        
        return p;
    }
    
    Ref<Untyped_AST_Symbol> parse_symbol(Ref<Untyped_AST_Ident> prev = nullptr) {
        Ref<Untyped_AST_Ident> lhs;
        Ref<Untyped_AST_Symbol> rhs = nullptr;
        
        if (prev) {
            lhs = prev;
        } else {
            auto id_tok = expect(Token_Kind::Ident, "Expected identifier to begin symbol.");
            lhs = Mem.make<Untyped_AST_Ident>(id_tok.data.s.clone(), id_tok.location);
        }
        
        if (match(Token_Kind::Double_Colon)) {
            rhs = parse_symbol();
        } else if (check(Token_Kind::Ident)) {
            auto id_tok = expect(Token_Kind::Ident, "Expected identifier to begin symbol.");
            rhs = Mem.make<Untyped_AST_Ident>(id_tok.data.s.clone(), id_tok.location);
        }
        
        Ref<Untyped_AST_Symbol> sym;
        if (rhs) {
            sym = Mem.make<Untyped_AST_Path>(lhs, rhs, lhs->location);
        } else {
            sym = lhs;
        }
        
        return sym;
    }
    
    Ref<Value_Type> parse_type_signature() {
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
                    auto ident = Mem.make<Untyped_AST_Ident>(id.clone(), token.location);
                    
                    Ref<Untyped_AST_Symbol> sym;
                    if (check(Token_Kind::Double_Colon)) {
                        sym = parse_symbol(ident);
                    } else {
                        sym = ident;
                    }
                    
                    *type = value_types::unresolved(sym.as_ptr());
                }
            } break;
            case Token_Kind::Star: {
                type->kind = Value_Type_Kind::Ptr;
                bool is_mut = match(Token_Kind::Mut);
                type->data.ptr.child_type = parse_type_signature().as_ptr();
                type->data.ptr.child_type->is_mut = is_mut;
            } break;
            case Token_Kind::Left_Paren: {
                std::vector<Ref<Value_Type>> subtypes;
                do {
                    if (check(Token_Kind::Right_Paren)) break;
                    subtypes.push_back(parse_type_signature());
                } while (match(Token_Kind::Comma) && has_more());
                
                expect(Token_Kind::Right_Paren, "Expected ')' in type signiture.");
                
                if (match(Token_Kind::Thin_Right_Arrow)) {
                    auto return_type = parse_type_signature().as_ptr();
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
                Value_Type *element_type = parse_type_signature().as_ptr();
                element_type->is_mut = is_mut;
                
                if (type->kind == Value_Type_Kind::Array) {
                    type->data.array.element_type = element_type;
                } else {
                    type->data.slice.element_type = element_type;
                }
            } break;
            default:
                error(token.location, "Invalid type signiture.");
                break;
        }
        return type;
    }
    
    Ref<Untyped_AST_If> parse_if_statement(Token token) {
        auto cond = parse_expression();
        auto then = parse_block();
        
        Ref<Untyped_AST> else_ = nullptr;
        if (match(Token_Kind::Else)) {
            if (match(Token_Kind::If)) {
                else_ = parse_if_statement(peek(-1)); // @WARNING: This relies on integer overflow
            } else {
                else_ = parse_block();
            }
        }
        
        return Mem.make<Untyped_AST_If>(cond, then, else_, token.location);
    }
    
    Ref<Untyped_AST_Binary> parse_while_statement(Token token) {
        auto cond = parse_expression();
        auto body = parse_block();
        return Mem.make<Untyped_AST_Binary>(Untyped_AST_Kind::While, cond, body, token.location);
    }
    
    Ref<Untyped_AST_For> parse_for_statement(Token token) {
        auto target = parse_pattern();
        
        String counter = "";
        if (match(Token_Kind::Comma)) {
            auto counter_tok = expect(Token_Kind::Ident, "Expected identifier of counter variable of for-loop.");
            counter = counter_tok.data.s.clone();
        }
        
        expect(Token_Kind::In, "Expected 'in' keyword in for-loop.");
        
        auto iterable = parse_expression();
        auto body = parse_block();
        
        return Mem.make<Untyped_AST_For>(target, counter, iterable, body, token.location);
    }
    
    Ref<Untyped_AST_Match> parse_match_statement(Token token) {
        auto cond = parse_expression();
        
        auto curly_tok = expect(Token_Kind::Left_Curly, "Expected '{' in 'match' statement.");
        auto arms = Mem.make<Untyped_AST_Multiary>(Untyped_AST_Kind::Comma, curly_tok.location);
        Ref<Untyped_AST> default_arm = nullptr;
        while (!check(Token_Kind::Right_Curly) && has_more()) {
            auto pat = parse_pattern(/*allow_value_pattern*/ true);
            auto arrow_tok = expect(Token_Kind::Fat_Right_Arrow, "Expected '=>' while parsing arm of 'match' statement.");
            auto body = parse_statement();
            if (pat->kind == Untyped_AST_Kind::Pattern_Underscore) {
                verify(!default_arm, pat->location, "There can only be one default arm in 'match' statement.");
                default_arm = body;
            } else {
                auto arm = Mem.make<Untyped_AST_Binary>(Untyped_AST_Kind::Match_Arm, pat, body, arrow_tok.location);
                arms->add(arm);
            }
        }
        
        expect(Token_Kind::Right_Curly, "Expected '}' to terminate 'match' statement.");
        
        return Mem.make<Untyped_AST_Match>(
            cond,
            default_arm,
            arms,
            token.location
        );
    }

    Ref<Untyped_AST_Unary> parse_defer_statement(Token token) {
        auto deferred_statement = parse_statement();
        return Mem.make<Untyped_AST_Unary>(Untyped_AST_Kind::Defer, deferred_statement, token.location);
    }
    
    Ref<Untyped_AST_Return> parse_return_statement(Token token) {
        Ref<Untyped_AST> sub = nullptr;
        if (!check(Token_Kind::Semi)) {
            sub = parse_expression();
        }
        return Mem.make<Untyped_AST_Return>(sub, token.location);
    }

    Ref<Untyped_AST_Loop_Control> parse_loop_control(Token token) {
        bool is_break = token.kind == Token_Kind::Break;
        const char *control_str = is_break ? "break" : "continue";

        auto label = String{};
        if (match(Token_Kind::Left_Paren)) {
            auto label_tok = expect(Token_Kind::Ident, "Expected identifier in parenetheses of %s statement.", control_str);
            label = label_tok.data.s.clone();
            expect(Token_Kind::Right_Paren, "Expected ')' after identifer in parentheses of %s statement.", control_str);
        }

        return Mem.make<Untyped_AST_Loop_Control>(
            is_break ? Untyped_AST_Kind::Break : Untyped_AST_Kind::Continue,
            label,
            token.location
        );
    }
    
    Ref<Untyped_AST> parse_expression_or_assignment() {
        return parse_precedence(Precedence::Assignment);
    }
    
    Ref<Untyped_AST> parse_expression() {
        auto expr = parse_expression_or_assignment();
        verify(expr->kind != Untyped_AST_Kind::Assignment, expr->location, "Cannot assign in expression context.");
        return expr;
    }
    
    Ref<Untyped_AST> parse_precedence(Precedence prec) {
        auto token = next();
        verify(token.kind != Token_Kind::Eof, token.location, "Unexpected end of input.");
        
        auto prev = parse_prefix(token);
        verify(prev, current_location(), "Expected expression.");
        
        while (prec <= token_precedence(peek())) {
            auto token = next();
            prev = parse_infix(token, prev);
            verify(prev, current_location(), "Unexpected token in middle of expression.");
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
                    a = parse_comma_separated_expressions(a->location, a);
                    a->kind = Untyped_AST_Kind::Tuple;
                }
                expect(Token_Kind::Right_Paren, "Expected ')' to terminate parenthesized expression.");
                break;
            case Token_Kind::Left_Bracket:
                a = parse_array_literal(token.location);
                break;
            case Token_Kind::At:
                a = parse_builtin(token.location);
                break;
                
            // literals
            case Token_Kind::Ident:
                a = Mem.make<Untyped_AST_Ident>(token.data.s.clone(), token.location);
                if (check_beginning_of_struct_literal()) {
                    a = parse_struct_literal(a);
                } else if (check_beginning_of_generic_specification()) {
                    a = parse_generic_specification(a);
                }
                break;
            case Token_Kind::True:
                a = Mem.make<Untyped_AST_Bool>(true, token.location);
                break;
            case Token_Kind::False:
                a = Mem.make<Untyped_AST_Bool>(false, token.location);
                break;
            case Token_Kind::Int:
                a = Mem.make<Untyped_AST_Int>(token.data.i, token.location);
                break;
            case Token_Kind::Float:
                a = Mem.make<Untyped_AST_Float>(token.data.f, token.location);
                break;
            case Token_Kind::Char:
                a = Mem.make<Untyped_AST_Char>(token.data.c, token.location);
                break;
            case Token_Kind::String:
                a = Mem.make<Untyped_AST_Str>(token.data.s, token.location);
                break;
                
            // keywords
            case Token_Kind::Noinit:
                a = Mem.make<Untyped_AST_Nullary>(Untyped_AST_Kind::Noinit, token.location);
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
                    a = Mem.make<Untyped_AST_Unary>(Untyped_AST_Kind::Negation, sub, token.location);
                }
            } break;
            case Token_Kind::Bang:
                a = parse_unary(Untyped_AST_Kind::Not, token.location);
                break;
            case Token_Kind::Ampersand:
                a = parse_unary(Untyped_AST_Kind::Address_Of, token.location);
                break;
            case Token_Kind::Ampersand_Mut:
                a = parse_unary(Untyped_AST_Kind::Address_Of_Mut, token.location);
                break;
            case Token_Kind::Star:
                a = parse_unary(Untyped_AST_Kind::Deref, token.location);
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
                a = parse_invocation(prev, token.location);
                break;
            case Token_Kind::Comma:
                a = parse_comma_separated_expressions(token.location, prev);
                break;
            case Token_Kind::Double_Colon: {
                auto lhs = prev.cast<Untyped_AST_Ident>();
                verify(lhs, token.location, "Symbol paths can only consist of identifiers.");
                
                auto rhs = parse_symbol();
                
                a = Mem.make<Untyped_AST_Path>(lhs, rhs, token.location);
                
                if (check_beginning_of_struct_literal()) {
                    a = parse_struct_literal(a);
                } else if (check_beginning_of_generic_specification()) {
                    a = parse_generic_specification(a);
                }
            } break;
            case Token_Kind::Colon:
                a = parse_binary(Untyped_AST_Kind::Binding, prec, prev, token.location);
                break;
            case Token_Kind::Left_Bracket:
                a = parse_binary(Untyped_AST_Kind::Subscript, Precedence::Assignment + 1, prev, token.location);
                expect(Token_Kind::Right_Bracket, "Expected ']' in subscript expression.");
                break;
                
            // operators
            case Token_Kind::Plus:
                a = parse_binary(Untyped_AST_Kind::Addition, prec, prev, token.location);
                break;
            case Token_Kind::Plus_Eq:
                a = parse_operator_assignment(Untyped_AST_Kind::Addition, prev, token.location);
                break;
            case Token_Kind::Dash:
                a = parse_binary(Untyped_AST_Kind::Subtraction, prec, prev, token.location);
                break;
            case Token_Kind::Dash_Eq:
                a = parse_operator_assignment(Untyped_AST_Kind::Subtraction, prev, token.location);
                break;
            case Token_Kind::Star:
                a = parse_binary(Untyped_AST_Kind::Multiplication, prec, prev, token.location);
                break;
            case Token_Kind::Star_Eq:
                a = parse_operator_assignment(Untyped_AST_Kind::Multiplication, prev, token.location);
                break;
            case Token_Kind::Slash:
                a = parse_binary(Untyped_AST_Kind::Division, prec, prev, token.location);
                break;
            case Token_Kind::Slash_Eq:
                a = parse_operator_assignment(Untyped_AST_Kind::Division, prev, token.location);
                break;
            case Token_Kind::Percent:
                a = parse_binary(Untyped_AST_Kind::Mod, prec, prev, token.location);
                break;
            case Token_Kind::Percent_Eq:
                a = parse_operator_assignment(Untyped_AST_Kind::Mod, prev, token.location);
                break;
            case Token_Kind::Eq:
                a = parse_binary(Untyped_AST_Kind::Assignment, prec, prev, token.location);
                break;
            case Token_Kind::Double_Eq:
                a = parse_binary(Untyped_AST_Kind::Equal, prec, prev, token.location);
                break;
            case Token_Kind::Bang_Eq:
                a = parse_binary(Untyped_AST_Kind::Not_Equal, prec, prev, token.location);
                break;
            case Token_Kind::Left_Angle:
                a = parse_binary(Untyped_AST_Kind::Less, prec, prev, token.location);
                break;
            case Token_Kind::Left_Angle_Eq:
                a = parse_binary(Untyped_AST_Kind::Less_Eq, prec, prev, token.location);
                break;
            case Token_Kind::Right_Angle:
                a = parse_binary(Untyped_AST_Kind::Greater, prec, prev, token.location);
                break;
            case Token_Kind::Right_Angle_Eq:
                a = parse_binary(Untyped_AST_Kind::Greater_Eq, prec, prev, token.location);
                break;
            case Token_Kind::And:
                a = parse_binary(Untyped_AST_Kind::And, prec, prev, token.location);
                break;
            case Token_Kind::Or:
                a = parse_binary(Untyped_AST_Kind::Or, prec, prev, token.location);
                break;
            case Token_Kind::Dot:
                a = parse_dot_operator(prev, token.location);
                break;
            case Token_Kind::Double_Dot:
                a = parse_binary(Untyped_AST_Kind::Range, prec, prev, token.location);
                break;
            case Token_Kind::Triple_Dot:
                a = parse_binary(Untyped_AST_Kind::Inclusive_Range, prec, prev, token.location);
                break;
            case Token_Kind::As:
                a = parse_cast_operator(prev, token.location);
                break;
                
            default:
                break;
        }
        return a;
    }
    
    Ref<Untyped_AST_Unary> parse_unary(Untyped_AST_Kind kind, Code_Location location) {
        auto sub = parse_precedence(Precedence::Unary);
        return Mem.make<Untyped_AST_Unary>(kind, sub, location);
    }
    
    Ref<Untyped_AST_Binary> parse_binary(
        Untyped_AST_Kind kind, 
        Precedence prec, 
        Ref<Untyped_AST> lhs, 
        Code_Location location) 
    {
        auto rhs = parse_precedence(prec + 1);
        return Mem.make<Untyped_AST_Binary>(kind, lhs, rhs, location);
    }
    
    Ref<Untyped_AST_Binary> parse_operator_assignment(
        Untyped_AST_Kind kind, 
        Ref<Untyped_AST> lhs, 
        Code_Location location) 
    {
        auto rhs_lhs = lhs->clone();
        auto rhs_rhs = parse_expression();
        auto rhs = Mem.make<Untyped_AST_Binary>(kind, rhs_lhs, rhs_rhs, location);
        return Mem.make<Untyped_AST_Binary>(Untyped_AST_Kind::Assignment, lhs, rhs, location);
    }
    
    Ref<Untyped_AST_Multiary> parse_block() {
        auto curly_tok = expect(Token_Kind::Left_Curly, "Expected '{' to begin block.");
        auto block = Mem.make<Untyped_AST_Multiary>(Untyped_AST_Kind::Block, curly_tok.location);
        while (!check(Token_Kind::Right_Curly) && has_more()) {
            block->add(parse_declaration());
        }
        expect(Token_Kind::Right_Curly, "Expected '}' to end block.");
        return block;
    }
    
    Ref<Untyped_AST_Binary> parse_invocation(Ref<Untyped_AST> lhs, Code_Location location) {
        auto args = parse_comma_separated_expressions(location);
        expect(Token_Kind::Right_Paren, "Expected ')' to terminate function call.");
        return Mem.make<Untyped_AST_Binary>(Untyped_AST_Kind::Invocation, lhs, args, location);
    }
    
    Ref<Untyped_AST_Multiary> parse_comma_separated_expressions(
        Code_Location location,
        Ref<Untyped_AST> prev = nullptr)
    {
        auto comma = Mem.make<Untyped_AST_Multiary>(Untyped_AST_Kind::Comma, location);
        if (prev) comma->add(prev);
        do {
            if (check_terminating_delimeter()) break;
            comma->add(parse_expression());
        } while (match(Token_Kind::Comma) && has_more());
        return comma;
    }
    
    Ref<Untyped_AST> parse_dot_operator(Ref<Untyped_AST> lhs, Code_Location location) {
        Ref<Untyped_AST> dot;
        if (check(Token_Kind::Int)) {
            auto i = next();
            auto rhs = Mem.make<Untyped_AST_Int>(i.data.i, i.location);
            Untyped_AST_Kind kind = Untyped_AST_Kind::Field_Access_Tuple;
            dot = Mem.make<Untyped_AST_Binary>(kind, lhs, rhs, location);
        } else {
            verify(check(Token_Kind::Ident), peek().location, "Expected an identifier after '.'.");
            auto id_str = next().data.s.clone();
            if (match(Token_Kind::Left_Paren)) {
                dot = parse_dot_call_operator(lhs, id_str, location);
            } else {
                dot = Mem.make<Untyped_AST_Field_Access>(lhs, id_str, location);
            }
        }
        return dot;
    }
    
    Ref<Untyped_AST> parse_dot_call_operator(
        Ref<Untyped_AST> receiver,
        String method_id,
        Code_Location location)
    {
        auto args = parse_comma_separated_expressions(previous_location());
        expect(Token_Kind::Right_Paren, "Expected ')' to terminate method call.");
        
        return Mem.make<Untyped_AST_Dot_Call>(receiver, method_id, args, location);
    }
    
    Ref<Untyped_AST> parse_array_literal(Code_Location location) {
        bool infer_count = false;
        size_t count = 0;
        
        if (match(Token_Kind::Right_Bracket)) {
            return parse_slice_literal(location);
        }
        
        if (match(Token_Kind::Underscore)) {
            infer_count = true;
        } else {
            auto count_tok = next();
            verify(count_tok.kind == Token_Kind::Int, count_tok.location, "Expected an int to specify count for array literal.");
            count = count_tok.data.i;
        }
        expect(Token_Kind::Right_Bracket, "Expected ']' in array literal.");
        
        Ref<Value_Type> element_type = nullptr;
        if (check(Token_Kind::Left_Curly)) {
            element_type = Mem.make<Value_Type>();
            element_type->kind = Value_Type_Kind::None;
        } else if (match(Token_Kind::Mut)) {
            if (check(Token_Kind::Left_Curly)) {
                element_type = Mem.make<Value_Type>();
                element_type->kind = Value_Type_Kind::None;
            } else {
                element_type = parse_type_signature();
            }
            element_type->is_mut = true;
        } else {
            element_type = parse_type_signature();
        }
        
        auto curly_tok = expect(Token_Kind::Left_Curly, "Expected '{' in array literal.");
        auto element_nodes = parse_comma_separated_expressions(curly_tok.location);
        expect(Token_Kind::Right_Curly, "Expected '}' to terminate array literal.");
        
        if (!infer_count) {
            verify(count == element_nodes->nodes.size(), element_nodes->location, "Incorrect number of elements in array literal. Expected %zu but was given %zu.", count, element_nodes->nodes.size());
        }
        
        count = element_nodes->nodes.size();
        
        Ref<Value_Type> array_type = Mem.make<Value_Type>(value_types::array_of(count, element_type.as_ptr()));
        
        return Mem.make<Untyped_AST_Array>(
            Untyped_AST_Kind::Array,
            count,
            array_type,
            element_nodes,
            location
        );
    }
    
    Ref<Untyped_AST_Binary> parse_slice_literal(Code_Location location) {
        bool is_mut = match(Token_Kind::Mut);
        auto element_type = parse_type_signature();
        element_type->is_mut = is_mut;
        auto element_sig = Mem.make<Untyped_AST_Type_Signature>(element_type, Code_Location{ 0,0,"<value-type-loc>" });
        
        auto curly_tok = expect(Token_Kind::Left_Curly, "Expected '{' in slice literal.");
        auto slice_fields = parse_comma_separated_expressions(curly_tok.location);
        expect(Token_Kind::Right_Curly, "Expected '}' in slice literal.");
        
        return Mem.make<Untyped_AST_Binary>(
            Untyped_AST_Kind::Slice,
            element_sig,
            slice_fields,
            location
        );
    }
    
    Ref<Untyped_AST> parse_builtin(Code_Location location) {
        auto id_str = expect(Token_Kind::Ident, "Expected identifier of builtin after '@'.").data.s.clone();
        
        Ref<Untyped_AST> parsed;
        if (id_str == "size_of") {
            expect(Token_Kind::Left_Paren, "Expected '(' after '@size_of'.");
            auto value_type = parse_type_signature();
            auto sig = Mem.make<Untyped_AST_Type_Signature>(value_type, Code_Location{ 0, 0, "<value-type-loc>" });
            expect(Token_Kind::Right_Paren, "Expected ')' to terminate '@size_of' builtin.");
            parsed = Mem.make<Untyped_AST_Unary>(Untyped_AST_Kind::Builtin_Sizeof, sig, location);
        } else if (id_str == "alloc") {
            expect(Token_Kind::Left_Paren, "Expected '(' after '@alloc'.");
            auto value_type = parse_type_signature();
            auto sig = Mem.make<Untyped_AST_Type_Signature>(value_type, Code_Location{ 0, 0, "<value-type-loc>" });
            expect(Token_Kind::Comma, "Expected ',' after type signature in '@alloc' builtin.");
            auto size_expr = parse_expression();
            expect(Token_Kind::Right_Paren, "Expected ')' to terminate '@alloc' builtin.");
            parsed = Mem.make<Untyped_AST_Binary>(Untyped_AST_Kind::Builtin_Alloc, sig, size_expr, location);
        } else if (id_str == "puts" || id_str == "print") {
            expect(Token_Kind::Left_Paren, "Expected '(' after '@%.*s'.", id_str.size(), id_str.c_str());
            auto kind = id_str == "puts" ? Untyped_AST_Builtin_Printlike::Puts : Untyped_AST_Builtin_Printlike::Print;
            auto arg = parse_expression();
            expect(Token_Kind::Right_Paren, "Expected ')' to terminate '@%.*s' builtin.", id_str.size(), id_str.c_str());
            parsed = Mem.make<Untyped_AST_Builtin_Printlike>(kind, arg, location);
        } else {
            parsed = Mem.make<Untyped_AST_Builtin>(id_str, location);
        }
        
        return parsed;
    }
    
    Ref<Untyped_AST_Struct_Literal> parse_struct_literal(Ref<Untyped_AST> id) {
        auto curly_tok = expect(Token_Kind::Left_Curly, "Expected '{' in struct literal.");
        auto bindings = parse_comma_separated_expressions(curly_tok.location);
        expect(Token_Kind::Right_Curly, "Expected '}' to terminate struct literal.");
        return Mem.make<Untyped_AST_Struct_Literal>(
            id.cast<Untyped_AST_Symbol>(),
            bindings.cast<Untyped_AST_Multiary>(),
            id->location
        );
    }
    
    Ref<Untyped_AST_Generic_Specification> parse_generic_specification(
        Ref<Untyped_AST> id)
    {
        auto angle_tok = expect(Token_Kind::Left_Angle, "Expected '<' to begin generic specification.");        
        auto type_params = Mem.make<Untyped_AST_Multiary>(Untyped_AST_Kind::Comma, angle_tok.location);
        do {
            if (check(Token_Kind::Right_Curly)) break;
            auto value_type = parse_type_signature();
            auto type_sig = Mem.make<Untyped_AST_Type_Signature>(value_type, Code_Location{ 0,0,"<value-type-loc>" });
            type_params->add(type_sig);
        } while (match(Token_Kind::Comma) && has_more());
        
        expect(Token_Kind::Right_Angle, "Expected '>' to terminate generic specification.");
        
        return Mem.make<Untyped_AST_Generic_Specification>(
            id.cast<Untyped_AST_Ident>(),
            type_params,
            angle_tok.location
        );
    }
    
    Ref<Untyped_AST_Binary> parse_cast_operator(Ref<Untyped_AST> expr, Code_Location location) {
        auto value_type = parse_type_signature();
        auto sig = Mem.make<Untyped_AST_Type_Signature>(value_type, Code_Location{ 0,0,"<value-type-lic>" });
        return Mem.make<Untyped_AST_Binary>(Untyped_AST_Kind::Cast, expr, sig, location);
    }
};

Ref<Untyped_AST_Multiary> parse(const std::vector<Token> &tokens) {
    auto p = Parser { tokens };
    auto nodes = Mem.make<Untyped_AST_Multiary>(Untyped_AST_Kind::Block, p.current_location());
    
    while (p.has_more()) {
        nodes->add(p.parse_declaration());
    }
    
    return nodes;
}
