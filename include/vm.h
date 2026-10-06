#ifndef VM_H
#define VM_H

#include <parser.h>

#include <stdint.h>

#define VM_REG 4
#define VM_MEM 256

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
	size_t size;
	uint8_t *start;
	uint8_t *current;
	uint8_t *end;
	void *field;
} mem_t;

typedef struct {
	int a;
	reg_t r[VM_REG];
	mem_t mem;
} vm_t;

extern int exec(insts_t *insts);

#endif