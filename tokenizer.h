//
//  tokenizer.h
//  Fox
//
//  Created by Denver Lacey on 7/7/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#pragma once
#include <cstddef>
#include <vector>
#include "String.h"
#include "definitions.h"

enum class Token_Kind {
    // internal
    Eof,
    Err,
    
    // literals
    True,
    False,
    Int,
    Float,
    Char,
    String,
    Ident,
    
    // delimeters
    Semi,           // ;
    Colon,          // :
    Double_Colon,   // ::
    Comma,          // ,
    Left_Paren,     // (
    Right_Paren,    // )
    Left_Curly,     // {
    Right_Curly,    // }
    Left_Bracket,   // [
    Right_Bracket,  // ]
    At,             // @
    
    // Arrows
    Thin_Right_Arrow,   // ->
    Fat_Right_Arrow,    // =>
    
    // keyword
    Underscore,
    Noinit,
    Const,
    Let,
    Mut,
    If,
    Else,
    While,
    For,
    Match,
    Defer,
    Fn,
    Struct,
    Enum,
    Trait,
    Impl,
    And,
    Or,
    In,
    Return,
    Import,
    As,
    Vararg,
    
    // operators
    Plus,           // +
    Plus_Eq,        // +=
    Dash,           // -
    Dash_Eq,        // -=
    Star,           // *
    Star_Eq,        // *=
    Slash,          // /
    Slash_Eq,       // /=
    Percent,        // %
    Percent_Eq,     // %=
    Bang,           // !
    Bang_Eq,        // !=
    Eq,             // =
    Double_Eq,      // ==
    Left_Angle,     // <
    Left_Angle_Eq,  // <=
    Right_Angle,    // >
    Right_Angle_Eq, // >=
    Ampersand,      // &
    Ampersand_Mut,  // &mut
    Dot,            // .
    Double_Dot,     // ..
    Triple_Dot,     // ...
};

union Token_Data {
    int64_t i;
    double f;
    char32_t c;
    String s;
};

struct Token {
    Token_Kind kind;
    Token_Data data;
    Code_Location location;
    
    void print() const;
};

std::vector<Token> tokenize(String source, const char *filename);
