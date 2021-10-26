alter system set autovacuum = off;
alter system reset autovacuum_max_workers;
select gp_segment_id, pg_reload_conf() from gp_id union select gp_segment_id, pg_reload_conf() from gp_dist_random('gp_id');
