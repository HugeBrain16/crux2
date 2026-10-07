#include <vm.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static insts_t *insts = NULL;
static ids_t *ids = NULL;

static void insts_push(inst_t *inst) {
    if (insts->count == insts->length - 1) {
        insts->length *= 2;
        insts->array = realloc(insts->array, sizeof(inst_t)*insts->length);
    }

    insts->array[insts->count++] = inst;
}

static void ids_push(id2_t *id) {
    if (ids->count == ids->length - 1) {
        ids->length *= 2;
        ids->array = realloc(ids->array, sizeof(id2_t)*ids->length);
    }

    ids->array[ids->count++] = id;
}

static token_t *parse_num_arg(token_t *inst, token_t *arg) {
    if (!arg) {
        printf("Error: Value expected (l: %d, p: %d)\n", inst->lineno, inst->pos);
        return NULL;
    }

    if (arg->type != TOKEN_NUM) {
        printf("Error: Value expected (l: %d, p: %d)\n", arg->lineno, arg->pos);
        return NULL;
    }

    return arg->next;
}

static token_t *parse_id_arg(token_t *inst, token_t *arg) {
    if (!arg) {
        printf("Error: Value expected (l: %d, p: %d)\n", inst->lineno, inst->pos);
        return NULL;
    }

    if (arg->type != TOKEN_ID) {
        printf("Error: Value expected (l: %d, p: %d)\n", arg->lineno, arg->pos);
        return NULL;
    }

    return arg->next;
}

static token_t *parse_jump_cond(token_t *inst, token_t *arg) {
    /* jump to defined label if condition met */

    if (!arg) {
        printf("Error: Value expected (l: %d, p: %d)\n", inst->lineno, inst->pos);
        return NULL;
    }

    if (arg->type != TOKEN_ID) {
        printf("Error: Value expected (l: %d, p: %d)\n", arg->lineno, arg->pos);
        return NULL;
    }

    token_t *next = arg->next;
    if (!next) {
        printf("Error: Value expected (l: %d, p: %d)\n", arg->lineno, arg->pos);
        return NULL;
    }

    if (next->type != TOKEN_NUM) {
        printf("Error: Value expected (l: %d, p: %d)\n", next->lineno, next->pos);
        return NULL;
    }

    return next->next;
}

static token_t *parse_no_arg(token_t *inst, token_t *arg) {
    return arg;
}

static uint32_t get_index(char *name) {
    if (!ids) {
        ids = malloc(sizeof(ids_t));
        ids->index = 0;
        ids->count = 0;
        ids->length = 2;
        ids->array = malloc(sizeof(id2_t)*ids->length);
    }

    for (size_t i = 0; i < ids->count; i++) {
        id2_t *id = ids->array[i];

        if (!strcmp(id->name, name))
            return id->id;
    }

    id2_t *id = malloc(sizeof(id2_t));
    id->id = ids->index++;
    id->name = name;
    ids_push(id);

    return id->id;
}

static lit_t *parse_args(token_t *arg) {
    lit_t *head = NULL;
    lit_t *current = NULL;

    token_t *curr = arg;
    while (curr && curr->type != TOKEN_END) {
        lit_t *lit = malloc(sizeof(lit_t));
        lit->lineno = curr->lineno;
        lit->pos = curr->pos;
        switch (curr->type) {
            case TOKEN_NUM:
                lit->type = L_NUM;
                lit->num = atoi(curr->value);
                break;
            case TOKEN_ID:
                lit->type = L_ID;
                lit->id = get_index(curr->value);
                break;
            default:
                lit->type = L_GEN;
                lit->gen = curr->value;
                break;
        }

        if (!head) {
            head = lit;
            current = lit;
        } else {
            current->next = lit;
            current = lit;
        }

        curr = curr->next;
    }

    return head;
}

typedef token_t *(*parser_fn)(token_t*,token_t*);
typedef struct parser {
    char *name;
    inst_e type;
    parser_fn fn;
} parser_t;

static parser_t parsers[] = {
    {"select", IN_SELECT, parse_num_arg},

    {"load_i", IN_LOAD_I, parse_num_arg},
    {"load",   IN_LOAD,   parse_num_arg},

    {"add", IN_ADD, parse_num_arg},
    {"sub", IN_SUB, parse_num_arg},
    {"mul", IN_MUL, parse_num_arg},
    {"div", IN_DIV, parse_num_arg},
    {"mod", IN_MOD, parse_num_arg},

    {"label",  IN_LABEL,  parse_id_arg},
    {"jump",   IN_JUMP,   parse_id_arg},

    {"jump_eq", IN_JUMP_EQ, parse_jump_cond},
    {"jump_ne", IN_JUMP_NE, parse_jump_cond},
    {"jump_lt", IN_JUMP_LT, parse_jump_cond},
    {"jump_gt", IN_JUMP_GT, parse_jump_cond},
    {"jump_le", IN_JUMP_LE, parse_jump_cond},
    {"jump_ge", IN_JUMP_GE, parse_jump_cond},

    {"dump", IN_DUMP, parse_no_arg},
};

insts_t *parse(token_t *tokens) {
    insts = malloc(sizeof(insts_t));
    insts->length = 4;
    insts->count = 0;
    insts->array = malloc(sizeof(inst_t)*insts->length);

    token_t *curr = tokens;
    while (curr) {
        // start of instruction
        if (curr->type == TOKEN_ID) {
            token_t *end = NULL;
            token_t *start = curr;
            curr = curr->next;

            inst_e type;
            int parsed = 0;
            for (size_t i = 0; i < sizeof(parsers) / sizeof(parsers[0]); i++) {
                parser_t parser = parsers[i];

                if (!strcmp(start->value, parser.name)) {
                    end = parser.fn(start, curr);
                    type = parser.type;
                    parsed = 1;
                    break;
                }
            }

            if (!parsed) {
                printf("Error: Unknown instruction (l: %d, p: %d)\n", curr->lineno, curr->pos);
                break;
            }

            // error occured
            if (!end)
                break;

            if (!end || end->type != TOKEN_END) {
                printf("Error: Missing semicolon (l: %d, p: %d)\n", start->lineno, start->pos);
                break;
            }

            inst_t *inst = malloc(sizeof(inst_t));
            inst->type = type;
            inst->lineno = start->lineno;
            inst->pos = start->pos;
            inst->arg = start->next && start->next->type != TOKEN_END ? parse_args(start->next) : NULL;
            insts_push(inst);

            curr = end->next;
        } else {
            printf("Error: Must start with instruction (l: %d, p: %d)\n", curr->lineno, curr->pos);
            break;
        }
    }

    /*
    for (size_t i = 0; i < ids->count; i++) {
        id2_t *id = ids->array[i];

        printf("id=%d name=%s\n", id->id, id->name);
    }
    */

    insts->ids = ids;
    return insts;
}