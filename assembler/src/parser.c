#include "../include/parser.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define STATEMENT_LIST_BASE_CAPACITY 20
// directonality
#define RM_TO_REG 0
#define REG_TO_RM 1
// instruction modes
#define MOD_RM_IND 0
#define MOD_ABSOLUTE 1
#define MOD_RM_DISP 2
#define MOD_REG_REG 3
// AEX prefix
#define MOD_SIB 0
#define MOD_IMMEDIATE 1
#define MOD_SIB_DISP 2

struct operand_analysis {
    /* ========= Register side ========= */

    i8  has_reg;                 // any explicit register operand
    enum registers reg;          // logical REG field (ModR/M.reg)
    u8  reg_src_index;           // index in inst->oprs[] it came from


    /* ========= Memory side ========= */

    i8  has_mem;                 // instruction references memory
    enum {
        MEM_NONE,
        MEM_RM,                  // [base]
        MEM_SIB,                 // [base + index * scale]
        MEM_ABS                  // [absolute]
    } mem_kind;

    enum registers rm;            // base register (ModR/M.rm)
    u8  rm_src_index;

    /* --- SIB details (if MEM_SIB) --- */
    i8  has_sib;
    enum registers sib_base;
    enum registers sib_index;
    u8  sib_scale;
    u8  sib_src_index;


    /* ========= Displacement ========= */

    i8  has_disp;
    i32 disp_value;
    u8  disp_size;               // 1 or 2 bytes
    u8  disp_src_index;


    /* ========= Immediate ========= */

    i8  has_imm;
    i64 imm_value;
    u8  imm_size;
    u8  imm_src_index;
    i8  imm_to_mem;              // AEX mode 11 vs 01


    /* ========= Derived encoding info ========= */

    u8  mod;                     // final ModR/M.mod value
    u8  direction;               // REG→RM or RM→REG

    i8  needs_modrm;
    i8  needs_sib;
    i8  needs_aex;

    /* ========= Bookkeeping ========= */

    u8  operand_span_start;      // first operand index consumed
    u8  operand_span_end;        // last operand index consumed
};

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

        struct operand *abs_opr = &inst->oprs[inst->operands-1];
        abs_opr->type = OPERAND_ABS;

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
        if (current_tok->type == TT_NEWLINE) return;

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

void first_pass(const token_list *tokens, token *current_tok, size_t *tok_idx, struct statement_list *result) {
    while (current_tok->type != TT_EOF) {
        struct statement stmnt;

        if (current_tok->type == TT_MNEMONIC) {
            struct instruction inst;
            inst.opc = matchOpcode(current_tok->value);
            inst.pos = current_tok->pos;
            inst.prefix_count = 0;
            inst.operands = 0;

            // token mnemonic = current_tok;
            consume(tokens, tok_idx, current_tok);

            parse_operands(tokens, current_tok, tok_idx, &inst);

            stmnt.type = ST_INSTRUCTION;
            stmnt.instruction = inst;
            push_statement(result, &stmnt);
            continue;
        }

        if (current_tok->type == TT_IDENTIFIER) {
            struct symbol sym;
            sym.name = strdup(current_tok->value);
            sym.pos = current_tok->pos;

            consume(tokens, tok_idx, current_tok);

            if (current_tok->type != TT_COLON) {
                print_error(&current_tok->pos, "expected \":\" after identifier\n");
                exit(EXIT_FAILURE);
            }

            consume(tokens, tok_idx, current_tok);
            sym.defined = 1;
            sym.type = SYM_LABEL;

            stmnt.type = ST_SYMBOL;
            stmnt.symbol = sym;

            push_statement(result, &stmnt);

            continue;
        }

        consume(tokens, tok_idx, current_tok);
    }
}

i32 is_memory(const enum operand_type type) {
    if (type == OPERAND_RM || type == OPERAND_ABS || type == OPERAND_SIB) {
        return 1;
    }
    return 0;
}

i8 has_prefix(const i8 prefix, struct instruction *inst) {
    for (i8 i = 0; i < inst->prefix_count; i++) {
        if ((inst->prefixes[i] & 0xF0) == prefix) {
            return 1;
        }
    }
    return 0;
}

i32 calc_mode(const u32 *idx, struct instruction *inst) {
    // idx is the operand wanted to be indexed that will be used to calculate the mode
    // the instruction this operation is happening on

    // mode = 00 if rm
    // mode = 10 if rm ± disp

    // AEX prefix + mode = 00 if SIB
    // AEX prefix + mode = 01 if immediate
    // AEX prefix + mode = 10 if SIB ± disp

    if (inst->oprs[*idx].type == OPERAND_RM) {
        if (*idx+1 < inst->operands && inst->oprs[*idx+1].type == OPERAND_DISP) {
            return MOD_RM_DISP;
        }
        return MOD_RM_IND;
    }

    if (inst->oprs[*idx].type == OPERAND_SIB) {
        if (!has_prefix(AEX, inst))
            inst->prefixes[inst->prefix_count++] = AEX_PREFIX(1);
        if (*idx+1 < inst->operands && inst->oprs[*idx+1].type == OPERAND_DISP) {
            return MOD_SIB_DISP;
        }
        return MOD_SIB;
    }

    if (inst->oprs[*idx].type == OPERAND_ABS) {
        if (!has_prefix(AEX, inst))
            inst->prefixes[inst->prefix_count++] = AEX_PREFIX(1);
        return MOD_IMMEDIATE;
    }

    return -1; // mode not found
}

/*
  for each instruction:
    for each operand:
        if operand is REG:
            if both operands are REG:   # simple reg-to-reg
                ModR/M.mod = 3         # reg-reg mode
                ModR/M.reg = dest reg
                ModR/M.rm  = src reg
            else if one operand is memory:
                ModR/M.mod = compute_mode(memory)
                ModR/M.reg = reg operand
                ModR/M.rm  = memory operand
                if SIB needed:
                    fill SIB structure
        else if operand is MEM:
            compute ModR/M.mod based on addressing
            set ModR/M.rm field
            if addressing requires SIB:
                fill SIB structure
            if displacement present:
                store displacement
        else if operand is IMM:
            store immediate value
    validate operand combination against instruction class rules
 */

void merge_to_modrm(struct instruction *inst) {
    struct operand modrm = {0};
    modrm.type = OPERAND_MODRM;
    u32 i = 0;
    while (i < inst->operands) {
        if (inst->oprs[i].type == OPERAND_REG) {
            modrm.modrm.directionality = RM_TO_REG;
            modrm.modrm.reg = inst->oprs[i].reg; // dest
            i++;

            if (i < inst->operands && inst->oprs[i].type == OPERAND_REG) {
                modrm.modrm.mod = MOD_REG_REG;
                modrm.modrm.rm = inst->oprs[i].reg; // src
            } else if (i < inst->operands && is_memory(inst->oprs[i].type)) {
                modrm.modrm.mod = calc_mode(&i, inst);
                modrm.modrm.rm = inst->oprs[i].reg;

                if (has_prefix(AEX, inst)) {
                    // all modes that are in the AEX prefix do not use rm
                    modrm.modrm.rm = NONE;
                }
            }
            i++;
        } else if (is_memory(inst->oprs[i].type)) {
            // operands may need to be rearranged

            modrm.modrm.directionality = REG_TO_RM;
            modrm.modrm.mod = calc_mode(&i, inst);

            modrm.modrm.rm = inst->oprs[i].reg;

            if (has_prefix(AEX, inst)) {
                // all modes that are in the AEX prefix do not use rm
                modrm.modrm.rm = NONE;
            }
            i++;
        }
    }
}

void second_pass(const struct statement_list *result) {
    for (int i = 0; i < result->count; i++) {
        struct statement *stmnt = &result->statements[i];
        if (stmnt->type == ST_INSTRUCTION) {
            merge_to_modrm(&stmnt->instruction);
        }
    }
}

struct statement_list parse(const token_list *tokens) {
    struct statement_list result;
    init_statement_list(&result);

    size_t tok_idx = 0;
    token current_tok = tokens->data[tok_idx];

    first_pass(tokens, &current_tok, &tok_idx, &result);

    for (int i = 0; i < result.count; i++) {
        // printf("i = %d:\n", i);
        print_statement(&result.statements[i]);
    }

    second_pass(&result);

    return result;
}
