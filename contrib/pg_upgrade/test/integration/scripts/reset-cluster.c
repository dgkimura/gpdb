#include <stdlib.h>
#include <stdio.h>

static void
resetGpdbFiveDataDirectories(void)
{
	printf("\nMaking a copy of gpdb5 data directories.\n");
	system("rsync -a --delete ./gpdb5-data-copy/ ./gpdb5-data");
}

static void
resetGpdbSixDataDirectories(void)
{
	printf("\nMaking a copy of gpdb6 data directories.\n");
	system("rsync -a --delete ./gpdb6-data-copy/ ./gpdb6-data");
}

int
main(int argc, char *argv[])
{
	resetGpdbFiveDataDirectories();
	resetGpdbSixDataDirectories();
}

