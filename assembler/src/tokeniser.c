#include "../include/tokeniser.h"
#include "../include/asmlib.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TOKENLISTBASESIZE 8

void init_token_list(token_list *tokenList) {
    if (tokenList == NULL) {
        return;
    }
    tokenList->capacity = TOKENLISTBASESIZE;
    tokenList->size = 0;
    tokenList->data = malloc(tokenList->capacity * sizeof(token));
}

void push_token(token_list *tokenList, const token tok) {
    if (tokenList->size == tokenList->capacity) {
        tokenList->capacity *= 2;
        token *tmp = realloc(tokenList->data, tokenList->capacity * sizeof(token));
        if (tmp == NULL) {
            printf("Error: Out of memory\n");
            exit(-1);
        }
        tokenList->data = tmp;
    }
    tokenList->data[tokenList->size] = tok;
    tokenList->size++;
}

int advance(position* pos, const char *string) {
    pos->column++;
    if (string[pos->idx] == '\n') {
        pos->line++;
        pos->column = 1;
    }
    pos->idx++;
    return pos->idx;
}

long parse_num(token_list *tokenList, position *pos, const char *string) {
    const int *i = &pos->idx;
    char tmp[MAXTEMPSIZE];
    long num = 0;
    int tmp_idx = 0;
    while (isalnum(string[*i])) {
        tmp[tmp_idx++] = string[advance(pos, string)-1];
    }
    tmp[tmp_idx] = '\0';
    if (tmp[0] == '0' && (tmp[1] == 'x' || tmp[1] == 'X')) {
        num = strtol(tmp + 2, NULL, 16);
    } else if (tmp[0] == '0' && (tmp[1] == 'b' || tmp[1] == 'B')) {
        num = strtol(tmp + 2, NULL, 2);
    } else {
        num = strtol(tmp, NULL, 10);
    }
    return num;
}

char *type_to_string(const enum token_type type) {
    switch (type) {
        case TT_MNEMONIC:
            return "TT_MNEMONIC";
        case TT_REGISTER:
            return "TT_REGISTER";
        case TT_IMMEDIATE:
            return "TT_IMMEDIATE";
        case TT_LBRACKET:
            return "TT_LBRACKET";
        case TT_RBRACKET:
            return "TT_RBRACKET";
        case TT_COMMA:
            return "TT_COMMA";
        case TT_PLUS:
            return "TT_PLUS";
        case TT_MINUS:
            return "TT_MINUS";
        case TT_ASTERISK:
            return "TT_ASTERISK";
        case TT_SIZESPEC:
            return "TT_SIZESPEC";
        case TT_CHAR_LITERAL:
            return "TT_CHAR_LITERAL";
        case TT_STRING_LITERAL:
            return "TT_STRING_LITERAL";
        case TT_IDENTIFIER:
            return "TT_IDENTIFIER";
        case TT_DIRECTIVE:
            return "TT_DIRECTIVE";
        case TT_COLON:
            return "TT_COLON";
        case TT_SEMICOLON:
            return "TT_SEMICOLON";
        case TT_EOF:
            return "TT_EOF";
        default:
            return "NAN";
    }
}

char *repr_position(position pos) {
    char *return_string = malloc(sizeof(char) * MAXTEMPSIZE);
    sprintf(return_string, "position(filename=%s, idx=%d, column=%d, line=%d)", pos.filename, pos.idx, pos.column, pos.line);
    return return_string;
}

char *repr_token(token *tok) {
    char *return_string = malloc(sizeof(char) * MAXTEMPSIZE);
    char *pos = repr_position(tok->pos);
    char *type = type_to_string(tok->type);
    if (tok->type == TT_IMMEDIATE) {
        sprintf(return_string, "token(type=%s, pos=%s, value=%d)", type, pos, *(int *)tok->value);
    } else {
        sprintf(return_string, "token(type=%s, pos=%s, value=%s)", type, pos, (char *)tok->value);
    }
    free(pos);
    return return_string;
}

void print_token_list(token_list *tokenList) {
    int tok_idx = 0;
    char *repr;
    while (tokenList->data[tok_idx].type != TT_EOF) {
        repr = repr_token(&tokenList->data[tok_idx]);
        printf("%s\n", repr);
        free(repr);
        tok_idx++;
    }
    repr = repr_token(&tokenList->data[tok_idx]);
    printf("%s\n", repr);
    free(repr);
}

token_list *tokenise(char *filename, const char *string) {
    position current_pos = {filename, 0, 1, 1};
    const int *i = &current_pos.idx;
    token_list *tokens = malloc(sizeof(token_list));
    if (tokens == NULL) {
        exit(-1);
    }

    init_token_list(tokens);

    while (string[*i] != '\0') {
        if (isspace(string[*i])) {
            advance(&current_pos, string);
            continue;
        }
        token t;
        t.value = NULL;

        if (string[*i] == ';') {
            while (string[*i] != '\n') {
                if (string[*i] == '\0') {
                    break;
                }
                advance(&current_pos, string);
            }
            continue;
        }

        if (string[*i] == '[') {
            t.type = TT_LBRACKET;
            t.pos = current_pos;
            advance(&current_pos, string);
            push_token(tokens, t);
            continue;
        }

        if (string[*i] == ']') {
            t.type = TT_RBRACKET;
            t.pos = current_pos;
            advance(&current_pos, string);
            push_token(tokens, t);
            continue;
        }

        if (string[*i] == '+') {
            t.type = TT_PLUS;
            t.pos = current_pos;
            advance(&current_pos, string);
            push_token(tokens, t);
            continue;
        }

        if (string[*i] == '-') {
            t.type = TT_MINUS;
            t.pos = current_pos;
            advance(&current_pos, string);
            push_token(tokens, t);
            continue;
        }

        if (string[*i] == '*') {
            t.type = TT_ASTERISK;
            t.pos = current_pos;
            advance(&current_pos, string);
            push_token(tokens, t);
            continue;
        }

        if (string[*i] == ',') {
            t.type = TT_COMMA;
            t.pos = current_pos;
            advance(&current_pos, string);
            push_token(tokens, t);
            continue;
        }

        if (string[*i] == ':') {
            t.type = TT_COLON;
            t.pos = current_pos;
            advance(&current_pos, string);
            push_token(tokens, t);
            continue;
        }

        if (isdigit(string[*i])) {
            t.type = TT_IMMEDIATE;
            t.pos = current_pos;
            long num = parse_num(tokens, &current_pos, string);
            t.value = malloc(sizeof(long));
            memcpy(t.value, &num, sizeof(long));
            push_token(tokens, t);
            continue;
        }

        if (string[*i] == '\'') {
            t.pos = current_pos;
            t.type = TT_CHAR_LITERAL;
            advance(&current_pos, string);

            char tmp[2];
            tmp[0] = string[*i];
            tmp[1] = '\0';

            t.value = strdup(tmp);
            advance(&current_pos, string);
            advance(&current_pos, string);
            push_token(tokens, t);
            continue;
        }

        if (string[*i] == '"') {
            t.pos = current_pos;
            t.type = TT_STRING_LITERAL;
            advance(&current_pos, string);

            size_t buf_capacity = 16;
            size_t buf_len = 0;
            char *tmp = malloc(sizeof(char) * 16);
            while (string[*i] != '"') {
                if (buf_len + 1 >= buf_capacity) {
                    buf_capacity *= 2;
                    tmp = (char *)realloc(tmp, sizeof(char) * buf_capacity);
                    if (!tmp) exit(-1);
                }
                tmp[buf_len++] = string[advance(&current_pos, string)-1];
            }
            tmp[buf_len] = '\0';

            t.value = strdup(tmp);
            advance(&current_pos, string);
            push_token(tokens, t);
            continue;
        }

        if (isalpha(string[*i]) || string[*i] == '.') {
            t.pos = current_pos;

            size_t buf_capacity = 16;
            size_t buf_len = 0;
            char *tmp = malloc(sizeof(char) * 16);
            while (isalnum(string[*i]) || string[*i] == '.' || string[*i] == '_') {
                if (buf_len + 1 >= buf_capacity) {
                    buf_capacity *= 2;
                    tmp = (char *)realloc(tmp, sizeof(char) * buf_capacity);
                    if (!tmp) exit(-1);
                }
                tmp[buf_len++] = string[advance(&current_pos, string)-1];
            }
            tmp[buf_len] = '\0';

            t.type = TT_IDENTIFIER;
            t.value = strdup(tmp);
            if (ismnemonic(tmp)) {
                t.type = TT_MNEMONIC;
            } else if (isregister(tmp)) {
                t.type = TT_REGISTER;
            } else if (tmp[0] == '.') {
                t.type = TT_DIRECTIVE;
                t.value = strdup(tmp+1);
            } else if (issizespec(tmp)) {
                t.type = TT_SIZESPEC;
            }

            push_token(tokens, t);
            continue;
        }

        advance(&current_pos, string);
    }

    token t;
    t.value = NULL;
    t.type = TT_EOF;
    t.pos = current_pos;
    push_token(tokens, t);

    // print_token_list(tokens);

    return tokens;
}