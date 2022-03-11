#include "udp_owamp.h"
#include "./sqlite/sqlite3.h"



extern char database_path[PATH_LEN];
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

int query(char *DB_name, char *table_name, char *SIP, char *DIP, char *col_name, int mode, int limit){
   sqlite3 *db;
   char *zErrMsg = 0;
   int rc;
   char sql[256] = "SELECT ";
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
   snprintf(sql, sizeof(sql),"SELECT %s FROM %s WHERE Source_IP=\'%s\' AND Dest_IP=\'%s\' AND mode=%d AND %s IS NOT NULL ORDER BY Time_stamp DESC LIMIT %d;",\
      col_name, table_name, SIP, DIP, mode, col_name, limit); 
   printf("%s\n",sql);


   /* Execute SQL statement */
   res_cnt = 0;
   rc = sqlite3_exec(db, sql, callback_query, (void*)data, &zErrMsg);
   if( rc != SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
   }
   sqlite3_close(db);
   return 0;
}

int send_rate_init(double *rate_addr, char * src_IP, char * dst_IP){
    if (*rate_addr > 0) return 0;
    memset(query_res, 0, sizeof(query_res));
    isExist = 0;
    printf("database: %s\n", database_path);
    table_is_exist(database_path, TABLE_NAME);
    if(isExist == 0){
        create_table(database_path, create_sql);
        *rate_addr = MAXRATE;
        return 0;
    }
    query(database_path,TABLE_NAME,src_IP,dst_IP,"ABW_sd", 2, 10);
    if (res_cnt == 0){
       *rate_addr = MAXRATE;
       return 0;
    }
    *rate_addr = query_res[1] * 1.2;
    int i;
    for(i=2; i<res_cnt+1; i++){
       *rate_addr = max(*rate_addr, query_res[i]);
    }
    return 0;
}

float get_max_AB(char * src_IP, char * dst_IP){
    memset(query_res, 0, sizeof(query_res));
    table_is_exist(database_path, TABLE_NAME);
    if(isExist == 0){
        create_table(database_path, create_sql);
        return -1.0;
    }
    query(database_path,TABLE_NAME,src_IP,dst_IP,"ABW_sd", 2, MAX_LIMIT);
    if (res_cnt == 0){
       return -1.0;
    }
    double max_AB = query_res[1];
    int i;
    for(i=2; i<res_cnt+1; i++){
       max_AB = max(max_AB, query_res[i]);
    }
    return max_AB;
}
//int owd_sd, float Jitter_sd, float LossRate_sd, float ABW_sd
int insert_mode2(char * task_name, char * src_IP, char * dst_IP, struct Measurement result){
   char insert_sql[256];
   snprintf(insert_sql, sizeof(insert_sql),"INSERT INTO %s VALUES(NULL, \'%s\', %ld, \'%s\', \'%s\', %d, %d, NULL, NULL, %.3f, NULL, NULL, %.6f, NULL, NULL, %3f, NULL);", \
      TABLE_NAME, task_name, result.time_stamp, src_IP, dst_IP, result.mode, result.OWD_sd,\
      result.Jitter_sd, result.LossRate_sd, result.ABW_sd);
   printf("%s\n", insert_sql);
   insert(database_path, insert_sql);
   return 0;
}

int insert_mode1(char * task_name, char * src_IP, char * dst_IP, struct Measurement result){
   char insert_sql[256] = "INSERT INTO ";
   //char *sql1 = "INSERT INTO PerfRecords VALUES( NULL , 'Test1', 1635825644469669 , '8.8.8.8' , '1.1.1.1', 2,101010,NULL, 202020, 456.5, NULL, 890.6, 0.01, NULL, 0.02, 10000000012.0,  NULL);" ;
   snprintf(insert_sql, sizeof(insert_sql),"INSERT INTO %s VALUES(NULL, \'%s\', %ld, \'%s\', \'%s\', %d, %d, %d, %d, %.3f, %.3f, %.3f, %.6f, %.6f, %.6f, NULL, NULL);", \
      TABLE_NAME, task_name, result.time_stamp, src_IP, dst_IP, result.mode, result.OWD_sd, result.OWD_ds, result.RTT,\
      result.Jitter_sd, result.Jitter_ds, result.Jitter_rtt, result.LossRate_sd, result.LossRate_ds, result.LossRate_rtt);
   printf("%s\n", insert_sql);
   insert(database_path, insert_sql);
   return 0;
}