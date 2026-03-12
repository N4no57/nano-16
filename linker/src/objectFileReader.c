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

    for (int i = 0; i < obj->header.num_segments; i++) {
        fread(&obj->segment_table[i].name, sizeof(segment), 1, f);
    }

    fclose(f);
}
