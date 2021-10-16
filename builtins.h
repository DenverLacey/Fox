//
//  intrinsics.h
//  Fox
//
//  Created by Denver Lacey on 16/10/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#pragma once
#include "value.h"

typedef void(*Builtin)(struct Stack &stack, Address arg_start);

struct Builtin_Definition {
    Builtin builtin;
    Value_Type type;
};

void load_builtins(struct Interpreter *interp);
