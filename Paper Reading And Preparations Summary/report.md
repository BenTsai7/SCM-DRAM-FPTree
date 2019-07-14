# 论文阅读与前期工作总结
#### 使用示意图展示普通文件IO方式(fwrite等)的流程，即进程与系统内核，磁盘之间的数据交换如何进行？为什么写入完成后要调用fsync?
以下为普通文件I/O方式fwrite和fread的数据交换流程

![Untitled Diagram](https://github.com/youlitaia/DB10/blob/master/Paper%20Reading%20And%20Preparations%20Summary/imgs/1.jpg)

对于C/C++下标准I/O库中的文件I/O函数如fwrite和fread，其被定义为带I/O缓冲的函数。
它们在用户进程中开辟了一片缓冲区，当用户进程需要I/O时，首先将I/O数据放入用户进程缓冲区中，若缓冲区满或用户手动清空缓冲区才通过系统调用写入/写出数据，这减少了用户I/O内核调用的次数。

对于写数据到磁盘文件上的情况，用户进程首先调用fwrite将所写数据写入用户进程缓冲区，接着系统调用write或缓冲区冲洗函数fflush将用户进程缓冲区中的数据拷贝到内核缓冲区中，此时经过了用户态到内核态的转换。接着位于内核缓冲的数据可以等待操作系统的DMA调度或者fsync函数的指示，将内核缓冲的数据最终写入磁盘。

对于读磁盘文件到内存的情况，操作系统通过DMA调度将磁盘数据读入内核内存缓冲区，接着read系统调用将数据拷贝到用户进程缓冲区，最终得到内存中的磁盘数据。

在数据库应用中,写入完成后必须调用fsync函数以确保数据的一致性和安全性。原因是数据库应用无论是调用fwrite函数还是write系统调用，数据都不是立即写到磁盘上，而是存留在内核缓冲区中等待DMA调度。若等待的时间中内核崩溃，则数据丢失。因此写入后必须调用fsync函数，fsync函数强制与文件描述符相关文件的全部改动过的数据（包含核内I/O缓冲区中的数据）传送到外部永久介质（即磁盘），刷新文件的全部信息，而且等待写磁盘操作结束，然后返回。调用 fsync()的进程将堵塞直到设备报告传送已经完毕。这样成功地确保了数据的一致性。

#### 简述文件映射的方式如何操作文件。与普通IO区别？为什么写入完成后要调用msync？文件内容什么时候被载入内存？

![img](https://github.com/youlitaia/DB10/blob/master/Paper%20Reading%20And%20Preparations%20Summary/imgs/2.png)

文件映射将位于物理地址中的文件映射为进程中的逻辑地址，与普通I/O的区别是普通I/O通过fread或fwrite函数读写文件，而文件映射通过物理文件的逻辑指针对文件进行操作，这意味着在对文件进行处理时将不必再为文件申请并分配缓存，所有的文件缓存操作均由系统直接管理，由于取消了将文件数据加载到内存、数据从内存到文件的回写以及释放内存块等步骤，使得内存映射文件在处理大数据量的文件时能起到相当重要的作用。

以linux下内存映射文件写入过程为例:
1.首先调用open()打开文件，获得对应的文件描述符fd
2.调用mmap()，获得对应的文件的逻辑地址指针p
3.通过指针p加减偏移量,对指定位置的逻辑文件内存进行修改写入
4.调用msync()将内存中文件修改的部分写回磁盘中

与上一题一样，通过指针对文件写入后,修改的结果保存在内存中，并不会自动写回磁盘，因此需要调用msync()将内存中文件修改的部分写回磁盘中，确保数据的一致性。

对于文件内容的映射，调用mmap()建立文件映射时，文件并不是在此时载入内存，此时只是建立了逻辑文件地址到物理文件地址的映射，文件内容依然只位于磁盘中。当用户试图使用逻辑文件地址的指针去访问实际物理文件的映射中时，由于实际的物理地址并不在内存中，操作系统会发生页错误中断，将磁盘文件的页调入内存中，此时文件内容才真正被载入内存。

#### 参考Intel的NVM模拟教程模拟NVM环境，用fio等工具测试模拟NVM的性能并与磁盘对比（关键步骤结果截图）

实验环境：
虚拟机软件：Vmware WorkStation
操作系统：Ubuntu 18.04 LTS 64-bit
处理器：Intel i5-6300HQ
内存：8GB DDR3

(1)根据Intel的NVM模拟教程来配置NVM环境，
由于Ubuntu 18.04 LTS 64-bit的默认内核为4.18，且通过make nconfig发现NVMM选项全部默认配置完毕，所以可以跳过内核配置，编译，安装的过程。
首先直接对grub文件进行配置修改。加上红线划出的命令行，表示从内存4GB的位置开始，划分出4GB的NVM内存区，即将8GB内存划分为0-4GB的DRAM内存，和4GB-8GB的仿真NVM内存。

![1555821027216](https://github.com/youlitaia/DB10/blob/master/Paper%20Reading%20And%20Preparations%20Summary/imgs/3.png)

根据 grub 模板生成启动配置文件。

![1555821339449](https://github.com/youlitaia/DB10/blob/master/Paper%20Reading%20And%20Preparations%20Summary/imgs/4.png)

重启后，对硬件进行检测。

![Snipaste_2019-04-19_13-24-08](https://github.com/youlitaia/DB10/blob/master/Paper%20Reading%20And%20Preparations%20Summary/imgs/5.png)

可发现0x100000000-0x1ffffffff内存为persistent状态，说明共2的32次方内存即4GB内存被成功设置为NVM内存。

接着查看磁盘设备。

![Snipaste_2019-04-19_13-39-26](https://github.com/youlitaia/DB10/blob/master/Paper%20Reading%20And%20Preparations%20Summary/imgs/6.png)

可以发现生成了pmem0文件夹，为第一个也是唯一一个NVM内存块。
接着通过如下设置文件系统，将/mnt/pmemdir作为输入文件夹，现在可以在新加载的分区上创建文件，并作为输入提供给 NVML 池。

![1555822509087](https://github.com/youlitaia/DB10/blob/master/Paper%20Reading%20And%20Preparations%20Summary/imgs/7.png)

接着在linux上安装fio，测试磁盘和NVM内存的读写性能。
用fio -filename=[设备号] b -direct=1 -iodepth 1 -thread -rw=[读写方式]  -size=[写文件大小] -runtime=[测试时间] -group_reporting -name=[测试名] 指令分别测试磁盘和NVM的读写性能。

![1555822896693](https://github.com/youlitaia/DB10/blob/master/Paper%20Reading%20And%20Preparations%20Summary/imgs/8.png)

由图可得，disk(磁盘)的设备号为sda，NVM内存的设备号为pmem0，不能直接对其分别进行测试。
应该将NVM内存设备挂载到某个文件夹中，接着fio测试时对磁盘文件，文件路径随意指定一个磁盘上非NVM的文件路径，而对于NVM，则指定挂载的文件夹，同时在这些文件夹中创建新文件。对文件进行读取。

首先对随机读1G文件性能进行比较:

![1555826174488](https://github.com/youlitaia/DB10/blob/master/Paper%20Reading%20And%20Preparations%20Summary/imgs/9.png)

对于磁盘，随机读速度大约为26.2MB/s。

![1555826198998](https://github.com/youlitaia/DB10/blob/master/Paper%20Reading%20And%20Preparations%20Summary/imgs/10.png)

对于NVM，随机读速度大约为1351MB/s。

接着对随机写1G文件性能进行比较：
对于磁盘，随机写速度大约为15.7MB/s。

![1555826630048](https://github.com/youlitaia/DB10/blob/master/Paper%20Reading%20And%20Preparations%20Summary/imgs/11.png)

对于NVM，随机写速度大约为1108MB/s。

![1555826741295](https://github.com/youlitaia/DB10/blob/master/Paper%20Reading%20And%20Preparations%20Summary/imgs/12.png)

观察结果，可以发现NVM的读写速度远远高于磁盘，同时NVM的读写速度不平衡。

#### 使用PMDK的libpmem库编写样例程序操作模拟NVM（关键实验结果截图，附上编译命令和简单样例程序）。

根据pmdk的github仓库的教程在linux上安装各种依赖库，最后make，make check检测安装是否成功，make install将libpmem安装进标准库中。
最后编写样例程序操作模拟NVM，结果如下。

![1555994350756](https://github.com/youlitaia/DB10/blob/master/Paper%20Reading%20And%20Preparations%20Summary/imgs/13.png)

无error输出，同时成功向a.txt中写入“hello, persistent memory"。说明libpmem库链接运行成功。

样例程序a.cpp如下：

```c++
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <libpmem.h>
/* using 4k of pmem for this example */
#define PMEM_LEN 4096
#define PATH "a.txt"
int main(int argc, char *argv[])
{
	char *pmemaddr;
	size_t mapped_len;
	int is_pmem;
	/* create a pmem file and memory map it */
	if ((pmemaddr = (char*)pmem_map_file(PATH, PMEM_LEN, PMEM_FILE_CREATE,
			0666, &mapped_len, &is_pmem)) == NULL) {
		perror("_U(pmem_map_file)");
		exit(1);
	}
	/* store a string to the persistent memory */
	strcpy(pmemaddr, "hello, persistent memory");
	/* flush above strcpy to persistence */
	if (is_pmem)
		pmem_persist(pmemaddr, mapped_len);
	else
		pmem_msync(pmemaddr, mapped_len);
	/*
	 * Delete the mappings. The region is also
	 * automatically unmapped when the process is
	 * terminated.
	 */
	pmem_unmap(pmemaddr, mapped_len);
}
```

#### 总结一下本文的主要贡献和观点(500字以内)(不能翻译摘要)。
（回答本文工作的动机背景是什么，做了什么，有什么技术原理，解决了什么问题，其意义是什么）

本文工作的动机背景：SCM的发展使得SCM未来有可能代替DRAM作为内存兼磁盘的介质。同时，已经有不少研究着眼于在SCM中构建持久树，但是由于SCM的延迟要比DRAM高，因此之前的研究构建的树的性能都不太好，急需要一种在SCM中提高持久树性能的构建方法。同时在SCM中的持久树没有很好地解决并发，数据恢复的功能。

本文工作：提出了基于SCM-DRAM混合的持久并发的B+树。

技术原理：
通过将关键部分的叶节点构建在SCM上，而其他可以重建的非叶部分构建在DRAM上，实现了崩溃后持久树重建。同时叶子节点使用了指纹化fingerprint技术，能减少叶子节点的key搜寻时间。
使用HTM和加锁实现叶子节点的并发互斥访问，实现了并发。

解决的问题：由于该树在非叶节点的部分为DRAM构建，所以对非叶节点部分的操作性能接近于DRAM，而这提高了原来纯SCM树的性能瓶颈，同时通过HTM和加速实现了对树的并发访问。

意义：实现了基于SCM和DRAM的混合新型持久树，可以在软件崩溃后进行快速数据修复与持久树重建，同时支持对树的并发访问，并且相较于传统的SCM树提高了性能。

#### SCM硬件有什么特性？与普通磁盘有什么区别？普通数据库以页的粒度读写磁盘的方式适合操作SCM吗？

SCM硬件的特性有非易失性，大容量（存储级内存），成本低，读写快，读写速度不对称，能源效率高。
对比磁盘：相同点是它们容量都很大，成本低，并且断电后都不会丢失信息。不同点是SCM的读写速度要远远高于普通机械磁盘的读写速度。

不合适，普通数据库以页的粒度读写磁盘的原因是内存太小，而存于磁盘上的数据文件可能过大，无法一次性读入，所以要将其分页读入内存缓冲区中。
而对于SCM(Storage Class Memory)其容量非常大，可以直接将数据放入其中，又由于其能进行字节随机访问，所以直接在SCM读数据就可以了，分页反而要造成开销。
然而在本文中，为了方便数据写入错误时的恢复，论文中使用了partial writes，即每次写以p-原子大小写入块。

#### 操作SCM为什么要调用CLFLUSH等指令？
(写入后不调用，发生系统崩溃有什么后果)

![1555907444752](https://github.com/youlitaia/DB10/blob/master/Paper%20Reading%20And%20Preparations%20Summary/imgs/14.png)

SCM内存的硬件调用链如下，写入时，CPU不是直接在SCM中写，而是要经过Cache，Buffer，NVM controller的交互与缓冲，CLFLUSH（Cache Line Flush，缓存行刷回）能够把指定缓存行（Cache Line）从所有级缓存中淘汰，若该缓存行中的数据被修改过，则将该数据写入主存，确保数据一致性。
若写入后不调用，发生系统崩溃时Cache，Buffer中的数据可能还未写入NVM中，由于Cache，Buffer为易失性，此时数据将丢失。
接下来对SCM的Intel架构下指令进行逐条详细解释。操作SCM时会有CLFLUSH, MFENCE, SFENCE三种的指令，分别有不同的操作以及功能。
CLFLUSH： 可以将cache的一行撤回，并且判断该内存的数据是否有发生变化， 若发生变化，就把新的数据以及地址传入到nvm controller中，从而写入到nvm的对应位置中，以此来更新nvm中的数据。
功能： 根据cache的特性，在平时的读写操作中，系统会将cpu读写过的nvm内存单位的地址以及内容放入到cache中，cpu会直接对cache中的这一条记录中的数据进行读写操作，如果有进行过写操作，不会立刻写回nvm内存中，而是会在这一条记录过时的时候（长时间不使用且与其他新记录冲突满了）写回到nvm内存中，通过这种写回机制来加快cpu对nvm的读写。而大多数情况下，这一条记录如果经常使用的话，就会一直留在cache中，而cache也是一种快速读取的易失性内存，而在系统崩溃的时候，很有可能会因断电而失去其所存储的信息，一旦这些信息失去的话，之前所做过的写操作也会失去，导致数据的丢失。所以我们需要在写操作完成后，将cache中我们写操作得到的结果写回到内存中，以防止异常使其丢失。
MFENCE与SFENCE： 前者使系统等待前面读写操作的运行完毕然后才允许下一步的读写操作，后者使系统等待前面写操作的运行完毕然后才允许下一步的写操作。主要应用于数据的并发读写，以防着读脏数据、幻影读等影响数据有效行的异常。

#### FPTree的指纹技术有什么重要作用？

总体而言，将叶子节点的键映射为1字节的哈希值，搜索时避免了线性扫描键未排序的叶子节点，加快了键的搜索速度。
(1)指纹技术是运用在FPTree叶节点中，类似于hash的一种技术，通过hash函数建立key与对应指纹的映射，从而快速的找到我们所需要的key值位置以及value。
(2)在FPTree中，叶节点作为数据节点存储与nvm中，nvm不同于内存的是，其写操作的代价十分的大，所以这里的叶节点为了减小写操作的代价，就不采用有序存储（顺序存储会有大量数据的移动），而是使用了bitmap来记录叶节点中各个数据位置是否有数据，以便于插入以及搜索。但是这种无序的存储使得key的几乎匹配成了线性匹配，最多可能需要与所有的key进行匹配才能找到value。所以，这里为了提高匹配的性能，这里于是用了类似于哈希表的指纹技术，通过特定的哈希函数作用与key值，而通过好的哈希函数，可以使得查找中key值的匹配次数减少到为一次通过此来提高FPTree中对值的查找。

#### 为了保证指纹技术的数学证明成立，哈希函数应如何选取？
（哈希函数生成的哈希值具有什么特征，能简单对键值取模生成吗？）

设key总共有m个，哈希函数需要将m个key映射到n个值上（由于题目中是1 byte，即256个值），并且映射是uniformly distributed (均匀分布)的。
可以根据键值取模生成，因为若m个key是不同的话，那么取模时对于每个key，其映射到256个值中的概率是相同的，所以是均匀分布的。

#### 持久化指针的作用是什么？与课上学到的什么类似？

持久化指针位于叶子节点中，指向下一个叶子节点。持久化的作用是使程序崩溃时，由于是SCM指针，所以指针指向的内容依然存在，能够通过SCM叶子节点重建整个持久树。

![1555996977667](https://github.com/youlitaia/DB10/blob/master/Paper%20Reading%20And%20Preparations%20Summary/imgs/15.png)

类似于B+树底层连接叶子节点的指针。
