//
// Created by brego on 08/03/2026.
//

#ifndef OBJECT_FILE_WRITER_H
#define OBJECT_FILE_WRITER_H

#include "asmlib.h"
#include "parser.h"
#include "byte_emitter.h"

typedef struct {
    u8 magic[4]; // NANO
    u8 version;
    u8 num_segments;
    u16 num_symbols;
    u16 num_relocations;
    u32 segment_data_offset;
} header;

typedef struct {
    header header;
    segment_table segment_table;
    struct symbol_table symbol_table;
    relocation_table relocation_table;
    u8 *data;
} obj_file;

void writeObjFile(obj_file *obj, const char *filename);

#endif //OBJECT_FILE_WRITER_H
