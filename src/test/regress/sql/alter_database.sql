-- Given we create a database
DROP DATABASE IF EXISTS alter_database_connection_limit;
CREATE DATABASE alter_database_connection_limit;
\c alter_database_connection_limit

ALTER DATABASE alter_database_connection_limit CONNECTION LIMIT 2;

SELECT datconnlimit FROM pg_database WHERE datname='alter_database_connection_limit';
SELECT datconnlimit FROM gp_dist_random('pg_database') WHERE datname='alter_database_connection_limit';
