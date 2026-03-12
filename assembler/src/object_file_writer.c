//
// Created by brego on 08/03/2026.
//

#include "../include/object_file_writer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void writeObjFile(obj_file *obj, const char *filename) {
    remove(filename);

    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        printf("Error opening file %s\n", filename);
        exit(1);
    }

    u8 padding = 0x00;

    // header
    fwrite(obj->header.magic, sizeof(obj->header.magic), 1, file);
    fwrite(&obj->header.version, sizeof(obj->header.version), 1, file);
    fwrite(&obj->header.num_segments, sizeof(obj->header.num_segments), 1, file);
    fwrite(&obj->header.num_symbols, sizeof(obj->header.num_symbols), 1, file);
    fwrite(&obj->header.num_relocations, sizeof(obj->header.num_relocations), 1, file);
    long header_loc = ftell(file);
    fwrite(&padding, sizeof(padding), sizeof(obj->header.segment_data_offset), file); // when the data starts

    // segment table
    u64 total_size = 0;
    for (u32 i = 0; i < obj->header.num_segments; i++) {
        segment *seg = &obj->segment_table.segments[i];
        fwrite(seg->name, strlen(seg->name), 1, file);
        fwrite(&padding, sizeof(padding), 1, file); // null terminator
        fwrite(&total_size, sizeof(total_size), 1, file); // where it is in the data section
        fwrite(&seg->size, sizeof(seg->size), 1, file); // size
        total_size += seg->size;
    }

    // symbol table
    for (u64 i = 0; i < obj->header.num_symbols; i++) {
        struct symbol *sym = &obj->symbol_table.symbols[i];
        fwrite(sym->name, strlen(sym->name), 1, file);
        fwrite(&padding, sizeof(padding), 1, file); // null terminator
        fwrite(&sym->seg_id, sizeof(sym->seg_id), 1, file);
        fwrite(&sym->offset, sizeof(sym->offset), 1, file);
        fwrite(&sym->defined, sizeof(sym->defined), 1, file);
    }

    // relocation table
    for (u64 i = 0; i < obj->header.num_relocations; i++) {
        relocation_entry *reloc = &obj->relocation_table.entries[i];
        fwrite(&reloc->seg_id, sizeof(reloc->seg_id), 1, file);
        fwrite(&reloc->seg_offset, sizeof(reloc->seg_offset), 1, file);
        fwrite(&reloc->sym_idx, sizeof(reloc->sym_idx), 1, file);
        fwrite(&reloc->type, sizeof(reloc->type), 1, file);
    }

    // offset of data
    u32 data_start = (u32)ftell(file);
    fseek(file, header_loc, SEEK_SET);
    fwrite(&data_start, sizeof(obj->header.segment_data_offset), 1, file);
    fseek(file, 0, SEEK_END);

    // actual data
    for (u64 i = 0; i < obj->header.num_segments; i++) {
        segment *seg = &obj->segment_table.segments[i];
        fwrite(seg->data, seg->size, 1, file);
    }

    fclose(file);
}
