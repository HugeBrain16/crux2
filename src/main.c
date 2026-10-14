#include <parser.h>
#include <vm.h>
#include <defs.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
	size_t count;
	size_t size;
	char **items;
} args_t;

static args_t args;
static args_t options;

static int has_option(const char *src) {
	for (size_t i = 0; i < options.count; i++) {
		if (!strcmp(src, options.items[i]))
			return 1;
	}
	return 0;
}

int main(int argc, char *argv[]) {
	args = (args_t){0};
	options = (args_t){0};

	for (int i = 1; i < argc; i++) {
		if (strncmp(argv[i], "--", 2) == 0) {
			if (options.count >= options.size) {
				options.size++;
				options.items = realloc(options.items, sizeof(*options.items)*options.size);
			}
			options.items[options.count++] = argv[i] + 2;
		} else {
			if (args.count >= args.size) {
				args.size++;
				args.items = realloc(args.items, sizeof(*args.items)*args.size);
			}
			args.items[args.count++] = argv[i];
		}
	}

	if (options.count > 0) {
		if (has_option("debug"))
			crux_debug = 1;
		else if (has_option("help")) {
			printf("CRUX2 r%d\nUsage: crux <file>\n\nArgs:\n\t--help\n\t\tShow help and exit.\n\t--debug\n\t\tShow debug informations.\n", CRUX_VERSION);
			return 0;
		}
	}

	if (args.count < 1)
		return fprintf(stderr, "Usage: crux <file>\n");

	const char *input_file = args.items[0];
	FILE *file = fopen(input_file, "r");
	if (!file)
		return fprintf(stderr, "Error: file not found = %s\n", input_file);

	size_t size;
	fseek(file, 0, SEEK_END);
	size = ftell(file);
	rewind(file);

	char buf[size + 1];
	fread(buf, sizeof(char), size, file);
	buf[size] = '\0';

	token_t *tokens = tokenize(buf, size);
	insts_t *insts = parse(tokens);
	int err = exec(insts);

	fclose(file);
	return err;
}