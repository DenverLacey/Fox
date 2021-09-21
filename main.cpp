//
//  main.cpp
//  Fox
//
//  Created by Denver Lacey on 7/7/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

// @TODOS:
// [x]  Add negation operation.
// [ ]  Add absolute-value operator (unary +)?
// [ ]  Add increment and decrement opcode in vm as an optimisation?
// [ ]  Use increment opcode in for-range loop.
// [x]  Implement compile-time code execution.
// [ ]  Maybe get rid of AST_Type_Signiture cause it might be pointless and easier
//      just to use Ref<Value_Type>.
// [ ]  Implement mutable function parameters.
// [ ]  Implement return checking.
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
