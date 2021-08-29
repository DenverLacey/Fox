//
//  main.cpp
//  Fox
//
//  Created by Denver Lacey on 7/7/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

// @TODOS:
// [x]  Do the stack_top stuff soon or else you'll need to do it for everything
//      like last time and you'll make a mistake.
// [x]  Actually setup the reading from a source file instead of using a String
//      to pass source code.
// [x]  Fix issue where something can't be deleted because it was never allocated.
//      Issue was caused by Scope being a struct defined in typer.cpp and
//      compiler.h. Now that they are called Typer_Scope and Compiler_Scope
//      respectively, the issue is fixed.
// [x]  Add checking that the assignment operators only appear in statement
//      context. Right now it works like C where they are expressions.
// [x]  Fix bug where comments at the end of the file trip up skip_whitespace() in
//      tokenizer.
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
