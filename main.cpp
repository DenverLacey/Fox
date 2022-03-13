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
// [ ]  Simplify Array stuff now that slices aren't coupled with them anymore.
// [ ]  Diverge variadic function calls and returns from normal calls and returns
//      to alleviate overhead.
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
