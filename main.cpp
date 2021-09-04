//
//  main.cpp
//  Fox
//
//  Created by Denver Lacey on 7/7/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

// @TODOS:
// [x]  Beef-up the tokenizer to accept more complicated number literals and
//      identifiers.
// [x]  Beef-up the tokenizer to accept more complicated string and char literals
//      with escape characters.
// [x]  Add negation operation.
// [ ]  Maybe add absolute-value operator (unary +)?
// [x]  Add array types.
// [ ]  Add slice types.
// [x]  Implement Allocation strategy for strings.
// [x]  Implement Allocation strategy for AST Nodes.
// [x]  Fix issue where tokenizer confuses Dot_Tuple for a floating point literal.
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
