#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char *
uri_build(const char *base, int argc, char * const argv[])
{
	int i;
	size_t totallen;
	char *p;
	char * const sep = "+";
	const char *sep0;

	assert(argc > 0);

	/* XXX: consider character encoding and URI escaping... */
	totallen = strlen(base) + 1;
	for (i = 0; i < argc; i++) {
		assert(argv[i] != NULL);
		totallen += strlen(argv[i]) + 1 /* separator or terminator */;
	}

	p = malloc(totallen);
	if (p == NULL)
		return NULL;

	(void)strlcpy(p, base, totallen);
	for (i = 0, sep0 = ""; i < argc; i++, sep0 = sep)
		snprintf(p, totallen, "%s%s%s", p, sep0, argv[i]);

	return p;
}

void
uri_free(char *uri)
{

	free(uri);
}
