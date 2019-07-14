#include <stdlib.h>
#include <string>

#ifndef UTILITY_VALUE

#define UTILITY_VALUE
#define MAX_DEGREE 256
#define MIN_DEGREE 2
#define LEAF_DEGREE 56 // 叶子的度
#define INNER_DEGREE 4096 // 中间节点的度

#define MAX_KEY UINT64_MAX; // 键值最大值
#define MAX_VALUE UINT64_MAX; // 值的最大值

#define LEAF_GROUP_AMOUNT 16 // 一个LeafGroup含有的叶子数
#define ILLEGAL_FILE_ID   0

#endif

using std::string;

typedef unsigned char Byte;

typedef uint64_t  Key;    // key(8 byte)
typedef uint64_t  Value;  // value(8 byte)

//leaves file and pallocator data storing place
const string DATA_DIR =  "data/"; // TODO

// leaf header length, the bitmap is simply one byte for a leaf
const uint64_t LEAF_GROUP_HEAD = sizeof(uint64_t) + LEAF_GROUP_AMOUNT;

typedef struct t_PPointer
{
    /* data */
    uint64_t fileId;
    uint64_t offset;

    bool operator==(const t_PPointer p) const;
} PPointer;

uint64_t calLeafSize();

uint64_t countOneBits(Byte b);

Byte keyHash(Key k);

PPointer getPNext(PPointer p);
