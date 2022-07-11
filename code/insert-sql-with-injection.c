/* 

Insertion in a SQL database (PostgreSQL).

CODE VULNERABLE TO SQL INJECTIONS. ONLY FOR DEMONSTRATION. 

*/

#define SQL_COMMAND "SELECT * FROM students WHERE name='%s';"

#include <postgresql/libpq-fe.h>
#include <stdlib.h>

#define MAX_SQL_SIZE 1024

void
fatal(char *msg)
{
    fprintf(stderr, "Fatal error: %s\n", msg);
    exit(1);
}

int
main(argc, argv)
    int             argc;
    char          **argv;
{

    PGconn         *conn = NULL;
    ConnStatusType  status;
    PGresult       *result;
    int             nresults;
    char            sql_command[MAX_SQL_SIZE];

    if (argc != 2) {
        fatal("Usage: test name");
    }
    conn = PQconnectdb("dbname=essais");
    if (conn == NULL) {
        fatal("Cannot connect to the database (unknown reason)");
    }
    status = PQstatus(conn);
    if (status != CONNECTION_OK) {
        fatal(PQerrorMessage(conn));
    }
	/* SECURITY ALERT: this is bad, we should use prepared statements */
    snprintf(sql_command, MAX_SQL_SIZE, SQL_COMMAND, argv[1]);
    result = PQexec(conn, sql_command);
    if (PQresultStatus(result) == PGRES_TUPLES_OK) {
        nresults = PQntuples(result);
        fprintf(stdout, "OK, %i tuple(s) pending for '%s'\n", nresults, sql_command);
    } else {
        fprintf(stdout, "Error, result for '%s' is %i\n", sql_command,
                PQresultStatus(result));
        fatal(PQresultErrorMessage(result));
    }
    PQfinish(conn);
    return 0;
}
