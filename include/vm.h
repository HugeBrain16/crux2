#ifndef VM_H
#define VM_H

#include <parser.h>

#include <stdint.h>

#define VM_REG 32
#define VM_MEM 65536

typedef struct {
	uint8_t type;
	int val;
} reg_t;

typedef struct {
	int a;
	uint32_t f;
	reg_t r[VM_REG];
	uint8_t mem[VM_MEM];
} vm_t;

extern int exec(insts_t *insts);

#endif
