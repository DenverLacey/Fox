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
// [x]  Implement Typed_AST_For::compile().
// [ ]  Add increment and decrement opcode in vm as an optimisation?
// [ ]  Use increment opcode in for-range loop.
// [ ]  Implement compile-time code execution.
// [x]  Support negative indexing subscript (only with literals).
// [x]  Refactor compile_negative_subscript_operator to work with the dynamic code
//      stuff cause then we can assign to an expression like that too.
//

#include <iostream>

#include "vm.h"

int main(int argc, const char * argv[]) {
    if (argc > 1) {
        interpret(argv[1]);
    } else {
        printf("Error: No path given. Fox needs to know what to compile to run.\n");
    }
}
