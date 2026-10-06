#include <vm.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static vm_t vm;
static insts_t *g_insts = NULL;

static int reg_count() {
    return sizeof(vm.r) / sizeof(vm.r[0]);
}

static int reg_valid(int r, token_t *arg) {
    if (r < 0 || r >= reg_count()) {
        printf("Error: Invalid register (l: %d, p: %d)\n", arg->lineno, arg->pos);
        return 0;
    }

    return 1;
}

static size_t block_size(size_t size) {
    return size + sizeof(block_t);
}

static uint8_t *vm_alloc(size_t size) {
    if (size == 0) return NULL;
    size = (size + 15) & ~15;

    mem_t mem = vm.mem;

    if (mem.current + block_size(size) > mem.end)
        return NULL;

    block_t *block = (block_t*)mem.current;
    block->size = size;
    block->flags = 0;
    block->next = NULL;
    mem.current += block_size(size);

    return (uint8_t*)block + sizeof(block_t);
}

static void vm_init() {
    vm = (vm_t){0};

    vm.mem.size = VM_MEM;
    vm.mem.field = malloc(vm.mem.size);
    vm.mem.start = (uint8_t*)vm.mem.field;
    vm.mem.current = vm.mem.start;
    vm.mem.end = (uint8_t*)(vm.mem.start + vm.mem.size);
}

static int dupe_label(const char *label) {
    if (!g_insts)
        return 0;

    int count = 0;
    for (size_t i = 0; i < g_insts->count; i++) {
        inst_t *inst = g_insts->array[i];

        if (!strcmp(inst->inst->value, "label") && !strcmp(inst->arg->value, label))
            count++;

        if (count > 1)
            return 1;
    }

    return 0;
}

static int find_label(const char *label) {
    if (!g_insts)
        return -1;
    
    for (size_t i = 0; i < g_insts->count; i++) {
        inst_t *inst = g_insts->array[i];

        if (!strcmp(inst->inst->value, "label") && !strcmp(inst->arg->value, label))
            return i;
    }

    return -1;
}

static int eval_select(inst_t *inst, size_t *i) {
    int r = atoi(inst->arg->value);
    if (!reg_valid(r, inst->arg))
        return 1;

    vm.a = r;
    return 0;
}

static int eval_load_i(inst_t *inst, size_t *i) {
    vm.r[vm.a].val = atoi(inst->arg->value);
    return 0;
}

static int eval_load(inst_t *inst, size_t *i) {
    int r = atoi(inst->arg->value);
    if (!reg_valid(r, inst->arg))
        return 1;

    vm.r[vm.a].val = vm.r[r].val;
    return 0;
}

static int eval_dump(inst_t *inst, size_t *i) {
    printf("A: %d\n", vm.a);
    for (int i = 0; i < reg_count(); i++)
        printf("R%d: %d\n", i, vm.r[i].val);
    return 0;
}

static int eval_jump(inst_t *inst, size_t *i) {
    const char *label = inst->arg->value;

    int j = find_label(label);
    if (j == -1) {
        printf("Error: Undefined label (l: %d, p: %d)\n", inst->arg->lineno, inst->arg->pos);
        return 1;
    }

    *i = j;
    return 0;
}

static int eval_jump_eq(inst_t *inst, size_t *i) {
    const char *label = inst->arg->value;

    int j = find_label(label);
    if (j == -1) {
        printf("Error: Undefined label (l: %d, p: %d)\n", inst->arg->lineno, inst->arg->pos);
        return 1;
    }

    token_t *reg = inst->arg->next;
    int r = atoi(reg->value);
    if (!reg_valid(r, reg))
        return 1;

    if (vm.r[vm.a].val == vm.r[r].val)
        *i = j;
    return 0;
}

static int eval_jump_ne(inst_t *inst, size_t *i) {
    const char *label = inst->arg->value;

    int j = find_label(label);
    if (j == -1) {
        printf("Error: Undefined label (l: %d, p: %d)\n", inst->arg->lineno, inst->arg->pos);
        return 1;
    }

    token_t *reg = inst->arg->next;
    int r = atoi(reg->value);
    if (!reg_valid(r, reg))
        return 1;

    if (vm.r[vm.a].val != vm.r[r].val)
        *i = j;
    return 0;
}

static int eval_jump_lt(inst_t *inst, size_t *i) {
    const char *label = inst->arg->value;

    int j = find_label(label);
    if (j == -1) {
        printf("Error: Undefined label (l: %d, p: %d)\n", inst->arg->lineno, inst->arg->pos);
        return 1;
    }

    token_t *reg = inst->arg->next;
    int r = atoi(reg->value);
    if (!reg_valid(r, reg))
        return 1;

    if (vm.r[vm.a].val < vm.r[r].val)
        *i = j;
    return 0;
}

static int eval_jump_gt(inst_t *inst, size_t *i) {
    const char *label = inst->arg->value;

    int j = find_label(label);
    if (j == -1) {
        printf("Error: Undefined label (l: %d, p: %d)\n", inst->arg->lineno, inst->arg->pos);
        return 1;
    }

    token_t *reg = inst->arg->next;
    int r = atoi(reg->value);
    if (!reg_valid(r, reg))
        return 1;

    if (vm.r[vm.a].val > vm.r[r].val)
        *i = j;
    return 0;
}

static int eval_jump_le(inst_t *inst, size_t *i) {
    const char *label = inst->arg->value;

    int j = find_label(label);
    if (j == -1) {
        printf("Error: Undefined label (l: %d, p: %d)\n", inst->arg->lineno, inst->arg->pos);
        return 1;
    }

    token_t *reg = inst->arg->next;
    int r = atoi(reg->value);
    if (!reg_valid(r, reg))
        return 1;

    if (vm.r[vm.a].val <= vm.r[r].val)
        *i = j;
    return 0;
}

static int eval_jump_ge(inst_t *inst, size_t *i) {
    const char *label = inst->arg->value;

    int j = find_label(label);
    if (j == -1) {
        printf("Error: Undefined label (l: %d, p: %d)\n", inst->arg->lineno, inst->arg->pos);
        return 1;
    }

    token_t *reg = inst->arg->next;
    int r = atoi(reg->value);
    if (!reg_valid(r, reg))
        return 1;

    if (vm.r[vm.a].val >= vm.r[r].val)
        *i = j;
    return 0;
}

static int eval_label(inst_t *inst, size_t *i) {
    const char *label = inst->arg->value;

    if (dupe_label(label)) {
        printf("Error: Duplicate label (l: %d, p: %d)\n", inst->arg->lineno, inst->arg->pos);
        return 1;
    }

    return 0;
}

static int eval_add(inst_t *inst, size_t *i) {
    int r = atoi(inst->arg->value);
    if (!reg_valid(r, inst->arg))
        return 1;

    vm.r[vm.a].val += vm.r[r].val;
    return 0;
}

static int eval_sub(inst_t *inst, size_t *i) {
    int r = atoi(inst->arg->value);
    if (!reg_valid(r, inst->arg))
        return 1;

    vm.r[vm.a].val -= vm.r[r].val;
    return 0;
}

static int eval_mul(inst_t *inst, size_t *i) {
    int r = atoi(inst->arg->value);
    if (!reg_valid(r, inst->arg))
        return 1;

    vm.r[vm.a].val *= vm.r[r].val;
    return 0;
}

static int eval_div(inst_t *inst, size_t *i) {
    int r = atoi(inst->arg->value);
    if (!reg_valid(r, inst->arg))
        return 1;

    vm.r[vm.a].val /= vm.r[r].val;
    return 0;
}

void exec(insts_t *insts) {
    g_insts = insts;

    vm_init();

    for (size_t i = 0; i < insts->count; i++) {
        int err = 0;
        inst_t *inst = insts->array[i];

        switch (inst->type) {
            case IN_LOAD:
                err = eval_load(inst, &i);
                break;
            case IN_LOAD_I:
                err = eval_load_i(inst, &i);
                break;
            case IN_SELECT:
                err = eval_select(inst, &i);
                break;
            case IN_ADD:
                err = eval_add(inst, &i);
                break;
            case IN_SUB:
                err = eval_sub(inst, &i);
                break;
            case IN_MUL:
                err = eval_mul(inst, &i);
                break;
            case IN_DIV:
                err = eval_div(inst, &i);
                break;
            case IN_JUMP:
                err = eval_jump(inst, &i);
                break;
            case IN_JUMP_EQ:
                err = eval_jump_eq(inst, &i);
                break;
            case IN_JUMP_NE:
                err = eval_jump_ne(inst, &i);
                break;
            case IN_JUMP_LT:
                err = eval_jump_lt(inst, &i);
                break;
            case IN_JUMP_GT:
                err = eval_jump_gt(inst, &i);
                break;
            case IN_JUMP_LE:
                err = eval_jump_le(inst, &i);
                break;
            case IN_JUMP_GE:
                err = eval_jump_ge(inst, &i);
                break;
            case IN_DUMP:
                err = eval_dump(inst, &i);
                break;
            case IN_LABEL:
                err = eval_label(inst, &i);
                break;
        }

        if (err)
            break;
    }
}