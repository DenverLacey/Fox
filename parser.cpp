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
    Assignment, // =
    Comma,      // ,
    Or,         // ||
    And,        // &&
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
    Primary,
};

Precedence token_precedence(Token token) {
    switch (token.kind) {
        // internal
        case Token_Kind::Eof: return Precedence::Primary;
        case Token_Kind::Err: return Precedence::Primary;
            
        // literals
        case Token_Kind::True: return Precedence::None;
        case Token_Kind::False: return Precedence::None;
        case Token_Kind::Int: return Precedence::None;
        case Token_Kind::Float: return Precedence::None;
        case Token_Kind::Char: return Precedence::None;
        case Token_Kind::String: return Precedence::None;
        case Token_Kind::Ident: return Precedence::None;
            
        // delimeters
        case Token_Kind::Semi: return Precedence::None;
        case Token_Kind::Colon: return Precedence::None;
        case Token_Kind::Comma: return Precedence::Comma;
        case Token_Kind::Left_Paren: return Precedence::Call;
        case Token_Kind::Right_Paren: return Precedence::None;
        case Token_Kind::Left_Curly: return Precedence::None;
        case Token_Kind::Right_Curly: return Precedence::None;
            
        // keywords
        case Token_Kind::Let: return Precedence::None;
        case Token_Kind::If: return Precedence::None;
        case Token_Kind::Else: return Precedence::None;
        case Token_Kind::While: return Precedence::None;
        case Token_Kind::Fn: return Precedence::None;
        case Token_Kind::Struct: return Precedence::None;
        case Token_Kind::Enum: return Precedence::None;
            
        // operators
        case Token_Kind::Plus: return Precedence::Term;
        case Token_Kind::Dash: return Precedence::Term;
        case Token_Kind::Star: return Precedence::Factor;
        case Token_Kind::Slash: return Precedence::Factor;
        case Token_Kind::Percent: return Precedence::Factor;
        case Token_Kind::Bang: return Precedence::Unary;
        case Token_Kind::Eq: return Precedence::Assignment;
        case Token_Kind::Double_Eq: return Precedence::Equality;
    }
    assert(false);
}

Precedence operator++(Precedence p) {
    auto q = (int)p + 1;
    return q < (int)Precedence::Primary ? (Precedence)q : Precedence::Primary;
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
        } else {
            return parse_statement();
        }
    }
    
    Ref<Untyped_AST> parse_fn_declaration() {
        return nullptr;
    }
    
    Ref<Untyped_AST> parse_struct_declaration() {
        return nullptr;
    }
    
    Ref<Untyped_AST> parse_enum_declaration() {
        return nullptr;
    }
    
    Ref<Untyped_AST> parse_statement() {
        Ref<Untyped_AST> s;
        if (match(Token_Kind::Let)) {
            s = parse_let_statement();
            expect(Token_Kind::Semi, "Expected ';' after statement.");
        } else if (match(Token_Kind::If)) {
            s = parse_if_statement();
        } else if (match(Token_Kind::While)) {
            s = parse_while_statement();
        } else {
            s = parse_expression();
            expect(Token_Kind::Semi, "Expected ';' after statement.");
        }
        return s;
    }
    
    Ref<Untyped_AST_Let> parse_let_statement() {
        bool is_mut = match(Token_Kind::Mut);
    
        auto id = expect(Token_Kind::Ident, "Expected identifier after '%s'.", is_mut ? "let mut" : "let").data.s.clone();
        
        Ref<Untyped_AST_Type_Signiture> specified_type = nullptr;
        if (match(Token_Kind::Colon)) {
            auto type = parse_type_signiture();
            specified_type = make<Untyped_AST_Type_Signiture>(std::move(type));
        }
        
        expect(Token_Kind::Eq, "Expected '=' before initializer expression.");
        
        auto initializer = parse_expression();
        
        return make<Untyped_AST_Let>(id, is_mut, std::move(specified_type), std::move(initializer));
    }
    
    Ref<Value_Type> parse_type_signiture() {
        auto type = make<Value_Type>();
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
                    type->data.ptr.subtype = parse_type_signiture().release();
                    type->data.ptr.subtype->is_mut = true;
                } else {
                    type->data.ptr.subtype = parse_type_signiture().release();
                }
            } break;
            default:
                assert(false);
                break;
        }
        return type;
    }
    
    Ref<Untyped_AST_Ternary> parse_if_statement() {
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
        
        return make<Untyped_AST_Ternary>(Untyped_AST_Kind::If, std::move(cond), std::move(then), std::move(else_));
    }
    
    Ref<Untyped_AST_Binary> parse_while_statement() {
        auto cond = parse_expression();
        auto body = parse_block();
        return make<Untyped_AST_Binary>(Untyped_AST_Kind::While, std::move(cond), std::move(body));
    }
    
    Ref<Untyped_AST> parse_expression() {
        return parse_precedence(Precedence::Assignment);
    }
    
    Ref<Untyped_AST> parse_precedence(Precedence prec) {
        auto token = next();
        verify(token.kind != Token_Kind::Eof, "Unexpected end of input.");
        
        auto prev = parse_prefix(token);
        verify(prev != nullptr, "Expected expression.");
        
        while (prec <= token_precedence(peek())) {
            auto token = next();
            prev = parse_infix(token, std::move(prev));
            verify(prev != nullptr, "Unexpected token in middle of expression.");
        }
        return prev;
    }
    
    Ref<Untyped_AST> parse_prefix(Token token) {
        Ref<Untyped_AST> a = nullptr;
        switch (token.kind) {
            // delimeters
            case Token_Kind::Left_Paren:
                a = parse_expression();
                expect(Token_Kind::Right_Paren, "Expected ')' to terminate parenthesized expression.");
                break;
                
            // literals
            case Token_Kind::Ident:
                 a = make<Untyped_AST_Ident>(token.data.s.clone());
                break;
            case Token_Kind::True:
                 a = make<Untyped_AST_Bool>(true);
                break;
            case Token_Kind::False:
                 a = make<Untyped_AST_Bool>(false);
                break;
            case Token_Kind::Int:
                 a = make<Untyped_AST_Int>(token.data.i);
                break;
            case Token_Kind::Float:
                 a = make<Untyped_AST_Float>(token.data.f);
                break;
            case Token_Kind::Char:
                 a = make<Untyped_AST_Char>(token.data.c);
                break;
            case Token_Kind::String:
                 a = make<Untyped_AST_Str>(token.data.s.clone());
                break;
                
            // operators
            case Token_Kind::Bang:
                a = parse_unary(Untyped_AST_Kind::Not);
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
                // a = parse_invocation(prev);
                break;
                
            // operators
            case Token_Kind::Plus:
                a = parse_binary(Untyped_AST_Kind::Addition, prec, std::move(prev));
                break;
            case Token_Kind::Dash:
                a = parse_binary(Untyped_AST_Kind::Subtraction, prec, std::move(prev));
                break;
            case Token_Kind::Star:
                a = parse_binary(Untyped_AST_Kind::Multiplication, prec, std::move(prev));
                break;
            case Token_Kind::Slash:
                a = parse_binary(Untyped_AST_Kind::Division, prec, std::move(prev));
                break;
            case Token_Kind::Percent:
                a = parse_binary(Untyped_AST_Kind::Mod, prec, std::move(prev));
                break;
            case Token_Kind::Eq:
                a = parse_binary(Untyped_AST_Kind::Assignment, prec, std::move(prev));
                break;
            case Token_Kind::Double_Eq:
                a = parse_binary(Untyped_AST_Kind::Equal, prec, std::move(prev));
                break;
                
            default:
                break;
        }
        return a;
    }
    
    Ref<Untyped_AST_Unary> parse_unary(Untyped_AST_Kind kind) {
        auto sub = parse_precedence(Precedence::Unary);
        return make<Untyped_AST_Unary>(kind, std::move(sub));
    }
    
    Ref<Untyped_AST_Binary> parse_binary(Untyped_AST_Kind kind, Precedence prec, Ref<Untyped_AST> lhs) {
        auto rhs = parse_precedence(++prec);
        return make<Untyped_AST_Binary>(kind, std::move(lhs), std::move(rhs));
    }
    
    Ref<Untyped_AST_Block> parse_block() {
        auto block = make<Untyped_AST_Block>(Untyped_AST_Kind::Block);
        expect(Token_Kind::Left_Curly, "Expected '{' to begin block.");
        while (!check(Token_Kind::Right_Curly) && has_more()) {
            block->add(parse_declaration());
        }
        expect(Token_Kind::Right_Curly, "Expected '}' to end block.");
        return block;
    }
};

Ref<Untyped_AST_Block> parse(const std::vector<Token> &tokens) {
    Parser p(tokens);
    auto nodes = make<Untyped_AST_Block>(Untyped_AST_Kind::Block);
    
    while (p.has_more()) {
        nodes->add(p.parse_declaration());
    }
    
    return nodes;
}
