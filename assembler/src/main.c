#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/byte_emitter.h"
#include "../include/tokeniser.h"
#include "../include/parser.h"

int main(int argc, char **argv) {
    char filename[] = "test.asm";

    FILE *f = fopen(filename, "r");

    if (f == NULL) {
        printf("Error opening file %s\n", filename);
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
        exit(EXIT_FAILURE);
    }

    relocation_table reloc_table;
    reloc_table_init(&reloc_table);

    emit_bytes(&stmnt_list, &symtbl, &seg_table, &reloc_table);

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