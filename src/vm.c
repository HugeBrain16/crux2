#include <vm.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static vm_t vm;
static insts_t *g_insts = NULL;

static int reg_count() {
    return sizeof(vm.r) / sizeof(vm.r[0]);
}

static int reg_valid(int r, lit_t *arg) {
    if (r < 0 || r >= reg_count()) {
        printf("Error: Invalid register (l: %d, p: %d)\n", arg->lineno, arg->pos);
        return 0;
    }

    return 1;
}

static int get_val(lit_t *arg, int *val) {
    if (arg->type == L_REF) {
        if (!reg_valid(arg->ref, arg))
            return 0;

        *val = vm.r[arg->ref].val;
    } else {
        *val = arg->num;
    }

    return 1;
}

static int get_reg(lit_t *arg, int *val) {
    if (!get_val(arg, val))
        return 0;

    return reg_valid(*val, arg);
}

static int off_valid(int off, lit_t *arg, size_t size) {
    if (off < 0 || off + size > (size_t)(vm.mem.end - vm.mem.field)) {
        printf("Error: Offset is out of memory range (l: %d, p: %d)\n", arg->lineno, arg->pos);
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

    vm.mem = mem;

    return (uint8_t*)block + sizeof(block_t);
}

static void vm_init() {
    vm = (vm_t){0};

    vm.mem.start = vm.mem.field;
    vm.mem.current = vm.mem.field;
    vm.mem.end = vm.mem.start + VM_MEM;
}

static int find_label(uint32_t id) {
    if (!g_insts)
        return -1;
    
    for (size_t i = 0; i < g_insts->count; i++) {
        inst_t *inst = g_insts->array[i];

        if (inst->type == IN_LABEL && inst->arg->id == id)
            return i;
    }

    return -1;
}

static int eval_select(inst_t *inst, size_t *i) {
    int r;
    if (!get_reg(inst->arg, &r))
        return 1;

    vm.a = r;
    return 0;
}

static int eval_load_i(inst_t *inst, size_t *i) {
    int v;
    if (!get_val(inst->arg, &v))
        return 1;

    vm.r[vm.a].val = v;
    return 0;
}

static int eval_load(inst_t *inst, size_t *i) {
    int r;
    if (!get_reg(inst->arg, &r))
        return 1;

    vm.r[vm.a].val = vm.r[r].val;
    return 0;
}

static int eval_dump(inst_t *inst, size_t *i) {
    printf("A: %d\n", vm.a);
    printf("F: %d\n", vm.f);
    for (int i = 0; i < reg_count(); i++)
        printf("R%d: %d\n", i, vm.r[i].val);
    return 0;
}

static int eval_memdump(inst_t *inst, size_t *i) {
    int col = 0;
    printf("MEM 0x%08x - 0x%08x\n", (unsigned)(vm.mem.start - vm.mem.field), (unsigned)(vm.mem.end - vm.mem.field));
    for (size_t i = 0; i < sizeof(vm.mem.field); i++) {
        printf("%02x ", vm.mem.field[i]);
        col++;

        if (col == 8) {
            printf(" ");
        } else if (col == 16) {
            printf("\n");
            col = 0;
        }
    }
    return 0;
}

static int eval_printchar(inst_t *inst, size_t *i) {
    printf("%c", (char)vm.r[vm.a].val);
    return 0;
}

static int eval_printnum(inst_t *inst, size_t *i) {
    printf("%d", vm.r[vm.a].val);
    return 0;
}

static int eval_jump(inst_t *inst, size_t *i) {
    int j = find_label(inst->arg->id);
    if (j == -1) {
        printf("Error: Undefined label (l: %d, p: %d)\n", inst->arg->lineno, inst->arg->pos);
        return 1;
    }

    *i = j;
    return 0;
}

static int eval_jump_eq(inst_t *inst, size_t *i) {
    int j = find_label(inst->arg->id);
    if (j == -1) {
        printf("Error: Undefined label (l: %d, p: %d)\n", inst->arg->lineno, inst->arg->pos);
        return 1;
    }

    lit_t *reg = inst->arg->next;
    int r;
    if (!get_reg(reg, &r))
        return 1;

    if (vm.r[vm.a].val == vm.r[r].val)
        *i = j;
    return 0;
}

static int eval_jump_ne(inst_t *inst, size_t *i) {
    int j = find_label(inst->arg->id);
    if (j == -1) {
        printf("Error: Undefined label (l: %d, p: %d)\n", inst->arg->lineno, inst->arg->pos);
        return 1;
    }

    lit_t *reg = inst->arg->next;
    int r;
    if (!get_reg(reg, &r))
        return 1;

    if (vm.r[vm.a].val != vm.r[r].val)
        *i = j;
    return 0;
}

static int eval_jump_lt(inst_t *inst, size_t *i) {
    int j = find_label(inst->arg->id);
    if (j == -1) {
        printf("Error: Undefined label (l: %d, p: %d)\n", inst->arg->lineno, inst->arg->pos);
        return 1;
    }

    lit_t *reg = inst->arg->next;
    int r;
    if (!get_reg(reg, &r))
        return 1;

    if (vm.r[vm.a].val < vm.r[r].val)
        *i = j;
    return 0;
}

static int eval_jump_gt(inst_t *inst, size_t *i) {
    int j = find_label(inst->arg->id);
    if (j == -1) {
        printf("Error: Undefined label (l: %d, p: %d)\n", inst->arg->lineno, inst->arg->pos);
        return 1;
    }

    lit_t *reg = inst->arg->next;
    int r;
    if (!get_reg(reg, &r))
        return 1;

    if (vm.r[vm.a].val > vm.r[r].val)
        *i = j;
    return 0;
}

static int eval_jump_le(inst_t *inst, size_t *i) {
    int j = find_label(inst->arg->id);
    if (j == -1) {
        printf("Error: Undefined label (l: %d, p: %d)\n", inst->arg->lineno, inst->arg->pos);
        return 1;
    }

    lit_t *reg = inst->arg->next;
    int r;
    if (!get_reg(reg, &r))
        return 1;

    if (vm.r[vm.a].val <= vm.r[r].val)
        *i = j;
    return 0;
}

static int eval_jump_ge(inst_t *inst, size_t *i) {
    int j = find_label(inst->arg->id);
    if (j == -1) {
        printf("Error: Undefined label (l: %d, p: %d)\n", inst->arg->lineno, inst->arg->pos);
        return 1;
    }

    lit_t *reg = inst->arg->next;
    int r;
    if (!get_reg(reg, &r))
        return 1;

    if (vm.r[vm.a].val >= vm.r[r].val)
        *i = j;
    return 0;
}

static int eval_label(inst_t *inst, size_t *i) {
    uint32_t id = inst->arg->id;
    int count = 0;

    for (size_t i = 0; i < g_insts->count; i++) {
        inst = g_insts->array[i];

        if (inst->type == IN_LABEL && inst->arg->type == L_ID && inst->arg->id == id)
            count++;

        if (count == 2) {
            printf("Error: Duplicate label (l: %d, p: %d)\n", inst->arg->lineno, inst->arg->pos);
            return 1;
        }
    }

    return 0;
}

static int eval_add(inst_t *inst, size_t *i) {
    int r;
    if (!get_reg(inst->arg, &r))
        return 1;

    vm.r[vm.a].val += vm.r[r].val;
    return 0;
}

static int eval_sub(inst_t *inst, size_t *i) {
    int r;
    if (!get_reg(inst->arg, &r))
        return 1;

    vm.r[vm.a].val -= vm.r[r].val;
    return 0;
}

static int eval_mul(inst_t *inst, size_t *i) {
    int r;
    if (!get_reg(inst->arg, &r))
        return 1;

    vm.r[vm.a].val *= vm.r[r].val;
    return 0;
}

static int eval_div(inst_t *inst, size_t *i) {
    int r;
    if (!get_reg(inst->arg, &r))
        return 1;

    if (vm.r[r].val == 0) {
        printf("Error: Division by zero (l: %d, p: %d)\n", inst->arg->lineno, inst->arg->pos);
        return 1;
    }

    vm.r[vm.a].val /= vm.r[r].val;
    return 0;
}

static int eval_mod(inst_t *inst, size_t *i) {
    int r;
    if (!get_reg(inst->arg, &r))
        return 1;

    if (vm.r[r].val == 0) {
        printf("Error: Division by zero (l: %d, p: %d)\n", inst->arg->lineno, inst->arg->pos);
        return 1;
    }

    vm.r[vm.a].val %= vm.r[r].val;
    return 0;
}

static int eval_and(inst_t *inst, size_t *i) {
    int r;
    if (!get_reg(inst->arg, &r))
        return 1;

    vm.r[vm.a].val &= vm.r[r].val;
    return 0;
}

static int eval_or(inst_t *inst, size_t *i) {
    int r;
    if (!get_reg(inst->arg, &r))
        return 1;

    vm.r[vm.a].val |= vm.r[r].val;
    return 0;
}

static int eval_xor(inst_t *inst, size_t *i) {
    int r;
    if (!get_reg(inst->arg, &r))
        return 1;

    vm.r[vm.a].val ^= vm.r[r].val;
    return 0;
}

static int eval_not(inst_t *inst, size_t *i) {
    int r;
    if (!get_reg(inst->arg, &r))
        return 1;

    vm.r[vm.a].val = ~vm.r[r].val;
    return 0;
}

static int eval_memw8(inst_t *inst, size_t *i) {
    int off;
    if (!get_val(inst->arg, &off))
        return 1;

    if (!off_valid(off, inst->arg, sizeof(uint8_t)))
        return 1;

    lit_t *reg = inst->arg->next;

    int r;
    if (!get_reg(reg, &r))
        return 1;

    vm.mem.field[off] = (uint8_t)vm.r[r].val;
    return 0;
}

static int eval_memr8(inst_t *inst, size_t *i) {
    int off;
    if (!get_val(inst->arg, &off))
        return 1;

    if (!off_valid(off, inst->arg, sizeof(uint8_t)))
        return 1;

    vm.r[vm.a].val = vm.mem.field[off];
    return 0;
}

static int eval_memw16(inst_t *inst, size_t *i) {
    int off;
    if (!get_val(inst->arg, &off))
        return 1;

    if (!off_valid(off, inst->arg, sizeof(uint16_t)))
        return 1;

    lit_t *reg = inst->arg->next;

    int r;
    if (!get_reg(reg, &r))
        return 1;

    uint16_t v = (uint16_t)vm.r[r].val;
    memcpy(vm.mem.field + off, &v, sizeof(v));
    return 0;
}

static int eval_memr16(inst_t *inst, size_t *i) {
    int off;
    if (!get_val(inst->arg, &off))
        return 1;

    if (!off_valid(off, inst->arg, sizeof(uint16_t)))
        return 1;

    uint16_t v;
    memcpy(&v, vm.mem.field + off, sizeof(v));
    vm.r[vm.a].val = v;
    return 0;
}

static int eval_memw32(inst_t *inst, size_t *i) {
    int off;
    if (!get_val(inst->arg, &off))
        return 1;

    if (!off_valid(off, inst->arg, sizeof(uint32_t)))
        return 1;

    lit_t *reg = inst->arg->next;

    int r;
    if (!get_reg(reg, &r))
        return 1;

    uint32_t v = (uint32_t)vm.r[r].val;
    memcpy(vm.mem.field + off, &v, sizeof(v));
    return 0;
}

static int eval_memr32(inst_t *inst, size_t *i) {
    int off;
    if (!get_val(inst->arg, &off))
        return 1;

    if (!off_valid(off, inst->arg, sizeof(uint32_t)))
        return 1;

    uint32_t v;
    memcpy(&v, vm.mem.field + off, sizeof(v));
    vm.r[vm.a].val = v;
    return 0;
}

static int eval_memws(inst_t *inst, size_t *i) {
    int off;
    if (!get_val(inst->arg, &off))
        return 1;

    lit_t *str = inst->arg->next;

    if (!off_valid(off, inst->arg, strlen(str->str)))
        return 1;

    for (size_t i = 0; i < strlen(str->str); i++)
        vm.mem.field[off + i] = str->str[i];
    return 0;
}

static int eval_inc(inst_t *inst, size_t *i) {
    vm.r[vm.a].val += 1;
    return 0;
}

static int eval_dec(inst_t *inst, size_t *i) {
    vm.r[vm.a].val -= 1;
    return 0;
}

int exec(insts_t *insts) {
    g_insts = insts;

    vm_init();

    int err = 0;
    for (size_t i = 0; i < insts->count; i++) {
        inst_t *inst = insts->array[i];

        switch (inst->type) {
            case IN_LOAD: err = eval_load(inst, &i); break;
            case IN_LOAD_I: err = eval_load_i(inst, &i); break;
            case IN_SELECT: err = eval_select(inst, &i); break;
            case IN_ADD: err = eval_add(inst, &i); break;
            case IN_SUB: err = eval_sub(inst, &i); break;
            case IN_MUL: err = eval_mul(inst, &i); break;
            case IN_DIV: err = eval_div(inst, &i); break;
            case IN_MOD: err = eval_mod(inst, &i); break;
            case IN_AND: err = eval_and(inst, &i); break;
            case IN_OR: err = eval_or(inst, &i); break;
            case IN_XOR: err = eval_xor(inst, &i); break;
            case IN_NOT: err = eval_not(inst, &i); break;
            case IN_JUMP: err = eval_jump(inst, &i); break;
            case IN_JUMP_EQ: err = eval_jump_eq(inst, &i); break;
            case IN_JUMP_NE: err = eval_jump_ne(inst, &i); break;
            case IN_JUMP_LT: err = eval_jump_lt(inst, &i); break;
            case IN_JUMP_GT: err = eval_jump_gt(inst, &i); break;
            case IN_JUMP_LE: err = eval_jump_le(inst, &i); break;
            case IN_JUMP_GE: err = eval_jump_ge(inst, &i); break;
            case IN_DUMP: err = eval_dump(inst, &i); break;
            case IN_MEMDUMP: err = eval_memdump(inst, &i); break;
            case IN_PRINTCHAR: err = eval_printchar(inst, &i); break;
            case IN_PRINTNUM: err = eval_printnum(inst, &i); break;
            case IN_LABEL: err = eval_label(inst, &i); break;
            case IN_MEMW8: err = eval_memw8(inst, &i); break;
            case IN_MEMR8: err = eval_memr8(inst, &i); break;
            case IN_MEMW16: err = eval_memw16(inst, &i); break;
            case IN_MEMR16: err = eval_memr16(inst, &i); break;
            case IN_MEMW32: err = eval_memw32(inst, &i); break;
            case IN_MEMR32: err = eval_memr32(inst, &i); break;
            case IN_MEMWS: err = eval_memws(inst, &i); break;
            case IN_INC: err = eval_inc(inst, &i); break;
            case IN_DEC: err = eval_dec(inst, &i); break;
            default:
                err = 1;
                printf("Error: Unhandled instruction (l: %d, p: %d)\n", inst->lineno, inst->pos);
                break;
        }

        if (err)
            break;
    }

    return err;
}