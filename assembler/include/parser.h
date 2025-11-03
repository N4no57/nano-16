#ifndef PARSER_H
#define PARSER_H

#define MAX_OPCODE_LEN 2
#define MAX_PREFIX 3
#define MAX_OPERANDS 4 * 2 // double the number for any extra stuff before being shortened to true operands

#include <stddef.h>

#include "asmlib.h"
#include "tokeniser.h"

enum opcode {
    ADD, SUB, AND, OR, XOR, CMP, TEST,
    SHL, SHR, // pseudo instruction
    ADC, INC, DEC, MUL, DIV, NOT, MOV, PUSH,
    POP, INB, OUTB, LEA, JMP, JZ, JNZ,
    JE, JNE, // pseudo instruction
    JC, JNC, CALL, RET, JA, JAE, JB, JBE,
    JG, JGE, JL, LE, NOP, HLT
};

enum registers {
    A = 0, B = 1, C = 2, D = 3, SI = 4, DI = 5,
    BP = 13, SP = 14, IP = 15
};

enum sizeSpec {
    BYTE,
    WORD,
    DWORD,
    QWORD
};

enum operand_type {
    OPERAND_NONE,
    // compound operands
    OPERAND_MODRM,
    OPERAND_SIB,

    // simple operands
    OPERAND_IMM,
    OPERAND_ABS,
    OPERAND_DISP,
    OPERAND_REG,
    OPERAND_RM
};

enum symbol_type {
    SYM_LABEL,
    SYM_UNRESOLVED
};

struct symbol {
    char *name;
    long value;
    enum symbol_type type;
    char defined;
    position pos;
};

struct operand {
    enum operand_type type;
    u8 size;
    union {
        u32 imm; // immediate/absolute value
        i32 disp; // displacement
        enum registers reg;

        struct  {
            u8 mod;
            u8 idx;
            u8 base;
        } sib; // SIB byte fields

        struct {
            u8 mod;
            u8 reg;
            u8 rm;
        } modrm; // ModR/M byte fields
    };
};

struct instruction { // an instruction after parsing
    enum opcode opc; // IR for the opcode

    position pos; // position in file and filename

    i8 prefix_count;
    i8 prefixes[MAX_PREFIX]; // prefix bytes

    u8 operands; // how many operands
    struct operand oprs[MAX_OPERANDS];
};

struct directive {
    char *name;
    token_list args;
    position pos;
};

enum statement_type { ST_INSTRUCTION, ST_DIRECTIVE, ST_SYMBOL };

struct statement {
    enum statement_type type;
    union {
        struct instruction instruction;
        struct directive directive;
        struct symbol symbol;
    };
};

struct statement_list {
    size_t capacity;
    size_t count;
    struct statement *statements;
};

struct statement_list parse(const token_list *tokens);

#endif //PARSER_H
