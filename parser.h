//
//  parser.h
//  Fox
//
//  Created by Denver Lacey on 7/7/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#pragma once

#include "mem.h"
#include "ast.h"
#include "tokenizer.h"

Ref<Untyped_AST_Block> parse(const std::vector<Token> &tokens);
