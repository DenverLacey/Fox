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

static void error_impl(const char *err_type, const char *err_msg, va_list args) {
    fprintf(stderr, "%s", err_type);
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
    error_impl("Error: ", err, args);
    exit(EXIT_FAILURE);
}

[[noreturn]]
void internal_error(const char *err, ...) {
    va_list args;
    va_start(args, err);
    internal_error(err, args);
    va_end(args);
}

[[noreturn]]
void internal_error(const char *err, va_list args) {
    error_impl("Internal Error: ", err, args);
    assert(false);
}

[[noreturn]]
void todo(const char *err, ...) {
    va_list args;
    va_start(args, err);
    todo(err, args);
    va_end(args);
}

[[noreturn]]
void todo(const char *err, va_list args) {
    error_impl("Todo: ", err, args);
    assert(false);
}
