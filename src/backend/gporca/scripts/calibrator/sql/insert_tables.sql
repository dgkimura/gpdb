--\SET num_rows 10000000

-- insert into temp table.
-- integer stop value (suggested value is 10,000,000)
INSERT INTO cal_temp_ids SELECT x, random() FROM (SELECT * FROM generate_series(1,1000000)) T(x);

INSERT INTO cal_txtest
SELECT f_id,
       f_id,
       f_id%10 + 1,
       f_id%100 + 1,
       f_id%1000 + 1,
       f_id%10000 + 1,
       f_id%10 + 1,
       f_id%100 + 1,
       f_id%1000 + 1,
       f_id%10000 + 1,
       repeat('a', 960)
FROM cal_temp_ids
order by f_rand;

INSERT INTO cal_dim SELECT x, x, repeat('d', 100) FROM (SELECT * FROM generate_series(1,10000)) T(x);

-- use a row_number() function to create column values that are strongly correlated
-- to the physical order of the rows on disk
INSERT INTO cal_brintest
SELECT ordered_id,
       ceil(ordered_id*(10.0/1000000)),
       ceil(ordered_id*(100.0/1000000)),
       ceil(ordered_id*(1000.0/1000000)),
       ceil(ordered_id*(10000.0/1000000)),
       ordered_id,
       f_id%10 + 1,
       f_id%100 + 1,
       f_id%1000 + 1,
       f_id%10000 + 1,
       f_id,
       repeat('a', 956)
FROM (select row_number() over(order by f_rand) as ordered_id, f_id, f_rand from cal_temp_ids) src
order by f_rand;
