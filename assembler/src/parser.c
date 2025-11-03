#include "../include/parser.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define STATEMENT_LIST_BASE_CAPACITY 20

const char* opcode_to_string(enum opcode opc) {
    switch (opc) {
        case ADD: return "ADD"; case SUB: return "SUB"; case AND: return "AND";
        case OR: return "OR"; case XOR: return "XOR"; case CMP: return "CMP";
        case TEST: return "TEST"; case SHL: return "SHL"; case SHR: return "SHR";
        case ADC: return "ADC"; case INC: return "INC"; case DEC: return "DEC";
        case MUL: return "MUL"; case DIV: return "DIV"; case NOT: return "NOT";
        case MOV: return "MOV"; case PUSH: return "PUSH"; case POP: return "POP";
        case INB: return "INB"; case OUTB: return "OUTB"; case LEA: return "LEA";
        case JMP: return "JMP"; case JZ: return "JZ"; case JNZ: return "JNZ";
        case JE: return "JE"; case JNE: return "JNE"; case JC: return "JC";
        case JNC: return "JNC"; case CALL: return "CALL"; case RET: return "RET";
        case JA: return "JA"; case JAE: return "JAE"; case JB: return "JB";
        case JBE: return "JBE"; case JG: return "JG"; case JGE: return "JGE";
        case JL: return "JL"; case JLE: return "JLE"; case NOP: return "NOP";
        case HLT: return "HLT";
        default: return "UNKNOWN_OPCODE";
    }
}

const char* reg_to_string(enum registers r) {
    switch (r) {
        case A: return "A"; case B: return "B"; case C: return "C"; case D: return "D";
        case SI: return "SI"; case DI: return "DI"; case BP: return "BP";
        case SP: return "SP"; case IP: return "IP";
        default: return "UNKNOWN_REG";
    }
}

void print_operand(struct operand *op) {
    switch (op->type) {
        case OPERAND_NONE:
            printf("NONE");
        break;
        case OPERAND_IMM:
            printf("IMM:%u", op->imm);
        break;
        case OPERAND_ABS:
            printf("ABS:%u", op->imm);
        break;
        case OPERAND_DISP:
            printf("DISP:%d", op->disp);
        break;
        case OPERAND_REG:
            printf("REG:%s", reg_to_string(op->reg));
        break;
        case OPERAND_RM:
            printf("RM:%s", reg_to_string(op->reg));
        break;
        case OPERAND_MODRM:
            printf("ModR/M(mod=%u reg=%s rm=%s)", op->modrm.mod, reg_to_string(op->modrm.reg), reg_to_string(op->modrm.rm));
        break;
        case OPERAND_SIB:
            printf("SIB(scale=%u idx=%s base=%s)", op->sib.mod, reg_to_string(op->sib.idx), reg_to_string(op->sib.base));
        break;
        default:
            printf("UNKNOWN_OPERAND");
    }
}

void print_instruction(struct instruction *ins) {
    printf("Instruction: mnemonic=%s, opc=%d, operands=%d\n", opcode_to_string(ins->opc),  ins->opc, ins->operands);

    for (int i = 0; i < ins->operands; i++) {
        printf("  Operand %d: ", i);
        print_operand(&ins->oprs[i]);
        printf("\n");
    }
}

void print_symbol(struct symbol *sym) {
    printf("Symbol: name=%s, value=%ld, type=%d, defined=%d\n",
           sym->name, sym->value, sym->type, sym->defined);
}

void print_directive(struct directive *dir) {
    printf("Directive: name=%s\n", dir->name);
    // You could expand args if needed
}

void print_statement(struct statement *stmt) {
    switch (stmt->type) {
        case ST_INSTRUCTION:
            print_instruction(&stmt->instruction);
        break;
        case ST_SYMBOL:
            print_symbol(&stmt->symbol);
        break;
        case ST_DIRECTIVE:
            print_directive(&stmt->directive);
        break;
        default:
            printf("Unknown statement type\n");
    }
}

enum opcode matchOpcode(const char *mnemonic) {
    return getmnemonic(mnemonic);
}

enum sizeSpec matchSizeSpec(const char *str) {
    return getsizespec(str);
}

enum registers matchRegister(const char *str) {
    return getregister(str);
}

void init_statement_list(struct statement_list *list) {
    list->capacity = STATEMENT_LIST_BASE_CAPACITY;
    list->count = 0;
    list->statements = malloc(sizeof(struct statement) * list->capacity);
}

void push_statement(struct statement_list *list, const struct statement *statement) {
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        struct statement *tmp = realloc(list->statements, sizeof(struct statement) * list->capacity);
        if (!tmp) {
            perror("realloc");
            exit(EXIT_FAILURE);
        }
        list->statements = tmp;
    }
    list->statements[list->count] = *statement;
    list->count++;
}

void consume(const token_list *tokens, size_t *idx, token *tok) {
    (*idx)++;
    *tok = tokens->data[*idx];
}

void parse_value(const token_list *tokens, token *current_tok, size_t *tok_idx, struct instruction *inst) {
    struct operand oprd;
    if (current_tok->type == TT_IDENTIFIER) { // identifier
        // dunno how to handle this rn
    } else { // actual number
        oprd.type = OPERAND_IMM;
        oprd.imm = (unsigned int)current_tok->type;
        oprd.size = oprd.imm > 0xFF ? 2 : 1; // not full proof need to update this when refining assembler
        inst->oprs[inst->operands++] = oprd;
    }
    consume(tokens, tok_idx, current_tok);
}

void parse_disp(const token_list *tokens, token *current_tok, size_t *tok_idx, struct instruction *inst) {
    struct operand oprd;
    oprd.type = OPERAND_DISP;
    char sign = current_tok->type == TT_PLUS ? '+' : '-';
    consume(tokens, tok_idx, current_tok); // consume "+"/"-"
    oprd.disp = sign == '+' ? *(int *)current_tok->value : *(int *)current_tok->value * -1;
    oprd.size = oprd.disp > 127 || oprd.disp < -128 ? 2 : 1; // incomplete, be fixed in assembler revision
    inst->oprs[inst->operands++] = oprd;
    consume(tokens, tok_idx, current_tok); // consume disp
}

void parse_mem_op(const token_list *tokens, token *current_tok, size_t *tok_idx, struct instruction *inst) {
    struct operand oprd;
    consume(tokens, tok_idx, current_tok); // consume "["

    // ABS
    if (current_tok->type == TT_IDENTIFIER || current_tok->type == TT_IMMEDIATE) {
        parse_value(tokens, current_tok, tok_idx, inst);

        if (current_tok->type != TT_RBRACKET) {
            print_error(&current_tok->pos, "Expected \"]\" after absolute");
            exit(EXIT_FAILURE);
        }
        consume(tokens, tok_idx, current_tok); // consume "]"
        return;
    }

    if (current_tok->type == TT_REGISTER) {
        // REG
        u8 operand_count = inst->operands;
        oprd.type = OPERAND_RM;
        oprd.reg = matchRegister(current_tok->value);
        oprd.size = 2;
        inst->oprs[inst->operands++] = oprd;
        consume(tokens, tok_idx, current_tok); // consume reg

        if (current_tok->type == TT_PLUS && tokens->data[(*tok_idx)+1].type == TT_REGISTER) {
            // SIB
            struct operand *sib = &inst->oprs[operand_count];
            sib->type = OPERAND_SIB;
            sib->sib.base = inst->oprs[operand_count].reg; // turn operand to SIB

            consume(tokens, tok_idx, current_tok); // consume "+"

            if (current_tok->type != TT_REGISTER) {
                print_error(&current_tok->pos, "Expected reg after \"+\"");
                exit(EXIT_FAILURE);
            }

            sib->sib.idx = matchRegister(current_tok->value);
            consume(tokens, tok_idx, current_tok); // consume reg

            if (current_tok->type != TT_ASTERISK) {
                print_error(&current_tok->pos, "Expected \"*\"");
                exit(EXIT_FAILURE);
            }

            consume(tokens, tok_idx, current_tok); // consume "*"

            if (current_tok->type != TT_IMMEDIATE) {
                print_error(&current_tok->pos, "Expected immediate after \"*\"");
                exit(EXIT_FAILURE);
            }

            const u8 scale = *(int *)current_tok->value;
            consume(tokens, tok_idx, current_tok); // consume scale

            if (scale != 1 && scale != 2 && scale != 4 && scale != 8) {
                print_error(&current_tok->pos, "Invalid scale\nScale should be 1, 2, 4 or 8");
                exit(EXIT_FAILURE);
            }

            sib->sib.mod = scale;
        }

        if (current_tok->type == TT_PLUS || current_tok->type == TT_MINUS) {
            // DISP
            parse_disp(tokens, current_tok, tok_idx, inst);
        }

        if (current_tok->type != TT_RBRACKET) {
            print_error(&current_tok->pos, "Expected \"]\"");
            exit(EXIT_FAILURE);
        }
        consume(tokens, tok_idx, current_tok); // consume "]"
    }
}

void parse_operands(const token_list *tokens, token *current_tok, size_t *tok_idx, struct instruction *inst) {
    do {
        struct operand oprd;
        if (current_tok->type == TT_COMMA) consume(tokens, tok_idx, current_tok);

        if (current_tok->type == TT_REGISTER) {
            oprd.type = OPERAND_REG;
            oprd.reg = matchRegister(current_tok->value);
            oprd.size = 2;

            inst->oprs[inst->operands++] = oprd;

            consume(tokens, tok_idx, current_tok);
            continue;
        }

        // both treated as immediate
        // but one is not the same as the other semantically but doesn't really matter at this stage
        if (current_tok->type == TT_IDENTIFIER || current_tok->type == TT_IMMEDIATE) {
            parse_value(tokens, current_tok, tok_idx, inst);
            continue;
        }

        if (current_tok->type == TT_LBRACKET) {
            parse_mem_op(tokens, current_tok, tok_idx, inst);
            continue;
        }

        if (current_tok->type == TT_EOF) {
            break;
        }
    } while (current_tok->type == TT_COMMA);
}

struct statement_list parse(const token_list *tokens) {
    struct statement_list result;
    init_statement_list(&result);

    size_t tok_idx = 0;
    token current_tok = tokens->data[tok_idx];

    while (current_tok.type != TT_EOF) {
        struct statement stmnt;

        if (current_tok.type == TT_MNEMONIC) {
            struct instruction inst;
            inst.opc = matchOpcode(current_tok.value);
            inst.pos = current_tok.pos;
            inst.prefix_count = 0;
            inst.operands = 0;

            // token mnemonic = current_tok;
            consume(tokens, &tok_idx, &current_tok);

            parse_operands(tokens, &current_tok, &tok_idx, &inst);

            stmnt.type = ST_INSTRUCTION;
            stmnt.instruction = inst;
            push_statement(&result, &stmnt);
            continue;
        }

        if (current_tok.type == TT_IDENTIFIER) {
            struct symbol sym;
            sym.name = strdup(current_tok.value);
            sym.pos = current_tok.pos;

            consume(tokens, &tok_idx, &current_tok);

            if (current_tok.type != TT_COLON) {
                print_error(&current_tok.pos, "expected \":\" after identifier\n");
                exit(EXIT_FAILURE);
            }

            consume(tokens, &tok_idx, &current_tok);
            sym.defined = 1;
            sym.type = SYM_LABEL;

            stmnt.type = ST_SYMBOL;
            stmnt.symbol = sym;

            push_statement(&result, &stmnt);

            continue;
        }

        consume(tokens, &tok_idx, &current_tok);
    }

    for (int i = 0; i < result.count; i++) {
        printf("i = %d:\n", i);
        print_statement(&result.statements[i]);
    }

    return result;
}
