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
    Primary,
};

Precedence token_precedence(Token token) {
    switch (token.kind) {
        // internal
        case Token_Kind::Eof: return Precedence::None;
        case Token_Kind::Err: return Precedence::None;
            
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
        case Token_Kind::Left_Bracket: return Precedence::Call;
        case Token_Kind::Right_Bracket: return Precedence::None;
            
        // keywords
        case Token_Kind::Let: return Precedence::None;
        case Token_Kind::Mut: return Precedence::None;
        case Token_Kind::Const: return Precedence::None;
        case Token_Kind::If: return Precedence::None;
        case Token_Kind::Else: return Precedence::None;
        case Token_Kind::While: return Precedence::None;
        case Token_Kind::For: return Precedence::None;
        case Token_Kind::In: return Precedence::None;
        case Token_Kind::Fn: return Precedence::None;
        case Token_Kind::Struct: return Precedence::None;
        case Token_Kind::Enum: return Precedence::None;
        case Token_Kind::And: return Precedence::And;
        case Token_Kind::Or: return Precedence::Or;
        case Token_Kind::Underscore: return Precedence::None;
            
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
    assert(false);
}

Precedence operator+(Precedence p, int step) {
    auto q = (int)p + step;
    q = std::clamp(q, (int)Precedence::None, (int)Precedence::Primary);
    return (Precedence)q;
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
        } else if (match(Token_Kind::Const)) {
            s = parse_const_statement();
            expect(Token_Kind::Semi, "Expected ';' after statement.");
        } else if (match(Token_Kind::If)) {
            s = parse_if_statement();
        } else if (match(Token_Kind::While)) {
            s = parse_while_statement();
        } else if (match(Token_Kind::For)) {
            s = parse_for_statement();
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
        
        Ref<Untyped_AST_Type_Signiture> specified_type = nullptr;
        if (match(Token_Kind::Colon)) {
            auto type = parse_type_signiture();
            specified_type = Mem.make<Untyped_AST_Type_Signiture>(type);
        }
        
        Ref<Untyped_AST> initializer = nullptr;
        if (match(Token_Kind::Eq)) {
            initializer = parse_expression();
        }
        
        verify(specified_type || initializer, "Type signiture required in 'let' statement without an initializer.");
        verify(initializer || target->are_all_variables_mut(), "'let' statements without an initializer must be marked 'mut'.");
        
        return Mem.make<Untyped_AST_Let>(false, target, specified_type, initializer);
    }
    
    //
    // @NOTE:
    //      Might be better to incorporate this into parse_let_statement()
    //      since they share lots of code.
    //
    Ref<Untyped_AST_Let> parse_const_statement() {
        // @TODO: check that nothings marked 'mut'
        auto target = parse_pattern();
        
        Ref<Untyped_AST_Type_Signiture> specified_type = nullptr;
        if (match(Token_Kind::Colon)) {
            auto type = parse_type_signiture();
            specified_type = Mem.make<Untyped_AST_Type_Signiture>(type);
        }
        
        expect(Token_Kind::Eq, "Expected '=' in 'const' statement because it requires an initializer expression.");
        
        auto initilizer = parse_expression();

        return Mem.make<Untyped_AST_Let>(true, target, specified_type, initilizer);
    }
    
    Ref<Untyped_AST_Pattern> parse_pattern() {
        Ref<Untyped_AST_Pattern> p = nullptr;
        auto n = next();
        
        switch (n.kind) {
            case Token_Kind::Underscore: {
                p = Mem.make<Untyped_AST_Pattern_Underscore>();
            } break;
            case Token_Kind::Ident: {
                auto id = n.data.s.clone();
                if (match(Token_Kind::Left_Curly)) {
                    internal_error("Struct Patterns not yet implmented.");
                } else if (match(Token_Kind::Left_Paren)) {
                    internal_error("Enum Patterns not yet implemented.");
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
                    tp->add(parse_pattern());
                } while (match(Token_Kind::Comma) && has_more());
                expect(Token_Kind::Right_Paren, "Expected ')' to terminate tuple pattern.");
                p = tp;
            } break;
                
            default:
                error("Invalid pattern.");
                break;
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
                if (!check(Token_Kind::Right_Paren)) {
                    do {
                        subtypes.push_back(parse_type_signiture());
                    } while (match(Token_Kind::Comma) && has_more());
                }
                expect(Token_Kind::Right_Paren, "Expected ')' in type signiture.");
                
                // @TODO: Check for '->' if function signiture
                
                *type = value_types::tup_from(subtypes.size(), flatten(subtypes));
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
                expect(Token_Kind::Right_Paren, "Expected ')' to terminate parenthesized expression.");
                if (a->kind == Untyped_AST_Kind::Comma) {
                    a->kind = Untyped_AST_Kind::Tuple;
                }
                break;
            case Token_Kind::Left_Bracket:
                a = parse_array_literal();
                break;
                
            // literals
            case Token_Kind::Ident:
                 a = Mem.make<Untyped_AST_Ident>(token.data.s.clone());
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
                // a = parse_invocation(prev);
                break;
            case Token_Kind::Comma:
                a = parse_comma_separated_expressions(prev);
                break;
            case Token_Kind::Left_Bracket:
                a = parse_binary(Untyped_AST_Kind::Subscript, Precedence::Comma + 1, prev);
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
    
    Ref<Untyped_AST_Multiary> parse_comma_separated_expressions(
        Ref<Untyped_AST> prev = nullptr)
    {
        auto comma = Mem.make<Untyped_AST_Multiary>(Untyped_AST_Kind::Comma);
        if (prev) comma->add(prev);
        do {
            if (check_terminating_delimeter()) break;
            comma->add(parse_precedence(Precedence::Comma + 1));
        } while (match(Token_Kind::Comma) && has_more());
        return comma;
    }
    
    Ref<Untyped_AST_Binary> parse_dot_operator(Ref<Untyped_AST> lhs) {
        Ref<Untyped_AST> rhs;
        Untyped_AST_Kind kind;
        if (check(Token_Kind::Int)) {
            auto i = next().data.i;
            rhs = Mem.make<Untyped_AST_Int>(i);
            kind = Untyped_AST_Kind::Dot_Tuple;
        } else {
            verify(check(Token_Kind::Ident), "Expected an identifier after '.'.");
            auto id_str = next().data.s.clone();
            auto id = Mem.make<Untyped_AST_Ident>(id_str);
            if (match(Token_Kind::Left_Paren)) {
                // dot call
                assert(false);
            } else {
                rhs = id;
                kind = Untyped_AST_Kind::Dot;
            }
        }
        return Mem.make<Untyped_AST_Binary>(kind, lhs, rhs);
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
};

Ref<Untyped_AST_Multiary> parse(const std::vector<Token> &tokens) {
    auto p = Parser { tokens };
    auto nodes = Mem.make<Untyped_AST_Multiary>(Untyped_AST_Kind::Block);
    
    while (p.has_more()) {
        nodes->add(p.parse_declaration());
    }
    
    return nodes;
}
