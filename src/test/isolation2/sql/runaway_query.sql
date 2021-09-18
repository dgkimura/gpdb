CREATE EXTENSION IF NOT EXISTS gp_inject_fault;
CREATE OR REPLACE LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION wait_for_gpdb_wrappers_get_comparison_operator_fault() returns void as $$
DECLARE optimizer text; /* in func */
BEGIN /* in func */
	SELECT setting INTO optimizer FROM pg_settings WHERE name='optimizer'; /* in func */
	IF (optimizer = 'on') THEN /* in func */
		PERFORM gp_wait_until_triggered_fault('gpdbwrappers_get_comparison_operator', 1, 1); /* in func */
	ELSE /*in func*/
		RAISE EXCEPTION 'optimizer=off - gpdbwrappers_get_comparison_operator vmem_protect_limit fault will not triggered'; /*in func*/
	END IF; /* in func */
END; /* in func */
$$ LANGUAGE PLPGSQL EXECUTE ON MASTER;

CREATE TABLE runaway_query_test_table(a bigint NOT NULL) DISTRIBUTED BY (a);

-- Use vmem_protect_limit fault to simulate vmem protect error and force cancel query.
SELECT gp_inject_fault_infinite('gpdbwrappers_get_comparison_operator', 'vmem_protect_limit', 1);

-- Run query that will trip the vmem protect limit fault.
EXPLAIN (COSTS OFF) SELECT a FROM runaway_query_test_table WHERE (a = ANY ((ARRAY[42])));

SELECT wait_for_gpdb_wrappers_get_comparison_operator_fault();

SELECT gp_inject_fault('all', 'reset', dbid) FROM gp_segment_configuration;
