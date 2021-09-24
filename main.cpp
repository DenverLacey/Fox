//
//  main.cpp
//  Fox
//
//  Created by Denver Lacey on 7/7/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

// @TODOS:
// [ ]  Maybe get rid of AST_Type_Signiture cause it might be pointless and easier
//      just to use Ref<Value_Type>.
// [x]  Implement return checking.
// [x]  Change variable declarations without initializers to accept immutable arrays
//      where the element type is mutable.
// [ ]  Handle array subscript for constant arrays.
// [ ]  Maybe handle dereferencing constant pointers.
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
