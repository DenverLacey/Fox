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
    
    char32_t peek(size_t n) const {
        char *it = cur;
        while (n-- > 0) {
            utf8::next(it, end);
        }
        return utf8::next(it, end);
    }
    
    bool match(char32_t c) {
        if (peek() == c) {
            next();
            return true;
        }
        return false;
    }
    
    bool match(const char *s) {
        size_t len = strlen(s);
        auto num_remaining = end - cur;
        if (num_remaining < len) return false;
        if (memcmp(s, cur, len) != 0) return false;
        cur += len;
        return true;
    }
    
    void skip_whitespace() {
        while ((isspace(peek()) || peek() == '#') && has_more()) {
            if (peek() == '#') {
                while (peek() != '\n' && has_more()) {
                    // keep getting next until we've passed the new line
                    next();
                }
            }
            if (peek() != '\0') next();
        }
    }
};

static bool is_beginning_of_number(Source_Iterator &src) {
    return isdigit(src.peek()) ||
    ((src.peek() == '-' || src.peek() == '.') && isdigit(src.peek(1))) ||
    ((src.peek() == '-' && src.peek(1) == '.') && isdigit(src.peek(2)));
}

static void remove_underscores(char *s, size_t len) {
    int i = 0;
    while (i < len) {
        if (s[i] == '_') {
            memmove(&s[i], &s[i + 1], len - i);
            continue;
        }
        i++;
    }
}

static Token number(Source_Iterator &src) {
    char *word = src.cur;
    if (src.peek() == '-') src.next();
    char *word_end = src.cur;
    bool underscores = false;
    
    while (isdigit(src.peek()) || src.peek() == '_') {
        if (src.peek() == '_') underscores = true;
        src.next();
        word_end = src.cur;
    }
    
    bool is_float = false;
    if (src.match('.')) {
        is_float = true;
        while (isdigit(src.peek()) || src.peek() == '_') {
            if (src.peek() == '_') underscores = true;
            src.next();
            word_end = src.cur;
        }
    }
    
    size_t len = word_end - word;
    char *num_str = strndup(word, len);
    if (underscores) remove_underscores(num_str, len);
    Token t;
    
    if (is_float) {
        t.kind = Token_Kind::Float;
        t.data.f = atof(num_str);
    } else {
        t.kind = Token_Kind::Int;
        t.data.i = atoll(num_str);
    }
    
    free(num_str);
    
    return t;
}

static size_t replace_escape_sequence(char *s, size_t len) {
    int i = 0;
    while (i < len) {
        if (s[i] != '\\') {
            i++;
            continue;
        }
        
        switch (s[i+1]) {
            case '0':
                s[i] = '\0';
                break;
            case 'n':
                s[i] = '\n';
                break;
            case 't':
                s[i] = '\t';
                break;
            case '\\':
                s[i] = '\\';
                break;
            case '"':
                s[i] = '"';
                break;
            case '\'':
                s[i] = '\'';
                break;
                
            default:
                error("Expected an escape sequence but got '%.*s'", len-i, &s[i]);
                break;
        }
        
        memmove(&s[i+1], &s[i+2], len - i - 1);
        i++;
        len--;
    }
    
    return len;
}

static Token character(Source_Iterator &src) {
    verify(src.next() == '\'', "Character literals must start with a '.");
    
    char *word = src.cur;
    char *word_end = src.cur;
    bool escape_sequences = false;
    while (src.peek() != '"') {
        if (src.peek() == '\\') {
            escape_sequences = true;
            src.next();
        }
        src.next();
        word_end = src.cur;
    }
    
    verify(src.next() == '\'', "Character literals must end with a '.");
    
    size_t len = word_end - word;
    char *cs = strndup(word, len);
    if (escape_sequences) len = replace_escape_sequence(cs, len);
    verify(len == 1, "Character literals must contain exactly one character.");
    char c = *cs;
    free(cs);
    
    Token t;
    t.kind = Token_Kind::Char;
    t.data.c = c;
    return t;
}

static Token string(Source_Iterator &src) {
    verify(src.next() == '"', "String literals must start with a \".");
    
    char *word = src.cur;
    char *word_end = src.cur;
    bool escape_sequences = false;
    while (src.peek() != '"') {
        if (src.peek() == '\\') {
            escape_sequences = true;
            src.next();
        }
        src.next();
        word_end = src.cur;
    }
    
    verify(src.next() == '"', "String literals must end with a \".");
    
    size_t len = word_end - word;
    char *cs = strndup(word, len);
    if (escape_sequences) len = replace_escape_sequence(cs, len);
    String s(cs, len);
    
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
        case '[': t.kind = Token_Kind::Left_Bracket; break;
        case ']': t.kind = Token_Kind::Right_Bracket; break;
            
        // keywords
        case '_': t.kind = Token_Kind::Underscore; break;
            
        // operators
        case '+':
            if (src.match('=')) {
                t.kind = Token_Kind::Plus_Eq;
            } else {
                t.kind = Token_Kind::Plus;
            }
            break;
        case '-':
            if (src.match('=')) {
                t.kind = Token_Kind::Dash_Eq;
            } else {
                t.kind = Token_Kind::Dash;
            }
            break;
        case '*':
            if (src.match('=')) {
                t.kind = Token_Kind::Star_Eq;
            } else {
                t.kind = Token_Kind::Star;
            }
            break;
        case '/':
            if (src.match('=')) {
                t.kind = Token_Kind::Slash_Eq;
            } else {
                t.kind = Token_Kind::Slash;
            }
            break;
        case '%':
            if (src.match('=')) {
                t.kind = Token_Kind::Percent_Eq;
            } else {
                t.kind = Token_Kind::Percent;
            }
            break;
        case '!':
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
        case '&':
            if (src.match('=')) {
                assert(false);
            } else if (src.match("mut")) {
                t.kind = Token_Kind::Ampersand_Mut;
            } else {
                t.kind = Token_Kind::Ampersand;
            }
            break;
        case '.':
            if (src.match('.')) {
                assert(false);
                if (src.match('=')) {
                    
                } else {
                    
                }
            } else {
                t.kind = Token_Kind::Dot;
            }
            break;
            
        default:
            auto _c = utf8char_t::from_char32(c);
            error("Unexpected punctuation '%s'.\n", _c.buf);
    }
    
    return t;
}

static bool is_ident_char(char32_t c) {
    return c == '_' || !(ispunct(c) || isspace(c) || isblank(c) || iscntrl(c));
}

static Token identifier_or_keyword(Source_Iterator &src) {
    char *_word = src.cur;
    char *_word_end = src.cur;
    while (is_ident_char(src.peek())) {
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
    } else if (word == "and") {
        t.kind = Token_Kind::And;
    } else if (word == "or") {
        t.kind = Token_Kind::Or;
    } else {
        t.kind = Token_Kind::Ident;
        t.data.s = word;
    }
    
    return t;
}

static Token next_token(Source_Iterator &src) {
    auto c = src.peek();
    Token t;
    if (is_beginning_of_number(src)) {
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
