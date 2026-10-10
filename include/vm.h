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

#define BLOCK_FREE (1 << 0)

typedef struct block {
	size_t size;
	uint8_t flags;
	struct block *next;
} block_t;

typedef struct {
	uint8_t *start;
	uint8_t *current;
	uint8_t *end;
	uint8_t field[VM_MEM];
} mem_t;

typedef struct {
	int a;
	uint32_t f;
	reg_t r[VM_REG];
	mem_t mem;
} vm_t;

extern int exec(insts_t *insts);

#endif
