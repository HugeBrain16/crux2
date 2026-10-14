#include <vm.h>
#include <defs.h>

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static vm_t vm;
static insts_t *g_insts = NULL;
static int *labels = NULL;

static int reg_count() {
    return sizeof(vm.r) / sizeof(vm.r[0]);
}

static int reg_valid(int r, lit_t *arg) {
    if (r < 0 || r >= reg_count()) {
        fprintf(stderr, "Error: Invalid register = %d (l: %d, p: %d)\n", r, arg->lineno, arg->pos);
        return 0;
    }

    return 1;
}

static int get_val(lit_t *arg, int *val) {
    if (arg->type == L_REF) {
        if (!reg_valid(arg->int_type, arg))
            return 0;

        *val = vm.r[arg->int_type];
    } else if (arg->type == L_INT) {

        // resolve internal reference
        char *id = arg->char_type;
        if      (!strcmp(id, "mem_size")) *val = VM_MEM;
        else if (!strcmp(id, "reg_count")) *val = reg_count();
        else if (!strcmp(id, "cx_ver")) *val = CRUX_VERSION;

        else {
            fprintf(stderr, "Error: Unknown internal name = %s (l: %d, p: %d)\n", id, arg->lineno, arg->pos);
            return 0;
        }
    } else {
        *val = arg->int_type;
    }

    return 1;
}

static int get_reg(lit_t *arg, int *val) {
    if (!get_val(arg, val))
        return 0;

    return reg_valid(*val, arg);
}

static int off_valid(int off, lit_t *arg, size_t size) {
    if (off < 0 || off + size > VM_MEM) {
        fprintf(stderr, "Error: Offset %d (0x%08x) is out of memory range (l: %d, p: %d)\n", off, off, arg->lineno, arg->pos);
        return 0;
    }

    return 1;
}

static int eval_select(inst_t *inst, size_t *i) {
    UNUSED(i);

    int r;
    if (!get_reg(inst->arg, &r))
        return 1;

    vm.a = r;
    return 0;
}

static int eval_load_i(inst_t *inst, size_t *i) {
    UNUSED(i);

    int v;
    if (!get_val(inst->arg, &v))
        return 1;

    vm.r[vm.a] = v;
    return 0;
}

static int eval_load(inst_t *inst, size_t *i) {
    UNUSED(i);

    int r;
    if (!get_reg(inst->arg, &r))
        return 1;

    vm.r[vm.a] = vm.r[r];
    return 0;
}

static int eval_dump(inst_t *inst, size_t *i) {
    UNUSED(inst); UNUSED(i);

    printf("A: %d\n", vm.a);
    printf("F: %d\n", vm.f);
    for (int i = 0; i < reg_count(); i++)
        printf("R%d: %d\n", i, vm.r[i]);
    return 0;
}

static int eval_memdump(inst_t *inst, size_t *i) {
    UNUSED(i);

    /*
    int col = 0;
    printf("MEM 0x%08x - 0x%08x\n", (unsigned)vm.mem, (unsigned)VM_MEM);
    for (size_t i = 0; i < VM_MEM; i++) {
        printf("%02x ", vm.mem[i]);
        col++;

        if (col == 8) {
            printf(" ");
        } else if (col == 16) {
            printf("\n");
            col = 0;
        }
    }
    */

    FILE *f = fopen("memory.bin", "wb");
    if (!f) {
        fprintf(stderr, "Error: Failed to dump memory (l: %d, p: %d)\n", inst->lineno, inst->pos);
        return 1;
    }
    fwrite(vm.mem, 1, sizeof(vm.mem), f);
    return 0;
}

static int eval_printchar(inst_t *inst, size_t *i) {
    UNUSED(inst); UNUSED(i);

    printf("%c", (char)vm.r[vm.a]);
    return 0;
}

static int eval_printnum(inst_t *inst, size_t *i) {
    UNUSED(inst); UNUSED(i);

    printf("%d", vm.r[vm.a]);
    return 0;
}

static int eval_jump(inst_t *inst, size_t *i) {
    int j = labels[inst->arg->uid];
    if (j == -1) {
        fprintf(stderr, "Error: Undefined label = %s (l: %d, p: %d)\n", inst->arg->char_type, inst->arg->lineno, inst->arg->pos);
        return 1;
    }

    *i = j;
    return 0;
}

static int eval_jump_eq(inst_t *inst, size_t *i) {
    int j = labels[inst->arg->uid];
    if (j == -1) {
        fprintf(stderr, "Error: Undefined label = %s (l: %d, p: %d)\n", inst->arg->char_type, inst->arg->lineno, inst->arg->pos);
        return 1;
    }

    lit_t *reg = inst->arg->next;
    int r;
    if (!get_reg(reg, &r))
        return 1;

    if (vm.r[vm.a] == vm.r[r])
        *i = j;
    return 0;
}

static int eval_jump_ne(inst_t *inst, size_t *i) {
    int j = labels[inst->arg->uid];
    if (j == -1) {
        fprintf(stderr, "Error: Undefined label = %s (l: %d, p: %d)\n", inst->arg->char_type, inst->arg->lineno, inst->arg->pos);
        return 1;
    }

    lit_t *reg = inst->arg->next;
    int r;
    if (!get_reg(reg, &r))
        return 1;

    if (vm.r[vm.a] != vm.r[r])
        *i = j;
    return 0;
}

static int eval_jump_lt(inst_t *inst, size_t *i) {
    int j = labels[inst->arg->uid];
    if (j == -1) {
        fprintf(stderr, "Error: Undefined label = %s (l: %d, p: %d)\n", inst->arg->char_type, inst->arg->lineno, inst->arg->pos);
        return 1;
    }

    lit_t *reg = inst->arg->next;
    int r;
    if (!get_reg(reg, &r))
        return 1;

    if (vm.r[vm.a] < vm.r[r])
        *i = j;
    return 0;
}

static int eval_jump_gt(inst_t *inst, size_t *i) {
    int j = labels[inst->arg->uid];
    if (j == -1) {
        fprintf(stderr, "Error: Undefined label = %s (l: %d, p: %d)\n", inst->arg->char_type, inst->arg->lineno, inst->arg->pos);
        return 1;
    }

    lit_t *reg = inst->arg->next;
    int r;
    if (!get_reg(reg, &r))
        return 1;

    if (vm.r[vm.a] > vm.r[r])
        *i = j;
    return 0;
}

static int eval_jump_le(inst_t *inst, size_t *i) {
    int j = labels[inst->arg->uid];
    if (j == -1) {
        fprintf(stderr, "Error: Undefined label = %s (l: %d, p: %d)\n", inst->arg->char_type, inst->arg->lineno, inst->arg->pos);
        return 1;
    }

    lit_t *reg = inst->arg->next;
    int r;
    if (!get_reg(reg, &r))
        return 1;

    if (vm.r[vm.a] <= vm.r[r])
        *i = j;
    return 0;
}

static int eval_jump_ge(inst_t *inst, size_t *i) {
    int j = labels[inst->arg->uid];
    if (j == -1) {
        fprintf(stderr, "Error: Undefined label = %s (l: %d, p: %d)\n", inst->arg->char_type, inst->arg->lineno, inst->arg->pos);
        return 1;
    }

    lit_t *reg = inst->arg->next;
    int r;
    if (!get_reg(reg, &r))
        return 1;

    if (vm.r[vm.a] >= vm.r[r])
        *i = j;
    return 0;
}

static int eval_label(inst_t *inst, size_t *i) {
    UNUSED(inst); UNUSED(i);
    return 0;
}

static int eval_add(inst_t *inst, size_t *i) {
    UNUSED(i);

    int r;
    if (!get_reg(inst->arg, &r))
        return 1;

    vm.r[vm.a] += vm.r[r];
    return 0;
}

static int eval_sub(inst_t *inst, size_t *i) {
    UNUSED(i);

    int r;
    if (!get_reg(inst->arg, &r))
        return 1;

    vm.r[vm.a] -= vm.r[r];
    return 0;
}

static int eval_mul(inst_t *inst, size_t *i) {
    UNUSED(i);

    int r;
    if (!get_reg(inst->arg, &r))
        return 1;

    vm.r[vm.a] *= vm.r[r];
    return 0;
}

static int eval_div(inst_t *inst, size_t *i) {
    UNUSED(i);

    int r;
    if (!get_reg(inst->arg, &r))
        return 1;

    if (vm.r[r] == 0) {
        fprintf(stderr, "Error: Division by zero (l: %d, p: %d)\n", inst->arg->lineno, inst->arg->pos);
        return 1;
    }

    vm.r[vm.a] /= vm.r[r];
    return 0;
}

static int eval_mod(inst_t *inst, size_t *i) {
    UNUSED(i);

    int r;
    if (!get_reg(inst->arg, &r))
        return 1;

    if (vm.r[r] == 0) {
        fprintf(stderr, "Error: Division by zero (l: %d, p: %d)\n", inst->arg->lineno, inst->arg->pos);
        return 1;
    }

    vm.r[vm.a] %= vm.r[r];
    return 0;
}

static int eval_and(inst_t *inst, size_t *i) {
    UNUSED(i);

    int r;
    if (!get_reg(inst->arg, &r))
        return 1;

    vm.r[vm.a] &= vm.r[r];
    return 0;
}

static int eval_or(inst_t *inst, size_t *i) {
    UNUSED(i);

    int r;
    if (!get_reg(inst->arg, &r))
        return 1;

    vm.r[vm.a] |= vm.r[r];
    return 0;
}

static int eval_xor(inst_t *inst, size_t *i) {
    UNUSED(i);

    int r;
    if (!get_reg(inst->arg, &r))
        return 1;

    vm.r[vm.a] ^= vm.r[r];
    return 0;
}

static int eval_not(inst_t *inst, size_t *i) {
    UNUSED(i);

    int r;
    if (!get_reg(inst->arg, &r))
        return 1;

    vm.r[vm.a] = ~vm.r[r];
    return 0;
}

static int eval_memw8(inst_t *inst, size_t *i) {
    UNUSED(i);

    int off;
    if (!get_val(inst->arg, &off))
        return 1;

    if (!off_valid(off, inst->arg, sizeof(uint8_t)))
        return 1;

    lit_t *reg = inst->arg->next;

    int r;
    if (!get_reg(reg, &r))
        return 1;

    vm.mem[off] = (uint8_t)vm.r[r];
    return 0;
}

static int eval_memr8(inst_t *inst, size_t *i) {
    UNUSED(i);

    int off;
    if (!get_val(inst->arg, &off))
        return 1;

    if (!off_valid(off, inst->arg, sizeof(uint8_t)))
        return 1;

    vm.r[vm.a] = vm.mem[off];
    return 0;
}

static int eval_memw16(inst_t *inst, size_t *i) {
    UNUSED(i);

    int off;
    if (!get_val(inst->arg, &off))
        return 1;

    if (!off_valid(off, inst->arg, sizeof(uint16_t)))
        return 1;

    lit_t *reg = inst->arg->next;

    int r;
    if (!get_reg(reg, &r))
        return 1;

    uint16_t v = (uint16_t)vm.r[r];
    memcpy(vm.mem + off, &v, sizeof(v));
    return 0;
}

static int eval_memr16(inst_t *inst, size_t *i) {
    UNUSED(i);

    int off;
    if (!get_val(inst->arg, &off))
        return 1;

    if (!off_valid(off, inst->arg, sizeof(uint16_t)))
        return 1;

    uint16_t v;
    memcpy(&v, vm.mem + off, sizeof(v));
    vm.r[vm.a] = v;
    return 0;
}

static int eval_memw32(inst_t *inst, size_t *i) {
    UNUSED(i);

    int off;
    if (!get_val(inst->arg, &off))
        return 1;

    if (!off_valid(off, inst->arg, sizeof(uint32_t)))
        return 1;

    lit_t *reg = inst->arg->next;

    int r;
    if (!get_reg(reg, &r))
        return 1;

    uint32_t v = (uint32_t)vm.r[r];
    memcpy(vm.mem + off, &v, sizeof(v));
    return 0;
}

static int eval_memr32(inst_t *inst, size_t *i) {
    UNUSED(i);

    int off;
    if (!get_val(inst->arg, &off))
        return 1;

    if (!off_valid(off, inst->arg, sizeof(uint32_t)))
        return 1;

    uint32_t v;
    memcpy(&v, vm.mem + off, sizeof(v));
    vm.r[vm.a] = v;
    return 0;
}

static int eval_memws(inst_t *inst, size_t *i) {
    UNUSED(i);

    int off;
    if (!get_val(inst->arg, &off))
        return 1;

    lit_t *str = inst->arg->next;

    size_t size = strlen(str->char_type);
    if (!off_valid(off, inst->arg, size))
        return 1;

    for (size_t i = 0; i < size; i++)
        vm.mem[off + i] = str->char_type[i];
    return 0;
}

static int eval_inc(inst_t *inst, size_t *i) {
    UNUSED(inst); UNUSED(i);

    vm.r[vm.a] += 1;
    return 0;
}

static int eval_dec(inst_t *inst, size_t *i) {
    UNUSED(inst); UNUSED(i);

    vm.r[vm.a] -= 1;
    return 0;
}

static int eval_time(inst_t *inst, size_t *i) {
    UNUSED(inst); UNUSED(i);

    vm.r[vm.a] = clock() * 1000 / CLOCKS_PER_SEC;
    return 0;
}

static int eval_shift_l(inst_t *inst, size_t *i) {
    UNUSED(i);

    int src;
    if (!get_reg(inst->arg, &src))
        return 1;

    lit_t *next = inst->arg->next;
    int dst;
    if (!get_reg(next, &dst))
        return 1;

    vm.r[dst] = vm.r[vm.a] << vm.r[src];
    return 0;
}

static int eval_call(inst_t *inst, size_t *i) {
    int sp = vm.r[vm.a];
    size_t ret = *i + 1;

    sp -= sizeof(size_t);
    memcpy(vm.mem + sp, &ret, sizeof(size_t));
    vm.r[vm.a] = sp;

    return eval_jump(inst, i);
}

static int eval_return(inst_t *inst, size_t *i) {
    UNUSED(inst);

    int sp = vm.r[vm.a];
    size_t ret;

    memcpy(&ret, vm.mem + sp, sizeof(size_t));
    sp += sizeof(size_t);
    vm.r[vm.a] = sp;

    *i = ret - 1;
    return 0;
}

static int eval_shift_r(inst_t *inst, size_t *i) {
    UNUSED(i);

    int src;
    if (!get_reg(inst->arg, &src))
        return 1;

    lit_t *next = inst->arg->next;
    int dst;
    if (!get_reg(next, &dst))
        return 1;

    vm.r[dst] = vm.r[vm.a] >> vm.r[src];
    return 0;
}

static int index_labels() {
    size_t max_id = 0;
    for (size_t i = 0; i < g_insts->count; i++) {
        inst_t *inst = g_insts->array[i];

        if (inst->type == IN_LABEL && inst->arg->uid > max_id)
            max_id = inst->arg->uid;
    }

    labels = malloc(sizeof(*labels) * (max_id + 1));
    for (size_t i = 0; i <= max_id; i++)
        labels[i] = -1;

    for (size_t i = 0; i < g_insts->count; i++) {
        inst_t *inst = g_insts->array[i];

        if (inst->type == IN_LABEL) {
            if (labels[inst->arg->uid] == -1)
                labels[inst->arg->uid] = i;
            else {
                fprintf(stderr, "Error: Duplicate label = %s (l: %d, p: %d)\n", inst->arg->char_type, inst->arg->lineno, inst->arg->pos);
                return 0;
            }
        }
    }

    return 1;
}

int exec(insts_t *insts) {
    g_insts = insts;

    vm = (vm_t){0};
    if(!index_labels())
        return 1;

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
            case IN_SHIFT_L: err = eval_shift_l(inst, &i); break;
            case IN_SHIFT_R: err = eval_shift_r(inst, &i); break;
            case IN_JUMP: err = eval_jump(inst, &i); break;
            case IN_JUMP_EQ: err = eval_jump_eq(inst, &i); break;
            case IN_JUMP_NE: err = eval_jump_ne(inst, &i); break;
            case IN_JUMP_LT: err = eval_jump_lt(inst, &i); break;
            case IN_JUMP_GT: err = eval_jump_gt(inst, &i); break;
            case IN_JUMP_LE: err = eval_jump_le(inst, &i); break;
            case IN_JUMP_GE: err = eval_jump_ge(inst, &i); break;
            case IN_CALL: err = eval_call(inst, &i); break;
            case IN_RETURN: err = eval_return(inst, &i); break;
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
            case IN_TIME: err = eval_time(inst, &i); break;
            default:
                err = 1;
                fprintf(stderr, "Error: Unhandled instruction = %d (l: %d, p: %d)\n", inst->type, inst->lineno, inst->pos);
                break;
        }

        if (err)
            break;
    }

    return err;
}
