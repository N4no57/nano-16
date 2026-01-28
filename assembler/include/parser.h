#ifndef PARSER_H
#define PARSER_H

#define MAX_OPCODE_LEN 2
#define MAX_PREFIX 3
#define MAX_OPERANDS 5 * 2 // double the number for any extra stuff before being shortened to true operands

#define NONE 0

// prefixes
#define REX 0x40
#define AEX 0x50
#define OEX 0x60

#define REX_PREFIX(M, R, X, B) (REX | ((M << 3) & 8) | ((R << 2) & 4) | ((X << 1) & 2) | (B & 1))
#define AEX_PREFIX(M) (AEX | (M & 1))
#define OEX_PREFIX(W) (OEX | (W & 1))

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
    JG, JGE, JL, JLE, NOP, HLT
};

enum registers {
    A = 0, B = 1, C = 2, D = 3, SI = 4, DI = 5,
    BP = 13, SP = 14, IP = 15, NIL = 16
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
    SYM_VAR,
    SYM_UNRESOLVED
};

struct symbol {
    // thank you Chat... what the hell is this
    // nah I'm not dumb, its what I should have already done!
    // doesn't that make me stupid for that foresight? ah well its fine
    // same as last time
    char *name;            // symbol name
    // already was here
    // problem... this is absolute value... nah its fine segment positions are tracked... right? probably
    u64 value;             // resolved value (or placeholder)
    // already was here
    enum symbol_type type; // SYM_LABEL or SYM_UNRESOLVED / SYM_VAR
    // yup
    char defined;          // 1 if value is resolved
    // ooo this is new
    u8 segment;            // segment code: CODE, DATA, BSS, etc.
    // what the fuck is this? oh yeah actual debug info for the ppl who will use it... those ppl being me
    // probably should make it do something... naaaaaaaaah
    // future me problem
    position pos;          // where it was declared
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
            enum registers idx;
            enum registers base;
        } sib; // SIB byte fields

        struct {
            u8 mod;
            u8 directionality;
            enum registers reg;
            enum registers rm;
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
