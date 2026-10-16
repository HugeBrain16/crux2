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

static int reg_valid(int64_t r, lit_t *arg) {
    if (r < 0 || r >= reg_count()) {
        fprintf(stderr, "Error: Invalid register = %ld (l: %d, p: %d)\n", r, arg->lineno, arg->pos);
        return 0;
    }

    return 1;
}

static int get_val(lit_t *arg, int64_t *val) {
    if (arg->type == L_REF) {
        if (!reg_valid(arg->int_type, arg))
            return 0;

        *val = vm.r[arg->int_type];
    } else if (arg->type == L_INT) {
        // resolve internal reference
        switch (arg->uid) {
            case INT_MEM_SIZE: *val = VM_MEM; break;
            case INT_REG_COUNT: *val = reg_count(); break;
            case INT_CX_VER: *val = CRUX_VERSION; break;
            case INT_STDOUT: *val = (uintptr_t)stdout; break;
            case INT_STDIN: *val = (uintptr_t)stdin; break;
            case INT_STDERR: *val = (uintptr_t)stderr; break;
            default:
                fprintf(stderr, "Error: Unknown internal name = %s (l: %d, p: %d)\n", arg->repr + 1, arg->lineno, arg->pos);
                return 0;
        }
    } else {
        *val = arg->int_type;
    }

    return 1;
}

static int get_reg(lit_t *arg, int64_t *val) {
    if (!get_val(arg, val))
        return 0;

    return reg_valid(*val, arg);
}

static int off_valid(int64_t off, lit_t *arg, size_t size) {
    if (off < 0 || off + size > VM_MEM) {
        fprintf(stderr, "Error: Offset %ld (0x%08x) is out of memory range (l: %d, p: %d)\n", off, (unsigned)off, arg->lineno, arg->pos);
        return 0;
    }

    return 1;
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
                fprintf(stderr, "Error: Duplicate label = %s (l: %d, p: %d)\n", inst->arg->repr, inst->arg->lineno, inst->arg->pos);
                return 0;
            }
        }
    }

    return 1;
}

/* -- eval instructions start */

static int eval_select(inst_t *inst, size_t *i) {
    UNUSED(i);

    int64_t r;
    if (!get_reg(inst->arg, &r))
        return 1;

    vm.a = r;
    return 0;
}

static int eval_load_i(inst_t *inst, size_t *i) {
    UNUSED(i);

    int64_t v;
    if (!get_val(inst->arg, &v))
        return 1;

    vm.r[vm.a] = v;
    return 0;
}

static int eval_load(inst_t *inst, size_t *i) {
    UNUSED(i);

    int64_t r;
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
        printf("R%d: %ld\n", i, vm.r[i]);
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

static int eval_jump(inst_t *inst, size_t *i, inst_e type) {
    int j = labels[inst->arg->uid];
    if (j == -1) {
        fprintf(stderr, "Error: Undefined label = %s (l: %d, p: %d)\n", inst->arg->repr, inst->arg->lineno, inst->arg->pos);
        return 1;
    }

    if (type == IN_JUMP) {
        *i = j;
        return 0;
    }

    lit_t *reg = inst->arg->next;
    int64_t r;
    if (!get_reg(reg, &r))
        return 1;

    switch (type) {
        case IN_JUMP_EQ: if (vm.r[vm.a] == vm.r[r]) *i = j; break;
        case IN_JUMP_NE: if (vm.r[vm.a] != vm.r[r]) *i = j; break;
        case IN_JUMP_LT: if (vm.r[vm.a] < vm.r[r]) *i = j; break;
        case IN_JUMP_GT: if (vm.r[vm.a] > vm.r[r]) *i = j; break;
        case IN_JUMP_LE: if (vm.r[vm.a] <= vm.r[r]) *i = j; break;
        case IN_JUMP_GE: if (vm.r[vm.a] >= vm.r[r]) *i = j; break;
        default: {
            fprintf(stderr, "Error: Unhandled jump (l: %d, p: %d)\n", inst->arg->lineno, inst->arg->pos);
            return 1;
        }
    }
    return 0;
}

static int eval_label(inst_t *inst, size_t *i) {
    UNUSED(inst); UNUSED(i);
    return 0;
}

static int eval_binop(inst_t *inst, size_t *i, inst_e type) {
    UNUSED(i);

    int64_t r;
    if (!get_reg(inst->arg, &r))
        return 1;

    switch (type) {
        case IN_ADD: vm.r[vm.a] += vm.r[r]; break;
        case IN_SUB: vm.r[vm.a] -= vm.r[r]; break;
        case IN_MUL: vm.r[vm.a] *= vm.r[r]; break;
        case IN_DIV:
            if (vm.r[r] == 0) {
                fprintf(stderr, "Error: Division by zero (l: %d, p: %d)\n", inst->arg->lineno, inst->arg->pos);
                return 1;
            }
            vm.r[vm.a] /= vm.r[r];
            break;
        case IN_MOD:
            if (vm.r[r] == 0) {
                fprintf(stderr, "Error: Division by zero (l: %d, p: %d)\n", inst->arg->lineno, inst->arg->pos);
                return 1;
            }
            vm.r[vm.a] %= vm.r[r];
            break;

        case IN_AND: vm.r[vm.a] &= vm.r[r]; break;
        case IN_OR: vm.r[vm.a] |= vm.r[r]; break;
        case IN_XOR: vm.r[vm.a] ^= vm.r[r]; break;
        case IN_NOT: vm.r[vm.a] = ~vm.r[r]; break;
        default: {
            fprintf(stderr, "Error: Unhandled operation (l: %d, p: %d)\n", inst->arg->lineno, inst->arg->pos);
            return 1;
        }
    }
    return 0;
}

static int eval_memw8(inst_t *inst, size_t *i) {
    UNUSED(i);

    int64_t off;
    if (!get_val(inst->arg, &off))
        return 1;

    if (!off_valid(off, inst->arg, sizeof(uint8_t)))
        return 1;

    lit_t *reg = inst->arg->next;

    int64_t r;
    if (!get_reg(reg, &r))
        return 1;

    vm.mem[off] = (uint8_t)vm.r[r];
    return 0;
}

static int eval_memr8(inst_t *inst, size_t *i) {
    UNUSED(i);

    int64_t off;
    if (!get_val(inst->arg, &off))
        return 1;

    if (!off_valid(off, inst->arg, sizeof(uint8_t)))
        return 1;

    vm.r[vm.a] = vm.mem[off];
    return 0;
}

static int eval_memw16(inst_t *inst, size_t *i) {
    UNUSED(i);

    int64_t off;
    if (!get_val(inst->arg, &off))
        return 1;

    if (!off_valid(off, inst->arg, sizeof(uint16_t)))
        return 1;

    lit_t *reg = inst->arg->next;

    int64_t r;
    if (!get_reg(reg, &r))
        return 1;

    uint16_t v = (uint16_t)vm.r[r];
    memcpy(vm.mem + off, &v, sizeof(v));
    return 0;
}

static int eval_memr16(inst_t *inst, size_t *i) {
    UNUSED(i);

    int64_t off;
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

    int64_t off;
    if (!get_val(inst->arg, &off))
        return 1;

    if (!off_valid(off, inst->arg, sizeof(uint32_t)))
        return 1;

    lit_t *reg = inst->arg->next;

    int64_t r;
    if (!get_reg(reg, &r))
        return 1;

    uint32_t v = (uint32_t)vm.r[r];
    memcpy(vm.mem + off, &v, sizeof(v));
    return 0;
}

static int eval_memr32(inst_t *inst, size_t *i) {
    UNUSED(i);

    int64_t off;
    if (!get_val(inst->arg, &off))
        return 1;

    if (!off_valid(off, inst->arg, sizeof(uint32_t)))
        return 1;

    uint32_t v;
    memcpy(&v, vm.mem + off, sizeof(v));
    vm.r[vm.a] = v;
    return 0;
}

static int eval_memw64(inst_t *inst, size_t *i) {
    UNUSED(i);

    int64_t off;
    if (!get_val(inst->arg, &off))
        return 1;

    if (!off_valid(off, inst->arg, sizeof(uint64_t)))
        return 1;

    lit_t *reg = inst->arg->next;

    int64_t r;
    if (!get_reg(reg, &r))
        return 1;

    uint64_t v = (uint64_t)vm.r[r];
    memcpy(vm.mem + off, &v, sizeof(v));
    return 0;
}

static int eval_memr64(inst_t *inst, size_t *i) {
    UNUSED(i);

    int64_t off;
    if (!get_val(inst->arg, &off))
        return 1;

    if (!off_valid(off, inst->arg, sizeof(uint64_t)))
        return 1;

    uint64_t v;
    memcpy(&v, vm.mem + off, sizeof(v));
    vm.r[vm.a] = v;
    return 0;
}

static int eval_memws(inst_t *inst, size_t *i) {
    UNUSED(i);

    int64_t off;
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

static int eval_memws2(inst_t *inst, size_t *i) {
    UNUSED(i);

    int64_t off;
    if (!get_val(inst->arg, &off))
        return 1;

    lit_t *addr = inst->arg->next; 
    int64_t p;
    if (!get_val(addr, &p))
        return 1;

    for (size_t i = 0; p + i < VM_MEM && vm.mem[p + i]; i++) {
        if (!off_valid(off + i, inst->arg, i))
            return 1;
        vm.mem[off + i] = vm.mem[p + i];
    }
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

static int eval_shift(inst_t *inst, size_t *i, inst_e type) {
    UNUSED(i);

    int64_t src;
    if (!get_reg(inst->arg, &src))
        return 1;

    lit_t *next = inst->arg->next;
    int64_t dst;
    if (!get_reg(next, &dst))
        return 1;

    if (type == IN_SHIFT_L) vm.r[dst] = vm.r[vm.a] << vm.r[src];
    if (type == IN_SHIFT_R) vm.r[dst] = vm.r[vm.a] >> vm.r[src];
    return 0;
}

static int eval_call(inst_t *inst, size_t *i) {
    int sp = vm.r[vm.a];
    size_t ret = *i + 1;
    
    if (sp < (int)sizeof(size_t) || sp > VM_MEM) {
        fprintf(stderr, "Error: Stack pointer out of range = %d (0x%08x) (l: %d, p: %d)\n", sp, sp, inst->lineno, inst->pos);
        return 1;
    }

    sp -= sizeof(size_t);
    memcpy(vm.mem + sp, &ret, sizeof(size_t));
    vm.r[vm.a] = sp;

    return eval_jump(inst, i, IN_JUMP);
}

static int eval_return(inst_t *inst, size_t *i) {
    UNUSED(inst);

    int sp = vm.r[vm.a];
    size_t ret;

    if (sp < (int)sizeof(size_t) || sp > VM_MEM) {
        fprintf(stderr, "Error: Stack pointer out of range %d (0x%08x) (l: %d, p: %d)\n", sp, sp, inst->lineno, inst->pos);
        return 1;
    }

    memcpy(&ret, vm.mem + sp, sizeof(size_t));
    sp += sizeof(size_t);
    vm.r[vm.a] = sp;

    *i = ret - 1;
    return 0;
}

static int eval_iow8(inst_t *inst, size_t *i) {
    UNUSED(i);

    int64_t addr;
    if (!get_val(inst->arg, &addr))
        return 1;

    int64_t src;
    if (!get_reg(inst->arg->next, &src))
        return 1;

    FILE *io = (FILE*)addr;
    uint8_t val = (uint8_t)vm.r[src];
    fwrite(&val, sizeof(uint8_t), 1, io);
    return 0;
}

static int eval_ior8(inst_t *inst, size_t *i) {
    UNUSED(i);

    int64_t addr;
    if (!get_val(inst->arg, &addr))
        return 1;

    FILE *io = (FILE*)addr;
    uint8_t val;
    fread(&val, sizeof(uint8_t), 1, io);
    vm.r[vm.a] = (int64_t)val;
    return 0;
}

static int eval_iow16(inst_t *inst, size_t *i) {
    UNUSED(i);

    int64_t addr;
    if (!get_val(inst->arg, &addr))
        return 1;

    int64_t src;
    if (!get_reg(inst->arg->next, &src))
        return 1;

    FILE *io = (FILE*)addr;
    uint16_t val = (uint16_t)vm.r[src];
    fwrite(&val, sizeof(uint16_t), 1, io);
    return 0;
}

static int eval_ior16(inst_t *inst, size_t *i) {
    UNUSED(i);

    int64_t addr;
    if (!get_val(inst->arg, &addr))
        return 1;

    FILE *io = (FILE*)addr;
    uint16_t val;
    fread(&val, sizeof(uint16_t), 1, io);
    vm.r[vm.a] = (int64_t)val;
    return 0;
}

static int eval_iow32(inst_t *inst, size_t *i) {
    UNUSED(i);

    int64_t addr;
    if (!get_val(inst->arg, &addr))
        return 1;

    int64_t src;
    if (!get_reg(inst->arg->next, &src))
        return 1;

    FILE *io = (FILE*)addr;
    uint32_t val = (uint32_t)vm.r[src];
    fwrite(&val, sizeof(uint32_t), 1, io);
    return 0;
}

static int eval_ior32(inst_t *inst, size_t *i) {
    UNUSED(i);

    int64_t addr;
    if (!get_val(inst->arg, &addr))
        return 1;

    FILE *io = (FILE*)addr;
    uint32_t val;
    fread(&val, sizeof(uint32_t), 1, io);
    vm.r[vm.a] = (int64_t)val;
    return 0;
}

static int eval_iow64(inst_t *inst, size_t *i) {
    UNUSED(i);

    int64_t addr;
    if (!get_val(inst->arg, &addr))
        return 1;

    int64_t src;
    if (!get_reg(inst->arg->next, &src))
        return 1;

    FILE *io = (FILE*)addr;
    uint64_t val = (uint64_t)vm.r[src];
    fwrite(&val, sizeof(uint64_t), 1, io);
    return 0;
}

static int eval_ior64(inst_t *inst, size_t *i) {
    UNUSED(i);

    int64_t addr;
    if (!get_val(inst->arg, &addr))
        return 1;

    FILE *io = (FILE*)addr;
    uint64_t val;
    fread(&val, sizeof(uint64_t), 1, io);
    vm.r[vm.a] = (int64_t)val;
    return 0;
}

static int eval_iows(inst_t *inst, size_t *i) {
    UNUSED(i);

    int64_t addr;
    if (!get_val(inst->arg, &addr))
        return 1;

    lit_t *str = inst->arg->next;

    FILE *io = (FILE*)addr;
    fwrite(str->char_type, sizeof(char), strlen(str->char_type), io);
    return 0;
}

static int eval_iows2(inst_t *inst, size_t *i) {
    UNUSED(i);

    int64_t addr;
    if (!get_val(inst->arg, &addr))
        return 1;

    lit_t *addr2 = inst->arg->next;
    FILE *io = (FILE*)addr;

    int64_t p;
    if (!get_val(addr2, &p))
        return 1;

    for (size_t i = 0; p + i < VM_MEM && vm.mem[p + i]; i++)
        fwrite(&vm.mem[p + i], sizeof(uint8_t), 1, io);
    return 0;
}

static int eval_memwsn(inst_t *inst, size_t *i) {
    UNUSED(i);

    int64_t off;
    if (!get_val(inst->arg, &off))
        return 1;

    int64_t r;
    if (!get_reg(inst->arg->next, &r))
        return 1;

    int64_t val = vm.r[r];
    int size = snprintf(NULL, 0, "%ld", val);
    char *str = malloc(size + 1);
    sprintf(str, "%ld", val);

    for (int i = 0; i < size + 1; i++) {
        if (!off_valid(off + i, inst->arg, i))
            return 1;
        vm.mem[off + i] = str[i];
    }
    return 0;
}

static int eval_memwsh(inst_t *inst, size_t *i) {
    UNUSED(i);

    int64_t off;
    if (!get_val(inst->arg, &off))
        return 1;

    int64_t r;
    if (!get_reg(inst->arg->next, &r))
        return 1;

    int64_t val = vm.r[r];
    int size = 8;
    char *str = malloc(size + 1);
    sprintf(str, "%08x", (unsigned)val);

    for (int i = 0; i < size + 1; i++) {
        if (!off_valid(off + i, inst->arg, i))
            return 1;
        vm.mem[off + i] = str[i];
    }
    return 0;
}

/* -- eval instructions end */

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
            case IN_ADD:
            case IN_SUB:
            case IN_MUL:
            case IN_DIV:
            case IN_MOD:
            case IN_AND:
            case IN_OR:
            case IN_XOR:
            case IN_NOT: err = eval_binop(inst, &i, inst->type); break;
            case IN_SHIFT_L: err = eval_shift(inst, &i, inst->type); break;
            case IN_SHIFT_R: err = eval_shift(inst, &i, inst->type); break;
            case IN_JUMP:
            case IN_JUMP_EQ:
            case IN_JUMP_NE:
            case IN_JUMP_LT:
            case IN_JUMP_GT:
            case IN_JUMP_LE:
            case IN_JUMP_GE: err = eval_jump(inst, &i, inst->type); break;
            case IN_CALL: err = eval_call(inst, &i); break;
            case IN_RETURN: err = eval_return(inst, &i); break;
            case IN_DUMP: err = eval_dump(inst, &i); break;
            case IN_MEMDUMP: err = eval_memdump(inst, &i); break;
            case IN_LABEL: err = eval_label(inst, &i); break;
            case IN_MEMW8: err = eval_memw8(inst, &i); break;
            case IN_MEMR8: err = eval_memr8(inst, &i); break;
            case IN_MEMW16: err = eval_memw16(inst, &i); break;
            case IN_MEMR16: err = eval_memr16(inst, &i); break;
            case IN_MEMW32: err = eval_memw32(inst, &i); break;
            case IN_MEMR32: err = eval_memr32(inst, &i); break;
            case IN_MEMW64: err = eval_memw64(inst, &i); break;
            case IN_MEMR64: err = eval_memr64(inst, &i); break;
            case IN_MEMWS: err = eval_memws(inst, &i); break;
            case IN_MEMWS2: err = eval_memws2(inst, &i); break;
            case IN_INC: err = eval_inc(inst, &i); break;
            case IN_DEC: err = eval_dec(inst, &i); break;
            case IN_TIME: err = eval_time(inst, &i); break;
            case IN_IOW8: err = eval_iow8(inst, &i); break;
            case IN_IOR8: err = eval_ior8(inst, &i); break;
            case IN_IOW16: err = eval_iow16(inst, &i); break;
            case IN_IOR16: err = eval_ior16(inst, &i); break;
            case IN_IOW32: err = eval_iow32(inst, &i); break;
            case IN_IOR32: err = eval_ior32(inst, &i); break;
            case IN_IOW64: err = eval_iow64(inst, &i); break;
            case IN_IOR64: err = eval_ior64(inst, &i); break;
            case IN_IOWS: err = eval_iows(inst, &i); break;
            case IN_IOWS2: err = eval_iows2(inst, &i); break;
            case IN_MEMWSN: err = eval_memwsn(inst, &i); break;
            case IN_MEMWSH: err = eval_memwsh(inst, &i); break;
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
