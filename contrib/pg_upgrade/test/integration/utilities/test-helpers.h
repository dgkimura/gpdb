#include "libpq-fe.h"

PGconn *connectToFive(void);
PGconn *connectToFiveInSchema(char *schema);
void resetGpdbFiveDataDirectories(void);

PGconn *connectToSix(void);
PGconn *connectToSixInSchema(char *schema);
void resetGpdbSixDataDirectories(void);

