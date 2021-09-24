//
//  error.h
//  Fox
//
//  Created by Denver Lacey on 7/7/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#pragma once

#include <stdarg.h>

[[noreturn]] void error(const char *err, ...);
[[noreturn]] void error(const char *err, va_list args);

#define verify(cond, ...) \
    if (!(cond)) error(__VA_ARGS__)

[[noreturn]] void internal_error(const char *err, ...);
[[noreturn]] void internal_error(const char *err, va_list args);

#define internal_verify(cond, ...) \
    if (!(cond)) internal_error(__VA_ARGS__)

[[noreturn]] void todo(const char *err, ...);
[[noreturn]] void todo(const char *err, va_list args);
