#ifndef ASMLIB_H
#define ASMLIB_H

#define MAXTEMPSIZE 100

#include <stdint.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef struct position {
    char *filename;
    int idx;
    int line;
    int column;
} position;

void toUpper(char *str);
void toLower(char *str);

int ismnemonic(const char *string);
int isregister(const char *string);
int issizespec(const char *string);

int getmnemonic(const char *string);
int getregister(const char *string);
int getsizespec(const char *string);

int print_error(const position *pos, char *details);

#endif //ASMLIB_H
