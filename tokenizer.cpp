//
//  tokenizer.cpp
//  Fox
//
//  Created by Denver Lacey on 7/7/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#include "error.h"
#include "tokenizer.h"
#include "String.h"
#include "utfcpp/utf8.h"

static Token eof_token() {
    Token t;
    t.kind = Token_Kind::Eof;
    return t;
}

static Token err_token(const char *err, ...) {
    char *_err;
    va_list args;
    va_start(args, err);
    vasprintf(&_err, err, args);
    va_end(args);
    
    Token t;
    t.kind = Token_Kind::Err;
    t.data.s = { _err, strlen(_err) };
    return t;
}

struct Source_Iterator {
    char *cur;
    char *end;
    
    bool has_more() const {
        return cur != end;
    }
    
    char32_t next() {
        return utf8::next(cur, end);
    }
    
    char32_t peek() const {
        return utf8::unchecked::peek_next(cur);
    }
    
    bool match(char32_t c) {
        if (peek() == c) {
            next();
            return true;
        }
        return false;
    }
    
    void skip_whitespace() {
        while ((isspace(peek()) || peek() == '#') && has_more()) {
            if (peek() == '#') {
                while (peek() != '\n' && has_more()) {
                    // keep getting next until we've passed the new line
                    next();
                }
            }
            next();
        }
    }
};

static Token number(Source_Iterator &src) {
    char *word = src.cur;
    char *word_end = src.cur;
    while (isdigit(src.peek())) {
        src.next();
        word_end = src.cur;
    }
    
    bool is_float = false;
    if (src.match('.')) {
        is_float = true;
        while (isdigit(src.peek())) {
            src.next();
            word_end = src.cur;
        }
    }
    
    String num_str = String::copy(word, word_end - word);
    Token t;
    
    if (is_float) {
        t.kind = Token_Kind::Float;
        t.data.f = atof(num_str.c_str());
    } else {
        t.kind = Token_Kind::Int;
        t.data.i = atoll(num_str.c_str());
    }
    
    num_str.free();
    
    return t;
}

static Token character(Source_Iterator &src) {
    verify(src.next() == '\'', "Character literals must start with a '.");
    auto c = src.next();
    verify(src.next() == '\'', "Character literals must end with a '.");
    Token t;
    t.kind = Token_Kind::Char;
    t.data.c = c;
    return t;
}

static Token string(Source_Iterator &src) {
    src.next(); // skip first "
    char *word = src.cur;
    char *word_end = src.cur;
    while (src.next() != '"') {
        word_end = src.cur;
    }
    String s(word, word_end - word);
    
    Token t;
    t.kind = Token_Kind::String;
    t.data.s = s;
    return t;
}

static Token punctuation(Source_Iterator &src) {
    Token t;
    
    auto c = src.next();
    switch (c) {
        // delimeters
        case ';': t.kind = Token_Kind::Semi; break;
        case ':': t.kind = Token_Kind::Colon; break;
        case ',': t.kind = Token_Kind::Comma; break;
        case '(': t.kind = Token_Kind::Left_Paren; break;
        case ')': t.kind = Token_Kind::Right_Paren; break;
        case '{': t.kind = Token_Kind::Left_Curly; break;
        case '}': t.kind = Token_Kind::Right_Curly; break;
            
        // operators
        case '+': // t.kind = Token_Kind::Plus; break;
            if (src.match('=')) {
                t.kind = Token_Kind::Plus_Eq;
            } else {
                t.kind = Token_Kind::Plus;
            }
            break;
        case '-': // t.kind = Token_Kind::Dash; break;
            if (src.match('=')) {
                t.kind = Token_Kind::Dash_Eq;
            } else {
                t.kind = Token_Kind::Dash;
            }
            break;
        case '*': // t.kind = Token_Kind::Star; break;
            if (src.match('=')) {
                t.kind = Token_Kind::Star_Eq;
            } else {
                t.kind = Token_Kind::Star;
            }
            break;
        case '/': // t.kind = Token_Kind::Slash; break;
            if (src.match('=')) {
                t.kind = Token_Kind::Slash_Eq;
            } else {
                t.kind = Token_Kind::Slash;
            }
            break;
        case '%': // t.kind = Token_Kind::Percent; break;
            if (src.match('=')) {
                t.kind = Token_Kind::Percent_Eq;
            } else {
                t.kind = Token_Kind::Percent;
            }
            break;
        case '!': // t.kind = Token_Kind::Bang; break;
            if (src.match('=')) {
                t.kind = Token_Kind::Bang_Eq;
            } else {
                t.kind = Token_Kind::Bang;
            }
            break;
        case '=':
            if (src.match('=')) {
                t.kind = Token_Kind::Double_Eq;
            } else {
                t.kind = Token_Kind::Eq;
            }
            break;
        case '<':
            if (src.match('=')) {
                t.kind = Token_Kind::Left_Angle_Eq;
            } else {
                t.kind = Token_Kind::Left_Angle;
            }
            break;
        case '>':
            if (src.match('=')) {
                t.kind = Token_Kind::Right_Angle_Eq;
            } else {
                t.kind = Token_Kind::Right_Angle;
            }
            break;
            
        default:
            auto _c = utf8char_t::from_char32(c);
            error("Unexpected punctuation '%s'.\n", _c.buf);
    }
    
    return t;
}

static bool is_alphabetic(char32_t c) {
    return !(isdigit(c) || ispunct(c) || isspace(c) || isblank(c) || iscntrl(c));
}

static Token identifier_or_keyword(Source_Iterator &src) {
    char *_word = src.cur;
    char *_word_end = src.cur;
    while (is_alphabetic(src.peek())) {
        src.next();
        _word_end = src.cur;
    }
    String word(_word, _word_end - _word);
    
    Token t;
    
    if (word == "true") {
        t.kind = Token_Kind::True;
    } else if (word == "false") {
        t.kind = Token_Kind::False;
    } else if (word == "let") {
        t.kind = Token_Kind::Let;
    } else if (word == "mut") {
        t.kind = Token_Kind::Mut;
    } else if (word == "if") {
        t.kind = Token_Kind::If;
    } else if (word == "else") {
        t.kind = Token_Kind::Else;
    } else if (word == "while") {
        t.kind = Token_Kind::While;
    } else {
        t.kind = Token_Kind::Ident;
        t.data.s = word;
    }
    
    return t;
}

static Token next_token(Source_Iterator &src) {
    auto c = src.peek();
    Token t;
    if (isdigit(c)) {
        t = number(src);
    } else if (c == '\'') {
        t = character(src);
    } else if (c == '"') {
        t = string(src);
    } else if (ispunct(c)) {
        t = punctuation(src);
    } else {
        t = identifier_or_keyword(src);
    }
    return t;
}

std::vector<Token> tokenize(String source) {
    std::vector<Token> tokens;
    Source_Iterator src = { source.begin(), source.end() };
    
    while (true) {
        src.skip_whitespace();
        if (!src.has_more()) break;
        tokens.push_back(next_token(src));
    }
    
    tokens.push_back(eof_token());
    
    return tokens;
}
