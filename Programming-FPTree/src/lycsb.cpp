/**
lycsb.cpp

Author: Ben Tsai ( @BenTsai7 )

Descrption:run leveldb and test it with YCSB Benchmark workloads

**/
#include <leveldb/db.h>
#include <string>
#include <stdlib.h>
#define KEY_LEN 8
#define VALUE_LEN 8
using namespace std;

const string workload = "../../workloads/";

const string load = workload + "220w-rw-50-50-load.txt"; // TODO: the workload_load filename
const string run  = workload + "220w-rw-50-50-run.txt"; // TODO: the workload_run filename

const string filePath = "leveldb";

const int READ_WRITE_NUM = 350000; // TODO: how many operations
void removeFile(){
	system("rm -rf leveldb");	
};
int main()
{        
    leveldb::DB* db;
    leveldb::Options options;
    leveldb::WriteOptions write_options;

    // TODO: open and initial the levelDB
    //
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options,filePath,&db);
    if(!status.ok()){
        printf("Open LevelDB Error!\n");
        return 0;
    }
    //

    uint64_t inserted = 0, queried = 0, t = 0;
    uint64_t* key = new uint64_t[2200000]; // the key and value are same
    bool* ifInsert = new bool[2200000]; // the operation is insertion or not
	FILE *ycsb_load, *ycsb_run; // the files that store the ycsb operations
	char *buf = NULL;
	size_t len = 0;
    struct timespec start, finish; // use to caculate the time
    double single_time; // single operation time

    printf("Load phase begins \n");

    // TODO: read the ycsb_load and store
    //
    ycsb_load = fopen(load.c_str(),"r");
    char operation[max(strlen("INSERT"),strlen("READ"))+1];
    int count=0;
    printf("%s\n",load.c_str());
    while(fscanf(ycsb_load,"%s %lu",operation,&key[count])!=EOF){
        ifInsert[count] = (strcmp(operation,"INSERT")==0);
        ++count;
    }
    //
    clock_gettime(CLOCK_MONOTONIC, &start);

    // TODO: load the workload in LevelDB
    //
    for(int i=0;i<count;++i){
        //keys stored in LevelDB are (slices)strings or chars
        //to store our uint64_t key, transform it to chars firstly.
        char dbkey[8/sizeof(char)];
        string tmp;
        memcpy(&dbkey,&key[i],8);
        if(ifInsert[i]){
            status = db->Put(write_options,dbkey,dbkey);
            if(!status.ok()){
                printf("Insert key-value pair Error!");
                return 0;
            }
            ++inserted;
        }
        else{
            status = db->Get(leveldb::ReadOptions(),dbkey,&tmp);
            if(!status.ok()){
                printf("Read key-value pair Error!");
                return 0;
            }
            ++queried;
        }
    }
    //

    clock_gettime(CLOCK_MONOTONIC, &finish);
	single_time = (finish.tv_sec - start.tv_sec) * 1000000000.0 + (finish.tv_nsec - start.tv_nsec);

    printf("Load phase finishes: %lu items are inserted \n", inserted);
    printf("Load phase used time: %fs\n", single_time / 1000000000.0);
    printf("Load phase single insert time: %fns\n", single_time / inserted);

	int operation_num = 0;
    inserted = 0;		

    // TODO:read the ycsb_run and store
    //
    ycsb_run = fopen(run.c_str(),"r");
    count=0;
    while(fscanf(ycsb_run,"%s %lu",operation,&key[count])!=EOF){
        ifInsert[count] = (strcmp(operation,"INSERT")==0);
        ++count;
    }
    //

    clock_gettime(CLOCK_MONOTONIC, &start);

    // TODO: operate the levelDB
    //
    operation_num=count;
    queried=0;
    for(int i=0;i<count;++i){
        char dbkey[8/sizeof(char)];
        string tmp;
        memcpy(&dbkey,&key[i],8);
        if(ifInsert[i]){
            status = db->Put(write_options,dbkey,dbkey);
            if(!status.ok()){
                printf("Insert key-value pair Error!");
                return 0;
            }
            ++inserted;
        }
        else{
            status = db->Get(leveldb::ReadOptions(),dbkey,&tmp);      
            if(!status.ok()){
                printf("Read key-value pair Error!");
                return 0;
            }
            ++queried;
        }
    }
    //

	clock_gettime(CLOCK_MONOTONIC, &finish);
	single_time = (finish.tv_sec - start.tv_sec) + (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("Run phase finishes: %lu/%lu items are inserted/searched\n", operation_num - inserted, inserted);
    printf("Run phase throughput: %f operations per second \n", READ_WRITE_NUM/single_time);
    removeFile();
    return 0;
}
