//
//  error.h
//  Fox
//
//  Created by Denver Lacey on 7/7/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#pragma once

#include <stdarg.h>
#include <stddef.h>
#include "definitions.h"

[[noreturn]] void error(Code_Location loc, const char *err, ...);
[[noreturn]] void verror(Code_Location loc, const char *err, va_list args);

#define verify(cond, ...) \
    if (!(cond)) error(__VA_ARGS__)
#define vverify(cond, loc, err, args) \
    if (!(cond)) verror(loc, err, args)

[[noreturn]] void __private_internal_error(const char *file, size_t line, const char *err, ...);

#define internal_error(...) \
    __private_internal_error(__FILE__, __LINE__, __VA_ARGS__)

#define internal_verify(cond, ...) \
    if (!(cond)) internal_error(__VA_ARGS__)

[[noreturn]] void __private_todo(const char *file, size_t line, const char *err, ...);

#define todo(...) \
    __private_todo(__FILE__, __LINE__, __VA_ARGS__)
