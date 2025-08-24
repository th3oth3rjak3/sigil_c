// File:    value.c
// Purpose: implementation of value.h
// Author:  Jake Hathaway
// Date:    2025-08-17

#include "value.h"
#include "memory.h"
#include "object.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

void
init_value_array(ValueArray* array) {
    array->values = NULL;
    array->count = 0;
    array->capacity = 0;
}

void
write_value_array(ValueArray* array, Value value) {
    if (array->capacity < array->count + 1) {
        int old_capacity = array->capacity;
        array->capacity = GROW_CAPACITY(old_capacity);
        array->values =
            GROW_ARRAY(Value, array->values, old_capacity, array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

void
free_value_array(ValueArray* array) {
    FREE_ARRAY(Value, array->values, array->capacity);
    init_value_array(array);
}

void
print_value(Value value) {
    switch (value.type) {
        case VAL_BOOL:
            printf(AS_BOOL(value) ? "true" : "false");
            break;
        case VAL_NIL:
            printf("nil");
            break;
        case VAL_NUMBER: {
            ObjString* str = number_to_string(AS_NUMBER(value));
            printf("%s", str->chars);
            break;
        }
        case VAL_OBJ:
            print_object(value);
            break;
    }
}

bool
values_equal(Value a, Value b) {
    if (a.type != b.type)
        return false;
    switch (a.type) {
        case VAL_BOOL:
            return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NIL:
            return true;
        case VAL_NUMBER:
            return AS_NUMBER(a) == AS_NUMBER(b);
        case VAL_OBJ: {
            return AS_OBJ(a) == AS_OBJ(b);
        }
        default:
            return false; // Unreachable.
    }
}

ObjString*
number_to_string(double value) {
    // Handle special cases
    if (isnan(value)) {
        return copy_string("nan", 3);
    }
    if (isinf(value)) {
        return copy_string(value > 0 ? "inf" : "-inf", value > 0 ? 3 : 4);
    }

    // Handle zero
    if (value == 0.0) {
        return copy_string("0", 1);
    }

    // Handle negative numbers
    int is_negative = value < 0;
    if (is_negative) {
        value = -value;
    }

    // Separate integer and fractional parts
    long long integer_part = (long long)value;
    double    fractional_part = value - integer_part;

    // Count digits in integer part
    long long temp = integer_part;
    int       integer_digits = 0;
    if (integer_part == 0) {
        integer_digits = 1;
    } else {
        while (temp > 0) {
            integer_digits++;
            temp /= 10;
        }
    }

    // Add commas to integer part (every 3 digits from right)
    int comma_count = (integer_digits - 1) / 3;
    int total_integer_chars =
        integer_digits + comma_count + (is_negative ? 1 : 0);

    // Handle fractional part
    char fractional_buf[32] = {0};
    int  fractional_digits = 0;

    if (fractional_part > 1e-10) {
        // Convert to string with maximum precision
        char temp_buf[32];
        snprintf(temp_buf, sizeof(temp_buf), "%.15f", fractional_part);

        // Extract fractional part (skip "0.")
        if (strlen(temp_buf) >= 2 && temp_buf[0] == '0' && temp_buf[1] == '.') {
            char* frac_start = temp_buf + 2;
            int   len = strlen(frac_start);

            // Remove trailing zeros
            while (len > 0 && frac_start[len - 1] == '0') {
                len--;
            }

            if (len > 0) {
                fractional_digits = len;
                strncpy(fractional_buf, frac_start, len);
                fractional_buf[len] = '\0';
            }
        }
    }

    // Calculate total length
    int total_length = total_integer_chars;
    if (fractional_digits > 0) {
        total_length += 1 + fractional_digits; // +1 for decimal point
    }

    // Allocate buffer for the result
    char* result_buf = ALLOCATE(char, total_length + 1);
    if (!result_buf) {
        return NULL;
    }

    int pos = 0;

    // Add negative sign if needed
    if (is_negative) {
        result_buf[pos++] = '-';
    }

    // Convert integer part to string with commas (fixed logic)
    if (integer_part == 0) {
        result_buf[pos++] = '0';
    } else {
        char integer_buf[32];
        snprintf(integer_buf, sizeof(integer_buf), "%lld", integer_part);

        int len = strlen(integer_buf);

        // Calculate where commas should be placed
        int first_group_size = len % 3;
        if (first_group_size == 0) {
            first_group_size = 3;
        }

        // Add first group of digits
        for (int i = 0; i < first_group_size; i++) {
            result_buf[pos++] = integer_buf[i];
        }

        // Add remaining groups with commas
        for (int i = first_group_size; i < len; i += 3) {
            result_buf[pos++] = ',';
            for (int j = 0; j < 3 && (i + j) < len; j++) {
                result_buf[pos++] = integer_buf[i + j];
            }
        }
    }

    // Add fractional part if needed
    if (fractional_digits > 0) {
        result_buf[pos++] = '.';
        strcpy(result_buf + pos, fractional_buf);
        pos += fractional_digits;
    }

    result_buf[pos] = '\0';

    // Create ObjString from the buffer
    ObjString* result_str = take_string(result_buf, pos);
    return result_str;
}