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

[[noreturn]] void error(const char *err, ...);
[[noreturn]] void error(const char *err, va_list args);

#define verify(cond, ...) \
    if (!(cond)) error(__VA_ARGS__)

[[noreturn]] void __private_internal_error(const char *file, size_t line, const char *err, ...);
[[noreturn]] void __private_internal_error(const char *file, size_t line, const char *err, va_list args);

#define internal_error(...) \
    __private_internal_error(__FILE__, __LINE__, __VA_ARGS__)

#define internal_verify(cond, ...) \
    if (!(cond)) internal_error(__VA_ARGS__)

[[noreturn]] void __private_todo(const char *file, size_t line, const char *err, ...);
[[noreturn]] void __private_todo(const char *file, size_t line, const char *err, va_list args);

#define todo(...) \
    __private_todo(__FILE__, __LINE__, __VA_ARGS__)
