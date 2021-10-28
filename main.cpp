//
//  main.cpp
//  Fox
//
//  Created by Denver Lacey on 7/7/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

//
// Rust Language Reference:
//      https://doc.rust-lang.org/stable/reference/introduction.html
// Zig Language Reference:
//      https://ziglang.org/documentation/master/
//

// @TODOS:
// [x]  Handle array subscript for constant arrays.
// [ ]  Maybe handle dereferencing constant pointers.
// [ ]  Handle dot calls through pointers and handle all the deref stuff too.
// [ ]  Add way to access slice length and data pointer.
//

#include <iostream>

#include "interpreter.h"

int main(int argc, const char * argv[]) {
    if (argc > 1) {
        Interpreter interp;
        interp.interpret(argv[1]);
    } else {
        printf("Error: No path given. Fox needs to know what to compile to run.\n");
    }
}
