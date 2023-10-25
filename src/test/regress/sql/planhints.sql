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
CREATE TABLE my_table(a int, b int);
CREATE INDEX my_awesome_index ON my_table(a);
CREATE INDEX my_amazing_index ON my_table(a);
CREATE INDEX my_incredible_index ON my_table(a);
CREATE INDEX my_bitmap_index ON my_table USING bitmap (a);

CREATE TABLE your_table(a int, b int) WITH (appendonly=true);
CREATE INDEX your_awesome_index ON your_table(a);
CREATE INDEX your_amazing_index ON your_table(a);
CREATE INDEX your_incredible_index ON your_table(a);
CREATE INDEX your_bitmap_index ON your_table USING bitmap (a);

CREATE TABLE our_table(a int, b int) PARTITION BY RANGE (a) (PARTITION p1 START(0) END(10) EVERY(3));
CREATE INDEX our_awesome_index ON our_table(a);
CREATE INDEX our_amazing_index ON our_table(a);
CREATE INDEX our_incredible_index ON our_table(a);
CREATE INDEX our_bitmap_index ON our_table USING bitmap (a);

EXPLAIN (costs off) SELECT t1.a, t2.a, t3.a FROM my_table AS t1 JOIN your_table AS t2 ON t1.a=t2.a JOIN our_table AS t3 ON t3.a=t2.a WHERE t1.a<42;

--------------------------------------------------------------------
--
-- 1. [JOIN] Specific explicit scan type and implicit/explicit index
--
--------------------------------------------------------------------

/*+
    SeqScan(t1)
    SeqScan(t2)
    SeqScan(t3)
 */
EXPLAIN (costs off) SELECT t1.a, t2.a, t3.a FROM my_table AS t1 JOIN your_table AS t2 ON t1.a=t2.a JOIN our_table AS t3 ON t3.a=t2.a WHERE t1.a<42;

-- NB: IndexScan on AO table is invalid because AO tables do not support index
--     scan
/*+
    IndexScan(t1 my_incredible_index)
    IndexScan(t3 our_amazing_index)
 */
EXPLAIN (costs off) SELECT t1.a, t2.a, t3.a FROM my_table AS t1 JOIN your_table AS t2 ON t1.a=t2.a JOIN our_table AS t3 ON t3.a=t2.a WHERE t1.a<42;

-- NB: IndexScan on AO table is invalid because AO tables do not support index
--     scan (e.g. t2)
/*+
    IndexScan(t1)
    IndexScan(t3)
 */
EXPLAIN (costs off) SELECT t1.a, t2.a, t3.a FROM my_table AS t1 JOIN your_table AS t2 ON t1.a=t2.a JOIN our_table AS t3 ON t3.a=t2.a WHERE t1.a<42;

/*+
    IndexOnlyScan(t1 my_incredible_index)
    IndexOnlyScan(t2 your_amazing_index)
    IndexOnlyScan(t3 our_amazing_index)
 */
EXPLAIN (costs off) SELECT t1.a, t2.a, t3.a FROM my_table AS t1 JOIN your_table AS t2 ON t1.a=t2.a JOIN our_table AS t3 ON t3.a=t2.a WHERE t1.a<42;

/*+
    IndexOnlyScan(t1)
    IndexOnlyScan(t2)
    IndexOnlyScan(t3)
 */
EXPLAIN (costs off) SELECT t1.a, t2.a, t3.a FROM my_table AS t1 JOIN your_table AS t2 ON t1.a=t2.a JOIN our_table AS t3 ON t3.a=t2.a WHERE t1.a<42;

/*+
    BitmapScan(t1 my_bitmap_index)
    BitmapScan(t2 your_bitmap_index)
    BitmapScan(t3 our_bitmap_index)
 */
EXPLAIN (costs off) SELECT t1.a, t2.a, t3.a FROM my_table AS t1 JOIN your_table AS t2 ON t1.a=t2.a JOIN our_table AS t3 ON t3.a=t2.a WHERE t1.a<42;

/*+
    BitmapScan(t1)
    BitmapScan(t2)
    BitmapScan(t3)
 */
EXPLAIN (costs off) SELECT t1.a, t2.a, t3.a FROM my_table AS t1 JOIN your_table AS t2 ON t1.a=t2.a JOIN our_table AS t3 ON t3.a=t2.a WHERE t1.a<42;


--------------------------------------------------------------------
--
-- 2. [SCAN] Specific explicit scan type and implicit/explicit index
--
--------------------------------------------------------------------

/*+
    SeqScan(t1)
 */
EXPLAIN SELECT t1.a FROM my_table AS t1 WHERE t1.a<42;

/*+
    SeqScan(t2)
 */
EXPLAIN SELECT t2.a FROM your_table AS t2 WHERE t2.a<42;

/*+
    SeqScan(t3)
 */
EXPLAIN SELECT t3.a FROM our_table AS t3 WHERE t3.a<42;

/*+
    IndexScan(t1 my_incredible_index)
 */
EXPLAIN SELECT t1.a FROM my_table AS t1 WHERE t1.a<42;

/*+
    IndexScan(t1)
 */
EXPLAIN SELECT t1.a FROM my_table AS t1 WHERE t1.a<42;

-- NB: IndexScan on AO table is invalid because AO tables do not support index
--     scan (e.g. t2)
--/*+
--    IndexScan(t2 your_amazing_index)
-- */
--EXPLAIN SELECT t2.a FROM your_table AS t2 WHERE t2.a<42;

-- NB: IndexScan on AO table is invalid because AO tables do not support index
--     scan (e.g. t2)
--/*+
--    IndexScan(t2)
-- */
--EXPLAIN SELECT t2.a FROM your_table AS t2 WHERE t2.a<42;

/*+
    IndexScan(t3 our_amazing_index)
 */
EXPLAIN SELECT t3.a FROM our_table AS t3 WHERE t3.a<42;

/*+
    IndexScan(t3)
 */
EXPLAIN SELECT t3.a FROM our_table AS t3 WHERE t3.a<42;

/*+
    IndexOnlyScan(t1 my_incredible_index)
 */
EXPLAIN SELECT t1.a FROM my_table AS t1 WHERE t1.a<42;

/*+
    IndexOnlyScan(t1)
 */
EXPLAIN SELECT t1.a FROM my_table AS t1 WHERE t1.a<42;

/*+
    IndexOnlyScan(t2 your_amazing_index)
 */
EXPLAIN SELECT t2.a FROM your_table AS t2 WHERE t2.a<42;

/*+
    IndexOnlyScan(t2)
 */
EXPLAIN SELECT t2.a FROM your_table AS t2 WHERE t2.a<42;

/*+
    IndexOnlyScan(t3 our_amazing_index)
 */
EXPLAIN SELECT t3.a FROM our_table AS t3 WHERE t3.a<42;

/*+
    IndexOnlyScan(t3)
 */
EXPLAIN SELECT t3.a FROM our_table AS t3 WHERE t3.a<42;

/*+
    BitmapScan(t1 my_bitmap_index)
 */
EXPLAIN SELECT t1.a FROM my_table AS t1 WHERE t1.a<42;

/*+
    BitmapScan(t1)
 */
EXPLAIN SELECT t1.a FROM my_table AS t1 WHERE t1.a<42;

/*+
    BitmapScan(t2 your_bitmap_index)
 */
EXPLAIN SELECT t2.a FROM your_table AS t2 WHERE t2.a<42;

/*+
    BitmapScan(t2)
 */
EXPLAIN SELECT t2.a FROM your_table AS t2 WHERE t2.a<42;

/*+
    BitmapScan(t3 our_bitmap_index)
 */
EXPLAIN SELECT t3.a FROM our_table AS t3 WHERE t3.a<42;

/*+
    BitmapScan(t3)
 */
EXPLAIN SELECT t3.a FROM our_table AS t3 WHERE t3.a<42;


--------------------------------------------------------------------
--
-- 3. [JOIN] No scan type
--
--------------------------------------------------------------------

/*+
    NoSeqScan(t1)
    NoSeqScan(t2)
    NoSeqScan(t3)
 */
EXPLAIN (costs off) SELECT t1.a, t2.a, t3.a FROM my_table AS t1 JOIN your_table AS t2 ON t1.a=t2.a JOIN our_table AS t3 ON t3.a=t2.a WHERE t1.a<42;

/*+
    NoIndexScan(t1)
    NoIndexScan(t2)
    NoIndexScan(t3)
 */
EXPLAIN (costs off) SELECT t1.a, t2.a, t3.a FROM my_table AS t1 JOIN your_table AS t2 ON t1.a=t2.a JOIN our_table AS t3 ON t3.a=t2.a WHERE t1.a<42;

/*+
    NoIndexOnlyScan(t1)
    NoIndexOnlyScan(t2)
    NoIndexOnlyScan(t3)
 */
EXPLAIN (costs off) SELECT t1.a, t2.a, t3.a FROM my_table AS t1 JOIN your_table AS t2 ON t1.a=t2.a JOIN our_table AS t3 ON t3.a=t2.a WHERE t1.a<42;

/*+
    NoBitmapScan(t1)
    NoBitmapScan(t2)
    NoBitmapScan(t3)
 */
EXPLAIN (costs off) SELECT t1.a, t2.a, t3.a FROM my_table AS t1 JOIN your_table AS t2 ON t1.a=t2.a JOIN our_table AS t3 ON t3.a=t2.a WHERE t1.a<42;


--------------------------------------------------------------------
--
-- 4. [SCAN] No scan type
--
-- In order to perform negative test we disable as many of the alternatives as
-- possible. That means many of these tests will fallback. A couple exceptions
-- are when ORCA does not have a GUC to disable an alternative (e.g. dynamic
-- index scan and dynamic bitmap scan).
--
--------------------------------------------------------------------

SET optimizer_enable_indexscan=off;
SET optimizer_enable_indexonlyscan=off;
SET optimizer_enable_bitmapscan=off;
SET optimizer_enable_tablescan=off;
SET optimizer_enable_dynamictablescan=off;
SET optimizer_enable_dynamicindexonlyscan=off;

RESET optimizer_enable_tablescan;
RESET optimizer_enable_dynamictablescan;

/*+
    NoSeqScan(t1)
 */
EXPLAIN SELECT t1.a FROM my_table AS t1 WHERE t1.a<42;

/*+
    NoSeqScan(t2)
 */
EXPLAIN SELECT t2.a FROM your_table AS t2 WHERE t2.a<42;

/*+
    NoSeqScan(t3)
 */
EXPLAIN SELECT t3.a FROM our_table AS t3 WHERE t3.a<42;

SET optimizer_enable_tablescan=off;
SET optimizer_enable_dynamictablescan=off;
RESET optimizer_enable_indexscan;

/*+
    NoIndexScan(t1)
 */
EXPLAIN SELECT t1.a FROM my_table AS t1 WHERE t1.a<42;

/*+
    NoIndexScan(t2)
 */
EXPLAIN SELECT t2.a FROM your_table AS t2 WHERE t2.a<42;

/*+
    NoIndexScan(t3)
 */
EXPLAIN SELECT t3.a FROM our_table AS t3 WHERE t3.a<42;

SET optimizer_enable_indexscan=off;
RESET optimizer_enable_indexonlyscan;
RESET optimizer_enable_dynamicindexonlyscan;

/*+
    NoIndexOnlyScan(t1)
 */
EXPLAIN SELECT t1.a FROM my_table AS t1 WHERE t1.a<42;

/*+
    NoIndexOnlyScan(t2)
 */
EXPLAIN SELECT t2.a FROM your_table AS t2 WHERE t2.a<42;

/*+
    NoIndexOnlyScan(t3)
 */
EXPLAIN SELECT t3.a FROM our_table AS t3 WHERE t3.a<42;

SET optimizer_enable_indexonlyscan=off;
SET optimizer_enable_dynamicindexonlyscan=off;
RESET optimizer_enable_bitmapscan;

/*+
    NoBitmapScan(t1)
 */
EXPLAIN SELECT t1.a FROM my_table AS t1 WHERE t1.a<42;

/*+
    NoBitmapScan(t2)
 */
EXPLAIN SELECT t2.a FROM your_table AS t2 WHERE t2.a<42;

/*+
    NoBitmapScan(t3)
 */
EXPLAIN SELECT t3.a FROM our_table AS t3 WHERE t3.a<42;

RESET optimizer_enable_indexscan;
RESET optimizer_enable_indexonlyscan;
RESET optimizer_enable_bitmapscan;
RESET optimizer_enable_tablescan;
RESET optimizer_enable_dynamictablescan;
RESET optimizer_enable_dynamicindexonlyscan;

--------------------------------------------------------------------
--
-- 5. [VIEWS] Specific explicit scan type and implicit/explicit index
--
--------------------------------------------------------------------

CREATE VIEW everybody_view AS SELECT t1.a AS a1, t2.a AS a2, t3.a AS a3 FROM my_table AS t1 JOIN your_table AS t2 ON t1.a=t2.a JOIN our_table AS t3 ON t3.a=t2.a WHERE t1.a<42;

/*+
    SeqScan(t1)
    SeqScan(t2)
    SeqScan(t3)
 */
EXPLAIN (costs off) SELECT * FROM everybody_view;

-- NB: IndexScan on AO table is invalid because AO tables do not support index
--     scan (e.g. t2)
/*+
    IndexScan(t1 my_incredible_index)
    IndexScan(t3 our_amazing_index)
 */
EXPLAIN (costs off) SELECT * FROM everybody_view;

-- NB: IndexScan on AO table is invalid because AO tables do not support index
--     scan (e.g. t2)
/*+
    IndexScan(t1)
    IndexScan(t3)
 */
EXPLAIN (costs off) SELECT * FROM everybody_view;

/*+
    IndexOnlyScan(t1 my_incredible_index)
    IndexOnlyScan(t2 your_amazing_index)
    IndexOnlyScan(t3 our_amazing_index)
 */
EXPLAIN (costs off) SELECT * FROM everybody_view;

/*+
    IndexOnlyScan(t1)
    IndexOnlyScan(t2)
    IndexOnlyScan(t3)
 */
EXPLAIN (costs off) SELECT * FROM everybody_view;

/*+
    BitmapScan(t1 my_bitmap_index)
    BitmapScan(t2 your_bitmap_index)
    BitmapScan(t3 our_bitmap_index)
 */
EXPLAIN (costs off) SELECT * FROM everybody_view;

/*+
    BitmapScan(t1)
    BitmapScan(t2)
    BitmapScan(t3)
 */
EXPLAIN (costs off) SELECT * FROM everybody_view;

--------------------------------------------------------------------
--
-- 6. [CTE] Specific explicit scan type and implicit/explicit index
--
--------------------------------------------------------------------

/*+
    SeqScan(t1)
    SeqScan(t2)
    SeqScan(t3)
 */
EXPLAIN (costs off) WITH cte AS
(
    SELECT t1.a AS a1, t2.a AS a2, t3.a AS a3 FROM my_table AS t1 JOIN your_table AS t2 ON t1.a=t2.a JOIN our_table AS t3 ON t3.a=t2.a
)
SELECT a1, a2, a3 FROM cte WHERE a1<42;

-- NB: IndexScan on AO table is invalid because AO tables do not support index
--     scan (e.g. t2)
/*+
    IndexScan(t1 my_incredible_index)
    IndexScan(t3 our_amazing_index)
 */
EXPLAIN (costs off) WITH cte AS
(
    SELECT t1.a AS a1, t2.a AS a2, t3.a AS a3 FROM my_table AS t1 JOIN your_table AS t2 ON t1.a=t2.a JOIN our_table AS t3 ON t3.a=t2.a
)
SELECT a1, a2, a3 FROM cte WHERE a1<42;

-- NB: IndexScan on AO table is invalid because AO tables do not support index
--     scan (e.g. t2)
/*+
    IndexScan(t1)
    IndexScan(t3)
 */
EXPLAIN (costs off) WITH cte AS
(
    SELECT t1.a AS a1, t2.a AS a2, t3.a AS a3 FROM my_table AS t1 JOIN your_table AS t2 ON t1.a=t2.a JOIN our_table AS t3 ON t3.a=t2.a
)
SELECT a1, a2, a3 FROM cte WHERE a1<42;

/*+
    IndexOnlyScan(t1 my_incredible_index)
    IndexOnlyScan(t2 your_amazing_index)
    IndexOnlyScan(t3 our_amazing_index)
 */
EXPLAIN (costs off) WITH cte AS
(
    SELECT t1.a AS a1, t2.a AS a2, t3.a AS a3 FROM my_table AS t1 JOIN your_table AS t2 ON t1.a=t2.a JOIN our_table AS t3 ON t3.a=t2.a
)
SELECT a1, a2, a3 FROM cte WHERE a1<42;

/*+
    IndexOnlyScan(t1)
    IndexOnlyScan(t2)
    IndexOnlyScan(t3)
 */
EXPLAIN (costs off) WITH cte AS
(
    SELECT t1.a AS a1, t2.a AS a2, t3.a AS a3 FROM my_table AS t1 JOIN your_table AS t2 ON t1.a=t2.a JOIN our_table AS t3 ON t3.a=t2.a
)
SELECT a1, a2, a3 FROM cte WHERE a1<42;

/*+
    BitmapScan(t1 my_bitmap_index)
    BitmapScan(t2 your_bitmap_index)
    BitmapScan(t3 our_bitmap_index)
 */
EXPLAIN (costs off) WITH cte AS
(
    SELECT t1.a AS a1, t2.a AS a2, t3.a AS a3 FROM my_table AS t1 JOIN your_table AS t2 ON t1.a=t2.a JOIN our_table AS t3 ON t3.a=t2.a
)
SELECT a1, a2, a3 FROM cte WHERE a1<42;

/*+
    BitmapScan(t1)
    BitmapScan(t2)
    BitmapScan(t3)
 */
EXPLAIN (costs off) WITH cte AS
(
    SELECT t1.a AS a1, t2.a AS a2, t3.a AS a3 FROM my_table AS t1 JOIN your_table AS t2 ON t1.a=t2.a JOIN our_table AS t3 ON t3.a=t2.a
)
SELECT a1, a2, a3 FROM cte WHERE a1<42;


--------------------------------------------------------------------
--
-- 7. Unsupported hints
--
--------------------------------------------------------------------

/*+
    TidScan(t1)
 */
EXPLAIN SELECT t1.a FROM my_table AS t1 WHERE t1.a<42;

/*+
    NoTidScan(t1)
 */
EXPLAIN SELECT t1.a FROM my_table AS t1 WHERE t1.a<42;

/*+
    IndexScanRegexp(t1 '*awesome*')
 */
EXPLAIN SELECT t1.a FROM my_table AS t1 WHERE t1.a<42;

/*+
    IndexOnlyScanRegexp(t1 '*awesome*')
 */
EXPLAIN SELECT t1.a FROM my_table AS t1 WHERE t1.a<42;

/*+
    BitmapScanRegexp(t1 '*awesome*')
 */
EXPLAIN SELECT t1.a FROM my_table AS t1 WHERE t1.a<42;
