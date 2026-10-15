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
    IN_AND,
    IN_OR,
    IN_XOR,
    IN_NOT,
    IN_SHIFT_L,
    IN_SHIFT_R,
    IN_INC,
    IN_DEC,
    IN_JUMP,
    IN_JUMP_EQ,
    IN_JUMP_NE,
    IN_JUMP_LT,
    IN_JUMP_GT,
    IN_JUMP_LE,
    IN_JUMP_GE,
    IN_CALL,
    IN_RETURN,
    IN_MEMW8,
    IN_MEMR8,
    IN_MEMW16,
    IN_MEMR16,
    IN_MEMW32,
    IN_MEMR32,
    IN_MEMW64,
    IN_MEMR64,
    IN_MEMWS,
    IN_MEMWS2,
    IN_MEMWSN,
    IN_MEMWSH,
    IN_DUMP,
    IN_MEMDUMP,
    IN_TIME,
    IN_IOW8,
    IN_IOR8,
    IN_IOW16,
    IN_IOR16,
    IN_IOW32,
    IN_IOR32,
    IN_IOW64,
    IN_IOR64,
    IN_IOWS,
    IN_IOWS2,
} inst_e;

typedef enum {
    L_GEN, // generic, no specific type. stores token's value
    L_ID,
    L_NUM,
    L_STR,
    L_REF,
    L_INT,
} lit_e;

typedef enum {
    INT_UNKNOWN,
    INT_MEM_SIZE,
    INT_REG_COUNT,
    INT_CX_VER,
    INT_STDIN,
    INT_STDOUT,
    INT_STDERR,
} int_e;

typedef struct lit {
    lit_e type;
    int lineno;
    int pos;
    struct lit *next;
    union {
        uint32_t uid;
        int64_t int_type;
        char *char_type;
    };
    char *repr;
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