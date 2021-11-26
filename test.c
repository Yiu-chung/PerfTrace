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
    double rate = -1.0;
    double * rate_addr = &rate;
    send_rate_init(rate_addr, "8.8.8.8","1.1.1.1");
    printf("%lf\n", *rate_addr);
    insert_mode2("test0",1111,"1.1.1.1","8.8.8.8",12300,2200,0.02,13243543355.67);
    return 0;
}