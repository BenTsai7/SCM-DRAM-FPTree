#include <iostream>
#include <time.h>
#include"fptree/fptree.h"

using namespace std;
const string catalogPath = DATA_DIR + "p_allocator_catalog";
const string freePath = DATA_DIR + "free_list";
void removeFile() {
    uint64_t maxfile = PAllocator::getAllocator()->getMaxFileId();
    PAllocator::getAllocator()->~PAllocator();
    for(uint64_t i=1;i<maxfile;++i){
    const string file = DATA_DIR + to_string(i);
    remove(file.c_str());
    }
    remove(catalogPath.c_str());
    remove(freePath.c_str());
}
void testSplit() {
    FPTree* tree = new FPTree(4);

    for (int i = 1; i < 33; i++) {
        tree->insert(i, i * 10);
    }
    tree->printTree();
    delete tree;
}

int main() {
    clock_t start, end;
    start = clock();
    FPTree *tree = new FPTree(32);
    tree->printTree();

    for (int i = 40000; i <= 50000; i++) {
        tree->insert(i, i* 10);
    }
    end = clock();
    double dur = end - start;
    cout << "Insert Used Time = " << (dur / CLOCKS_PER_SEC) << endl;

    delete tree;
    tree = NULL;
    PAllocator::getAllocator()->~PAllocator();

    FPTree *t_tree = new FPTree(32);
    start = clock();
    for (int i = 40000; i <= 50000; i++) {
        t_tree->find(i);
    }
    end = clock();
    double t_dur = end - start;
    cout << "Find Used Time = " << (t_dur / CLOCKS_PER_SEC) << endl;
    removeFile();
}
