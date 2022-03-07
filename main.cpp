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
// [x]  Add pre-pass to impl blocks so that code doesn't have to be defined in
//      a specific order due to dependencies.
// [ ]  Change print builtins to just be '@print' and do typechecking to resolve.
// [x]  Add puts style builtins.
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
