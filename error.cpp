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

static void error_impl(const char *err_type, const char *err_msg, va_list args) {
    fprintf(stderr, err_type);
    vfprintf(stderr, err_msg, args);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

void error(const char *err, ...) {
    va_list args;
    va_start(args, err);
    error(err, args);
    va_end(args);
}

void error(const char *err, va_list args) {
    error_impl("Error: ", err, args);
}

void internal_error(const char *err, ...) {
    va_list args;
    va_start(args, err);
    internal_error(err, args);
    va_end(args);
}

void internal_error(const char *err, va_list args) {
    error_impl("Internal Error: ", err, args);
}
