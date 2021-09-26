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
// [ ]  Handle array subscript for constant arrays.
// [ ]  Maybe handle dereferencing constant pointers.
// [x]  Merge Typed_AST_Field_Access and Typed_AST_Field_Access_Tuple.
// [ ]  Implement typechecking for match statements.
// [ ]  Implement compilation for match statements.
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
