-- Test Optimizer Plan Hints Feature
--
-- Purpose: Test that plan hints may be used to coerce the plan shape generated
-- by the optimizer.

LOAD 'pg_hint_plan';

DROP SCHEMA IF EXISTS planhints CASCADE;

CREATE SCHEMA planhints;
SET search_path=planhints;
SET optimizer_trace_fallback=on;

-- Setup tables
CREATE TABLE heap_table(a int, b int);

CREATE TABLE ao_table(a int, b int) WITH (appendonly=true);

CREATE TABLE pt_table(a int, b int) PARTITION BY RANGE (a) (PARTITION p1 START(0) END(101) EVERY(20));

CREATE TABLE rep_table(a int, b int) DISTRIBUTED REPLICATED;

ANALYZE heap_table;
ANALYZE ao_table;
ANALYZE pt_table;
ANALYZE rep_table;

INSERT INTO heap_table SELECT i, i FROM generate_series(1, 100)i;
INSERT INTO ao_table SELECT i, i FROM generate_series(1, 100)i;
INSERT INTO pt_table SELECT i, i FROM generate_series(1, 100)i;
INSERT INTO rep_table SELECT i, i FROM generate_series(1, 100)i;

ANALYZE heap_table; ANALYZE ao_table; ANALYZE pt_table; ANALYZE rep_table;

/*+ Redistribute(t1) Broadcast(t2)*/
EXPLAIN (costs off)
SELECT * FROM heap_table t1 INNER JOIN heap_table t2 ON t1.b = t2.a WHERE t2.a=3;
/*+ Redistribute(t1) Broadcast(t2)*/
SELECT * FROM heap_table t1 INNER JOIN heap_table t2 ON t1.b = t2.a WHERE t2.a=3;

/*+ Broadcast(t1) Broadcast(t2)*/
EXPLAIN (costs off)
SELECT * FROM heap_table t1 INNER JOIN heap_table t2 ON t1.b = t2.a WHERE t2.a=3;
/*+ Broadcast(t1) Broadcast(t2)*/
SELECT * FROM heap_table t1 INNER JOIN heap_table t2 ON t1.b = t2.a WHERE t2.a=3;

/*+ Redistribute(t1) Redistribute(t2)*/
EXPLAIN (costs off)
SELECT * FROM heap_table t1 INNER JOIN heap_table t2 ON t1.b = t2.a WHERE t2.a=3;
/*+ Redistribute(t1) Redistribute(t2)*/
SELECT * FROM heap_table t1 INNER JOIN heap_table t2 ON t1.b = t2.a WHERE t2.a=3;


-- Testing replicated tables with motion hints

/*+ Broadcast(rt) Broadcast(ht) */
EXPLAIN (costs off)
SELECT * FROM rep_table rt INNER JOIN heap_table ht ON rt.b = ht.a WHERE ht.a=3;
/*+ Broadcast(rt) Broadcast(ht) */
SELECT * FROM rep_table rt INNER JOIN heap_table ht ON rt.b = ht.a WHERE ht.a=3;

/*+ Broadcast(rt) Redistribute(ht) */
EXPLAIN (costs off)
SELECT * FROM rep_table rt INNER JOIN heap_table ht ON rt.b = ht.a WHERE ht.a=3;
/*+ Broadcast(rt) Redistribute(ht) */
SELECT * FROM rep_table rt INNER JOIN heap_table ht ON rt.b = ht.a WHERE ht.a=3;

/*+ Redistribute(rt) Broadcast(ht) */
EXPLAIN (costs off)
SELECT * FROM rep_table rt INNER JOIN heap_table ht ON rt.b = ht.a WHERE ht.a=3;
/*+ Redistribute(rt) Broadcast(ht) */
SELECT * FROM rep_table rt INNER JOIN heap_table ht ON rt.b = ht.a WHERE ht.a=3;

/*+ Broadcast(rt) Broadcast(rt2)*/
EXPLAIN (costs off)
SELECT * FROM rep_table rt INNER JOIN rep_table rt2 ON rt.b = rt2.a WHERE rt2.a=3;
/*+ Broadcast(rt) Broadcast(rt2)*/
SELECT * FROM rep_table rt INNER JOIN rep_table rt2 ON rt.b = rt2.a WHERE rt2.a=3;

/*+ Redistribute(rt) Redistribute(rt2)*/
EXPLAIN (costs off)
SELECT * FROM rep_table rt INNER JOIN rep_table rt2 ON rt.b = rt2.a WHERE rt2.a=3;
/*+ Redistribute(rt) Redistribute(rt2)*/
SELECT * FROM rep_table rt INNER JOIN rep_table rt2 ON rt.b = rt2.a WHERE rt2.a=3;


-- TODO: Look at these later...
/*+ Broadcast(t2) Redistribute(t1) */
EXPLAIN (costs off) SELECT * FROM heap_table t2 WHERE t2.b NOT IN (SELECT t1.a FROM heap_table t1);
