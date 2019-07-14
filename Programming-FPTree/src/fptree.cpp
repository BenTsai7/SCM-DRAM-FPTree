/**
fptree.cpp

Descrption: implementation of FPTREE
    OKAY: INSERT
          BULKLOADING
          UPDATE
          DELETE
          REDISTRIBUTE
    ALL DONE
**/
#include"fptree/fptree.h"
#include<assert.h>
using namespace std;

// Initial the new InnerNode
InnerNode::InnerNode(const int& d, FPTree* const& t, bool _isRoot) {
    // TODO
    tree=t;
    degree=d;
    isLeaf=false;
    nKeys = 0;      // amount of keys
    nChild = 0;     // amount of children
    /** WARNING:
    In fact, in each B+ Tree Inner node,
    if degree is d,
    MAX keys are 2*d and MAX children pointers are 2*d+1
    here we allocate 2*d+1 keys and 2*d+2 pointers
    it's used to split node when it's full for convience
    and it DOSEN'T mean B+ Tree Inner node can contain 2*d+1 keys 
    you should PAY ATTENTION to this.
    **/
    keys = new Key[2*d+1];// max (2 * d + 1) keys
    childrens = new Node*[2*d+2];//max (2 * d + 2)  node pointers
    //
    isRoot = _isRoot;     // judge whether the node is root
}

// delete the InnerNode
InnerNode::~InnerNode() {
    // TODO
    //free memories
    delete []keys;
    delete []childrens;
}

// binary search the first key in the innernode larger than input key
int InnerNode::findIndex(const Key& k) {
    // TODO
    int low=0,high=nKeys-1;
    while(low<=high){
        int mid = low+(high-low)/2;
        if (k >= keys[mid])
            low = mid+1;
        else
            high = mid-1;
        }
    return low;
}

// insert the node that is assumed not full
// insert format:
// ======================
// | key | node pointer |
// ======================
// WARNING: can not insert when it has no entry
void InnerNode::insertNonFull(const Key& k, Node* const& node) {
    // TODO
    //the keys nodekey in node satisfies nodekey>=key;
    //if it has no entry,return
    if(nKeys==degree*2+1) return;
    int index=findIndex(k);
    //move keys and pointers
    for(int i=nKeys;i>index;--i)
        keys[i]=keys[i-1];
    for(int i=nChild+1;i>index+1;--i)
        childrens[i]=childrens[i-1];
    keys[index]=k;
    childrens[index+1]=node; 
    ++nKeys;
    ++nChild;
}

// insert func
// return value is not NULL if split, returning the new child and a key to insert
KeyNode* InnerNode::insert(const Key& k, const Value& v) {
    KeyNode* newChild = NULL;

    // 1.insertion to the first leaf(only one leaf)
    if (this->isRoot && this->nKeys == 0 && this->nChild==0) {
        // TODO
        //it must allocate new leafnode
        LeafNode* newLeaf = new LeafNode(tree);
        //insert newLeaf into Right newLeaf
        newLeaf->insert(k,v);
        //it's the first leaf so it must be empty
        childrens[0]=newLeaf;
        ++nChild;
        return newChild;
    }
    // 2.recursive insertion
    // TODO
    //find insert positon+
    int index = findIndex(k);
    newChild = childrens[index]->insert(k,v);
    if(newChild==NULL) return newChild;
    else{
        //if Innernode has enough space
        if(this->nKeys<degree*2){
            insertNonFull(newChild->key,newChild->node);
            delete newChild;
            newChild=NULL;
            return newChild;
        }
        //no free space, split
        else{
            insertNonFull(newChild->key,newChild->node);//insert overflow key-value
            newChild = this->split();
            if(this->isRoot){
                //setting new root
                InnerNode* newRoot= new InnerNode(degree,tree,true);
                this->isRoot=false;
                newRoot->childrens[0]=this;
                ++(newRoot->nChild);
                newRoot->insertNonFull(newChild->key,newChild->node);
                tree->changeRoot(newRoot);
                delete newChild;
            }
        }
        //
    }
    return newChild;
}

// ensure that the leaves inserted are ordered
// used by the bulkLoading func
// inserted data: | minKey of leaf | LeafNode* |
KeyNode* InnerNode::insertLeaf(const KeyNode& leaf) {
    KeyNode* newChild = NULL;
    // first and second leaf insertion into the tree
    if (this->isRoot && this->nKeys == 0 && this->nChild==0) {
        // TODO
        childrens[0]=leaf.node;
        ++nChild;
        return newChild;
    }
    
    // recursive insert
    // Tip: please judge whether this InnerNode is full
    // next level is not leaf, just insertLeaf
    // TODO

    // next level is leaf, insert to childrens array
    // TODO
    int index = findIndex(leaf.key);
    if(childrens[0]->ifLeaf()){ //if next level is leaf
        if(this->nKeys<degree*2){
            insertNonFull(leaf.key,leaf.node);
        }
        else{
            insertNonFull(leaf.key,leaf.node);//insert overflow key-value
            newChild = this->split();
            if(this->isRoot){
            InnerNode* newRoot= new InnerNode(degree,tree,true);
            this->isRoot=false;
            newRoot->childrens[0]=this;
            ++(newRoot->nChild);
            newRoot->insertNonFull(newChild->key,newChild->node);
            tree->changeRoot(newRoot);
            delete newChild;
            newChild=NULL;}
            return newChild;
        }
    }
    else{ //if next level is not leaf
    newChild = ((InnerNode*)(childrens[index]))->insertLeaf(leaf);
    if(newChild==NULL) return newChild;
    else{
        //if Innernode has enough space
        if(this->nKeys<degree*2){
            insertNonFull(newChild->key,newChild->node);
            delete newChild;
            newChild=NULL;
            return newChild;
        }
        //no free space, split
        else{
            insertNonFull(newChild->key,newChild->node);//insert overflow key-value
            newChild = this->split();
            if(this->isRoot){
                //setting new root
                InnerNode* newRoot= new InnerNode(degree,tree,true);
                this->isRoot=false;
                newRoot->childrens[0]=this;
                ++(newRoot->nChild);
                newRoot->insertNonFull(newChild->key,newChild->node);
                tree->changeRoot(newRoot);
                delete newChild;
                newChild=0;
            }
        }
        //
        }
    }
    return newChild;
}

KeyNode* InnerNode::split() {
    KeyNode* newChild = new KeyNode();
    // right half entries of old node to the new node, others to the old node. 
    // TODO
    //left: d keys d pointers   right: d keys d pointers, push up the d+1th key 
    int d=nKeys/2;
    Key newMidKey=keys[d];
    InnerNode* newNode = new InnerNode(d,tree,false);
    newNode->nKeys=d;
    newNode->nChild=d+1;
    for(int i=d+1;i<nKeys;++i){
        newNode->keys[i-d-1]=keys[i];
        newNode->childrens[i-d-1]=childrens[i];
    }
    newNode->childrens[nKeys-d-1]=childrens[nKeys];
    nKeys = d;
    nChild = d+1;
    newChild->key=newMidKey;
    newChild->node=newNode;
    return newChild;
}

// remove the target entry
// return TRUE if the children node is deleted after removement.
// the InnerNode need to be redistributed or merged after deleting one of its children node.
bool InnerNode::remove(const Key& k, const int& index, InnerNode* const& parent, bool &ifDelete) {
    bool ifRemove = false; 
    // only have one leaf
    // TODO
    if (this->isRoot && this->nKeys == 0 && this->nChild == 1 && getChild(0)->ifLeaf()) {
        LeafNode * removeLeaf = (LeafNode *)getChild(0);
        ifRemove = removeLeaf->remove(k, 0, this, ifDelete);
        if (ifDelete) {
            nChild --;
            delete removeLeaf;
        }
        return ifRemove;
    }
    // recursive remove
    // TODO
    int keyIdx = findIndex(k);
    int childIdx = keyIdx;
    Node * removeChild = (Node *)getChild(childIdx);
    if (removeChild == NULL) {
        return false;
    }
    ifRemove = removeChild->remove(k, childIdx, this, ifDelete);
    if (ifDelete) { //child node is delete and may need adjustment
        ifDelete = false;
        if (degree + 1 > nChild && !this->isRoot) {
            //if it is needed adjustment to node
            InnerNode *leftBro, *rightBro;
            /*
                get brother and if not null get brothers' childnum
            */
            int left_nChild = 0, right_nChild = 0;
            getBrother(index, parent, leftBro, rightBro);
            if(leftBro != NULL)
                left_nChild = leftBro->getChildNum();
            if (rightBro != NULL)
                right_nChild = rightBro->getChildNum();
            /*
                when node need adjustment
                the prior order of the adjustment below
                1. right have enough node to give -> redistributeRight
                2. left  have enough node to give -> redistributeLeft
                3. right have enough room to merge -> mergeRight
                4. left  have enough room to merge -> mergeLeft
                5. parent is root and only has two node -> mergeparent
            */
            if (degree + 1 <= right_nChild - 1) { 
                //right node have enougth children node to give
                this->redistributeRight(index, rightBro, parent);
            }
            else if (degree + 1 <= left_nChild - 1) {
                //left node have enougth children node to give
                this->redistributeLeft(index, leftBro, parent);
            }
            else if (right_nChild != 0 &&right_nChild + this->nChild <= 2 * degree + 1) {
                //right have enough room to merge
                auto rk = this->keys[keyIdx];   //the key between two node
                this->mergeRight(rightBro, rk);
                ifDelete = true;
                parent->removeChild(keyIdx, index); //delete the node being merged 
            }
            else if (left_nChild != 0 && left_nChild + this->nChild <= 2 * degree + 1) {
                //right have enough room to merge
                auto lk = this->keys[keyIdx - 1]; //the key between two node
                this->mergeLeft(leftBro, lk);
                ifDelete = true;
                parent->removeChild(keyIdx - 1, index); //delete the node being merged 
            }
            else if (parent->isRoot && parent->getChildNum() == 2){
                //merge parent and both node
                if (leftBro != NULL) //chose not null brother
                    mergeParentLeft(parent, leftBro);
                else
                    mergeParentRight(parent, rightBro);
                ifDelete = true;
            }
        }
    }
    return ifRemove;
}

// If the leftBro and rightBro exist, the rightBro is prior to be used
void InnerNode::getBrother(const int& index, InnerNode* const& parent, InnerNode* &leftBro, InnerNode* &rightBro) {
    // TODO
    if (parent == NULL) return;
    if (index - 1 >= 0)  //have not left brother
        leftBro = (InnerNode *)parent->getChild(index - 1);
    else
        leftBro = NULL;
    rightBro = (InnerNode *)parent->getChild(index + 1);
}

// merge this node, its parent and left brother(parent is root)
void InnerNode::mergeParentLeft(InnerNode* const& parent, InnerNode* const& leftBro) {
    // TODO
    auto midKey = parent->getKey(0);
    int leftKeyNum = leftBro->getKeyNum();
    int leftChildNum = leftBro->getChildNum();
    //adjust space for new come
    for (int i = 0; i < leftKeyNum; i ++ )
        parent->keys[i] = leftBro->keys[i];
    for (int i = 0; i < leftChildNum; i ++)
        parent->childrens[i] = leftBro->childrens[i];
    //set the parent key between both node
    parent->keys[leftKeyNum] = midKey;
    
    //move leftBro node
    for (int i = leftKeyNum + 1; i < leftKeyNum + 1 + this->nKeys; i ++)
        parent->keys[i] = this->keys[i];
    for (int i = leftChildNum; i < leftChildNum + this->nChild; i ++ )
        parent->childrens[i] = this->childrens[i];
    nChild = 0;
    nKeys = 0;
    //free both node pointer space 
    delete leftBro;
    delete this;
}

// merge this node, its parent and right brother(parent is root)
void InnerNode::mergeParentRight(InnerNode* const& parent, InnerNode* const& rightBro) {
    // TODO
    auto midKey = parent->getKey(0);
    int rightKeyNum = rightBro->getKeyNum();
    //int rightChildNum = rightBro->getChildNum();
    //adjust space for new come
    for (int i = 0; i < nKeys; i ++)
        parent->keys[i] = this->keys[i];
    for (int i = 0; i < nChild; i ++ )
        parent->childrens[i] = this->childrens[i];
    //set the parent key between both node
    parent->keys[nKeys] = midKey;
    //move rightBro node
    for (int i = nKeys + 1; i < nKeys + 1 + rightKeyNum; i ++ )
        parent->keys[i] = rightBro->keys[i];
    for (int i = nChild + 1; i < nChild + 1; i ++)
        parent->childrens[i] = rightBro->childrens[i];
    nChild = 0;
    nKeys = 0;
    //free both node pointer space 
    delete rightBro;
    delete this;
}

// this node and its left brother redistribute
// the left has more entries
void InnerNode::redistributeLeft(const int& index, InnerNode* const& leftBro, InnerNode* const& parent) {
    // TODO
    int left_nChild = leftBro->getChildNum();//origin num
    //get both node child num after adjust
    int nLeft = (left_nChild + nChild + 1) / 2; 
    int nThis = (left_nChild + nChild) / 2;
    //adjust for space for new come
    for (int i = this->nKeys - 1; i >= 0; i --)
        keys[i + nThis - this->nChild] = keys[i];
    for (int i = this->nChild - 1; i >= 0; i -- ) 
        childrens[i + nThis - this->nChild] = childrens[i];
    //pull down the middle key
    keys[nThis - this->nChild - 1] = parent->keys[index - 1];
    parent->keys[index - 1] = leftBro->getKey(nLeft);
    //move the child node
    for (int i = left_nChild - 1; i >= nLeft; i -- )
        childrens[i] = leftBro->childrens[i - nLeft];
    for (int i = left_nChild - 2; i >= nLeft; i --)  
        keys[i] = leftBro->keys[i - nLeft];
    this->nKeys += left_nChild - nLeft;
    this->nChild += left_nChild - nLeft;
    //delete
    for (int i = nLeft; i < left_nChild; i ++ ) {
        leftBro->keys[i] = 0;
        leftBro->childrens[i] = NULL;
        leftBro->nKeys --;
        leftBro->nChild --;
    }

}

// this node and its right brother redistribute
// the right has more entries
void InnerNode::redistributeRight(const int& index, InnerNode* const& rightBro, InnerNode* const& parent) {
    // TODO
    //origin num
    int right_nChild = rightBro->getChildNum();
    //int right_nKeys = rightBro->getKeyNum();
    //get both node child num after adjust
    int nRight = (right_nChild + nChild + 1) / 2;
    //int nThis = (right_nChild + nChild) / 2;
    //pull down the middle key
    keys[this->nKeys] = parent->getKey(index);
    parent->keys[index] = rightBro->getKey(right_nChild - nRight - 1);
    //directly move node to left
    for (int i = 0; i < right_nChild - nRight; i ++ ) {
        this->keys[i + this->nKeys + 1] = rightBro->keys[i];
        this->childrens[i + this->nChild] = rightBro->childrens[i];
    }
    this->nKeys += right_nChild - nRight;
    this->nChild += right_nChild - nRight;
    
    for (int i = 0; i < right_nChild - nRight; i ++ ) {
        rightBro->keys[i] = rightBro->keys[right_nChild - nRight];
        rightBro->childrens[i] = rightBro->childrens[right_nChild - nRight];
        rightBro->nKeys --;
        rightBro->nChild --;
    }
}

// merge all entries to its left bro, delete this node after merging.
void InnerNode::mergeLeft(InnerNode* const& leftBro, const Key& k) {
    // TODO
    //get origin num
    int left_nkeys = leftBro->getKeyNum();
    int left_nChild = leftBro->getChildNum();
    //pull down middle key
    leftBro->keys[left_nkeys] = k;
    //diretly move child node to left
    for (int i = left_nkeys + 1; i < left_nkeys + 1 + this->nKeys; i ++ )
        leftBro->keys[i] = this->keys[i];
    for (int i = left_nChild; i < left_nChild + this->nChild; i ++)
        leftBro->childrens[i] = this->childrens[i];
    leftBro->nKeys += this->nKeys + 1;
    leftBro->nChild += this->nChild;
    nChild = 0;
    nKeys = 0;
}

// merge all entries to its right bro, delete this node after merging.
void InnerNode::mergeRight(InnerNode* const& rightBro, const Key& k) {
    // TODO
    int right_nKeys = rightBro->getKeyNum();
    int right_nChild = rightBro->getChildNum();
    //adjust space for adding
    for (int i = right_nKeys - 1; i >= 0; i -- )
        rightBro->keys[i + this->nKeys + 1] = rightBro->keys[i];
    for (int i = right_nChild - 1; i >= 0; i -- )
        rightBro->childrens[i + this->nChild] = rightBro->childrens[i];
    rightBro->keys[this->nKeys] = k;    //pull down middle key
    //move node to right
    for (int i = 0; i < this->nKeys; i ++ )
        rightBro->keys[i] = this->keys[i];
    for (int i = 0; i < this->nChild; i ++ )
        rightBro->childrens[i] = this->childrens[i];
    rightBro->nKeys += this->nKeys + 1;
    rightBro->nChild += this->nChild;
    nChild = 0;
    nKeys = 0;
}

// remove a children from the current node, used by remove func
void InnerNode::removeChild(const int& keyIdx, const int& childIdx) {
    // TODO
    //remove the indicate key and child
    delete childrens[childIdx];
    for (int i = keyIdx; i < nKeys - 1; i ++ )
        keys[i] = keys[i + 1];
    for (int i = childIdx; i < nChild - 1; i ++ )
        childrens[i] = childrens[i + 1];
    this->nChild --;
    this->nKeys --;
}

// update the target entry, return true if the update succeed.
bool InnerNode::update(const Key& k, const Value& v) {
    // TODO
    if(nKeys==0&&nChild==0) return false;
    int idx=findIndex(k);
    return childrens[idx]->update(k,v);
}

// find the target value with the search key, return MAX_VALUE if it fails.
Value InnerNode::find(const Key& k) {
    // TODO
    if(nKeys==0&&nChild==0) return MAX_VALUE;
    int idx=findIndex(k);
    return childrens[idx]->find(k);
}

// get the children node of this InnerNode
Node* InnerNode::getChild(const int& idx) {
    // TODO
    if (idx < this->nChild){
        return this->childrens[idx];
    }
    return NULL;
}

// get the key of this InnerNode
Key InnerNode::getKey(const int& idx) {
    if (idx < this->nKeys) {
        return this->keys[idx];
    } else {
        return MAX_KEY;
    }
}

// print the InnerNode
void InnerNode::printNode() {
    cout << "||#|";
    for (int i = 0; i < this->nKeys; i++) {
        cout << " " << this->keys[i] << " |#|";
    }
    cout << "|" << "    ";
}

// print the LeafNode
void LeafNode::printNode() {
    cout << "||";
    for (int i = 0; i < 2 * this->degree; i++) {
        if (this->getBit(i)) {
            cout << " " << this->kv[i].k << " : " << this->kv[i].v << " |";
        }
    }
    cout << "|" << " ====>> ";
}

// new a empty leaf and set the valuable of the LeafNode
LeafNode::LeafNode(FPTree* t) {
    // TODO
    // Leaf : | bitmap | pNext | fingerprints array | KV array |
    //initial Node
    tree=t;
    degree=LEAF_DEGREE;
    isLeaf=true;
    //new a leaf
    PAllocator* PA = PAllocator::getAllocator();
    PA->getLeaf(pPointer,pmem_addr);
    //initial bitmap
    bitmap=(Byte*)pmem_addr;
    bitmapSize=(LEAF_DEGREE*2+7)/8;
    //initial pNext
    Byte* cursor = (Byte*) (pmem_addr);
    cursor = cursor+bitmapSize;
    pNext=(PPointer*) cursor;
    //initial fingerprints, use cursor to make a pointer move
    cursor = cursor+sizeof(PPointer);
    fingerprints=cursor;
    //initial kv
    cursor = cursor+ LEAF_DEGREE*2 * sizeof(Byte);
    kv=(KeyValue*)cursor;
    n=0;
    prev=NULL;
    next=NULL;
    filePath=DATA_DIR+ to_string(pPointer.fileId);
}

// reload the leaf with the specific Persistent Pointer
// need to call the PAllocator
LeafNode::LeafNode(PPointer p, FPTree* t) {
    // TODO
    //initial Node
    next=NULL;
    prev=NULL;
    tree=t;
    degree=LEAF_DEGREE;
    isLeaf=true;
    //reload a leaf
    PAllocator* PA = PAllocator::getAllocator();
    pmem_addr=PA->getLeafPmemAddr(p);      // the pmem address of the leaf node
    assert(pmem_addr!=NULL);
    pPointer=p;
    //initial bitmap
    bitmap=(Byte*)pmem_addr;
    bitmapSize=(LEAF_DEGREE*2+7)/8;
    //initial pNext
    Byte* cursor = (Byte*) (pmem_addr);
    cursor = cursor+bitmapSize;
    pNext = (PPointer*) cursor;
    //initial fingerprints, use cursor to make a pointer move
    cursor = cursor + sizeof(PPointer);
    fingerprints=cursor;
    //initial kv
    cursor = cursor+ LEAF_DEGREE*2 * sizeof(Byte);
    kv=(KeyValue*)cursor;
    //count n
    cursor=bitmap;
    n=0;
    for(uint64_t i=0;i<bitmapSize;++i){
        n+=countOneBits(*cursor);
        cursor+=1;
    }
    //initial pointer list
     if (pNext->fileId!=0){
        next = new LeafNode(*pNext, tree);
        next->prev = this;
    }
    filePath=DATA_DIR+ to_string(p.fileId);
}

LeafNode::~LeafNode() {
    // TODO
    this->persist();
}

// insert an entry into the leaf, need to split it if it is full
KeyNode* LeafNode::insert(const Key& k, const Value& v) {
    KeyNode* newChild = NULL;
    // TODO
    if(n<2*degree-1){
        insertNonFull(k,v);
    }
    //it's full
    else{
        insertNonFull(k,v);
        newChild=split();
        next=(LeafNode*)(newChild->node);
        ((LeafNode*)(newChild->node))->prev=this;
    }
    return newChild;
}

// insert into the leaf node that is assumed not full
void LeafNode::insertNonFull(const Key& k, const Value& v) {
    // TODO
    //find the first free slot
    int idx=findFirstZero();
    assert(idx!=-1);
    //set free to use bit
    setBit(idx);
    fingerprints[idx]=keyHash(k);
    kv[idx].k=k;
    kv[idx].v=v;
    ++n;
    persist();
}

// split the leaf node
KeyNode* LeafNode::split() {
    KeyNode* newChild = new KeyNode();
    // TODO
    //LeafNode split when n = 2*d-1;
    LeafNode* newLeaf = new LeafNode(tree);
    memset(bitmap,0,bitmapSize);
    Key midkey=findSplitKey();
    for(int i=0;i<n/2;++i){ //original leaf
        fingerprints[i]=keyHash(getKey(i));
        setBit(i);
    }
    for(int i=n/2;i<n;++i){//new Leaf
        newLeaf->insertNonFull(getKey(i),getValue(i));
    }
    n=n/2;
    *pNext = newLeaf->getPPointer();
    newChild->node=newLeaf;
    newChild->key=midkey;
    newLeaf->persist();
    this->persist();
    return newChild;
}

// use to find a mediant key and delete entries less then middle
// called by the split func to generate new leaf-node
// qsort first then find
inline int cmp_kv(const void* a,const void* b)
{
    return ((KeyValue*)a)->k>((KeyValue*)b)->k;
}
Key LeafNode::findSplitKey() {
    Key midKey = 0;
    // TODO
    qsort(kv,n,sizeof(KeyValue),cmp_kv);
    midKey = kv[n/2].k;
    return midKey;
}
// get the target bit in bitmap
// TIPS: bit operation
int LeafNode::getBit(const int& idx) {
    // TODO
    assert(idx<2*degree);
    int offset = idx%8;
    int pos=idx/8;
    Byte* cursor = bitmap;
    cursor += pos;
    Byte bits = *cursor;
    bits = (bits>>offset) & 1;
    return (int) bits;
}
//set the target bit to 1 in bitmap
void LeafNode::setBit(const int& idx){
    assert(idx<2*degree);
    int offset = idx%8;
    int pos=idx/8;
    Byte* cursor = bitmap;
    cursor += pos;
    Byte bits = *cursor;
    bits = bits | (1<<offset);
    *(cursor) = bits;
}
void LeafNode::resetBit(const int& idx){
    assert(idx<2*degree);
    int offset = idx%8;
    int pos=idx/8;
    Byte* cursor = bitmap;
    cursor += pos;
    Byte bits = *cursor;
    bits = ~((~bits) | (1<<offset));
    *(cursor) = bits;
}

Key LeafNode::getKey(const int& idx) {
    return this->kv[idx].k;
}

Value LeafNode::getValue(const int& idx) {
    return this->kv[idx].v;
}

PPointer& LeafNode::getPPointer() {
    return this->pPointer;
}

// remove an entry from the leaf
// if it has no entry after removement return TRUE to indicate outer func to delete this leaf.
// need to call PAllocator to set this leaf free and reuse it
bool LeafNode::remove(const Key& k, const int& index, InnerNode* const& parent, bool &ifDelete) {
    bool ifRemove = false;
    int hash = keyHash(k);
    //elemate an entry in leafnode
    for (int i = 0; i < 2 * degree; i ++ ) {
        if (getBit(i) == 1 && fingerprints[i] == hash)
            if(getKey(i) == k) {
                resetBit(i);
                n --;
                ifRemove = true;
                break;
            }
    }
    if (n == 0) {
        //the leafnode have no entry so free this leaf
        ifDelete = true;
        //connect the prev and next leaf
        if (this->prev != NULL)
            this->prev->next = this->next;
        if (this->next != NULL)
            this->next->prev = this->prev;
        //free leaf
        PAllocator *pa = PAllocator::getAllocator();
        auto pp = this->getPPointer();
        pa->freeLeaf(pp);
        parent->removeChild(index, index);
    }
    else persist(); //has entry so persist
    // TODO
    return ifRemove;
}

// update the target entry
// return TRUE if the update succeed
bool LeafNode::update(const Key& k, const Value& v) {
    bool ifUpdate = false;
    // TODO
    int hash = keyHash(k);
    for(int i=0;i<2*degree;++i){
        if(getBit(i)==1&&(fingerprints[i]==hash)){
            if(getKey(i)==k){
                kv[i].v=v;
                ifUpdate=true;
                break;
            }
        }
    } 
    persist();
    return ifUpdate;
}

// if the entry can not be found, return the max Value
Value LeafNode::find(const Key& k) {
    // TODO
    int hash = keyHash(k);
    Byte* cursor=fingerprints;
    for(int i=0;i<2*degree;++i){
        if(getBit(i)==1&&(fingerprints[i]==hash)){
            if(getKey(i)==k) return getValue(i);
        }
        ++cursor;
    }
    return MAX_VALUE;
}

// find the first empty slot
int LeafNode::findFirstZero() {
    // TODO
    int idx=-1;
    for(int i=0;i<2*degree;++i){
        if(getBit(i)==0) {idx=i;break;}
    }
    return idx;
}

// persist the entire leaf
// use PMDK
void LeafNode::persist() {
    // TODO
    pmem_msync(pmem_addr,calLeafSize());
}

// call by the ~FPTree(), delete the whole tree
void FPTree::recursiveDelete(Node* n) {
    if (n->isLeaf) {
        delete n;
    } else {
        for (int i = 0; i < ((InnerNode*)n)->nChild; i++) {
            recursiveDelete(((InnerNode*)n)->childrens[i]);
        }
        delete n;
    }
}

FPTree::FPTree(uint64_t t_degree) {
    FPTree* temp = this;
    this->root = new InnerNode(t_degree, temp, true);
    this->degree = t_degree;
    bulkLoading();
}

FPTree::~FPTree() {
    recursiveDelete(this->root);
}

// get the root node of the tree
InnerNode* FPTree::getRoot() {
    return this->root;
}

// change the root of the tree
void FPTree::changeRoot(InnerNode* newRoot) {
    this->root = newRoot;
}

void FPTree::insert(Key k, Value v) {
    if (root != NULL) {
        root->insert(k, v);
    }
}

bool FPTree::remove(Key k) {
    if (root != NULL) {
        bool ifDelete = false;
        InnerNode* temp = NULL;
        return root->remove(k, -1, temp, ifDelete);
    }
    return false;
}

bool FPTree::update(Key k, Value v) {
    if (root != NULL) {
        return root->update(k, v);
    }
    return false;
}

Value FPTree::find(Key k) {
    if (root != NULL) {
        return root->find(k);
    }
    return MAX_VALUE;
}

// call the InnerNode and LeafNode print func to print the whole tree
// TIPS: use Queue
void FPTree::printTree() {
    // TODO
    queue<Node*> q;
    q.push(root);
    while(!q.empty()){
        Node* tmp = q.front();
        if(!tmp->isLeaf){
            for(int i=0;i<((InnerNode*)(tmp))->nChild;++i) q.push(((InnerNode*)(tmp))->getChild(i));
        } 
        tmp->printNode();
        q.pop();
    }
}

// bulkLoading the leaf files and reload the tree
// need to traverse leaves chain
// if no tree is reloaded, return FALSE
// need to call the PALlocator
bool FPTree::bulkLoading() {
    // TODO
    PAllocator* PA = PAllocator::getAllocator();
    PPointer startPointer=PA->getStartPointer();
    if(startPointer.fileId==0) return false;
    LeafNode* startLeaf = new LeafNode(startPointer,this);
    //travesal list
    KeyNode bulkLeaf;
    for(auto leafnode=startLeaf;leafnode!=NULL;leafnode=leafnode->next){
        bulkLeaf.node=leafnode;
        Key min=leafnode->getKey(0);
        for(int i=1;i<leafnode->n;++i){
            if(leafnode->getKey(i)<min&&leafnode->getBit(i)) min=leafnode->getKey(i);
        }
        bulkLeaf.key=min;
        root->insertLeaf(bulkLeaf);
    }
    return true;
}
uint64_t  FPTree::getDegree(){return degree;}

