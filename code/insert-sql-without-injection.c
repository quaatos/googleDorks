/* 

Insertion in a SQL database (PostgreSQL).

Safe against SQL injections

*/

#define SQL_COMMAND "SELECT * FROM students WHERE name=$1;"
#define PREPARED_STMT "MyInsertion"
#define NPARAMS 1

#include <postgresql/libpq-fe.h>
#include <stdlib.h>

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
    char           *params[NPARAMS];

    if (argc != 2) {
        fatal("Usage: test name");
    }
    params[0] = argv[1];
    conn = PQconnectdb("dbname=essais");
    if (conn == NULL) {
        fatal("Cannot connect to the database (unknown reason)");
    }
    status = PQstatus(conn);
    if (status != CONNECTION_OK) {
        fatal(PQerrorMessage(conn));
    }
    result = PQprepare(conn, PREPARED_STMT, SQL_COMMAND, NPARAMS, NULL);
    if (PQresultStatus(result) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Error, result for statement preparation is %i\n",
                PQresultStatus(result));
        fatal(PQresultErrorMessage(result));
    }
    result =
        PQexecPrepared(conn, PREPARED_STMT, NPARAMS, (const char **) params, NULL,
                       NULL, 0);
    if (PQresultStatus(result) == PGRES_TUPLES_OK) {
        nresults = PQntuples(result);
        fprintf(stdout, "OK, %i tuple(s) pending for '%s'\n", nresults, SQL_COMMAND);
    } else {
        fprintf(stderr, "Error, result for '%s' is %i\n", SQL_COMMAND,
                PQresultStatus(result));
        fatal(PQresultErrorMessage(result));
    }
    PQfinish(conn);
    return 0;
}
