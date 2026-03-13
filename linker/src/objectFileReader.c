//
// Created by brego on 12/03/2026.
//

#include "../include/objectFileReader.h"

#include <stdio.h>
#include <stdlib.h>

u32 fstrlen(FILE *f) {
    long start = ftell(f);
    u32 count = 0;
    char c;

    while (c != '\0') {
        count++;
        fread(&c, 1, 1, f);
    }
    fseek(f, start, SEEK_SET);
    return count;
}

void read_obj(obj_file *obj, const char *filename) {
    FILE *f = fopen(filename, "rb");

    // header
    fread(obj->header.magic, sizeof(obj->header.magic), 1, f);
    fread(&obj->header.version, sizeof(obj->header.version), 1, f);
    fread(&obj->header.num_segments, sizeof(obj->header.num_segments), 1, f);
    fread(&obj->header.num_symbols, sizeof(obj->header.num_symbols), 1, f);
    fread(&obj->header.num_relocations, sizeof(obj->header.num_relocations), 1, f);
    fread(&obj->header.segment_data_offset, sizeof(obj->header.segment_data_offset), 1, f);

    // allocate space for the full header
    obj->segment_table = malloc(obj->header.num_segments * sizeof(segment));
    obj->symbol_table = malloc(obj->header.num_symbols * sizeof(symbol));
    obj->relocation_table = malloc(obj->header.num_relocations * sizeof(relocation_entry));
    if (obj->segment_table == NULL || obj->symbol_table == NULL || obj->relocation_table == NULL) {
        printf("Error allocating memory\n");
        exit(1);
    }

    u64 data_size = 0;

    // segment table
    for (int i = 0; i < obj->header.num_segments; i++) {
        u32 len = fstrlen(f);
        obj->segment_table[i].name = malloc(len + 1);
        fread(&obj->segment_table[i].name, len, 1, f);
        obj->segment_table[i].name[len] = '\0';
        fread(&obj->segment_table[i].offset, sizeof(obj->segment_table[i].offset), 1, f);
        fread(&obj->segment_table[i].size, sizeof(obj->segment_table[i].size), 1, f);
        obj->segment_table[i].data = malloc(obj->segment_table[i].size);
        data_size += obj->segment_table[i].size;
    }

    // symbol table
    for (int i = 0; i < obj->header.num_symbols; i++) {
        u32 len = fstrlen(f);
        obj->symbol_table[i].name = malloc(len + 1);
        fread(obj->symbol_table[i].name, len, 1, f);
        obj->symbol_table[i].name[len] = '\0';
        fread(&obj->symbol_table[i].type, sizeof(obj->symbol_table[i].type), 1, f);
        fread(&obj->symbol_table[i].seg_id, sizeof(obj->symbol_table[i].seg_id), 1, f);
        fread(&obj->symbol_table[i].offset, sizeof(obj->symbol_table[i].offset), 1, f);
        fread(&obj->symbol_table[i].value, sizeof(obj->symbol_table[i].value), 1, f);
        fread(&obj->symbol_table[i].defined, sizeof(obj->symbol_table[i].defined), 1, f);
    }

    // relocation table
    for (int i = 0; i < obj->header.num_relocations; i++) {
        relocation_entry *reloc = &obj->relocation_table[i];
        fwrite(&reloc->seg_id, sizeof(reloc->seg_id), 1, f);
        fwrite(&reloc->seg_offset, sizeof(reloc->seg_offset), 1, f);
        fwrite(&reloc->sym_idx, sizeof(reloc->sym_idx), 1, f);
        fwrite(&reloc->type, sizeof(reloc->type), 1, f);
    }

    for (int i = 0; i < obj->header.num_segments; i++) {
        if (ftell(f) >= data_size + obj->header.segment_data_offset) break;
        fread(obj->segment_table[i].data, sizeof(obj->segment_table[i].size), 1, f);
    }

    fclose(f);
}
