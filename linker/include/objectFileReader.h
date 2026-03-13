//
// Created by brego on 12/03/2026.
//

#ifndef OBJECTFILEREADER_H
#define OBJECTFILEREADER_H

#include "linklib.h"

typedef struct {
    u8 magic[4]; // NANO
    u8 version;
    u8 num_segments;
    u16 num_symbols;
    u16 num_relocations;
    u32 segment_data_offset;
} header;

typedef struct {
    char *name;
    u8 *data;
    u64 offset; // where it is in the data section
    u64 size;
} segment;

typedef enum {
    SYM_LABEL,
    SYM_VAR,
    SYM_UNRESOLVED
} symbol_type;

typedef struct {
    char *name;
    u64 value;
    symbol_type type;
    i8 defined;
    i64 seg_id;
    u64 offset;
} symbol;

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
    header header;
    segment *segment_table;
    symbol *symbol_table;
    relocation_entry *relocation_table;
    u8 *data;
} obj_file;

void read_obj(obj_file *obj, const char *filename);

#endif //OBJECTFILEREADER_H
