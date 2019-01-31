-- Given we create a database
DROP DATABASE IF EXISTS database_that_successfully_changes_tablespace;
CREATE DATABASE database_that_successfully_changes_tablespace;
\c database_that_successfully_changes_tablespace

-- And create a table in that database with data
DROP TABLE IF EXISTS mytable;
CREATE TABLE mytable(id int, name text);
INSERT INTO mytable VALUES(1, 'a');

-- When we create a tablespace
DROP TABLESPACE IF EXISTS mytablespace;
\! rm -rf '/tmp/mytablespace';
\! mkdir '/tmp/mytablespace';
CREATE TABLESPACE mytablespace LOCATION '/tmp/mytablespace';

-- And alter the database to use the new tablespace
\c postgres

ALTER DATABASE database_that_successfully_changes_tablespace SET TABLESPACE mytablespace;

-- Then all the database files from QD and all QEs should be moved into the new tablespace. We use the fact that default tablespace names look like base/<OID>/<TABLE OID> wherease user created tablespaces have format pg_tblspc/<other stuff>

\c database_that_successfully_changes_tablespace

SELECT COUNT(*) FROM (SELECT pg_relation_filepath('mytable')) a where a.pg_relation_filepath LIKE '%pg_tblspc%';
SELECT COUNT(*) FROM (SELECT gp_segment_id, pg_relation_filepath('mytable') FROM gp_dist_random('gp_id')) a where a.pg_relation_filepath LIKE '%pg_tblspc%';

SELECT gp_segment_id, pg_relation_filepath('mytable') FROM gp_dist_random('gp_id');

-- Next scenario where alter database set tablespace panics on segment

CREATE EXTENSION IF NOT EXISTS gp_inject_fault;
DROP DATABASE IF EXISTS database_that_errors_change_tablespace;
CREATE DATABASE database_that_errors_change_tablespace;
\c database_that_errors_change_tablespace

-- And create a table in that database with data
DROP TABLE IF EXISTS mytable;
CREATE TABLE mytable(id int, name text);
INSERT INTO mytable VALUES(1, 'a');

-- When we create a tablespace
DROP TABLESPACE IF EXISTS mytablespace;
\! rm -rf '/tmp/mytablespace';
\! mkdir '/tmp/mytablespace';

SELECT gp_inject_fault_infinite('inside_move_db_transaction', 'error', dbid) FROM gp_segment_configuration WHERE role = 'p' AND content = 0;

\c database_that_errors_change_tablespace

SELECT COUNT(*) FROM (SELECT pg_relation_filepath('mytable')) a where a.pg_relation_filepath LIKE '%pg_tblspc%';
SELECT COUNT(*) FROM (SELECT gp_segment_id, pg_relation_filepath('mytable') FROM gp_dist_random('gp_id')) a where a.pg_relation_filepath LIKE '%pg_tblspc%';

SELECT gp_segment_id, pg_relation_filepath('mytable') FROM gp_dist_random('gp_id');

-- Cleanup
SELECT gp_inject_fault('inside_move_db_transaction', 'reset', dbid) FROM gp_segment_configuration WHERE role = 'p' AND content = 0;
