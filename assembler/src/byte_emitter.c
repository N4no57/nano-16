//
// Created by brego on 31/01/2026.
//

#include "../include/byte_emitter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GEN_SIB(scale, index, base) (((scale) << 6) | ((index) << 3) | ((base)))
#define GEN_MODRM(mod, reg, rm) (((mod) << 6) | (((reg) & 7) << 3) | ((rm) & 7))
#define GEN_OPCODE(ext, dir, cls, opc) (((ext) << 7) | ((0) << 6) | ((dir) << 5) | ((cls) << 3) | (opc))

#define MAX_BYTES 200

struct opcode_info opcode_table[] = {
    // Class - 00
    {ADD, 0, 0b000, 0, 0, 0},
    {SUB, 0, 0b001, 0, 0, 0},
    {AND, 0, 0b010, 0, 0, 0},
    {OR, 0, 0b011, 0, 0, 0},
    {XOR, 0, 0b100, 0, 0, 0},
    {CMP, 0, 0b101, 0, 0, 0},
    {TEST, 0, 0b110, 0, 0, 0},
    {SHL, 0, 0b111, 0, 0, 0}, // pseudo
    {SHR, 0, 0b111, 1, 0, 0}, // pseudo
    {ADC, 0, 0, 0, 1, 0b00000000},
    {INC, 0, 0, 0, 1, 0b00000001},
    {DEC, 0, 0, 0, 1, 0b00000010},
    {MUL, 0, 0, 0, 1, 0b00000011},
    {DIV, 0, 0, 0, 1, 0b00000100},
    {NOT, 0, 0, 0, 1, 0b00000101},

    // Class - 01
    {MOV, 1, 0b000, 0, 0, 0},
    {PUSH, 1, 0b001, 0, 0, 0},
    {POP, 1, 0b010, 0, 0, 0},
    {INB, 1, 0b011, 0, 0, 0},
    {OUTB, 1, 0b100, 0, 0, 0},
    {LEA, 1, 0b101, 0, 0, 0},

    // Class - 10
    {JMP, 2, 0b000, 0, 0, 0},
    {JZ, 2, 0b001, 0, 0, 0},
    {JNZ, 2, 0b010, 0, 0, 0},
    {JE, 2, 0b001, 0, 0, 0}, // pseudo
    {JNE, 2, 0b010, 0, 0, 0}, // pseudo
    {JC, 2, 0b011, 0, 0, 0},
    {JNC, 2, 0b100, 0, 0, 0},
    {CALL, 2, 0b101, 0, 0, 0},
    {RET, 2, 0b110, 0, 0, 0},
    {JA, 2, 0, 0, 1, 0b00000000},
    {JAE, 2, 0, 0, 1, 0b00000001},
    {JB, 2, 0, 0, 1, 0b00000010},
    {JBE, 2, 0, 0, 1, 0b00000011},
    {JG, 2, 0, 0, 1, 0b00000100},
    {JGE, 2, 0, 0, 1, 0b00000101},
    {JL, 2, 0, 0, 1, 0b00000110},
    {JLE, 2, 0, 0, 1, 0b00000111},

    // Class - 11
    {NOP, 3, 0b000, 0, 0, 0},
    {HLT, 3, 0b001, 0, 0, 0},

    // if opcode is not found:
    {UNKNOWN, 0, 0, 0, 0, 0}
};

struct opcode_info get_opcode_info(enum opcode op) {
    i64 table_size = sizeof(opcode_table)/sizeof(opcode_table[0]);
    for (int i = 0; i < table_size; i++) {
        if (opcode_table[i].op == op) {
            return opcode_table[i];
        }
    }
    return opcode_table[table_size-1];
}

void emit_instruction(const struct instruction *inst, segment *cur_seg) {
    u8 bytes[MAX_BYTES] = {0};
    u64 byte_idx = 0;

    for (int j = 0; j < inst->prefix_count; j++) {
        bytes[byte_idx++] = inst->prefixes[j];
    }

    struct opcode_info info = get_opcode_info(inst->opc);
    u64 opcode_slot = byte_idx;
    byte_idx += 1 + info.is_ext;

    for (int j = 0; j < inst->operands; j++) {
        const struct operand *op = &inst->oprs[j];
        if (op->type == OPERAND_MODRM) {
            info.dir = op->modrm.directionality;
            bytes[byte_idx++] = GEN_MODRM(op->modrm.mod, op->modrm.reg, op->modrm.rm);
        } else if (op->type == OPERAND_SIB) {
            bytes[byte_idx++] = GEN_SIB(op->sib.mod, op->sib.idx, op->sib.base);
        } else if (op->type == OPERAND_ABS || op->type == OPERAND_IMM) {
            bytes[byte_idx++] = op->imm & 0xff;
            if (op->size > 255) {
                bytes[byte_idx++] = (op->imm & 0xff00) >> 8;
            }
        } else if (op->type == OPERAND_DISP) {
            bytes[byte_idx++] = op->disp & 0xff;
            if (op->size < -128 || op->size > 127) {
                bytes[byte_idx++] = (op->imm & 0xff00) >> 8;
            }
        } else if (op->type == OPERAND_SYM) {

        }
    }

    bytes[opcode_slot] = GEN_OPCODE(info.is_ext, info.dir, info.cls, info.opc);
    if (info.is_ext) bytes[opcode_slot+1] = info.ext_opc;

    push_bytes(cur_seg, bytes, byte_idx);
}

void emit_bytes(struct statement_list *stmnt_list, struct symbol_table *symtbl, segment_table *seg_table) {
    segment *cur_seg;
    for (int i = 0; i < stmnt_list->count; i++) {
        struct statement *stmnt = &stmnt_list->statements[i];
        if (stmnt->type == ST_INSTRUCTION) {
            emit_instruction(&stmnt->instruction, cur_seg);
        } else if (stmnt->type == ST_SYMBOL) {
            struct symbol *sym = &symtbl->symbols[find_symbol(symtbl, stmnt->symbol.name)];

            // The symbols are already in the statement list but also happen to have been pushed into the symbol table
            // so why are these being counted as errors?
            // so like heh?

            if (!sym) {
                // how the fuck did we get here?
                printf("undefined internal symbol\n");
                exit(1);
            }

            if (sym->defined) {
                // idk what to put here
                // but I will error
                printf("something about redefining or whatever\n");
                exit(1);
            }

            sym->defined = 1;
            sym->seg_id = seg_table->current_seg;
            sym->offset = cur_seg->size;
        } else {
            // can only be directives
            const struct directive *dir = &stmnt->directive;

            if (strcmp(dir->name, "segment") == 0) {
                if (dir->args.size == 0) {
                    printf("OI CUNT YA FUCKED UP SOMEWHERE\n");
                    printf("location info coming soon :)\n");
                    exit(1);
                }

                if (dir->args.data[0].type != TT_IDENTIFIER) {
                    printf("OI CUNT YA FUCKED UP SOMEWHERE\n");
                    printf("location info coming soon :)\n");
                    exit(1);
                }

                seg_table->current_seg = find_segment(seg_table, dir->args.data[0].value);
                cur_seg = &seg_table->segments[seg_table->current_seg];
            } else if (strcmp(dir->name, "db") == 0) { // TODO

            } else if (strcmp(dir->name, "dw") == 0) {

            } else if (strcmp(dir->name, "dd") == 0) {

            } else if (strcmp(dir->name, "dq") == 0) {

            }
        }
    }
}
