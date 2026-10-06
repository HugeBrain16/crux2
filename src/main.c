#include <parser.h>
#include <vm.h>

#include <stdio.h>

int main(int argc, char *argv[]) {
	if (argc < 2)
		return fprintf(stderr, "Usage: crux <file>\n");

	const char *input_file = argv[1];
	FILE *file = fopen(input_file, "r");
	if (!file)
		return fprintf(stderr, "Error: file not found = %s\n", argv[1]);

	size_t size;
	fseek(file, 0, SEEK_END);
	size = ftell(file);
	rewind(file);

	char buf[size + 1];
	fread(buf, sizeof(char), size, file);
	buf[size] = '\0';

	token_t *tokens = tokenize(buf, size);
	insts_t *insts = parse(tokens);
	exec(insts);

	fclose(file);
	return 0;
}