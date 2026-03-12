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

void reloc_table_init(relocation_table *table) {
    table->count = 0;
    table->capacity = 8;
    table->entries = malloc(table->capacity * sizeof(*table->entries));
    if (!table->entries) {
        printf("Failed to allocate memory for relocation table\n");
        exit(1);
    }
}

void reloc_table_push(relocation_table *table, const relocation_entry *entry) {
    if (table->count >= table->capacity) {
        table->capacity *= 2;
        relocation_entry *tmp = realloc(table->entries, table->capacity * sizeof(*table->entries));
        if (!tmp) {
            printf("Failed to allocate memory for relocation table\n");
            exit(1);
        }
        table->entries = tmp;
    }
    table->entries[table->count++] = *entry;
}

void reloc_table_free(const relocation_table *table) {
    free(table->entries);
}

void emit_byte(u8 *bytes, u64 *index, const u8 value) {
    bytes[(*index)++] = value;
}

void emit_word(u8 *bytes, u64 *index, const u16 value) {
    bytes[(*index)++] = value & 0xff;
    bytes[(*index)++] = value >> 8;
}

void emit_symbol(const struct operand *op, segment_table *seg_table, struct symbol_table *symtbl, relocation_table *relocs, segment *current_seg, u8 *bytes, u64 *idx) {
    i64 sym_idx = find_symbol(symtbl, op->sym.name);

    if (sym_idx == -1) {
        printf("This symbol doesn't fooking exist ya bastard");
        exit(1);
    }
    struct symbol *sym = &symtbl->symbols[sym_idx];
    relocation_entry reloc = {0};

    if (sym->type == SYM_VAR) {
        if (sym->defined) { // if defined then emit
            if (sym->value > 0xff) {
                emit_word(bytes, idx, sym->value);
            } else {
                emit_byte(bytes, idx, sym->value);
            }
        } else { // put a placeholder and make relocation entry
            emit_word(bytes, idx, 0);
            reloc.seg_id = seg_table->current_seg;
            reloc.seg_offset = current_seg->size + *idx;
            reloc.sym_idx = sym_idx;
            reloc.type = reloc_relax;
            reloc_table_push(relocs, &reloc);
        }
    } else if (sym->type == SYM_LABEL) {
        reloc.seg_id = seg_table->current_seg;
        reloc.seg_offset = current_seg->size + *idx;
        reloc.sym_idx = sym_idx;
        reloc.type = reloc_relax;
        if (sym->defined) {
            if (sym->seg_id == seg_table->current_seg) {
                i32 distance = (i32)sym->offset - (i32)reloc.seg_offset;
                if (distance >= -128 && distance <= 127) { // 8-bit
                    reloc.type = reloc_relative_8;
                    emit_byte(bytes, idx, 0);
                } else { // 16-bit
                    reloc.type = reloc_relative_16;
                    emit_word(bytes, idx, 0);
                }
            } else {
                emit_word(bytes, idx, 0);
            }
        }
        reloc_table_push(relocs, &reloc);
    }
    push_bytes(current_seg, bytes, *idx);
}

void emit_instruction(struct instruction *inst, struct symbol_table *symtbl, relocation_table *relocs, segment_table *seg_tbl, segment *cur_seg) {
    u8 bytes[MAX_BYTES] = {0};
    u64 byte_idx = 0;
    u8 has_oex = 0;

    for (int j = 0; j < inst->prefix_count; j++) {
        if (OEX == (inst->prefixes[j] & OEX)) has_oex = 1;
        bytes[byte_idx++] = inst->prefixes[j];
    }

    struct opcode_info info = get_opcode_info(inst->opc);
    u64 opcode_slot = byte_idx;
    byte_idx += 1 + info.is_ext;

    for (int j = 0; j < inst->operands; j++) {
        struct operand *op = &inst->oprs[j];
        if (op->type == OPERAND_MODRM) {
            info.dir = op->modrm.directionality;
            if (op->modrm.reg > 7) op->modrm.reg >> 1 & 7;
            if (op->modrm.rm > 7) op->modrm.rm >> 1 & 7;
            if (op->modrm.rm == NIL) op->modrm.rm = 0;
            bytes[byte_idx++] = GEN_MODRM(op->modrm.mod, op->modrm.reg, op->modrm.rm);
        } else if (op->type == OPERAND_SIB) {
            if (op->sib.idx > 7) op->sib.idx >> 1 & 7;
            if (op->sib.base > 7) op->sib.base >> 1 & 7;
            if (op->sib.mod == 2) op->sib.mod = 0b01;
            else if (op->sib.mod == 4) op->sib.mod = 0b10;
            else if (op->sib.mod == 8) op->sib.mod = 0b11;
            else op->sib.mod = 0b00;
            bytes[byte_idx++] = GEN_SIB(op->sib.mod, op->sib.idx, op->sib.base);
        } else if (op->type == OPERAND_ABS || op->type == OPERAND_IMM) {
            if (op->has_symbol) {
                emit_symbol(op, seg_tbl, symtbl, relocs, cur_seg, bytes, &byte_idx);
            } else {
                bytes[byte_idx++] = op->imm & 0xff;
                if (has_oex) {
                    bytes[byte_idx++] = (op->imm & 0xff00) >> 8;
                }
            }
        } else if (op->type == OPERAND_DISP) {
            if (op->has_symbol) {
                emit_symbol(op, seg_tbl, symtbl, relocs, cur_seg, bytes, &byte_idx);
            } else {
                bytes[byte_idx++] = op->disp & 0xff;
                if (has_oex) {
                    bytes[byte_idx++] = (op->disp & 0xff00) >> 8;
                }
            }
        }
    }

    bytes[opcode_slot] = GEN_OPCODE(info.is_ext, info.dir, info.cls, info.opc);
    if (info.is_ext) bytes[opcode_slot+1] = info.ext_opc;

    push_bytes(cur_seg, bytes, byte_idx);
}

void emit_bytes(struct statement_list *stmnt_list, struct symbol_table *symtbl, segment_table *seg_table, relocation_table *relocs) {
    segment *cur_seg = NULL;
    for (int i = 0; i < stmnt_list->count; i++) {
        struct statement *stmnt = &stmnt_list->statements[i];
        if (stmnt->type == ST_INSTRUCTION) {
            if (cur_seg == NULL) {
                printf("\"Where are me fookin segments at mate?\" said the statement");
                exit(1);
            }

            emit_instruction(&stmnt->instruction, symtbl, relocs, seg_table, cur_seg);
        } else if (stmnt->type == ST_SYMBOL) {
            if (cur_seg == NULL) {
                printf("\"Where are me fookin segments at mate?\" said the symbol");
                exit(1);
            }

            i64 idx = find_symbol(symtbl, stmnt->symbol.name);
            if (idx < 0) {
                printf("undefined internal symbol: %s\n", stmnt->symbol.name);
                exit(1);
            }
            struct symbol *sym = &stmnt->symbol;

            if (sym->type == SYM_LABEL) {
                sym->offset = cur_seg->size;
                sym->seg_id = seg_table->current_seg;
            }
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
            } else if (strcmp(dir->name, "db") == 0) {
                for (u32 j = 0; j < dir->value_count; j++) {
                    struct directive_value *value = &dir->values[j];
                    u8 bytes[MAX_BYTES] = {0};
                    u64 byte_idx = 0;

                    if (value->kind != DIR_LITERAL) {
                        print_error(&dir->pos, "You did not do this right ya moron\nIts only supposed to be numbers\n");
                    }
                    bytes[byte_idx++] = value->literal & 0xff;
                    push_bytes(cur_seg, bytes, byte_idx);
                }
            } else if (strcmp(dir->name, "dw") == 0) {
                for (u32 j = 0; j < dir->value_count; j++) {
                    struct directive_value *value = &dir->values[j];
                    u8 bytes[MAX_BYTES] = {0};
                    u64 byte_idx = 0;

                    if (value->kind != DIR_LITERAL) {
                        print_error(&dir->pos, "You did not do this right ya moron\nIts only supposed to be numbers\n");
                    }
                    bytes[byte_idx++] = value->literal & 0xff;
                    bytes[byte_idx++] = value->literal >> 8 & 0xff;
                    push_bytes(cur_seg, bytes, byte_idx);
                }
            } else if (strcmp(dir->name, "dd") == 0) {
                for (u32 j = 0; j < dir->value_count; j++) {
                    struct directive_value *value = &dir->values[j];
                    u8 bytes[MAX_BYTES] = {0};
                    u64 byte_idx = 0;

                    if (value->kind != DIR_LITERAL) {
                        print_error(&dir->pos, "You did not do this right ya moron\nIts only supposed to be numbers\n");
                    }
                    bytes[byte_idx++] = value->literal & 0xff;
                    bytes[byte_idx++] = value->literal >> 8 & 0xff;
                    bytes[byte_idx++] = value->literal >> 16 & 0xff;
                    bytes[byte_idx++] = value->literal >> 24 & 0xff;
                    push_bytes(cur_seg, bytes, byte_idx);
                }
            } else if (strcmp(dir->name, "dq") == 0) {
                for (u32 j = 0; j < dir->value_count; j++) {
                    struct directive_value *value = &dir->values[j];
                    u8 bytes[MAX_BYTES] = {0};
                    u64 byte_idx = 0;

                    if (value->kind != DIR_LITERAL) {
                        print_error(&dir->pos, "You did not do this right ya moron\nIts only supposed to be numbers\n");
                    }
                    bytes[byte_idx++] = value->literal & 0xff;
                    bytes[byte_idx++] = value->literal >> 8 & 0xff;
                    bytes[byte_idx++] = value->literal >> 16 & 0xff;
                    bytes[byte_idx++] = value->literal >> 24 & 0xff;
                    bytes[byte_idx++] = value->literal >> 32 & 0xff;
                    bytes[byte_idx++] = value->literal >> 40 & 0xff;
                    bytes[byte_idx++] = value->literal >> 48 & 0xff;
                    bytes[byte_idx++] = value->literal >> 56 & 0xff;
                    push_bytes(cur_seg, bytes, byte_idx);
                }
            }
        }
    }
}
