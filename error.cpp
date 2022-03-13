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

static void internal_error_impl(const char *err_type, const char *file, size_t line, const char *err_msg, va_list args) {
    fprintf(stderr, "%s:%zu: %s", file, line, err_type);
    vfprintf(stderr, err_msg, args);
    fprintf(stderr, "\n");
}

static void error_impl(Code_Location loc, const char *err, va_list args) {
    fprintf(stderr, "%s:%zu:%zu: Error: ", loc.filename, loc.l0 + 1, loc.c0 + 1);
    vfprintf(stderr, err, args);
    fprintf(stderr, "\n");
}

[[noreturn]]
void error(Code_Location loc, const char *err, ...) {
    va_list args;
    va_start(args, err);
    error(loc, err, args);
    va_end(args);
}

[[noreturn]]
void error(Code_Location loc, const char *err, va_list args) {
    error_impl(loc, err, args);
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
    internal_error_impl("Internal Error: ", file, line, err, args);
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
    internal_error_impl("Todo: ", file, line, err, args);
    exit(EXIT_FAILURE);
}
