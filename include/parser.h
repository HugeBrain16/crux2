#ifndef PARSER_H
#define PARSER_H

#include <token.h>

#include <stdint.h>

typedef enum {
    IN_SELECT,
    IN_LOAD_I,
    IN_LOAD,
    IN_LABEL,
    IN_ADD,
    IN_SUB,
    IN_MUL,
    IN_DIV,
    IN_JUMP,
    IN_JUMP_EQ,
    IN_JUMP_NE,
    IN_JUMP_LT,
    IN_JUMP_GT,
    IN_JUMP_LE,
    IN_JUMP_GE,
    IN_DUMP,
} inst_e;

typedef struct {
    inst_e type;
	token_t *inst;
	token_t *arg;
} inst_t;

typedef struct {
    size_t count;
    size_t length;
    inst_t **array;
} insts_t;

extern insts_t *parse(token_t *tokens);

#endif