/**
lycsb.cpp

Author: Ben Tsai ( @BenTsai7 )

Descrption:run leveldb and fptree, test them with YCSB Benchmark workloads

**/
#include "fptree/fptree.h"
#include <leveldb/db.h>
#include <string>
#include <stdlib.h>

#define KEY_LEN 8
#define VALUE_LEN 8
using namespace std;

const string workload = "../../workloads/"; // TODO: the workload folder filepath

const string load = workload + "10w-rw-50-50-load.txt"; // TODO: the workload_load filename
const string run  = workload + "10w-rw-50-50-run.txt"; // TODO: the workload_run filename

const int READ_WRITE_NUM = 100000; // TODO: amount of operations

const string catalogPath = DATA_DIR + "p_allocator_catalog";
const string freePath = DATA_DIR + "free_list";
void removeFile(){
	uint64_t maxfile = PAllocator::getAllocator()->getMaxFileId();
    PAllocator::getAllocator()->~PAllocator();
    for(uint64_t i=1;i<maxfile;++i){
    const string file = DATA_DIR + to_string(i);
    remove(file.c_str());
    }
    remove(catalogPath.c_str());
    remove(freePath.c_str());
	system("rm -rf leveldb");	
};
int main()
{        
    FPTree fptree(1028);
    uint64_t inserted = 0, queried = 0, t = 0;
    uint64_t* key = new uint64_t[2200000];
    bool* ifInsert = new bool[2200000];
	FILE *ycsb_load, *ycsb_run;
	char *buf = NULL;
	size_t len = 0;
    struct timespec start, finish;
    double single_time;

    printf("===================FPtreeDB===================\n");
    printf("Load phase begins \n");

    // TODO: read the ycsb_load
    ycsb_load = fopen(load.c_str(),"r");
    char operation[max(strlen("INSERT"),strlen("READ"))+1];
    int count=0;
     while(fscanf(ycsb_load,"%s %lu",operation,&key[count])!=EOF){
        ifInsert[count] = (strcmp(operation,"INSERT")==0);
        ++count;
    }
    clock_gettime(CLOCK_MONOTONIC, &start);
    

    // TODO: load the workload in the fptree
    for(int i=0;i<count;++i){
        //keys stored in LevelDB are (slices)strings or chars
        //to store our uint64_t key, transform it to chars firstly.
        if(ifInsert[i]){
            fptree.insert(key[i],key[i]);
            ++inserted;
        }
        else{
           fptree.find(key[i]);
            ++queried;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &finish);
	single_time = (finish.tv_sec - start.tv_sec) * 1000000000.0 + (finish.tv_nsec - start.tv_nsec);
    printf("Load phase finishes: %lu items are inserted \n", inserted);
    printf("Load phase used time: %fs\n", single_time / 1000000000.0);
    printf("Load phase single insert time: %fns\n", single_time / inserted);

	printf("Run phase begins\n");

	int operation_num = 0;
    inserted = 0;		
    // TODO: read the ycsb_run
    ycsb_run = fopen(run.c_str(),"r");
    count=0;
    while(fscanf(ycsb_run,"%s %lu",operation,&key[count])!=EOF){
        ifInsert[count] = (strcmp(operation,"INSERT")==0);
        ++count;
    }
    clock_gettime(CLOCK_MONOTONIC, &start);

    // TODO: operate the fptree
    operation_num=count;
    queried=0;
    for(int i=0;i<count;++i){
        if(ifInsert[i]){
            fptree.insert(key[i],key[i]);
            ++inserted;
        }
        else{
            fptree.find(key[i]);
            ++queried;
        }
    }
	clock_gettime(CLOCK_MONOTONIC, &finish);
	single_time = (finish.tv_sec - start.tv_sec) + (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("Run phase finishes: %lu/%lu items are inserted/searched\n", inserted, operation_num - inserted);
    printf("Run phase throughput: %f operations per second \n", READ_WRITE_NUM/single_time);	
    fclose(ycsb_load);
    fclose(ycsb_run);
    // LevelDB
    printf("===================LevelDB====================\n");
    const string filePath = "leveldb"; // data storing folder(NVM)

    memset(key, 0, 2200000);
    memset(ifInsert, 0, 2200000);

    leveldb::DB* db;
    leveldb::Options options;
    leveldb::WriteOptions write_options;
    // TODO: initial the levelDB
    options.create_if_missing = true;
    leveldb::Status status = leveldb::DB::Open(options,filePath,&db);
    if(!status.ok()){
        printf("Open LevelDB Error!\n");
        return 0;
    }
    inserted = 0;
    printf("Load phase begins \n");
    // TODO: read the ycsb_read
    count=0;
    ycsb_load = fopen(load.c_str(),"r");
    printf("%s\n",load.c_str());
    while(fscanf(ycsb_load,"%s %lu",operation,&key[count])!=EOF){
        ifInsert[count] = (strcmp(operation,"INSERT")==0);
        ++count;
    }
    clock_gettime(CLOCK_MONOTONIC, &start);
    // TODO: load the levelDB
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
    clock_gettime(CLOCK_MONOTONIC, &finish);
	single_time = (finish.tv_sec - start.tv_sec) * 1000000000.0 + (finish.tv_nsec - start.tv_nsec);
    printf("Load phase finishes: %lu items are inserted \n", inserted);
    printf("Load phase used time: %fs\n", single_time / 1000000000.0);
    printf("Load phase single insert time: %fns\n", single_time / inserted);

	printf("Run phase begin\n");
	operation_num = 0;
    inserted = 0;		
    // TODO: read the ycsb_run
    count=0;
    ycsb_run = fopen(run.c_str(),"r");
    while(fscanf(ycsb_run,"%s %lu",operation,&key[count])!=EOF){
        ifInsert[count] = (strcmp(operation,"INSERT")==0);
        ++count;
    }
    clock_gettime(CLOCK_MONOTONIC, &start);

    // TODO: run the workload_run in levelDB
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
	clock_gettime(CLOCK_MONOTONIC, &finish);
	single_time = (finish.tv_sec - start.tv_sec) + (finish.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("Run phase finishes: %lu/%lu items are inserted/searched\n", inserted, operation_num - inserted);
    printf("Run phase throughput: %f operations per second \n", READ_WRITE_NUM/single_time);
    fclose(ycsb_load);
    fclose(ycsb_run);
    removeFile();
    return 0;
}
