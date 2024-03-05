#ifndef _EX2_H
#define _EX2_H
#include "sim.h"

#define VOLUME_NAME	"MYEXT2"   //卷名
#define BLOCK_SIZE 512	       //块大小
#define DISK_SIZE	4611	   //磁盘总块数

#define DISK_START 0	       //磁盘开始地址

#define	GD_SIZE	64	           //块组描述符大小
#define GDT_START	(0)    //块组描述符起始地址

#define BLOCK_BITMAP (512) // 块位图起始地址
#define INODE_BITMAP (1024)// inode 位图起始地址

#define INODE_TABLE (1536) // 索引节点表起始地址 3*512
#define INODE_SIZE 128	   // 每个inode的大小是128B   
#define INODE_TABLE_COUNTS	4096 //inode entry 数

#define DATA_BLOCK (263680)	// 数据块起始地址 3*512+4096*64
#define DATA_BLOCK_COUNTS	4096	// 数据块数

#define BLOCKS_PER_GROUP	4611 // 每组中的块数


struct group_desc // 64 B
{
    char bg_volume_name[16]; //文件系统名
    unsigned short sb_disk_size; //磁盘总大小
	unsigned short sb_blocks_per_group; //每组中块数
	unsigned short sb_size_per_block;	//每块的大小
    unsigned short bg_block_bitmap; //块位图起始块号
    unsigned short bg_inode_bitmap; //inode 位图起始块号
    unsigned short bg_inode_table; //inode表的起始块号
    unsigned short bg_free_blocks_count; //组空闲块数
    unsigned short bg_free_inodes_count; //组空闲inode数
    unsigned short bg_used_dirs_count; //组分配给目录节点数
    char bg_usrname[12];//用户名
    char bg_password[12];//密码
    char bg_pad[6]; //填充
};
struct inode // 128 B
{
    unsigned short i_mode;    //文件类型及访问权限
    unsigned short i_blocks;  //文件所占的数据块个数
    char LastWriteTime[30];
    char CreateTime[30];
    char LastAccessTime[30];
    unsigned long i_size;    //文件大小
    unsigned short i_block[8]; //直接索引方式
    char i_pad[28];           //填充
};
struct dir_entry //16B
{
    unsigned short inode; 
    unsigned short rec_len; 
    unsigned short name_len; 
    char file_type; 
    char name[9]; 
};


static unsigned short last_alloc_inode; // 最近分配的节点号 
static unsigned short last_alloc_block; // 最近分配的数据块号 
static unsigned short current_dir;   // 当前目录的节点号 
static unsigned short last_dir; //前一目录节点号
static unsigned short current_dirlen; // 当前路径长度 

static short fopen_table[16]; // 文件打开表 

static struct group_desc gdt;	// 组描述符缓冲区
static struct inode inode_area;  // inode缓冲区
static unsigned char bitbuf[512]={0}; // 位图缓冲区
static unsigned char ibuf[512]={0};
static struct dir_entry dir[32];   // 目录项缓冲区 32*16=512
static char Buffer[512];  // 针对数据块的缓冲区
static char tempbuf[4096];	// 文件写入缓冲区
static FILE *fp;	// 虚拟磁盘指针


char current_path[256];    // 当前路径名 

char last_dir_name[100];  //前一目录文件名

static void update_super_block(void);   //更新超级块内容
static void reload_super_block(void);   //加载超级块内容
static void update_group_desc(void);    //更新组描述符内容
static void reload_group_desc(void);    //加载组描述符内容
static void update_inode_entry(unsigned short i); //更新indoe表
static void reload_inode_entry(unsigned short i); //加载inode表
static void update_block_bitmap(void);  //更新块位图
static void reload_block_bitmap(void);  //加载块位图
static void update_inode_bitmap(void);  //更新inode位图
static void reload_inode_bitmap(void);  //加载inode位图
static void update_dir(unsigned short i);//更新目录
static void reload_dir(unsigned short i);//加载目录
static void update_block(unsigned short i);//更新数据块
static void reload_block(unsigned short i);//加载数据库
static int alloc_block(void);//分配数据块
static int get_inode(void); //得到inode节点
static unsigned short reserch_file(char tmp[9],int file_type,unsigned short *inode_num,unsigned short *block_num,unsigned short *dir_num);//查找文件
static void dir_prepare(unsigned short tmp,unsigned short len,int type);
static void remove_block(unsigned short del_num);//删除数据块
static void remove_inode(unsigned short del_num);//删除inode节点
static unsigned short test_fd(unsigned short Ino);//在打开文件表中查找是否已打开文件
static void initialize_disk(void);//初始化磁盘


#endif // _EX2_H
