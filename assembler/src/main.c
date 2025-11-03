#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/tokeniser.h"
#include "../include/parser.h"

#define GEN_SIB(scale, index, base) (((scale) << 6) | ((index) << 3) | ((base)))
#define GEN_MODRM(mod, reg, rm) (((mod) << 6) | (((reg) & 7) << 3) | ((rm) & 7))

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

    struct statement_list list = parse(tokens);

    free(buffer);
    return 0;
}