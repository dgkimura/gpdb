-- Given that we built with and have zlib compression available
-- Test basic create table for AO/RO table succeeds for zlib compression
create table a_aoro_table_with_zlib_compression(col text) WITH (APPENDONLY=true, COMPRESSTYPE=zlib, compresslevel=1);
select pg_size_pretty(pg_relation_size('a_aoro_table_with_zlib_compression')),
       get_ao_compression_ratio('a_aoro_table_with_zlib_compression');
insert into a_aoro_table_with_zlib_compression values('ksjdhfksdhfksdhfksjhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh');
select pg_size_pretty(pg_relation_size('a_aoro_table_with_zlib_compression')),
       get_ao_compression_ratio('a_aoro_table_with_zlib_compression');


-- Given that we built with and have zstd compression available
-- Test basic create table for AO/RO table succeeds for zstd compression
create table a_aoro_table_with_zstd_compression(col text) WITH (APPENDONLY=true, COMPRESSTYPE=zstd, compresslevel=1);
select pg_size_pretty(pg_relation_size('a_aoro_table_with_zstd_compression')),
       get_ao_compression_ratio('a_aoro_table_with_zstd_compression');
insert into a_aoro_table_with_zstd_compression values('ksjdhfksdhfksdhfksjhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh');
select pg_size_pretty(pg_relation_size('a_aoro_table_with_zstd_compression')),
       get_ao_compression_ratio('a_aoro_table_with_zstd_compression');


-- Given that we built with and have quicklz compression available
-- Test basic create table for AO/RO table succeeds for quicklz compression
create table a_aoro_table_with_quicklz_compression(col text) WITH (APPENDONLY=true, COMPRESSTYPE=quicklz, compresslevel=1);
select pg_size_pretty(pg_relation_size('a_aoro_table_with_quicklz_compression')),
       get_ao_compression_ratio('a_aoro_table_with_quicklz_compression');
insert into a_aoro_table_with_quicklz_compression values('ksjdhfksdhfksdhfksjhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh');
select pg_size_pretty(pg_relation_size('a_aoro_table_with_quicklz_compression')),
       get_ao_compression_ratio('a_aoro_table_with_quicklz_compression');


-- Test basic create table for AO/RO table fails for rle compression. rle is only supported for columnar tables.
create table a_aoro_table_with_rle_type_compression(col text) WITH (APPENDONLY=true, COMPRESSTYPE=rle_type, compresslevel=1);
