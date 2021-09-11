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
// [x]  Add the concept of patterns
// [x]  Use patterns in variable declarations.
// [ ]  Use patterns in for-loop.
// [x]  Adapt typechecking for let statements to using patterns.
// [ ]  Rework if statements such that there can be else-if blocks and then-if blocks?
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
