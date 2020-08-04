#include <stdio.h>
#include "argv-fuzz-inl.h"

int main(int argc, char *argv[]) {
	AFL_INIT_SET0("a.out");

	int i;
	printf("argc = %d\n", argc);
	for (i = 0; i < argc; i++)
		printf("argv[%d] = '%s'\n", i, argv[i]);
	return 0;
}
