#pragma once

#include <cstdint>

typedef struct StringRef {
    char const *start;
    uint32_t length;
} StringRef;

typedef struct LocationInfo {
    uint32_t line;
    StringRef file;
} LocationInfo;

