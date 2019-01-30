-- Given we create a database
DROP DATABASE IF EXISTS mydatabase;
CREATE DATABASE mydatabase;
\c mydatabase

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
ALTER DATABASE mydatabase SET TABLESPACE mytablespace;

-- Then all the database files from QD and all QEs should be moved into the new tablespace. We use the fact that default tablespace names look like base/<OID>/<TABLE OID> wherease user created tablespaces have format pg_tblspc/<other stuff>
\c mydatabase
SELECT COUNT(*) FROM (SELECT pg_relation_filepath('mytable')) a where a.pg_relation_filepath LIKE '%pg_tblspc%';
SELECT COUNT(*) FROM (SELECT gp_segment_id, pg_relation_filepath('mytable') FROM gp_dist_random('gp_id')) a where a.pg_relation_filepath LIKE '%pg_tblspc%';

