-- Scenrio 1: Where alter database set tablespace successfully changes the
-- underlying tablespace directory.

-- Given we create a database
CREATE EXTENSION IF NOT EXISTS gp_inject_fault;
DROP DATABASE IF EXISTS mydatabase;
CREATE DATABASE mydatabase;
\c mydatabase

-- And create a table in that database with data
DROP TABLE IF EXISTS mytable;
CREATE TABLE mytable(id int, name text);
INSERT INTO mytable VALUES (1, 'a'), (2, 'b'), (4, 'c');

-- When we create a tablespace
-- start_ignore
DROP TABLESPACE IF EXISTS mytablespace;
-- end_ignore
\! rm -rf '/tmp/mytablespace';
\! mkdir '/tmp/mytablespace';
CREATE TABLESPACE mytablespace LOCATION '/tmp/mytablespace';

-- And alter the database to use the new tablespace
\c postgres
ALTER DATABASE mydatabase SET TABLESPACE mytablespace;

-- Then all the database files from QD and all QEs should be moved into the new
-- tablespace. We use the fact that default tablespace names begin with 'base'
-- where as user created tablespaces begin with 'pg_tblspc'
\c mydatabase

SELECT COUNT(*) FROM (SELECT pg_relation_filepath('mytable')) a where a.pg_relation_filepath LIKE 'pg_tblspc%';
SELECT COUNT(*) FROM (SELECT gp_segment_id, pg_relation_filepath('mytable') FROM gp_dist_random('gp_id')) a where a.pg_relation_filepath LIKE 'pg_tblspc%';

SELECT COUNT(*) FROM (SELECT pg_relation_filepath('mytable')) a where a.pg_relation_filepath LIKE 'base%';
SELECT COUNT(*) FROM (SELECT gp_segment_id, pg_relation_filepath('mytable') FROM gp_dist_random('gp_id')) a where a.pg_relation_filepath LIKE 'base%';

SELECT gp_segment_id, * FROM mytable;

-- Scenrio 2: Where alter database set tablespace fails on segment. In this
-- case we expect that the tablespace for database is unchanged.

\c postgres

-- Given we create a database
CREATE EXTENSION IF NOT EXISTS gp_inject_fault;
DROP DATABASE IF EXISTS mydatabase;
CREATE DATABASE mydatabase;
\c mydatabase

-- And create a table in that database with data
DROP TABLE IF EXISTS mytable;
CREATE TABLE mytable(id int, name text);
INSERT INTO mytable VALUES (1, 'a'), (2, 'b'), (4, 'c');

-- When we create a tablespace
DROP TABLESPACE IF EXISTS mytablespace;
\! rm -rf '/tmp/mytablespace';
\! mkdir '/tmp/mytablespace';
CREATE TABLESPACE mytablespace LOCATION '/tmp/mytablespace';

\c postgres

-- And error on a segment while altering the database to use the new tablespace
SELECT gp_inject_fault_infinite('inside_move_db_transaction', 'error', dbid) FROM gp_segment_configuration WHERE role = 'p' AND content = 0;
-- start_ignore
ALTER DATABASE mydatabase SET TABLESPACE mytablespace;
-- end_ignore

-- Then all the database files from QD and all QEs should continue to use the
-- old tablespace. The physical files may still exist on the new tablespace for
-- the segments that didn't fail, but the pg_class catalot entries point to the
-- old tablespace
\c mydatabase

SELECT COUNT(*) FROM (SELECT pg_relation_filepath('mytable')) a where a.pg_relation_filepath LIKE 'pg_tblspc%';
SELECT COUNT(*) FROM (SELECT gp_segment_id, pg_relation_filepath('mytable') FROM gp_dist_random('gp_id')) a where a.pg_relation_filepath LIKE 'pg_tblspc%';

SELECT COUNT(*) FROM (SELECT pg_relation_filepath('mytable')) a where a.pg_relation_filepath LIKE 'base%';
SELECT COUNT(*) FROM (SELECT gp_segment_id, pg_relation_filepath('mytable') FROM gp_dist_random('gp_id')) a where a.pg_relation_filepath LIKE 'base%';

SELECT gp_segment_id, * FROM mytable;

-- Cleanup
\c postgres
select gp_wait_until_triggered_fault('inside_move_db_transaction', 1, dbid) FROM gp_segment_configuration WHERE role = 'p' AND content = 0;
SELECT gp_inject_fault('inside_move_db_transaction', 'reset', dbid) FROM gp_segment_configuration WHERE role = 'p' AND content = 0;
