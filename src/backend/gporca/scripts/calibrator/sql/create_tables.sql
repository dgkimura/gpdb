--# "set default_table_access_method='ao_row';
CREATE TABLE cal_txtest(id int,
                        btreeunique int,
                        btree10 int,
                        btree100 int,
                        btree1000 int,
                        btree10000 int,
                        bitmap10 int,
                        bitmap100 int,
                        bitmap1000 int,
                        bitmap10000 int,
                        txt text) DISTRIBUTED BY (id);


CREATE TABLE cal_bfvtest (col1 integer,
                          wk_id int,
                          id integer) DISTRIBUTED BY (col1);


CREATE TABLE cal_ndvtest (id int, val int) DISTRIBUTED BY (id);


CREATE TABLE cal_brintest(id int,
                          clust_10 int,
                          clust_100 int,
                          clust_1000 int,
                          clust_10000 int,
                          clust_uniq int,
                          rand_10 int,
                          rand_100 int,
                          rand_1000 int,
                          rand_10000 int,
                          rand_uniq int,
                          txt text) DISTRIBUTED BY (id);


CREATE TABLE cal_temp_ids(f_id int, f_rand double precision) DISTRIBUTED BY (f_id);

CREATE TABLE cal_dim(dim_id int,
                     dim_id2 int,
                     txt text) DISTRIBUTED BY (dim_id);

CREATE TABLE cal_bfv_dim (id integer, col2 integer) DISTRIBUTED BY (id);
