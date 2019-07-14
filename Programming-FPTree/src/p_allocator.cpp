/**
p_allocator.cpp

Author: Ben Tsai ( @BenTsai7 )

Descrption:using pmem library to allocate memories for leafs and stored data in leaf-group files.

**/
#include"utility/p_allocator.h"
#include<iostream>
#include<string.h>
#include<assert.h>
using namespace std;

// the file that store the information of allocator
const string P_ALLOCATOR_CATALOG_NAME = "p_allocator_catalog";
// a list storing the free leaves
const string P_ALLOCATOR_FREE_LIST    = "free_list";

PAllocator* PAllocator::pAllocator = new PAllocator();

PAllocator* PAllocator::getAllocator() {
    if (PAllocator::pAllocator == NULL) {
        PAllocator::pAllocator = new PAllocator();
    }
    return PAllocator::pAllocator;
}

/* data storing structure of allocator
   In the catalog file, the data structure is listed below
   | maxFileId(8 bytes) | freeNum = m | treeStartLeaf(the PPointer) |
   In freeList file:
   | freeList{(fId, offset)1,...(fId, offset)m} |
*/
PAllocator::PAllocator() {
    string allocatorCatalogPath = DATA_DIR + P_ALLOCATOR_CATALOG_NAME;
    string freeListPath         = DATA_DIR + P_ALLOCATOR_FREE_LIST;
    ifstream allocatorCatalog(allocatorCatalogPath, ios::in|ios::binary);
    ifstream freeListFile(freeListPath, ios::in|ios::binary);
    // judge if the catalog exists

    //
    if (allocatorCatalog.is_open() && freeListFile.is_open()) {
        // exist
        // TODO
        //read AllocatorCatalog
        //no need for using pmem to read
        allocatorCatalog.read((char*)&(maxFileId), sizeof(uint64_t));
        allocatorCatalog.read((char*)&(freeNum), sizeof(uint64_t));
        allocatorCatalog.read((char*)&(startLeaf), sizeof(PPointer));
        //read FreeListFile
        PPointer flp;
        for(unsigned int i=0;i<this->freeNum;++i){
            freeListFile.read((char*)&(flp), sizeof(PPointer));
            freeList.push_back(flp);
            }
        allocatorCatalog.close();
        freeListFile.close();
        }
        else {
        // not exist, create catalog and free_list file, then open.
        // TODO
        ofstream filecreater; //using ios::out will create files.
        filecreater.open(allocatorCatalogPath, ios::out|ios::binary);
        assert(filecreater.is_open());
        //initial maxFileId freeNum and startLeaf
        maxFileId=1;
        freeNum=0;
        startLeaf.fileId=0;
        startLeaf.offset=LEAF_GROUP_HEAD;
        //write back to catalog
        filecreater.write((char*)&(maxFileId), sizeof(uint64_t));
        filecreater.write((char*)&(freeNum), sizeof(uint64_t));
        filecreater.write((char*)&(startLeaf), sizeof(PPointer));
        filecreater.close();
        filecreater.open(freeListPath, ios::out|ios::binary);
        assert(filecreater.is_open());
        filecreater.close();
    }
    this->initFilePmemAddr();
}

PAllocator::~PAllocator() {
    // TODO
    auto it= fId2PmAddr.begin();
    //traversal to sync and unmap leaf groups
    while(it != fId2PmAddr.end())
    {
        int leaf_group_len = 8 + LEAF_GROUP_AMOUNT + LEAF_GROUP_AMOUNT*calLeafSize();
        /*The pmem_msync() function is like pmem_persist() in that it forces any changes in the range [addr, addr+len) to be stored durably. 
        Since it calls msync(), this function works on either persistent memory or a memory mapped file on traditional storage. 
        so it's not necessary for us to judge on using pmem_persist() or pmem_msync()
        */
        pmem_msync(it->second,leaf_group_len);//sync the whole leaf group
        pmem_unmap(it->second,leaf_group_len);
        ++it;         
    }
    persistCatalog();
    if(PAllocator::pAllocator!=NULL) PAllocator::pAllocator=NULL;
}

// memory map all leaves to pmem address, storing them in the fId2PmAddr
void PAllocator::initFilePmemAddr() {
    // TODO
    int is_pmem;
    size_t mapped_len;
    int leaf_group_len = 8 + LEAF_GROUP_AMOUNT + LEAF_GROUP_AMOUNT*calLeafSize();//caculate the lenghth of a leaf group
    for(uint64_t i=1;i<maxFileId;++i){
        char *pmemaddr;
        string filePath = DATA_DIR + to_string(i);//transfer file name from uint64_t to string
        if ((pmemaddr = (char*)pmem_map_file(filePath.c_str(), leaf_group_len, PMEM_FILE_CREATE,
			0666, &mapped_len, &is_pmem)) == NULL) {
                assert(pmemaddr!=NULL);
                }
        fId2PmAddr.insert(pair<uint64_t, char*>(i,pmemaddr));
    }
}

// get the pmem address of the target PPointer from the map fId2PmAddr
char* PAllocator::getLeafPmemAddr(PPointer p){
    // TODO
    map<uint64_t, char*>::iterator iter=fId2PmAddr.find(p.fileId);
    if(iter==fId2PmAddr.end()) return NULL;
    char* addr = iter->second;//can't not find file in the map
    if(p.offset>LEAF_GROUP_HEAD+(LEAF_GROUP_AMOUNT-1)*calLeafSize()) return NULL; //overflow of leaf group
    return addr+p.offset;
}

// get and use a leaf for the fptree leaf allocation
// return
bool PAllocator::getLeaf(PPointer &p, char* &pmem_addr) {
    // TODO
    if(freeList.empty()){ //no free leaves to use, so allocate new leaf group
        bool res=newLeafGroup();
        if(!res)return false;//allocate new leaf group failed
    }
    p.fileId = freeList[freeList.size()-1].fileId; //take out the last free leaf
    p.offset = freeList[freeList.size()-1].offset;
    if((pmem_addr=getLeafPmemAddr(p))==NULL) return false;
    auto it=freeList.end();
    --it;
    freeList.erase(it);//delete the last free leaf from freelist
    //update bitmap and usedNum in LeafGroup
    map<uint64_t, char*>::iterator iter=fId2PmAddr.find(p.fileId);
    char* addr = iter->second;
    uint64_t usedNum;
    //take out usedNum
    memcpy(&usedNum,addr,sizeof(uint64_t));
    ++usedNum;//usedNum + 1
    memcpy(addr,&usedNum,sizeof(uint64_t)); //writeback
    int p_num=(p.offset-LEAF_GROUP_HEAD)/calLeafSize();//caculate the position of this leaf and set bitmap to use
    addr = addr + 8 + p_num;//go to the bitmap
    *addr = 1;//setting bitmap to 1 presents that the leaf is being used.
    addr = addr - 8 - p_num; //return to the leaf group header
    int leaf_group_len = LEAF_GROUP_HEAD + LEAF_GROUP_AMOUNT*calLeafSize();

    /*The pmem_msync() function is like pmem_persist() in that it forces any changes in the range [addr, addr+len) to be stored durably. 
    Since it calls msync(), this function works on either persistent memory or a memory mapped file on traditional storage. 
    so it's not necessary for us to judge on using pmem_persist() or pmem_msync()
    */
    pmem_msync(addr,leaf_group_len);//sync the whole leaf group
    --freeNum;
    return true;
}

bool PAllocator::ifLeafUsed(PPointer p) {
    // TODO
    if(!ifLeafExist(p)) return false;
    map<uint64_t, char*>::iterator iter=fId2PmAddr.find(p.fileId);
    if(iter==fId2PmAddr.end()) return false;
    char* addr = iter->second;
    int p_num=(p.offset-LEAF_GROUP_HEAD)/calLeafSize();//caculate the position of this leaf and set bitmap to use
    addr = addr + 8 + p_num; //to bitmap;
    return *addr==1; //0 in bitmap presents free leaf.
}

bool PAllocator::ifLeafFree(PPointer p){
    // TODO
    if(!ifLeafExist(p)) return false;
    return !ifLeafUsed(p);
}

// judge whether the leaf with specific PPointer exists. 
bool PAllocator::ifLeafExist(PPointer p) {
    // TODO
    return p.fileId<maxFileId && p.offset<=LEAF_GROUP_HEAD+(LEAF_GROUP_AMOUNT-1)*calLeafSize();//check overflow
}

// free and reuse a leaf
bool PAllocator::freeLeaf(PPointer p) {
    // TODO
    if(!ifLeafExist(p)) return false;
    if(ifLeafFree(p)) return false;
    //edit bitmap in leaf group
    map<uint64_t, char*>::iterator iter=fId2PmAddr.find(p.fileId);
    if(iter==fId2PmAddr.end()) return false;
    if(p.offset>LEAF_GROUP_HEAD+(LEAF_GROUP_AMOUNT-1)*calLeafSize()) return false;
    char* addr = iter->second;
    char* lgaddr=addr;
    uint64_t usedNum;
    //update usedNum
    memcpy(&usedNum,addr,sizeof(uint64_t));
    --usedNum;
    memcpy(addr,&usedNum,sizeof(uint64_t));
    int p_num=(p.offset-LEAF_GROUP_HEAD)/calLeafSize();//caculate the position of this leaf and set bitmap to use
    addr = addr + 8 + p_num; //to bitmap;
    *addr=0;
    int leaf_group_len = 8 + LEAF_GROUP_AMOUNT + LEAF_GROUP_AMOUNT*calLeafSize();
    pmem_msync(lgaddr,leaf_group_len);//write back to files and update the whole leaf group
    freeList.push_back(p);
    ++freeNum;
    return true;
}

bool PAllocator::persistCatalog() {
    // TODO
    string allocatorCatalogPath = DATA_DIR + P_ALLOCATOR_CATALOG_NAME;
    string freeListPath         = DATA_DIR + P_ALLOCATOR_FREE_LIST;
    ofstream allocatorCatalog(allocatorCatalogPath, ios::ate|ios::out|ios::binary); //open in clean mode
    ofstream freeListFile(freeListPath, ios::ate|ios::out|ios::binary);
    if(!(allocatorCatalog.is_open()&&freeListFile.is_open())) return false;
    //write AllocatorCatalog
    allocatorCatalog.write((char*)&(this->maxFileId), sizeof(uint64_t));
    allocatorCatalog.write((char*)&(this->freeNum), sizeof(uint64_t));
    allocatorCatalog.write((char*)&(this->startLeaf), sizeof(PPointer));
    //write FreeListFile
    for(unsigned int i=0;i<this->freeNum;++i){
        freeListFile.write((char*)&(freeList[i]), sizeof(PPointer));
    }
    //
    return true;
}

/*
  Leaf group structure: (uncompressed
  | usedNum(8b) | bitmap(n * byte) | leaf1 |...| leafn |
*/
// create a new leafgroup, one file per leafgroup
bool PAllocator::newLeafGroup() {
    // TODO
    string newFilePath = DATA_DIR + to_string(maxFileId);
    ofstream newLGFile(newFilePath, ios::out|ios::binary);
    if(!newLGFile.is_open()) return false;
    uint64_t usedNum = 0;//initial usedNum to 0
    const char *bitmap="0000000000000000";//LEAF_GROUP_AMOUNT = 16 so need 16bytes bitmap
    newLGFile.write((char*)&(usedNum), sizeof(uint64_t));
    newLGFile.write((char*)bitmap,16); //LEAF_GROUP_AMOUNT = 16 so need 16bytes bitmap
    char leaf_area[LEAF_GROUP_AMOUNT*calLeafSize()];//leaf areas
    memset(leaf_area,0,sizeof(leaf_area));
    newLGFile.write((char*)&(leaf_area), sizeof(leaf_area)); 
    //map new leaf group to pmem addr
    char *pmemaddr;
    int is_pmem;
    size_t mapped_len;
    int leaf_group_len = 8 + LEAF_GROUP_AMOUNT + LEAF_GROUP_AMOUNT*calLeafSize();
    if ((pmemaddr = (char*)pmem_map_file(newFilePath.c_str(), leaf_group_len, PMEM_FILE_CREATE,
		0666, &mapped_len, &is_pmem)) == NULL){
            assert(pmemaddr!=NULL);
            }
    //insert pmem addr into map
    fId2PmAddr.insert(pair<uint64_t, char*>(maxFileId,pmemaddr));
    for(uint64_t i=0;i<LEAF_GROUP_AMOUNT;++i){ //add new leaves into free list
        PPointer pp;
        pp.fileId=maxFileId;
        pp.offset=LEAF_GROUP_HEAD+i*calLeafSize();
        freeList.push_back(pp);
    }
    //update freeNUm and maxFileId and finally persist Catalog
    freeNum+=LEAF_GROUP_AMOUNT;
    if(maxFileId==1){ //if it's the first leaf group
    //chose the first leaf in this leaf group to be the start leaf
        startLeaf.fileId=1;
        startLeaf.offset=LEAF_GROUP_HEAD+(LEAF_GROUP_AMOUNT-1)*calLeafSize();
    }
    ++maxFileId;
    return persistCatalog();
}
