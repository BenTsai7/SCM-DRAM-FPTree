#include"utility/utility.h"
#include"utility/clhash.h"
#include<string>
#include<fstream>

using namespace std;

uint64_t calLeafSize() {
    int n = LEAF_DEGREE * 2;
    int bitArrNum = (n + 7) / 8;
    // Leaf : | bitmap | pNext | fingerprints array | KV array |
    uint64_t size = bitArrNum + sizeof(PPointer) 
                  + n * sizeof(Byte)
                  + n * (sizeof(Key) + sizeof (Value));
    return size;
}

uint64_t countOneBits(Byte b) {
    uint64_t count = 0;
    while(b != 0) {
        count++;
        b = b & (b - 1);
    }
    return count;
}

// func that generates the fingerprints
Byte keyHash(Key k) {
    clhasher h(UINT64_C(0x23a23cf5033c3c81),UINT64_C(0xb3816f6a2c68e530));
    string kStr = std::to_string(k);
    uint64_t temp = h(kStr);
    Byte result;
    memcpy(&result, &temp, sizeof(Byte));
    return result;
}

bool PPointer::operator==(const PPointer p) const {
    if (this->fileId == p.fileId && this->offset == p.offset) {
        return true;
    } else {
        return false;
    }
}

// get the pNext of the leaf in the leaf file
PPointer getPNext(PPointer p) {
    string leafPath = DATA_DIR + to_string(p.fileId);
    ifstream file(leafPath.c_str(), ios::in|ios::binary);
    PPointer t_p;
    t_p.fileId = 0;
    t_p.offset = 0;
    if (!file.is_open()) {
        return t_p;
    }
    int len = (LEAF_DEGREE * 2 + 7) / 8 + p.offset;
    file.seekg(len, ios::beg);
    file.read((char*)&(t_p), sizeof(PPointer));
    return t_p;
}