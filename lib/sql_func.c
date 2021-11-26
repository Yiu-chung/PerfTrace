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

int query(char *DB_name, char *table_name, char *SIP, char *DIP, char *col_name, int mode, int limit){
   sqlite3 *db;
   char *zErrMsg = 0;
   int rc;
   char sql[256] = "SELECT ";
   char limit_str[10];
   char mode_str[5];
   snprintf(limit_str, sizeof(limit_str),"%d",limit); 
   snprintf(mode_str, sizeof(mode_str),"%d",mode); 
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
   strcat(sql, "\' AND mode=");
   strcat(sql, mode_str);
   strcat(sql, " AND ");
   strcat(sql, col_name);
   strcat(sql, " IS NOT NULL ");
   strcat(sql, "ORDER BY Time_stamp DESC LIMIT ");
   strcat(sql, limit_str);
   strcat(sql, ";");
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
    table_is_exist(DATA_BASE, TABLE_NAME);
    if(isExist == 0){
        create_table(DATA_BASE, create_sql);
        *rate_addr = MAXRATE;
        return 0;
    }
    query(DATA_BASE,TABLE_NAME,src_IP,dst_IP,"ABW_sd", 2, 10);
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
    table_is_exist(DATA_BASE, TABLE_NAME);
    if(isExist == 0){
        create_table(DATA_BASE, create_sql);
        return -1.0;
    }
    query(DATA_BASE,TABLE_NAME,src_IP,dst_IP,"ABW_sd", 2, MAX_LIMIT);
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

int insert_mode2(char * task_name, long time_stamp, char * src_IP, char * dst_IP, int owd_sd, float Jitter_sd, float LossRate_sd, float ABW_sd){
   char insert_sql[256] = "INSERT INTO ";
   strcat(insert_sql, TABLE_NAME);
   strcat(insert_sql, " VALUES( NULL , \'");
   strcat(insert_sql, task_name);
   strcat(insert_sql, "\', ");
   char time_stamp_str[20];
   snprintf(time_stamp_str, sizeof(time_stamp_str),"%ld", time_stamp);
   strcat(insert_sql, time_stamp_str);
   strcat(insert_sql, ", \'");
   strcat(insert_sql, src_IP);
   strcat(insert_sql, "\', \'");
   strcat(insert_sql, dst_IP);
   strcat(insert_sql, "\', 2, ");
   char owd_sd_str[10];
   snprintf(owd_sd_str, sizeof(owd_sd_str),"%d", owd_sd);
   strcat(insert_sql, owd_sd_str);
   strcat(insert_sql, ", NULL, NULL, ");
   char jitter_sd_str[16];
   snprintf(jitter_sd_str, sizeof(jitter_sd_str),"%.3f", Jitter_sd);
   strcat(insert_sql, jitter_sd_str);
   strcat(insert_sql, ", NULL, NULL, ");
   char lossrate_sd_str[12];
   snprintf(lossrate_sd_str, sizeof(lossrate_sd_str),"%.9f", LossRate_sd);
   strcat(insert_sql, lossrate_sd_str);
   strcat(insert_sql, ", NULL, NULL, ");
   char abw_sd_str[24];
   snprintf(abw_sd_str, sizeof(abw_sd_str),"%.3f", ABW_sd);
   strcat(insert_sql, abw_sd_str);
   strcat(insert_sql, ", NULL);");
   //printf("%s\n", insert_sql);
   insert(DATA_BASE, insert_sql);
   return 0;
}

int insert_mode1(char * task_name, char * src_IP, char * dst_IP, struct Measurement result){
   char insert_sql[256] = "INSERT INTO ";
   //char *sql1 = "INSERT INTO PerfRecords VALUES( NULL , 'Test1', 1635825644469669 , '8.8.8.8' , '1.1.1.1', 2,101010,NULL, 202020, 456.5, NULL, 890.6, 0.01, NULL, 0.02, 10000000012.0,  NULL);" ;
   snprintf(insert_sql, sizeof(insert_sql),"INSERT INTO %s VALUES(NULL, \'%s\', %ld, \'%s\'", \
      TABLE_NAME, task_name, result.time_stamp, src_IP, );
   strcat(insert_sql, TABLE_NAME);
   strcat(insert_sql, " VALUES( NULL , \'");
   strcat(insert_sql, task_name);
   strcat(insert_sql, "\', ");
   char time_stamp_str[20];
   snprintf(time_stamp_str, sizeof(time_stamp_str),"%ld", result.time_stamp);
   strcat(insert_sql, time_stamp_str);
   strcat(insert_sql, ", \'");
   strcat(insert_sql, src_IP);
   strcat(insert_sql, "\', \'");
   strcat(insert_sql, dst_IP);
   strcat(insert_sql, "\', 2, ");
   char owd_sd_str[10], owd_ds_str;
   snprintf(owd_sd_str, sizeof(owd_sd_str),"%d", result.OWD_sd);
   strcat(insert_sql, owd_sd_str);

   strcat(insert_sql, ", NULL, NULL, ");
   char jitter_sd_str[16];
   snprintf(jitter_sd_str, sizeof(jitter_sd_str),"%.3f", Jitter_sd);
   strcat(insert_sql, jitter_sd_str);
   strcat(insert_sql, ", NULL, NULL, ");
   char lossrate_sd_str[12];
   snprintf(lossrate_sd_str, sizeof(lossrate_sd_str),"%.9f", LossRate_sd);
   strcat(insert_sql, lossrate_sd_str);
   strcat(insert_sql, ", NULL, NULL, ");
   char abw_sd_str[24];
   snprintf(abw_sd_str, sizeof(abw_sd_str),"%.3f", ABW_sd);
   strcat(insert_sql, abw_sd_str);
   strcat(insert_sql, ", NULL);");
   //printf("%s\n", insert_sql);
   insert(DATA_BASE, insert_sql);
   return 0;
   return 0;
}