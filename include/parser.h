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
    IN_MOD,
    IN_JUMP,
    IN_JUMP_EQ,
    IN_JUMP_NE,
    IN_JUMP_LT,
    IN_JUMP_GT,
    IN_JUMP_LE,
    IN_JUMP_GE,
    IN_DUMP,
} inst_e;

typedef enum {
    L_GEN, // generic, no specific type. stores token's value
    L_ID,
    L_NUM,
} lit_e;

typedef struct lit {
    lit_e type;
    int lineno;
    int pos;
    struct lit *next;
    union {
        uint32_t id;
        int num;
        char *gen;
    };
} lit_t;

typedef struct {
    inst_e type;
    int lineno;
    int pos;
	lit_t *arg;
} inst_t;

typedef struct {
    uint32_t id;
    char *name;
} id2_t; // named id2_t to avoid conflict in sys/types.h 

typedef struct {
    size_t count;
    size_t length;
    uint32_t index;
    id2_t **array;
} ids_t;

typedef struct {
    ids_t *ids;
    size_t count;
    size_t length;
    inst_t **array;
} insts_t;

extern insts_t *parse(token_t *tokens);

#endif