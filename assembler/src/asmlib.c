#include "../include/asmlib.h"
#include <stdio.h>
#include <string.h>

char *mnemonics[] = {
    "ADD", "SUB", "AND", "OR", "XOR", "CMP", "TEST",
    "SHL", "SHR", // pseudo instruction
    "ADC", "INC", "DEC", "MUL", "DIV", "NOT", "MOV",
    "PUSH", "POP", "INB", "OUTB", "LEA", "JMP", "JZ", "JNZ",
    "JE", "JNE", // pseudo instruction
    "JC", "JNC", "CALL", "RET", "JA", "JAE", "JB", "JBE",
    "JG", "JGE", "JL", "JLE", "NOP", "HLT"
};

char *registers[] = {
    "A",
    "B",
    "C",
    "D",
    "SI",
    "DI",
    "BP",
    "SP",
    "IP"
};

char *sizeSpecifiers[] = {
    "BYTE",
    "WORD"
};

void toUpper(char *str) {
    for (int i = 0; i < strlen(str); i++) {
        if (str[i] >= 'a' && str[i] <= 'z') {
            str[i] -= 32;
        }
    }
}

void toLower(char *str) {
    for (int i = 0; i < strlen(str); i++) {
        if (str[i] >= 'A' && str[i] <= 'Z') {
            str[i] += 32;
        }
    }
}

int ismnemonic(const char *string) {
    char tmp[MAXTEMPSIZE];
    strcpy(tmp, string);
    toUpper(tmp);
    for (int i = 0; i < 38; i++) {
        if (strcmp(tmp, mnemonics[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

int isregister(const char *string) {
    char tmp[MAXTEMPSIZE];
    strcpy(tmp, string);
    toUpper(tmp);
    for (int i = 0; i < sizeof(registers) / sizeof(registers[0]); i++) {
        if (strcmp(tmp, registers[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

int issizespec(const char *string) {
    char tmp[MAXTEMPSIZE];
    strcpy(tmp, string);
    toUpper(tmp);
    for (int i = 0; i < sizeof(sizeSpecifiers) / sizeof(sizeSpecifiers[0]); i++) {
        if (strcmp(tmp, sizeSpecifiers[i]) == 0) {
            return 1;
        }
    }
    return 0;
}

int getmnemonic(const char *string) {
    char tmp[MAXTEMPSIZE];
    strcpy(tmp, string);
    toUpper(tmp);
    for (int i = 0; i < sizeof(mnemonics) / sizeof(mnemonics[0]); i++) {
        if (strcmp(tmp, mnemonics[i]) == 0) {
            return i;
        }
    }
    return -1;
}

int getregister(const char *string) {
    char tmp[MAXTEMPSIZE];
    strcpy(tmp, string);
    toUpper(tmp);
    for (int i = 0; i < sizeof(registers) / sizeof(registers[0]); i++) {
        if (strcmp(tmp, registers[i]) == 0) {
            return i;
        }
    }
    return -1;
}

int getsizespec(const char *string) {
    char tmp[MAXTEMPSIZE];
    strcpy(tmp, string);
    toUpper(tmp);
    for (int i = 0; i < sizeof(sizeSpecifiers) / sizeof(sizeSpecifiers[0]); i++) {
        if (strcmp(tmp, sizeSpecifiers[i]) == 0) {
            return i;
        }
    }
    return -1;
}

int print_error(const position *pos, char *details) {
    char str[2048]; // should handle most cases cleanly
    const int result = sprintf(str, "%s:%d:%d: error: %s", pos->filename, pos->line, pos->column, details);
    if (result < 0) return result; // error leave early

    return printf("%s\n", str);
}
