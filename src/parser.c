#include <vm.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static insts_t *insts = NULL;

static void insts_push(inst_t *inst) {
    if (insts->count == insts->length - 1) {
        insts->length *= 2;
        insts->array = realloc(insts->array, sizeof(inst_t)*insts->length);
    }

    insts->array[insts->count++] = inst;
}

static token_t *parse_select(token_t *inst, token_t *arg) {
    /* select register */
    
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

static token_t *parse_load_i(token_t *inst, token_t *arg) {
    /* load value to selected register */

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

static token_t *parse_load(token_t *inst, token_t *arg) {
    /* load value to selected register from register */

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

static token_t *parse_add(token_t *inst, token_t *arg) {
    /* add value to selected register from specified register */

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

static token_t *parse_sub(token_t *inst, token_t *arg) {
    /* sub value to selected register from specified register */

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

static token_t *parse_label(token_t *inst, token_t *arg) {
    /* label for jumps */

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

static token_t *parse_jump(token_t *inst, token_t *arg) {
    /* jump to defined label */

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

static token_t *parse_jump_eq(token_t *inst, token_t *arg) {
    /* jump to defined label if equal to value of register */

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

static token_t *parse_jump_ne(token_t *inst, token_t *arg) {
    /* jump to defined label if not equal to value of register */

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

static token_t *parse_jump_lt(token_t *inst, token_t *arg) {
    /* jump to defined label if less than value of register */

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

static token_t *parse_jump_gt(token_t *inst, token_t *arg) {
    /* jump to defined label if greater than value of register */

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

static token_t *parse_jump_le(token_t *inst, token_t *arg) {
    /* jump to defined label if less than or equal to value of register */

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

static token_t *parse_jump_ge(token_t *inst, token_t *arg) {
    /* jump to defined label if greater than or equal to value of register */

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

static token_t *parse_dump(token_t *inst, token_t *arg) {
    /* dump vm info */
    return arg;
}

insts_t *parse(token_t *tokens) {
    insts = malloc(sizeof(insts_t));
    insts->length = 4;
    insts->count = 0;
    insts->array = malloc(sizeof(inst_t)*insts->length);

    token_t *curr = tokens;
    while (curr && curr->next) {
        // start of instruction
        if (curr->type == TOKEN_ID) {
            token_t *end = NULL;
            token_t *start = curr;
            curr = curr->next;

            inst_e type;
            if (!strcmp(start->value, "load_i")) {
                end = parse_load_i(start, curr);
                type = IN_LOAD_I;
            } else if (!strcmp(start->value, "load")) {
                end = parse_load(start, curr);
                type = IN_LOAD;
            } else if (!strcmp(start->value, "select")) {
                end = parse_select(start, curr);
                type = IN_SELECT;
            } else if (!strcmp(start->value, "add")) {
                end = parse_add(start, curr);
                type = IN_ADD;
            } else if (!strcmp(start->value, "sub")) {
                end = parse_sub(start, curr);
                type = IN_SUB;
            } else if (!strcmp(start->value, "mul")) {
                end = parse_add(start, curr);
                type = IN_MUL;
            } else if (!strcmp(start->value, "div")) {
                end = parse_sub(start, curr);
                type = IN_DIV;
            } else if (!strcmp(start->value, "label")) {
                end = parse_label(start, curr);
                type = IN_LABEL;
            } else if (!strcmp(start->value, "jump")) {
                end = parse_jump(start, curr);
                type = IN_JUMP;
            } else if (!strcmp(start->value, "jump_eq")) {
                end = parse_jump_eq(start, curr);
                type = IN_JUMP_EQ;
            } else if (!strcmp(start->value, "jump_ne")) {
                end = parse_jump_ne(start, curr);
                type = IN_JUMP_NE;
            } else if (!strcmp(start->value, "jump_lt")) {
                end = parse_jump_lt(start, curr);
                type = IN_JUMP_LT;
            } else if (!strcmp(start->value, "jump_gt")) {
                end = parse_jump_gt(start, curr);
                type = IN_JUMP_GT;
            } else if (!strcmp(start->value, "jump_le")) {
                end = parse_jump_le(start, curr);
                type = IN_JUMP_LE;
            } else if (!strcmp(start->value, "jump_ge")) {
                end = parse_jump_ge(start, curr);
                type = IN_JUMP_GE;
            } else if (!strcmp(start->value, "dump")) {
                end = parse_dump(start, curr);
                type = IN_DUMP;
            } else {
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
            inst->inst = start;
            inst->arg = start->next && start->next->type != TOKEN_END ? start->next : NULL;
            insts_push(inst);

            curr = end->next;
        } else {
            printf("Error: Must start with instruction (l: %d, p: %d)\n", curr->lineno, curr->pos);
            break;
        }
    }

    return insts;
}