#ifndef TOKEN_H
#define TOKEN_H

#include <stddef.h>

typedef enum token_enum {
	TOKEN_NULL,
	TOKEN_ID, //a-z0-9_
	TOKEN_NUM, // 0-9
	TOKEN_STR,
	TOKEN_END, // ;
	TOKEN_REF, // % register reference
} token_e;

typedef struct token {
	char *value;
	token_e type;
	int lineno;
	int pos;
	size_t size;
	struct token *next;
} token_t;

extern token_t *tokenize(const char *src, size_t len);

#endif