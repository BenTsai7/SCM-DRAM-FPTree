#include"gtest/gtest.h"
#include"utility/p_allocator.h"
#include<cstdio>
#include<iostream>

using namespace std;

const string catalogPath = DATA_DIR + "p_allocator_catalog";
const string freePath = DATA_DIR + "free_list";
const string file1 = DATA_DIR + "1";
const string file2 = DATA_DIR + "2";
const string file3 = DATA_DIR + "3";
const string file4 = DATA_DIR + "4";

void removeFile() {
    PAllocator::getAllocator()->~PAllocator();
    remove(catalogPath.c_str());
    remove(file1.c_str());
    remove(freePath.c_str());
}

TEST(PAllocatorTest, GetAllocator) {
    PAllocator* p = PAllocator::getAllocator();
    EXPECT_NE(p, nullptr);
    std::ifstream in(catalogPath);
    EXPECT_NE(in.good(), false);
    in.close();
    PAllocator* temp = PAllocator::getAllocator();
    EXPECT_EQ(temp->getMaxFileId(), 1);
    EXPECT_EQ(temp->getFreeNum(), 0);
    p->~PAllocator();
    remove(catalogPath.c_str());
    remove(freePath.c_str());
    
}

TEST(PAllocatorTest, NewLeafGroup) {
    PAllocator* p = PAllocator::getAllocator();
    EXPECT_NE(p, nullptr);
    bool result = p->newLeafGroup();
    EXPECT_EQ(result, true);
    p->~PAllocator();
    remove(catalogPath.c_str());
    remove(file1.c_str());
    remove(freePath.c_str());
}

TEST(PAllocatorTest, UseLeaf) {
    PAllocator* p = PAllocator::getAllocator();
    EXPECT_NE(p, nullptr);
    PPointer leaf;
    char* pmem_addr;
    bool result = p->getLeaf(leaf, pmem_addr);
    EXPECT_EQ(result, true);
    EXPECT_NE(pmem_addr, nullptr);
    EXPECT_EQ(leaf.fileId, 1);
    EXPECT_EQ(leaf.offset, LEAF_GROUP_HEAD + (LEAF_GROUP_AMOUNT - 1) * calLeafSize());

    Byte bit = 0;
    ifstream in(file1, ios::binary|ios::in);
    in.seekg(sizeof(uint64_t) + (LEAF_GROUP_AMOUNT - 1), ios::beg);
    in.read((char*)&(bit), sizeof(Byte));
    EXPECT_EQ(bit, 1);
    p->~PAllocator();
    remove(catalogPath.c_str());
    remove(file1.c_str());
    remove(freePath.c_str());
}

TEST(PAllocatorTest, AllocateMultiplyLeaves) {
    PAllocator* p = PAllocator::getAllocator();
    EXPECT_NE(p, nullptr);
    PPointer pointer;
    char* pmem_addr;
    for (int i = 0; i < LEAF_GROUP_AMOUNT * 4; i++) {
        bool flag = p->getLeaf(pointer, pmem_addr);
        EXPECT_EQ(flag, true);
        EXPECT_NE(pmem_addr, nullptr);
    }
    EXPECT_EQ(p->getFreeNum(), 0);

    ifstream f1(file1.c_str());
    ifstream f2(file2.c_str());
    ifstream f3(file3.c_str());
    ifstream f4(file4.c_str());
    EXPECT_EQ(f1.is_open(), true);
    EXPECT_EQ(f2.is_open(), true);
    EXPECT_EQ(f3.is_open(), true);
    EXPECT_EQ(f4.is_open(), true);
    f1.close();
    f1.clear();
    f2.close();
    f2.clear();
    f3.close();
    f3.clear();
    f4.close();
    f4.clear();

    p->~PAllocator();
    remove(catalogPath.c_str());
    remove(file1.c_str());
    remove(file2.c_str());
    remove(file3.c_str());
    remove(file4.c_str());   
}

TEST(PAllocatorTest, UseMultiplyLeaves) {
    PAllocator* p = PAllocator::getAllocator();
    EXPECT_NE(p, nullptr);
    PPointer leaf;
    char* pmem_addr;
    int offset = LEAF_GROUP_HEAD + calLeafSize() * (LEAF_GROUP_AMOUNT - 1);
    for (int i = 0; i < 4; i++) {
        p->getLeaf(leaf, pmem_addr);
        EXPECT_EQ(leaf.offset, offset);
        offset -= calLeafSize();
        PAllocator::getAllocator()->~PAllocator();
        p = PAllocator::getAllocator();
    }
    removeFile();
}

TEST(PAllocatorTest, FreeSingleLeaf) {
    PAllocator *p = PAllocator::getAllocator();
    PPointer leaf;
    char* pmem_addr;
    int offset = LEAF_GROUP_HEAD + calLeafSize() * (LEAF_GROUP_AMOUNT - 1);
    EXPECT_EQ(p->getLeaf(leaf, pmem_addr), true);
    EXPECT_EQ(leaf.offset, offset);
    EXPECT_EQ(p->freeLeaf(leaf), true);
    EXPECT_EQ(p->getLeaf(leaf, pmem_addr), true);
    EXPECT_EQ(leaf.offset, offset);
    removeFile();
}

TEST(PAllocatorTest, GetLeaf) {
    PAllocator* p = PAllocator::getAllocator();
    EXPECT_NE(p, nullptr);
    PPointer leaf;
    char* pmem_addr;
    bool result = p->getLeaf(leaf, pmem_addr);
    EXPECT_EQ(result, true);
    EXPECT_EQ(leaf.fileId, 1);
    EXPECT_EQ(leaf.offset, LEAF_GROUP_HEAD + (LEAF_GROUP_AMOUNT - 1) * calLeafSize());
    std::ifstream in(file1.c_str());
    EXPECT_NE(in.good(), false);
    in.close();
    in.clear();
    p->~PAllocator();
    remove(catalogPath.c_str());
    remove(file1.c_str());
    remove(freePath.c_str());
}
TEST(UtilityTest, CountOneBits) {
    int n = countOneBits(117);
    EXPECT_EQ(n, 5);
}

TEST(UtilityTest, Clhash) {
    Byte t_1 = keyHash(100);
    EXPECT_EQ(t_1, keyHash(100));
    EXPECT_NE(t_1, keyHash(200));
}
