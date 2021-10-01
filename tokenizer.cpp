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
#include "mem.h"

void Token::print() const {
    switch (kind) {
        case Token_Kind::Eof:
            printf("EOF\n");
            break;
        case Token_Kind::Err:
            printf("Err (%.*s)\n", data.s.size(), data.s.c_str());
            break;
        case Token_Kind::True:
            printf("True\n");
            break;
        case Token_Kind::False:
            printf("False\n");
            break;
        case Token_Kind::Int:
            printf("Int (%lld)\n", data.i);
            break;
        case Token_Kind::Float:
            printf("Float (%f)\n", data.f);
            break;
        case Token_Kind::Char:
            printf("Char '%s'\n", utf8char_t::from_char32(data.c).buf);
            break;
        case Token_Kind::String:
            printf("String \"%.*s\"\n", data.s.size(), data.s.c_str());
            break;
        case Token_Kind::Ident:
            printf("Ident `%.*s`\n", data.s.size(), data.s.c_str());
            break;
        case Token_Kind::Semi:
            printf("Semi\n");
            break;
        case Token_Kind::Colon:
            printf("Colon\n");
            break;
        case Token_Kind::Double_Colon:
            printf("Double_Colon\n");
            break;
        case Token_Kind::Comma:
            printf("Comma\n");
            break;
        case Token_Kind::Left_Paren:
            printf("Left_Paren\n");
            break;
        case Token_Kind::Right_Paren:
            printf("Right_Paren\n");
            break;
        case Token_Kind::Left_Curly:
            printf("Left_Curly\n");
            break;
        case Token_Kind::Right_Curly:
            printf("Right_Curly\n");
            break;
        case Token_Kind::Left_Bracket:
            printf("Left_Bracket\n");
            break;
        case Token_Kind::Right_Bracket:
            printf("Right_Bracket\n");
            break;
        case Token_Kind::Thin_Right_Arrow:
            printf("Thin_Right_Arrow\n");
            break;
        case Token_Kind::Fat_Right_Arrow:
            printf("Fat_Right_Arrow\n");
            break;
        case Token_Kind::Underscore:
            printf("Underscore\n");
            break;
        case Token_Kind::Noinit:
            printf("Noinit\n");
            break;
        case Token_Kind::Let:
            printf("Let\n");
            break;
        case Token_Kind::Mut:
            printf("Mut\n");
            break;
        case Token_Kind::Const:
            printf("Const\n");
            break;
        case Token_Kind::If:
            printf("If\n");
            break;
        case Token_Kind::Else:
            printf("Else\n");
            break;
        case Token_Kind::While:
            printf("While\n");
            break;
        case Token_Kind::For:
            printf("For\n");
            break;
        case Token_Kind::Match:
            printf("Match\n");
            break;
        case Token_Kind::Fn:
            printf("Fn\n");
            break;
        case Token_Kind::Struct:
            printf("Struct\n");
            break;
        case Token_Kind::Enum:
            printf("Enum\n");
            break;
        case Token_Kind::Impl:
            printf("Impl\n");
            break;
        case Token_Kind::And:
            printf("And\n");
            break;
        case Token_Kind::Or:
            printf("Or\n");
            break;
        case Token_Kind::In:
            printf("In\n");
            break;
        case Token_Kind::Return:
            printf("Return\n");
            break;
        case Token_Kind::Plus:
            printf("Plus\n");
            break;
        case Token_Kind::Plus_Eq:
            printf("Plus_Eq\n");
            break;
        case Token_Kind::Dash:
            printf("Dash\n");
            break;
        case Token_Kind::Dash_Eq:
            printf("Dash_Eq\n");
            break;
        case Token_Kind::Star:
            printf("Star\n");
            break;
        case Token_Kind::Star_Eq:
            printf("Star_Eq\n");
            break;
        case Token_Kind::Slash:
            printf("Slash\n");
            break;
        case Token_Kind::Slash_Eq:
            printf("Slash_Eq\n");
            break;
        case Token_Kind::Percent:
            printf("Percent\n");
            break;
        case Token_Kind::Percent_Eq:
            printf("Percent_Eq\n");
            break;
        case Token_Kind::Bang:
            printf("Bang\n");
            break;
        case Token_Kind::Bang_Eq:
            printf("Bang_Eq\n");
            break;
        case Token_Kind::Eq:
            printf("Eq\n");
            break;
        case Token_Kind::Double_Eq:
            printf("Double_Eq\n");
            break;
        case Token_Kind::Left_Angle:
            printf("Left_Angle\n");
            break;
        case Token_Kind::Left_Angle_Eq:
            printf("Left_Angle_Eq\n");
            break;
        case Token_Kind::Right_Angle:
            printf("Right_Angle\n");
            break;
        case Token_Kind::Right_Angle_Eq:
            printf("Right_Angle_Eq\n");
            break;
        case Token_Kind::Ampersand:
            printf("Ampersand\n");
            break;
        case Token_Kind::Ampersand_Mut:
            printf("Ampersand_Mut\n");
            break;
        case Token_Kind::Dot:
            printf("Dot\n");
            break;
        case Token_Kind::Double_Dot:
            printf("Double_Dot\n");
            break;
        case Token_Kind::Triple_Dot:
            printf("Triple_Dot\n");
            break;
            
        default:
            internal_error("Unknown Token_Kind: %d.", kind);
            break;
    }
}

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

struct Tokenizer {
    char *cur;
    char *end;
    std::vector<Token> tokens;
    
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

static bool might_evaluate_to_a_tuple(Token tok) {
    switch (tok.kind) {
        case Token_Kind::Ident:
        case Token_Kind::Right_Paren:
        case Token_Kind::Right_Bracket:
            return true;
    }
    return false;
}

static bool is_beginning_of_number(Tokenizer &t) {
    if (isdigit(t.peek()))
        return true;
    
    bool result = false;
    if (t.tokens.empty()) {
        result = t.peek() == '.' && isdigit(t.peek(1));
    } else if (t.peek() == '.') {
        auto previous = t.tokens.back();
        bool maybe_tuple = might_evaluate_to_a_tuple(previous);
        result = !maybe_tuple && isdigit(t.peek(1));
    }
    
    return result;
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

static Token number(Tokenizer &t) {
    char *word = t.cur;
    if (t.peek() == '-') t.next();
    char *word_end = t.cur;
    bool underscores = false;
    
    while (isdigit(t.peek()) || t.peek() == '_') {
        if (t.peek() == '_') underscores = true;
        t.next();
        word_end = t.cur;
    }
    
    bool is_float = false;
    if (t.peek() == '.' && (isdigit(t.peek(1)) || t.peek(1) == '_')) {
        t.next();
        is_float = true;
        while (isdigit(t.peek()) || t.peek() == '_') {
            if (t.peek() == '_') underscores = true;
            t.next();
            word_end = t.cur;
        }
    }
    
    size_t len = word_end - word;
    char *num_str = SMem.duplicate(word, len);
    if (underscores) remove_underscores(num_str, len);
    Token tok;
    
    if (is_float) {
        tok.kind = Token_Kind::Float;
        tok.data.f = atof(num_str);
    } else {
        tok.kind = Token_Kind::Int;
        tok.data.i = atoll(num_str);
    }
    
    SMem.deallocate(num_str, len);
    
    return tok;
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

static Token character(Tokenizer &t) {
    verify(t.next() == '\'', "Character literals must start with a '.");
    
    char *word = t.cur;
    char *word_end = t.cur;
    bool escape_sequences = false;
    while (t.peek() != '\'') {
        if (t.peek() == '\\') {
            escape_sequences = true;
            t.next();
        }
        t.next();
        word_end = t.cur;
    }
    
    verify(t.next() == '\'', "Character literals must end with a '.");
    
    size_t size = word_end - word;
    char *cs = SMem.duplicate(word, size);
    if (escape_sequences) size = replace_escape_sequence(cs, size);
    auto len = utf8::distance(cs, &cs[size]);
    verify(len == 1, "Character literals must contain exactly one character.");
    char32_t c = utf8::peek_next(cs, &cs[size]);
    SMem.deallocate(cs, word_end - word);
    
    Token tok;
    tok.kind = Token_Kind::Char;
    tok.data.c = c;
    return tok;
}

static Token string(Tokenizer &t) {
    verify(t.next() == '"', "String literals must start with a \".");
    
    char *word = t.cur;
    char *word_end = t.cur;
    bool escape_sequences = false;
    while (t.peek() != '"') {
        if (t.peek() == '\\') {
            escape_sequences = true;
            t.next();
        }
        t.next();
        word_end = t.cur;
    }
    
    verify(t.next() == '"', "String literals must end with a \".");
    
    size_t len = word_end - word;
    char *cs = SMem.duplicate(word, len);
    if (escape_sequences) len = replace_escape_sequence(cs, len);
    String s(cs, len);
    
    Token tok;
    tok.kind = Token_Kind::String;
    tok.data.s = s;
    return tok;
}

static Token punctuation(Tokenizer &t) {
    Token tok;
    
    auto c = t.next();
    switch (c) {
        // delimeters
        case ';': tok.kind = Token_Kind::Semi; break;
        case ':':
            if (t.match(':')) {
                tok.kind = Token_Kind::Double_Colon;
            } else {
                tok.kind = Token_Kind::Colon;
            }
            break;
        case ',': tok.kind = Token_Kind::Comma; break;
        case '(': tok.kind = Token_Kind::Left_Paren; break;
        case ')': tok.kind = Token_Kind::Right_Paren; break;
        case '{': tok.kind = Token_Kind::Left_Curly; break;
        case '}': tok.kind = Token_Kind::Right_Curly; break;
        case '[': tok.kind = Token_Kind::Left_Bracket; break;
        case ']': tok.kind = Token_Kind::Right_Bracket; break;
            
        // keywords
        case '_': tok.kind = Token_Kind::Underscore; break;
            
        // operators
        case '+':
            if (t.match('=')) {
                tok.kind = Token_Kind::Plus_Eq;
            } else {
                tok.kind = Token_Kind::Plus;
            }
            break;
        case '-':
            if (t.match('=')) {
                tok.kind = Token_Kind::Dash_Eq;
            } else if (t.match('>')) {
                tok.kind = Token_Kind::Thin_Right_Arrow;
            } else {
                tok.kind = Token_Kind::Dash;
            }
            break;
        case '*':
            if (t.match('=')) {
                tok.kind = Token_Kind::Star_Eq;
            } else {
                tok.kind = Token_Kind::Star;
            }
            break;
        case '/':
            if (t.match('=')) {
                tok.kind = Token_Kind::Slash_Eq;
            } else {
                tok.kind = Token_Kind::Slash;
            }
            break;
        case '%':
            if (t.match('=')) {
                tok.kind = Token_Kind::Percent_Eq;
            } else {
                tok.kind = Token_Kind::Percent;
            }
            break;
        case '!':
            if (t.match('=')) {
                tok.kind = Token_Kind::Bang_Eq;
            } else {
                tok.kind = Token_Kind::Bang;
            }
            break;
        case '=':
            if (t.match('=')) {
                tok.kind = Token_Kind::Double_Eq;
            } else if (t.match('>')) {
                tok.kind = Token_Kind::Fat_Right_Arrow;
            } else {
                tok.kind = Token_Kind::Eq;
            }
            break;
        case '<':
            if (t.match('=')) {
                tok.kind = Token_Kind::Left_Angle_Eq;
            } else {
                tok.kind = Token_Kind::Left_Angle;
            }
            break;
        case '>':
            if (t.match('=')) {
                tok.kind = Token_Kind::Right_Angle_Eq;
            } else {
                tok.kind = Token_Kind::Right_Angle;
            }
            break;
        case '&':
            if (t.match('=')) {
                todo("'&=' token not yet implemented.");
            } else if (t.match("mut")) {
                tok.kind = Token_Kind::Ampersand_Mut;
            } else {
                tok.kind = Token_Kind::Ampersand;
            }
            break;
        case '.':
            if (t.match('.')) {
                if (t.match('.')) {
                    tok.kind = Token_Kind::Triple_Dot;
                } else {
                    tok.kind = Token_Kind::Double_Dot;
                }
            } else {
                tok.kind = Token_Kind::Dot;
            }
            break;
            
        default:
            auto _c = utf8char_t::from_char32(c);
            error("Unexpected punctuation '%s'.\n", _c.buf);
    }
    
    return tok;
}

static bool is_ident_char(char32_t c) {
    return c == '_' || !(ispunct(c) || isspace(c) || isblank(c) || iscntrl(c));
}

static Token identifier_or_keyword(Tokenizer &t) {
    char *_word = t.cur;
    char *_word_end = t.cur;
    while (is_ident_char(t.peek())) {
        t.next();
        _word_end = t.cur;
    }
    String word(_word, _word_end - _word);
    
    Token tok;
    
    if (word == "true") {
        tok.kind = Token_Kind::True;
    } else if (word == "false") {
        tok.kind = Token_Kind::False;
    } else if (word == "noinit") {
        tok.kind = Token_Kind::Noinit;
    } else if (word == "let") {
        tok.kind = Token_Kind::Let;
    } else if (word == "const") {
        tok.kind = Token_Kind::Const;
    } else if (word == "mut") {
        tok.kind = Token_Kind::Mut;
    } else if (word == "if") {
        tok.kind = Token_Kind::If;
    } else if (word == "else") {
        tok.kind = Token_Kind::Else;
    } else if (word == "while") {
        tok.kind = Token_Kind::While;
    } else if (word == "for") {
        tok.kind = Token_Kind::For;
    } else if (word == "match") {
        tok.kind = Token_Kind::Match;
    } else if (word == "fn") {
        tok.kind = Token_Kind::Fn;
    } else if (word == "struct") {
        tok.kind = Token_Kind::Struct;
    } else if (word == "enum") {
        tok.kind = Token_Kind::Enum;
    } else if (word == "impl") {
        tok.kind = Token_Kind::Impl;
    } else if (word == "and") {
        tok.kind = Token_Kind::And;
    } else if (word == "or") {
        tok.kind = Token_Kind::Or;
    } else if (word == "in") {
        tok.kind = Token_Kind::In;
    } else if (word == "return") {
        tok.kind = Token_Kind::Return;
    } else {
        tok.kind = Token_Kind::Ident;
        tok.data.s = word;
    }
    
    return tok;
}

static Token next_token(Tokenizer &t) {
    auto c = t.peek();
    Token tok;
    if (is_beginning_of_number(t)) {
        tok = number(t);
    } else if (c == '\'') {
        tok = character(t);
    } else if (c == '"') {
        tok = string(t);
    } else if (ispunct(c)) {
        tok = punctuation(t);
    } else {
        tok = identifier_or_keyword(t);
    }
    return tok;
}

std::vector<Token> tokenize(String source) {
    auto t = Tokenizer { source.begin(), source.end() };
    
    while (true) {
        t.skip_whitespace();
        if (!t.has_more()) break;
        t.tokens.push_back(next_token(t));
    }
    
    t.tokens.push_back(eof_token());
    
    return t.tokens;
}
