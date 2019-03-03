DROP EXTENSION IF EXISTS pageinspect;
CREATE EXTENSION pageinspect;

DROP TABLE IF EXISTS heaptable;
CREATE TABLE heaptable(a int) DISTRIBUTED BY (a);

INSERT INTO heaptable SELECT i FROM generate_series(1, 10)i;

-- Show items from page on segment
\! PGOPTIONS='-c gp_session_role=utility' psql -p 25432 -d postgres -c "SELECT * FROM heap_page_items(get_raw_page('heaptable', 0));"

\! PGOPTIONS='-c gp_session_role=utility' psql -p 25432 -d postgres -c "SELECT * FROM page_header(get_raw_page('pg_class', 0))";

-- Raw page of unfiltered bits..
-- \! PGOPTIONS='-c gp_session_role=utility' psql -p 25432 -d postgres -c "SELECT * FROM get_raw_page('heaptable', 0);"


DROP TABLE IF EXISTS aotable;
CREATE TABLE aotable(a int) WITH (appendonly=true) DISTRIBUTED BY (a);

INSERT INTO aotable SELECT i FROM generate_series(1, 10)i;

\! PGOPTIONS='-c gp_session_role=utility' psql -p 25432 -d postgres -c "SELECT * FROM get_raw_ao_page('aotable', 0, 1);"
-- Show items from page on segment
-- \! PGOPTIONS='-c gp_session_role=utility' psql -p 25432 -d postgres -c "SELECT * FROM ao_page_items(get_raw_ao_page('aotable', 0));"

-- cdbappendonlystorageread.c:AppendOnlyStorageRead_Init
-- aosegfile.c:GetFileSegInfo(Relation parentrel, Snapshot appendOnlyMetaDataSnapshot, int segno) 
