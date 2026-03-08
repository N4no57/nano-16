//
// Created by brego on 31/01/2026.
//

#ifndef BYTE_EMITTER_H
#define BYTE_EMITTER_H

#include "parser.h"

// okay...
// in layman's terms
// go through the statement list while tracking the segment and chuck bytes into it
// simple as

struct opcode_info {
    enum opcode op;
    u8 cls;
    u8 opc;
    u8 dir;
    u8 is_ext;
    u8 ext_opc; // only if is_ext=1
};

typedef enum {
    reloc_none,
    reloc_absolute_8,
    reloc_absolute_16,
    reloc_relative_8,
    reloc_relative_16,
    reloc_relax
} relocation_type;

typedef struct {
    u64 sym_idx;
    i64 seg_id;
    u64 seg_offset;
    relocation_type type;
} relocation_entry;

typedef struct {
    u64 count;
    u64 capacity;
    relocation_entry *entries;
} relocation_table;

void reloc_table_init(relocation_table *table);
void reloc_table_push(relocation_table *table, const relocation_entry *entry);
void reloc_table_free(const relocation_table *table);
void emit_bytes(struct statement_list *stmnt_list, struct symbol_table *symtbl, segment_table *seg_table, relocation_table *relocs);

#endif //BYTE_EMITTER_H
