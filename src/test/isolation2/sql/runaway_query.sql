1:CREATE EXTENSION IF NOT EXISTS gp_inject_fault;

1:CREATE TABLE runaway_query_test_table(a bigint NOT NULL) DISTRIBUTED BY (a);

1:select gp_inject_fault_infinite('test', 'vmem_protect_limit', 1);
1:select gp_wait_until_triggered_fault('test', 1, 1);

2:explain (costs off) SELECT a FROM runaway_query_test_table WHERE (a = ANY ((ARRAY[42])));
