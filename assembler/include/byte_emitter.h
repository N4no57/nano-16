//
// Created by brego on 31/01/2026.
//

#ifndef BYTE_EMITTER_H
#define BYTE_EMITTER_H

#include "parser.h"

// okay...
// in layman's terms
// go through the statement list while tracking the segment and chuck bytes into it
// simple as

struct opcode_info {
    enum opcode op;
    u8 cls;
    u8 opc;
    u8 dir;
    u8 is_ext;
    u8 ext_opc; // only if is_ext=1
};

void emit_bytes(struct statement_list *stmnt_list, struct symbol_table *symtbl, segment_table *seg_table);

#endif //BYTE_EMITTER_H
