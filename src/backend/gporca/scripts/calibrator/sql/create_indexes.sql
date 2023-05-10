
CREATE INDEX cal_txtest_i_bitmap_10    ON cal_txtest USING bitmap(bitmap10);
CREATE INDEX cal_txtest_i_bitmap_100   ON cal_txtest USING bitmap(bitmap100);
CREATE INDEX cal_txtest_i_bitmap_1000  ON cal_txtest USING bitmap(bitmap1000);
CREATE INDEX cal_txtest_i_bitmap_10000 ON cal_txtest USING bitmap(bitmap10000);

CREATE INDEX idx_cal_bfvtest_bitmap ON cal_bfvtest USING bitmap(id);

CREATE INDEX cal_ndvtest_bitmap ON cal_ndvtest USING bitmap(val);

CREATE INDEX cal_txtest_i_btree_unique ON cal_txtest USING btree(btreeunique);
CREATE INDEX cal_txtest_i_btree_10     ON cal_txtest USING btree(btree10);
CREATE INDEX cal_txtest_i_btree_100    ON cal_txtest USING btree(btree100);
CREATE INDEX cal_txtest_i_btree_1000   ON cal_txtest USING btree(btree1000);
CREATE INDEX cal_txtest_i_btree_10000  ON cal_txtest USING btree(btree10000);
CREATE INDEX idx_cal_bfvtest_btree ON cal_bfvtest USING btree(id);
CREATE INDEX cal_ndvtest_btree ON cal_ndvtest USING btree(val);

CREATE INDEX cal_brintest_brin ON cal_brintest USING brin(
id, clust_10, clust_100, clust_1000, clust_10000, clust_uniq, rand_10, rand_100, rand_1000, rand_10000, rand_uniq, txt)
WITH(pages_per_range=4);
