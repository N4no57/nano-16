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

#include "asmlib.h"
#include "tokeniser.h"

enum opcode {
    ADD, SUB, AND, OR, XOR, CMP, TEST,
    SHL, SHR, // pseudo instruction
    ADC, INC, DEC, MUL, DIV, NOT, MOV, PUSH,
    POP, INB, OUTB, LEA, JMP, JZ, JNZ,
    JE, JNE, // pseudo instruction
    JC, JNC, CALL, RET, JA, JAE, JB, JBE,
    JG, JGE, JL, JLE, NOP, HLT,
    UNKNOWN
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
    i8 defined;          // 1 if value is resolved
    // ooo this is new
    i64 seg_id;            // segment code: CODE, DATA, BSS, etc.
    u64 offset; // where this is in said segment from wherever it starts idgaf as long as it workjs
    // what the fuck is this? oh yeah actual debug info for the ppl who will use it... those ppl being me
    // probably should make it do something... naaaaaaaaah
    // future me problem
    position pos;          // where it was declared
};

struct operand {
    enum operand_type type;
    u8 has_symbol;
    u8 size;
    union {
        u32 imm; // immediate/absolute value
        i32 disp; // displacement
        enum registers reg;

        struct {
            char *name;
        } sym;

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
    u8 prefixes[MAX_PREFIX]; // prefix bytes

    u8 global_size;

    u8 operands; // how many operands
    struct operand oprs[MAX_OPERANDS];
};

enum directive_kind {
    DIR_DB,
    DIR_DW,
    DIR_DD,
    DIR_DQ,

    DIR_SEGMENT,

    DIR_UNKNOWN
};

struct directive {
    char *name;
    token_list args;
    position pos;
    // okay new shit time for the pass 2 cus what already exists is NOT enough
    enum directive_kind kind;

    u8 elem_width; // so like if each thingy is 1 byte, 2 bytes or 4 bytes and so on
    u32 value_count;

    struct directive_value {
        enum {
            DIR_LITERAL,
            DIR_SYMBOL_REF
        } kind;

        union {
            i64 literal;
            u64 sym_id;
        };
    } *values;

    u64 offset; // directive where the fuck u at? where in the binary/segment it is in or smth
    u64 byte_size; // directive how tf u so fat? fr fr tho this is like the total number of bytes
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

// the schizo post has returned
// symbol table semantics
struct symbol_table {
    // I do not want this shi not ready for the object file generation in the same way as my last assembler
    // how tf did I go wrong? idk. imma check brb
    // okay so now that I check this shi gonna be really hard since symbols for my linker need to track the segment their in and thats kinda tricky
    // how tf do I do that? the symbol struct is kinda idk existent but not what I want for this struct... hmmmmmm
    // okay WHAT THE FUCK IS THIS?!?!?
    // nah I got this
    // I need the symbol to... uh oh... I remember now... symbols are either things like variables or labels!!!
    // AAAAAAAAAAAAAAAAAAAAA
    // man what the fuck is going on
    // screw this I'm asking GPT
    // no no... I got this
    u64 symbol_count;
    // okay this is a slight problem cus like how tf do I do labels and value placeholders?
    // NO NO NO!!!! type is tracked so like yeah... its fine
    // the goddamn thinga majiga... template or example or rather test assembly file doesn't use the
    // X = val ah symbol
    // but it is a feature I want?
    // cmere robot help me out
    // okay I know what to do
    // + I forgot this line but its here now
    u64 capacity;
    struct symbol *symbols;
    // I think now it would be fine
};

// the time has come for segments... how many tables are necessary?
// idk but another one has risen
// COPY PASTE BABY!!
typedef struct {
    // name cus like, how else am I supposed to know what this is?
    char *name;
    // data cus thats the FUCKING DATA
    u8 *data;
    // I don't know why size is a thing since the total space can be allocated in one go... maybe...
    // yeah it can
    // no this is the data size
    u64 size;
    // this is the thing that I said was not required or something
    u64 capacity;
} segment;

typedef struct {
    // pretty standard stuff, chill
    segment *segments;
    u64 count;
    u64 capacity;
    i64 current_seg;
} segment_table;

void init_symbol_table(struct symbol_table *symtbl);
i64 find_symbol(const struct symbol_table *tbl, const char *name);
void init_segment_table(segment_table *st);
i64 find_segment(const segment_table *st, const char *name);
void push_bytes(segment *seg, const u8 *data, u64 size);
void init_statement_list(struct statement_list *list);
void parse(const token_list *tokens, struct statement_list *stmnt_list, struct symbol_table *symtbl, segment_table *seg_table);

#endif //PARSER_H
