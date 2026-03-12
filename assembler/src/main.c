#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/byte_emitter.h"
#include "../include/tokeniser.h"
#include "../include/parser.h"
#include "../include/object_file_writer.h"

int run(char *filename) {
    FILE *f = fopen(filename, "r");

    if (f == NULL) {
        printf("Error opening file %s\n", filename);
        return 1;
    }

    fseek(f, 0, SEEK_END);

    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buffer = malloc(size+10);
    memset(buffer, 0, size+10);
    fread(buffer, 1, size, f);
    fclose(f);

    token_list *tokens = tokenise(filename, buffer);

    struct statement_list stmnt_list;
    init_statement_list(&stmnt_list);

    // just gonna slip this right in here
    struct symbol_table symtbl;
    init_symbol_table(&symtbl);

    // again just gonna slip this in
    segment_table seg_table;
    init_segment_table(&seg_table);

    parse(tokens, &stmnt_list, &symtbl, &seg_table);

    if (seg_table.count == 0) {
        printf("No symbol table found/generated\n");
        printf("what are you doing you donut\n");
        return 1;
    }

    relocation_table reloc_table;
    reloc_table_init(&reloc_table);

    emit_bytes(&stmnt_list, &symtbl, &seg_table, &reloc_table);

    obj_file obj = {0};

    // header file
    u8 magic[4] = {'N', 'A', 'N', 'O'};
    memcpy(obj.header.magic, magic, 4);
    obj.header.version = 1;
    obj.header.num_segments = seg_table.count;
    obj.header.num_symbols = symtbl.symbol_count;
    obj.header.num_relocations = reloc_table.count;

    // segment table
    obj.segment_table.capacity = seg_table.capacity;
    obj.segment_table.count = seg_table.count;
    obj.segment_table.segments = seg_table.segments;

    // symbol table
    obj.symbol_table.capacity = symtbl.capacity;
    obj.symbol_table.symbol_count = symtbl.symbol_count;
    obj.symbol_table.symbols = symtbl.symbols;

    // relocation table
    obj.relocation_table.capacity = reloc_table.capacity;
    obj.relocation_table.count = reloc_table.count;
    obj.relocation_table.entries = reloc_table.entries;

    writeObjFile(&obj, "test.o");

    free(buffer);
    free(tokens);
    free(stmnt_list.statements);
    free(symtbl.symbols);
    for (int i = 0; i < seg_table.count; i++) {
        free(seg_table.segments[i].data);
    }
    free(seg_table.segments);
    free(reloc_table.entries);
    return 0;
}

int main(int argc, char **argv) {
    return run("test.asm");
}