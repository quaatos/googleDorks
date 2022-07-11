#!/usr/bin/env python
import psycopg2, psycopg2.extensions #Enter in terminal --> pip3 install psycopg2
import time

def run(c):
    c.execute("SELECT * FROM foo WHERE id=1");
    for tuple in c.fetchall():
        print (tuple)

connection = psycopg2.connect("dbname=essais")
cursor = connection.cursor()
# Default is ISOLATION_LEVEL_READ_COMMITTED
connection.set_isolation_level(psycopg2.extensions.ISOLATION_LEVEL_SERIALIZABLE)
cursor.execute("BEGIN;")
for i in range(1, 4):
    run(cursor)
    time.sleep(5)
cursor.execute("COMMIT;")
cursor.close()
connection.close()
