//
//  main.cpp
//  Fox
//
//  Created by Denver Lacey on 7/7/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

// @TODOS:
// [ ]  Handle array subscript for constant arrays.
// [ ]  Maybe handle dereferencing constant pointers.
// [ ]  Implement Paths and sort out how that works with Idents etc.
// [x]  Implement enums.
// [x]  Fully implement enums with payloads.
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
