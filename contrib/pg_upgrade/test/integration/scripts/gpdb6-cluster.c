#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void
startGpdbSixCluster(void)
{
	system(""
		". ./gpdb6/greenplum_path.sh; "
		". ./configuration/gpdb6-env.sh; "
		"export MASTER_DATA_DIRECTORY=./gpdb6-data/qddir/demoDataDir-1; "
		"./gpdb6/bin/gpstart -a --skip_standby_check --no_standby"
	);
}

static void
stopGpdbSixCluster(void)
{
	system(""
		". ./gpdb6/greenplum_path.sh; \n"
		". ./configuration/gpdb6-env.sh; \n"
		"export MASTER_DATA_DIRECTORY=./gpdb6-data/qddir/demoDataDir-1; \n"
		"./gpdb6/bin/gpstop -af"
	);
}


int
main(int argc, char *argv[])
{
	if (argc != 2)
	{
		printf("\nusage: ./scripts/gpdb6-cluster [start|stop]\n");
		exit(1);
	}

	char	   *const command = argv[1];

	if (strncmp("start", command, 5) == 0)
	{
		startGpdbSixCluster();
	}

	if (strncmp("stop", command, 4) == 0)
	{
		stopGpdbSixCluster();
	}
}
