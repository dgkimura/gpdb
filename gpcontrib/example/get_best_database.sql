CREATE FUNCTION get_database_popularity(text) RETURNS text
LANGUAGE C VOLATILE AS '/home/ubuntu/gpdb/gpcontrib/example/get_best_database.so', 'get_database_popularity';
