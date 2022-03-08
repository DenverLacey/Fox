//
//  error.cpp
//  Fox
//
//  Created by Denver Lacey on 7/7/21.
//  Copyright © 2021 Denver Lacey. All rights reserved.
//

#include "error.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

static void error_impl(const char *err_type, const char *file, size_t line, const char *err_msg, va_list args) {
    if (file == nullptr) {
        fprintf(stderr, "%s", err_type);
    } else {
        fprintf(stderr, "%s:%zu: %s", file, line, err_type);
    }
    vfprintf(stderr, err_msg, args);
    fprintf(stderr, "\n");
}

[[noreturn]]
void error(const char *err, ...) {
    va_list args;
    va_start(args, err);
    error(err, args);
    va_end(args);
}

[[noreturn]]
void error(const char *err, va_list args) {
    error_impl("Error: ", nullptr, 0, err, args);
    exit(EXIT_FAILURE);
}

[[noreturn]]
void __private_internal_error(const char *file, size_t line, const char *err, ...) {
    va_list args;
    va_start(args, err);
    __private_internal_error(file, line, err, args);
    va_end(args);
}

[[noreturn]]
void __private_internal_error(const char *file, size_t line, const char *err, va_list args) {
    error_impl("Internal Error: ", file, line, err, args);
    exit(EXIT_FAILURE);
}

[[noreturn]]
void __private_todo(const char *file, size_t line, const char *err, ...) {
    va_list args;
    va_start(args, err);
    __private_todo(file, line, err, args);
    va_end(args);
}

[[noreturn]]
void __private_todo(const char *file, size_t line, const char *err, va_list args) {
    error_impl("Todo: ", file, line, err, args);
    exit(EXIT_FAILURE);
}
