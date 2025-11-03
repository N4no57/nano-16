#ifndef TOKENISER_H
#define TOKENISER_H

#include "asmlib.h"

enum token_type {
    TT_MNEMONIC,
    TT_IMMEDIATE,
    TT_REGISTER,
    TT_LBRACKET,
    TT_RBRACKET,
    TT_PLUS,
    TT_MINUS,
    TT_ASTERISK,
    TT_IDENTIFIER,
    TT_DIRECTIVE,
    TT_STRING_LITERAL,
    TT_CHAR_LITERAL,
    TT_COMMA,
    TT_SEMICOLON,
    TT_COLON,
    TT_NEWLINE,
    TT_SIZESPEC,
    TT_EOF
};

typedef struct token {
    enum token_type type;
    position pos;
    void *value;
} token;

typedef struct token_list {
    int size;
    int capacity;
    token *data;
} token_list;

token_list *tokenise(char *filename, const char *string);

#endif //TOKENISER_H
