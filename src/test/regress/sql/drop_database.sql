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

\c postgres

-- When we drop the database and error happened on a segment while droping the database
SELECT gp_inject_fault_infinite('inside_drop_db_transaction', 'error', dbid) FROM gp_segment_configuration WHERE role = 'p' AND content = 0;
-- start_ignore
DROP DATABASE mydatabase;
-- end_ignore

-- Then all the database files from QD and all QEs should remain
\c mydatabase

SELECT gp_segment_id, * FROM mytable;

-- Cleanup
\c postgres
SELECT gp_inject_fault('inside_drop_db_transaction', 'reset', dbid) FROM gp_segment_configuration WHERE role = 'p' AND content = 0;
