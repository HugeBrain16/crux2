#include <token.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int lineno;

static int isnum(char c) {
	return c >= '0' && c <= '9';
}

static int isletter(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static int isalpha2(char c) {
	return isnum(c) || isletter(c);
}

static int ishex(char c) {
	return (c >= '0' && c <= '9') ||
    	   (c >= 'a' && c <= 'f') ||
    	   (c >= 'A' && c <= 'F');
}

static int isbin(char c) {
	return c >= '0' && c <= '1';
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

	size_t c = 0;
	while (isalpha2(*src) || *src == '_') {
		if (c >= t->size) {
			t->size = t->size == 0 ? 4 : t->size * 2;
			t->value = realloc(t->value, t->size);
		}
		t->value[c++] = *src;

		src++;
		(*pos)++;
	}

	if (c >= t->size)
		t->value = realloc(t->value, t->size + 1);
	t->value[c] = '\0';

	return t;
}

static token_t *lex_num(const char *src, size_t *pos) {
	token_t *t = token_new(TOKEN_NUM);

	size_t c = 0;
	if (*src == '0' && *(src + 1) == 'x') {
		t->value = realloc(t->value, t->size += 2);
		t->value[c++] = *src;
		t->value[c++] = *(src + 1);
		src += 2;
		(*pos) += 2;

		if (!ishex(*src)) {
			printf("Error: Invalid hex value (l: %d, p: %d)\n", lineno, (int)(*pos));
			return NULL;
		}

		while (ishex(*src) || *src == '_') {
			if (*src != '_') {
				if (c >= t->size) {
					t->size *= 2;
					t->value = realloc(t->value, t->size);
				}
				t->value[c++] = *src;
			}

			src++;
			(*pos)++;
		}
	} else if (*src == '0' && *(src + 1) == 'b') {
		t->value = realloc(t->value, t->size += 2);
		t->value[c++] = *src;
		t->value[c++] = *(src + 1);
		src += 2;
		(*pos) += 2;

		if (!isbin(*src)) {
			printf("Error: Invalid binary value (l: %d, p: %d)\n", lineno, (int)(*pos));
			return NULL;
		}

		while (isbin(*src) || *src == '_') {
			if (*src != '_') {
				if (c >= t->size) {
					t->size *= 2;
					t->value = realloc(t->value, t->size);
				}
				t->value[c++] = *src;
			}

			src++;
			(*pos)++;
		}
	} else {
		// negative
		if (*src == '-') {
			t->value = realloc(t->value, t->size += 1);
			t->value[c++] = *src;
			src++;
			(*pos)++;
		}

		while (isnum(*src) || *src == '_') {
			if (*src != '_') {
				if (c >= t->size) {
					t->size = t->size == 0 ? 4 : t->size * 2;
					t->value = realloc(t->value, t->size);
				}
				t->value[c++] = *src;
			}

			src++;
			(*pos)++;
		}
	}

	if (c >= t->size)
		t->value = realloc(t->value, t->size + 1);
	t->value[c] = '\0';

	return t;
}

static token_t *lex_end(const char *src, size_t *pos) {
	token_t *t = token_def(TOKEN_END, ";", 1);

	src++;
	(*pos)++;

	return t;
}

static token_t *lex_ref(const char *src, size_t *pos) {
	token_t *t = token_def(TOKEN_REF, "%", 1);

	src++;
	(*pos)++;

	return t;
}

static token_t *lex_str(const char *src, size_t *pos) {
	token_t *t = token_new(TOKEN_STR);

	src++;
	(*pos)++;

	size_t c = 0;
	int is_escaped = 0;
	while ((*src != '"' || is_escaped) && *src != '\0') {
		if (c >= t->size) {
			t->size = t->size == 0 ? 4 : t->size * 2;
			t->value = realloc(t->value, t->size);
		}
		t->value[c++] = *src;

		if (is_escaped)
			is_escaped = 0;
		else if (*src == '\\')
			is_escaped = 1;

		src++;
		(*pos)++;
	}

	if (*src != '"') {
		printf("Error: Unclosed string (l: %d, p: %d)\n", lineno, (int)(*pos));
		free(t->value);
		free(t);
		return NULL;
	}

	src++;
	(*pos)++;

	if (c >= t->size)
		t->value = realloc(t->value, t->size + 1);
	t->value[c] = '\0';

	return t;
}

token_t *tokenize(const char *src, size_t len) {
	if (!len) return NULL;

	lineno = 1;
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
		else if (isnum(c) || (c == '-' && isnum(cn)) || (c == '0' && cn == 'x') || (c == '0' && cn == 'b'))
			tok = lex_num(src + i, &i);
		else if (c == ';')
			tok = lex_end(src + i, &i);
		else if (c == '"')
			tok = lex_str(src + i, &i);
		else if (c == '%')
			tok = lex_ref(src + i, &i);
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
		} else break; // error
	}

	return head;
}
