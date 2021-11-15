#include "udp_owamp.h"
#include "./sqlite/sqlite3.h"


int isExist = 0;
double query_res[MAX_LIMIT + 1];
int res_cnt;

static int callback_query(void *NotUsed, int argc, char **argv, char **azColName){
    int i;
    //printf("argc=%d\n",argc);
    for(i=0; i<argc; i++){
        res_cnt ++;
        query_res[res_cnt] = atof(argv[i]);
    }
    return 0;
}

static int callback_exist(void *NotUsed, int argc, char **argv, char **azColName){
    int i;
    for(i=0; i<argc; i++){
        isExist = argv[i] ? atoi(argv[i]) : 0;
    }
    return 0;
}

int table_is_exist(char * DB_name, char * table_name){
   sqlite3 *db;
   char *zErrMsg = 0;
   int  rc;
   char sql[100] = "SELECT count(*) as c FROM sqlite_master where type=\'table\' and name=\'";
   strcat(sql, table_name);
   strcat(sql, "\';");
   /* Open database */
   rc = sqlite3_open(DB_name, &db);
   if( rc ){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      return 0;
   }
   /* Execute SQL statement */
   rc = sqlite3_exec(db, sql, callback_exist, 0, &zErrMsg);
   if( rc != SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
   }
   sqlite3_close(db);
   return isExist;
}

int create_table(char * DB_name, char * sql){
   sqlite3 *db;
   char *zErrMsg = 0;
   int  rc;

   /* Open database */
   rc = sqlite3_open(DB_name, &db);
   if( rc ){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      exit(0);
   }

   /* Execute SQL statement */
   rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);
   if( rc != SQLITE_OK ){
   fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
   }
   sqlite3_close(db);
   return 0; 
}

int insert(char * DB_name, char * sql){
    sqlite3 *db;
   char *zErrMsg = 0;
   int rc;
   /* Open database */
   rc = sqlite3_open(DB_name, &db);
   if( rc ){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      exit(0);
   }

   /* Execute SQL statement */
   rc = sqlite3_exec(db, sql, 0, 0, &zErrMsg);
   if( rc != SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
   }
   sqlite3_close(db);
   return 0;
}

int query(char *DB_name, char *table_name, char *SIP, char *DIP, char *col_name, int limit){
   sqlite3 *db;
   char *zErrMsg = 0;
   int rc;
   char sql[256] = "SELECT ";
   char limit_str[10];
   snprintf(limit_str, sizeof(limit_str),"%d",limit); 
   const char* data = "Callback function called";

   if(limit > MAX_LIMIT){
      fprintf(stderr, "error: limit is too big.\n");
      exit(0);
   }

   /* Open database */
   rc = sqlite3_open(DB_name, &db);
   if( rc ){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      exit(0);
   }

   /* Create SQL statement */
   strcat(sql, col_name);
   strcat(sql, " FROM ");
   strcat(sql, table_name);
   strcat(sql, " WHERE Source_IP=\'");
   strcat(sql, SIP);
   strcat(sql, "\' AND Dest_IP=\'");
   strcat(sql, DIP);
   strcat(sql, "\' AND ");
   strcat(sql, col_name);
   strcat(sql, " IS NOT NULL ");
   strcat(sql, "ORDER BY Time_stamp DESC LIMIT ");
   strcat(sql, limit_str);
   strcat(sql, ";");
   printf("%s\n",sql);


   /* Execute SQL statement */
   rc = sqlite3_exec(db, sql, callback_query, (void*)data, &zErrMsg);
   if( rc != SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
   }
   sqlite3_close(db);
   return 0;
}