#include <stdlib.h>
#include <stdio.h>

#include "check.h"
#include "yconfig.h"

int main(int argc, char ** argv)
{
	int rv;
	struct yconfig * conf, * tmp;
	float fval1, fval2;

	conf = NULL;
	rv = 0;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
		return EXIT_FAILURE;
	}

	printf("Parsing %s ... \n", argv[1]);
	conf = yconfig_read_file(argv[1]);
	check(conf != NULL);

	check(yconfig_lookup(conf, "root-key") != NULL);
	check(yconfig_lookup(conf, "root-key:key-1") != NULL);
	check(yconfig_lookup(conf, "invalid") == NULL);

	tmp = yconfig_lookup(conf, "root-key:key-6");
	check(tmp != NULL);
	check(yconfig_get_float(tmp, &fval1) == 0);
	check(yconfig_lookup_float(conf, "root-key:key-6", &fval2) == 0);
	check(fval1 == 0.23f && fval1 == fval2);

	printf(" ... Success !\n");
	check(yconfig_write(conf, stdout) == 0)

	if (rv != 0)
		printf(" ... FAILED !\n");

	yconfig_destroy(conf);

	return EXIT_SUCCESS;
}
