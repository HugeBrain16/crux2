#include <token.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int isnum(char c) {
	return c >= '0' && c <= '9';
}

static int isletter(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static int isalpha(char c) {
	return isnum(c) || isletter(c);
}

static token_t *token_new(token_e type) {
	token_t *t = malloc(sizeof(token_t));
	t->type = type;
	t->size = 0;
	t->value = NULL;
	t->lineno = 0;
	t->next = NULL;
	return t;
}

static token_t *token_def(token_e type, const char *src, size_t len) {
	token_t *t = token_new(type);
	t->value = malloc(len);
	memcpy(t->value, src, len);
	return t;
}

static token_t *lex_id(const char *src, size_t *pos) {
	token_t *t = token_new(TOKEN_ID);

	while (isalpha(*src) || *src == '_') {
		t->value = realloc(t->value, t->size + 1);
		t->value[t->size++] = *src;
		src++;
		(*pos)++;
	}

	t->value = realloc(t->value, t->size + 1);
	t->value[t->size] = '\0';

	return t;
}

static token_t *lex_num(const char *src, size_t *pos) {
	token_t *t = token_new(TOKEN_NUM);

	// negative
	if (*src == '-') {
		t->value = realloc(t->value, t->size + 1);
		t->value[t->size++] = *src;
		src++;
		(*pos)++;
	}

	while (isnum(*src) || *src == '_') {
		if (*src != '_') {
			t->value = realloc(t->value, t->size + 1);
			t->value[t->size++] = *src;
		}

		src++;
		(*pos)++;
	}

	t->value = realloc(t->value, t->size + 1);
	t->value[t->size] = '\0';

	return t;
}

static token_t *lex_end(const char *src, size_t *pos) {
	token_t *t = token_def(TOKEN_END, ";", 1);

	src++;
	(*pos)++;

	return t;
}

token_t *tokenize(const char *src, size_t len) {
	if (!len) return NULL;

	int lineno = 1;
	token_t *head = NULL;
	token_t *curr = NULL;

	int in_comment = 0;

	for (size_t i = 0; i < len; i++) {
		token_t *tok = NULL;

		char c = src[i];
		char cn = i + 1 < len ? src[i + 1] : '\0';

		if (c == '#' && !in_comment) {
			if (cn == '-') {
				in_comment = 2;
				i++;
				continue;
			}

			in_comment = 1;
		}

		if (c == '-' && in_comment == 2 && cn == '#') {
			in_comment = 0;

			i++;
			continue;
		}

		if (c == '\n') {
			if (in_comment == 1)
				in_comment = 0;

			lineno++;
			continue;
		}

		if (c == ' ' || c == '\t' || c == '\r' || in_comment)
			continue;

		int start = i;
		if (isletter(c) || c == '_')
			tok = lex_id(src + i, &i);
		else if (isnum(c) || (c == '-' && isnum(cn)))
			tok = lex_num(src + i, &i);
		else if (c == ';')
			tok = lex_end(src + i, &i);
		else {
			printf("Error: Invalid syntax (l: %d, p: %d)\n", lineno, (int)i);
			break;
		}

		if (tok) {
			i--;

			tok->lineno = lineno;
			tok->pos = start;
			// printf("line=%d pos=%d value=%s\n", tok->lineno, tok->pos, tok->value);

			if (!head) {
				head = tok;
				curr = tok;
			} else {
				curr->next = tok;
				curr = tok;
			}
		}
	}

	return head;
}