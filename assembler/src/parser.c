#include "../include/parser.h"

#include <stdbool.h>
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


    /* ========= Absolute ========= */

    i8  has_abs;
    i64 abs_value;
    u8  abs_size;
    u8  abs_src_index;


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

i8 has_memory(const struct operand_analysis op) {
    if (op.has_abs == 1 || op.has_disp == 1 || op.has_sib == 1) {
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

i32 calc_mode(const struct operand_analysis *op) {
    // idx is the operand wanted to be indexed that will be used to calculate the mode
    // the instruction this operation is happening on

    // mode = 00 if rm
    // mode = 01 if Absolute
    // mode = 10 if R/M ± 8/16-bit displacement
    // mode = 11 if reg to reg

    // AEX prefix + mode = 00 if SIB
    // AEX prefix + mode = 01 if immediate (not to mem)
    // AEX prefix + mode = 10 if SIB ± disp
    // AEX prefix + mode = 11 if Immediate (to mem)

    // TODO
    if (op->has_abs == 1) {
        
    }
}

i32 determine_directionality(const struct operand_analysis *analysis) {
   // TODO
    if (analysis->has_reg && analysis->has_mem) {

    }
}

struct operand_analysis capture_operands(const struct instruction *inst) {
    struct operand_analysis result = {0};

    for (u8 i = 0; i < inst->operands; i++) {
        const struct operand op = inst->oprs[i];

        switch (op.type) {
            case OPERAND_REG:
                // if this is the first reg then it is the reg field in ModR/M
                if (!result.has_reg) {
                    result.has_reg = 1;
                    result.reg = op.reg;
                    result.reg_src_index = i;
                    break;
                }
                // if this is the 2nd reg then it is the rm field in ModR/M
                result.rm = op.reg;
                result.rm_src_index = i;
                // mark this as reg->reg
                result.mod = MOD_REG_REG;
                result.direction = RM_TO_REG;
                break;
            case OPERAND_RM:
                // is only RM if not SIB or ABS
                result.has_mem = 1;
                result.mem_kind = MEM_RM;

                result.rm = op.reg;
                result.rm_src_index = i;
                break;
            case OPERAND_SIB:
                result.has_sib = 1;
                result.sib_base = op.sib.base;
                result.sib_index = op.sib.idx;
                result.sib_scale = op.sib.mod;
                result.sib_src_index = i;
                break;
            case OPERAND_DISP:
                result.has_disp = 1;
                result.disp_value = op.disp;
                result.disp_src_index = i;
                result.disp_size = 2;
                if (op.disp <= 127 || op.disp >= -128) result.disp_size = 1;
                break;
            case OPERAND_ABS:
                result.has_abs = 1;
                result.abs_value = op.imm;
                result.abs_src_index = i;
                result.abs_size = 2;
                if (op.imm <= 127 || op.imm >= -128) result.abs_size = 1;
                break;
            case OPERAND_IMM:
                result.has_imm = 1;
                result.imm_value = op.imm;
                result.imm_src_index = i;
                result.imm_size = 2;
                if (op.imm <= 127 || op.imm >= -128) result.imm_size = 1;
                break;
            default:
                perror("operand analysis error");
                exit(EXIT_FAILURE);
        }
    }

    return result;
}

void second_pass(const struct statement_list *result) {
    for (int i = 0; i < result->count; i++) {
        const struct statement *stmnt = &result->statements[i];
        if (stmnt->type == ST_INSTRUCTION) { // TODO
            struct operand_analysis analysis = capture_operands(stmnt);
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
