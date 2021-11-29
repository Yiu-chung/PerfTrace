#include "./lib/sql_func.c"
#include "output.c"

int main(){
    /*int a = table_is_exist(DATA_BASE, TABLE_NAME);
    if(a == 0){
        create_table(DATA_BASE, create_sql);
    }
    int b = table_is_exist(DATA_BASE, TABLE_NAME);
    printf("%d %d\n", a, b);
    char *sql1 = "INSERT INTO PerfRecords VALUES( NULL , 'Test1', 1635825644469669 , '8.8.8.8' , '1.1.1.1', 2,101010,NULL, 202020, 456.5, NULL, 890.6, 0.01, NULL, 0.02, 10000000012.0,  NULL);" ;
    char *sql2 = "INSERT INTO PerfRecords VALUES( NULL , 'Test1', 1635825644469666 , '8.8.8.8' , '1.1.1.1', 2,101010,NULL, 202020, 456.5, NULL, 890.6, 0.01, NULL, 0.02, 5000012.0,  NULL);" ;
    insert(DATA_BASE, sql1);
    insert(DATA_BASE, sql2);
    query(DATA_BASE,"PerfRecords","8.8.8.8","1.1.1.1","ABW_sd", 2, 100);
    int i;
    for (i = 1; i <= res_cnt; i++)
    {
        printf("%f\n", query_res[i]);
    }*/
    query(DATA_BASE,"PerfRecords","8.8.8.8","1.1.1.1","ABW_sd", 2, 100);
    double rate = -1.0;
    double * rate_addr = &rate;
    send_rate_init(rate_addr, "8.8.8.8","1.1.1.1");
    printf("%lf\n", *rate_addr);
    struct Measurement meas_res;
    memset(&meas_res, 0, sizeof(meas_res));
    meas_res.mode = 2;
    insert_mode2("test0","1.1.1.1","8.8.8.8", meas_res);
    memset(&meas_res, 0, sizeof(meas_res));
    meas_res.mode = 1;
    insert_mode1("ttt","11.11.11.11","2.2.2.2", meas_res);

    return 0;
}