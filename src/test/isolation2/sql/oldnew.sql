1o: CREATE TABLE t(a int);
CREATE TABLE t(a int);

-- insert data on old cluster
1o: INSERT INTO t VALUES(1);

-- now perform pg_upgrade using like...
-- \! pg_upgrade --old-port=XXX --new-port=YYY ...

-- validate data on new cluster
SELECT * FROM t;
