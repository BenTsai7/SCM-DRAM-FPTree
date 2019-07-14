#include "fptree/fptree.h"
#include "gtest/gtest.h"
#include <iostream>

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

TEST(FPTreeTest, ReadLeaf) {
    
}

TEST(FPTreeTest, SingleInsert) {
    FPTree* tree = new FPTree(2);
    tree->insert(1, 100);
    LeafNode* leaf = (LeafNode*)(tree->getRoot()->getChild(0));
    EXPECT_NE(nullptr, leaf);
    if (!leaf) {
        return;
    }
    EXPECT_EQ(leaf->find(1), 100);
    EXPECT_EQ(leaf->getPPointer().fileId, 1);
    EXPECT_EQ(leaf->getPPointer().offset, LEAF_GROUP_HEAD + calLeafSize() * (LEAF_GROUP_AMOUNT - 1));
    LeafNode* t_leaf = new LeafNode(leaf->getPPointer(), NULL);
    EXPECT_EQ(t_leaf->getBit(0), 1);
    EXPECT_EQ(t_leaf->find(1), 100);
    removeFile();
}

TEST(FPTreeTest, InsertOneLeaf) {
    LeafNode* l1 = new LeafNode(NULL);
    for (int i = 1; i <= LEAF_DEGREE; i++) {
        // TODO: error, the insertion did not persist
        l1->insert(i, i * 10);
    }
    ifstream f1(file1, ios::binary|ios::in);
    Byte bit1;
    f1.seekg(sizeof(uint64_t) + LEAF_GROUP_AMOUNT - 1, ios::beg);
    f1.read((char*)&(bit1), sizeof(Byte));
    f1.close();
    EXPECT_EQ(bit1, 1);

    LeafNode* t_l1 = new LeafNode(l1->getPPointer(), NULL);
    EXPECT_EQ(t_l1->find(1), 10);
    LeafNode* l2 = new LeafNode(NULL);
    for (int i = LEAF_DEGREE + 1; i <= LEAF_DEGREE * 2; i++) {
        l2->insert(i, i * 10);
    }
    EXPECT_EQ(l1->getPPointer().offset, LEAF_GROUP_HEAD + calLeafSize() * (LEAF_GROUP_AMOUNT - 1));
    EXPECT_EQ(l2->getPPointer().offset, LEAF_GROUP_HEAD + calLeafSize() * (LEAF_GROUP_AMOUNT - 2));
    LeafNode* t_l11 = new LeafNode(l1->getPPointer(), NULL);
    LeafNode* t_l2 = new LeafNode(l2->getPPointer(), NULL);
    EXPECT_EQ(t_l11->find(1), 10);
    EXPECT_EQ(t_l2->find(LEAF_DEGREE + 1), 10 * (LEAF_DEGREE + 1));

    removeFile();
}

TEST(FPTreeTest, UpdateTest) {
    FPTree *tree = new FPTree(2);
    for (int i = 1; i <= LEAF_DEGREE; i++) {
        tree->insert(i, i * 100);
    }
    delete tree;
    tree = NULL;
    tree = new FPTree(2);
    for (int i = 1; i <= LEAF_DEGREE; i+=2) {
        EXPECT_EQ(tree->find(i), i * 100);
        EXPECT_EQ(tree->update(i, i * 200), true);
    }
    delete tree;
    tree = NULL;
    tree = new FPTree(2);
    for (int i = 1; i <= LEAF_DEGREE; i+=2) {
        EXPECT_EQ(tree->find(i), i * 200);
    }
    for (int i = 2; i <= LEAF_DEGREE; i+=2) {
        EXPECT_EQ(tree->find(i), i * 100);
    }
    delete tree;
    tree = NULL;
    removeFile();
}


TEST(FPTreeTest, BulkLoadingTwoLeaf) {

    PAllocator* pa = PAllocator::getAllocator();

    FPTree* tree1 = new FPTree(2);
    for (int i = 1; i <= LEAF_DEGREE * 2; i++) {
        tree1->insert(i, i * 100);
    }

    EXPECT_EQ(pa->getFreeNum(), LEAF_GROUP_AMOUNT - 2);
    delete tree1;
    ifstream f1(file1);
    EXPECT_EQ(f1.is_open(), true);
    f1.close();
    f1.clear();
    PAllocator::getAllocator()->~PAllocator();

    FPTree* tree2 = new FPTree(1);
    EXPECT_EQ(tree2->find(1), 100);

    removeFile();
}

TEST(FPTreeTest, PersistLeaf) {
    LeafNode* leaf = new LeafNode(NULL);
    leaf->insertNonFull(1, 100);
    leaf->insertNonFull(2, 200);
    leaf->persist();
    delete leaf;
    PPointer p;
    p.fileId = 1;
    p.offset = LEAF_GROUP_HEAD + (LEAF_GROUP_AMOUNT - 1) * calLeafSize();
    LeafNode* t_leaf = new LeafNode(p, NULL);
    EXPECT_EQ(t_leaf->find(1), 100);
    EXPECT_EQ(t_leaf->find(2), 200);

    removeFile();
}

TEST(FPTreeTest, BulkLoadingOneLeafGroup) {
    FPTree *tree = new FPTree(32);
    for (int i = 1; i < LEAF_DEGREE * 10; i++) {
        tree->insert(i, i * 10);
    }
    PAllocator::getAllocator()->~PAllocator();
    delete tree;
    FPTree *t_tree = new FPTree(2);
    for (int i = 1; i < LEAF_DEGREE * 10; i++) {
        EXPECT_EQ(t_tree->find(i), i * 10);
    }

    removeFile();
}

TEST(FPTreeTest, RemoveOneEntry) {
    FPTree* tree = new FPTree(4);
    for (int i = 1; i < 10; i++) {
        tree->insert(i, i * 10);
    }
    tree->remove(1);
    delete tree;
    PAllocator::getAllocator()->~PAllocator();
    tree = new FPTree(4);
    EXPECT_NE(tree->find(1), 10);
    uint64_t temp = MAX_VALUE;
    EXPECT_EQ(tree->find(1), temp);
    EXPECT_EQ(tree->find(2), 20);
    removeFile();
}

TEST(FPTreeTest, RemoveMultiplyEntries) {
    FPTree* tree = new FPTree(2);
    for (int i = 1; i <= LEAF_DEGREE * 2 * 6; i++) {
        tree->insert(i, i * 10);
    }
    // tree->printTree();
    // remove one leaf
    int i = 1;
    for (int j = 0; j < LEAF_DEGREE; i++, j++) {
        tree->remove(i);
    }
    tree->remove(i);
    i++;
    // tree->printTree();
    PPointer p;
    p.fileId = 1;
    p.offset = LEAF_GROUP_HEAD + calLeafSize() * (LEAF_GROUP_AMOUNT - 1);
    PAllocator* pa = PAllocator::getAllocator();
    EXPECT_EQ(pa->ifLeafUsed(p), false);
    EXPECT_EQ(pa->ifLeafFree(p), true);
    p.offset = LEAF_GROUP_HEAD + calLeafSize() * (LEAF_GROUP_AMOUNT - 2);
    EXPECT_EQ(pa->ifLeafUsed(p), true);
    EXPECT_EQ(pa->ifLeafFree(p), false);
    
    // remove 3 leaves
    for (int j = 0; j < LEAF_DEGREE * 3; j++, i++) {
        tree->remove(i);
    }
    tree->remove(i);
    i++;
    // tree->printTree();
    // remove 3 leaves;
    for (int j = 0; j < LEAF_DEGREE * 3; j++, i++) {
        tree->remove(i);
    }
    tree->remove(i);
    i++;
    // tree->printTree();
    removeFile();
}

TEST(InneNodeTest, RedistributeRightTest) {
    FPTree* tree = new FPTree(2);
    for (int i = 1; i <= LEAF_DEGREE * (3 * 4 + 5); i++) {
        tree->insert(i, i);
    }
    InnerNode* node1 = ((InnerNode*)(tree->getRoot()->getChild(3)));
    InnerNode* node2 = (InnerNode*)(tree->getRoot())->getChild(4);
    // tree->printTree();
    EXPECT_EQ(node1->getChildNum(), 3);
    for (int i = LEAF_DEGREE * 3 * 3 + 1; i <= LEAF_DEGREE * 10; i++) {
        tree->remove(i);
    }
    EXPECT_EQ(node1->getChildNum(), 3);
    EXPECT_EQ(node2->getChildNum(), 4);
    removeFile();
}

TEST(InnerNodeTest, MergeTest) {
    FPTree* tree = new FPTree(2);
    for (int i = 1; i <= LEAF_DEGREE * (3 * 5); i++) {
        tree->insert(i, i);
    }
    InnerNode* node1 = ((InnerNode*)(tree->getRoot()->getChild(3)));
    // tree->printTree();
    EXPECT_EQ(tree->getRoot()->getChildNum(), 5);
    EXPECT_EQ(node1->getChildNum(), 3);
    for (int i = LEAF_DEGREE * 3 * 4 + 1, j = 1; j <= LEAF_DEGREE; j++, i++) {
        tree->remove(i);
    }
    // tree->printTree();
    EXPECT_EQ(node1->getChildNum(), 5);
    EXPECT_EQ(tree->getRoot()->getChildNum(), 4);
    removeFile();
}
