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

    u8 padding = 0;

    // header
    fwrite(obj->header.magic, sizeof(obj->header.magic), 1, file);
    fwrite((void *)obj->header.version, sizeof(obj->header.version), 1, file);
    fwrite((void *)obj->header.num_segments, sizeof(obj->header.num_segments), 1, file);
    fwrite((void *)obj->header.num_symbols, sizeof(obj->header.num_symbols), 1, file);
    fwrite((void *)obj->header.num_relocations, sizeof(obj->header.num_relocations), 1, file);
    fwrite((void *)padding, sizeof(padding), sizeof(obj->header.segment_data_offset), file);

    // segment table
    for (u32 i = 0; i < obj->header.num_segments; i++) {
        segment seg = obj->segment_table.segments[i];
        fwrite(seg.name, strlen(seg.name), 1, file);
        fwrite(seg.);
    }
}
