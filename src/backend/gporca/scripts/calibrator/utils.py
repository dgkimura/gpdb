import re

import settings


try:
    from gppylib.db import dbconn
except ImportError as e:
    sys.exit('ERROR: Cannot import modules.  Please check that you have sourced greenplum_path.sh.  Detail: ' + str(e))


def log_output(str):
    if settings.glob_verbose:
        print(str)
    if settings.glob_log_file != None:
        settings.glob_log_file.write(str + "\n")


def connect(host, port_num, db_name):
    try:
        dburl = dbconn.DbURL(hostname=host, port=port_num, dbname=db_name)
        conn = dbconn.connect(dburl, encoding="UTF8", unsetSearchPath=False)

    except Exception as e:
        print(("Exception during connect: %s" % e))
        quit()

    return conn


def select_version(conn):
    sqlStr = "SELECT version()"
    curs = dbconn.query(conn, sqlStr)

    rows = curs.fetchall()
    for row in rows:
        log_output(row[0])
        settings.glob_gpdb_major_version = int(re.sub(".*Greenplum Database ([0-9]*)\..*", "\\1", row[0]))
        log_output("GPDB major version is %d" % settings.glob_gpdb_major_version)

    log_output("Backend pid:")
    sqlStr = "SELECT pg_backend_pid()"
    curs = dbconn.query(conn, sqlStr)

    rows = curs.fetchall()
    for row in rows:
        log_output(str(row[0]))


def execute_sql_arr(conn, sqlStrArr):
    for sqlStr in sqlStrArr:
        execute_sql(conn, sqlStr)


def execute_sql(conn, sqlStr):
    try:
        log_output("")
        log_output("Executing query: %s" % sqlStr)
        dbconn.execSQL(conn, sqlStr)
    except Exception as e:
        print("")
        print(("Error executing query: %s; Reason: %s" % (sqlStr, e)))
        dbconn.execSQL(conn, "abort")


def run_query(conn, sqlStr):
    curs = dbconn.query(conn, sqlStr)
    return curs.fetchall()


def select_first_int(conn, sqlStr):
    try:
        log_output("")
        log_output("Executing query: %s" % sqlStr)
        curs = dbconn.query(conn, sqlStr)
        rows = curs.fetchall()
        for row in rows:
            return int(row[0])

    except Exception as e:
        print("")
        print(("Error executing query: %s; Reason: %s" % (sqlStr, e)))
        dbconn.execSQL(conn, "abort")


# run an SQL statement and return the elapsed wallclock time, in seconds
def timed_execute_sql(conn, sqlStr):
    start = time.time()
    num_rows = select_first_int(conn, sqlStr)
    end = time.time()
    elapsed_time_in_msec = round((end - start) * 1000)
    log_output("Elapsed time (msec): %d, rows: %d" % (elapsed_time_in_msec, num_rows))
    return elapsed_time_in_msec, num_rows


# run an SQL statement n times, unless it takes longer than a timeout

def timed_execute_n_times(conn, sqlStr, exec_n_times):
    sum_exec_times = 0.0
    sum_square_exec_times = 0.0
    e = 0
    act_num_exes = exec_n_times
    num_rows = -1
    while e < act_num_exes:
        exec_time, local_num_rows = timed_execute_sql(conn, sqlStr)
        e = e + 1
        sum_exec_times += exec_time
        sum_square_exec_times += exec_time * exec_time
        if num_rows >= 0 and local_num_rows != num_rows:
            log_output("Inconsistent number of rows returned: %d and %d" % (num_rows, local_num_rows))
        num_rows = local_num_rows
        if exec_time > glob_exe_timeout:
            # we exceeded the timeout, don't keep executing this long query
            act_num_exes = e
            log_output("Query %s exceeded the timeout of %d seconds" % (sqlStr, glob_exe_timeout))

    # compute mean and standard deviation of the execution times
    mean = sum_exec_times / act_num_exes
    if exec_n_times == 1:
        # be safe, avoid any rounding errors
        variance = 0.0
    else:
        variance = sum_square_exec_times / act_num_exes - mean * mean
    return (round(mean, 3), round(math.sqrt(variance), 3), act_num_exes, num_rows)
