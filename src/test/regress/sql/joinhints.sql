-- Test Optimizer Row Hints Feature
--
-- Purpose: Test that join hints may be used to edit cardinality estimates

LOAD 'pg_hint_plan';

DROP SCHEMA IF EXISTS joinhints CASCADE;

CREATE SCHEMA joinhints;
SET search_path=joinhints;
SET optimizer_trace_fallback=on;

-- Setup tables
CREATE TABLE t1(a int, b int);
CREATE TABLE t2(a int, b int);
CREATE TABLE t3(a int, b int);
CREATE TABLE t4(a int, b int);
CREATE TABLE t5(a int, b int);

-- Baseline no hints
EXPLAIN (costs off) SELECT t1.a, t2.a FROM t1, t2, t3, t4, t5;

/*+
    Leading((t5 (((t4 t3) t2) t1)))
 */
EXPLAIN (costs off) SELECT t1.a, t2.a FROM t1, t2, t3, t4, t5;

/*+
    Leading((t2 (t1 t3)))
 */
EXPLAIN (costs off) SELECT * FROM t1, t2, (select * from t3 limit 42) as t3;

-- XXX: Fallback because ORCA didn't generate bushy join tree alternative
--/*+
--    Leading((t2 ((t5 t4) (t1 t3))))
-- */
--EXPLAIN (costs off) SELECT t1.a, t2.a FROM t1, t2, t3, t4, t5;

-- TODO: Fix SIGSEGV due to bad syntax (extra set of parens)
-- Leading((t2 (((t5 t4) (t1 t3)))))
