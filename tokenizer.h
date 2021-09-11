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
    Comma,          // ,
    Left_Paren,     // (
    Right_Paren,    // )
    Left_Curly,     // {
    Right_Curly,    // }
    Left_Bracket,   // [
    Right_Bracket,  // ]
    
    // keyword
    Underscore,
    Let,
    Mut,
    If,
    Else,
    Then,
    While,
    For,
    Fn,
    Struct,
    Enum,
    And,
    Or,
    In,
    
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
    
    void print() const;
};

std::vector<Token> tokenize(String source);
