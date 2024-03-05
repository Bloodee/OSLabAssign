#include <stdio.h>
#include <string.h>
#include "ex2.h"
#include "sim.h"
#include <time.h>
#include <stdlib.h>

static void update_group_desc(void) //写组描述符
{
    fp=fopen("./Ext2","r+");
    fseek(fp,GDT_START,SEEK_SET);
    fwrite(&gdt,GD_SIZE,1,fp);
    fflush(fp);
}

static void reload_group_desc(void) //读组描述符          
{

    fseek(fp,GDT_START,SEEK_SET);
    fread(&gdt,GD_SIZE,1,fp);
}

static void update_inode_entry(unsigned short i) //写第i个inode
{
    fp=fopen("./Ext2","r+");
    fseek(fp,INODE_TABLE+(i-1)*INODE_SIZE,SEEK_SET);//定位到读取位置
    fwrite(&inode_area,INODE_SIZE,1,fp);//将inode_area中信息写入文件
    fflush(fp);
}

static void reload_inode_entry(unsigned short i) // 
{
    fseek(fp,INODE_TABLE+(i-1)*INODE_SIZE,SEEK_SET);//定位到索引位置
    fread(&inode_area,INODE_SIZE,1,fp);//将文件信息读取到inode_area中
}

static void update_dir(unsigned short i) //更新目录
{
    fp=fopen("./Ext2","r+");
    fseek(fp,DATA_BLOCK+i*BLOCK_SIZE,SEEK_SET);
    fwrite(dir,BLOCK_SIZE,1,fp);
    fflush(fp);
}

static void reload_dir(unsigned short i) //读取目录
{
    fseek(fp,DATA_BLOCK+i*BLOCK_SIZE,SEEK_SET);
    fread(dir,BLOCK_SIZE,1,fp);
}

static void update_block_bitmap(void) //更新块位图
{
    fp=fopen("./Ext2","r+");
    fseek(fp,BLOCK_BITMAP,SEEK_SET);
    fwrite(bitbuf,BLOCK_SIZE,1,fp);
    fflush(fp);
}

static void reload_block_bitmap(void) //读取块位图
{
    fseek(fp,BLOCK_BITMAP,SEEK_SET);
    fread(bitbuf,BLOCK_SIZE,1,fp);
}

static void update_inode_bitmap(void) //更新块位图
{
    fp=fopen("./Ext2","r+");
    fseek(fp,INODE_BITMAP,SEEK_SET);
    fwrite(ibuf,BLOCK_SIZE,1,fp);
    fflush(fp);
}

static void reload_inode_bitmap(void) //读取块位图
{
    fseek(fp,INODE_BITMAP,SEEK_SET);
    fread(ibuf,BLOCK_SIZE,1,fp);
}

static void update_block(unsigned short i) //更新数据块
{
    fp=fopen("./Ext2","r+");
    fseek(fp,DATA_BLOCK+i*BLOCK_SIZE,SEEK_SET);
    fwrite(Buffer,BLOCK_SIZE,1,fp);
    fflush(fp);
}

static void reload_block(unsigned short i) //读取数据块
{
    fseek(fp,DATA_BLOCK+i*BLOCK_SIZE,SEEK_SET);
    fread(Buffer,BLOCK_SIZE,1,fp);
}


static int alloc_block(void) //分配数据块
{
    //bitbuf共512字节表示，4096bit来表示4096个数据块

    unsigned short cur=last_alloc_block;
    unsigned char con=128; // 1000 0000b
    int flag=0;
    if(gdt.bg_free_blocks_count==0)
    {
        printf("There is no block to be alloced!\n");
        return(0);
    }
    reload_block_bitmap();
    cur/=8;
    while(bitbuf[cur]==255)    //该字节8bit都已有数据    
    {
        if(cur==511)cur=0; //最后一个字节存满，从头开始寻找
        else cur++;
    }
    while(bitbuf[cur]&con) //在字节中找到空闲bit
    {
        con=con/2;
        flag++;
    }
    bitbuf[cur]=bitbuf[cur]+con;
    last_alloc_block=cur*8+flag;

    update_block_bitmap();
    gdt.bg_free_blocks_count--;
    update_group_desc();
    return last_alloc_block;
}

static int get_inode(void) //分配索引节点
{
    unsigned short cur=last_alloc_inode;
    unsigned char con=128;
    int flag=0;
    if(gdt.bg_free_inodes_count==0)
    {
        printf("There is no Inode to be alloced!\n");
        return 0;
    }
    reload_inode_bitmap();

    cur=(cur-1)/8; 
    while(ibuf[cur]==255)//该字节8bit都已有数据
    {
        if(cur==511)cur=0;//最后一个字节存满，从头开始寻找
        else cur++;
    }
    while(ibuf[cur]&con)//在字节中找到空闲bit 
    {
        con=con/2;
        flag++;
    }
    ibuf[cur]=ibuf[cur]+con;
    last_alloc_inode=cur*8+flag+1;
    update_inode_bitmap();
    gdt.bg_free_inodes_count--;
    update_group_desc();
    return last_alloc_inode;
}
    

static unsigned short reserch_file(char tmp[9],int file_type,unsigned short *inode_num,
														unsigned short *block_num,unsigned short *dir_num)
{
    unsigned short j,k;
    reload_inode_entry(current_dir); //加载当前目录
    j=0;
    while(j<inode_area.i_blocks)
    {
        reload_dir(inode_area.i_block[j]); //找到所寻找目录的块号
        k=0;
        while(k<32)
        {
            if(!dir[k].inode||dir[k].file_type!=file_type||strcmp(dir[k].name,tmp))
            {
            	k++;
            }//找到所寻文件在目录项的序号
            else
            {
                *inode_num=dir[k].inode;
                *block_num=j;
                *dir_num=k;
                return 1;
            }
        }
        j++;
    }
    return 0;
}

/*为新增目录或文件分配 dir_entry
对于新增文件，只需分配一个inode号
对于新增目录，除了inode号外，还需要分配数据区存储.和..两个目录项*/
static void dir_prepare(unsigned short tmp,unsigned short len,int type)
{
    reload_inode_entry(tmp);

    if(type==2) // Ŀ¼
    {
        inode_area.i_size=32;
        inode_area.i_blocks=1;
        inode_area.i_block[0]=alloc_block();
        dir[0].inode=tmp;
        dir[1].inode=current_dir;
        dir[0].name_len=len;
        dir[1].name_len=current_dirlen;
        dir[0].file_type=dir[1].file_type=2;

        for(type=2;type<32;type++)
            dir[type].inode=0;
            strcpy(dir[0].name,".");
            strcpy(dir[1].name,"..");
            update_dir(inode_area.i_block[0]);

            inode_area.i_mode=01006;
    }
    else
    {
        inode_area.i_size=0;
        inode_area.i_blocks=0;
        inode_area.i_mode=0407;
    }
    update_inode_entry(tmp);
}


//删除块   

static void remove_block(unsigned short del_num)
{
    unsigned short tmp;
    tmp=del_num/8;
    reload_block_bitmap();
    switch(del_num%8) //     blockλͼ        λ  Ϊ0
    {
        case 0:bitbuf[tmp]=bitbuf[tmp]&127;break; // bitbuf[tmp] & 0111 1111b
        case 1:bitbuf[tmp]=bitbuf[tmp]&191;break; //bitbuf[tmp]  & 1011 1111b
        case 2:bitbuf[tmp]=bitbuf[tmp]&223;break; //bitbuf[tmp]  & 1101 1111b
        case 3:bitbuf[tmp]=bitbuf[tmp]&239;break; //bitbbuf[tmp] & 1110 1111b
        case 4:bitbuf[tmp]=bitbuf[tmp]&247;break; //bitbuf[tmp]  & 1111 0111b
        case 5:bitbuf[tmp]=bitbuf[tmp]&251;break; //bitbuf[tmp]  & 1111 1011b
        case 6:bitbuf[tmp]=bitbuf[tmp]&253;break; //bitbuf[tmp]  & 1111 1101b
        case 7:bitbuf[tmp]=bitbuf[tmp]&254;break; // bitbuf[tmp] & 1111 1110b
    }
    update_block_bitmap();
    gdt.bg_free_blocks_count++;
    update_group_desc();
}


//删除索引节点

static void remove_inode(unsigned short del_num)
{
    unsigned short tmp;
    tmp=(del_num-1)/8;
    reload_inode_bitmap();
    switch((del_num-1)%8)//    blockλͼ
    {
        case 0:bitbuf[tmp]=bitbuf[tmp]&127;break;
        case 1:bitbuf[tmp]=bitbuf[tmp]&191;break;
        case 2:bitbuf[tmp]=bitbuf[tmp]&223;break;
        case 3:bitbuf[tmp]=bitbuf[tmp]&239;break;
        case 4:bitbuf[tmp]=bitbuf[tmp]&247;break;
        case 5:bitbuf[tmp]=bitbuf[tmp]&251;break;
        case 6:bitbuf[tmp]=bitbuf[tmp]&253;break;
        case 7:bitbuf[tmp]=bitbuf[tmp]&254;break;
    }
    update_inode_bitmap();
    gdt.bg_free_inodes_count++;
    update_group_desc();
}

//检测文件是否打开
static unsigned short test_fd(unsigned short Inode)
{
    unsigned short fopen_table_point=0;
    while(fopen_table_point<16&&fopen_table[fopen_table_point++]!=Inode);
    if(fopen_table_point==16)
    {
    	return 0;
    }
    return 1;
}

void initialize_disk(void)  //初始化磁盘   
{
    int i=0;
    printf("Creating the Ext2 file system\n");

    last_alloc_inode=1;
    last_alloc_block=0;
    for(i=0;i<16;i++)
    {
    	fopen_table[i]=0; //清空打开缓冲表 
    }
    for(i=0;i<BLOCK_SIZE;i++)
    {
    	Buffer[i]=0; //清空数据缓冲区
    }
    if(fp!=NULL)
    {
    	fclose(fp);//关闭文件
    }
    fp=fopen("./Ext2","w+"); 
    fseek(fp,DISK_START,SEEK_SET);//将文件指针定位到0起始处
    for(i=0;i<DISK_SIZE;i++)
    {
    	fwrite(Buffer,BLOCK_SIZE,1,fp); //清空文件
    }
    //初始化组描述符内容
    reload_group_desc();
    strcpy(gdt.bg_volume_name,VOLUME_NAME);
    gdt.sb_disk_size=DISK_SIZE;
    gdt.sb_blocks_per_group=BLOCKS_PER_GROUP;
    gdt.sb_size_per_block=BLOCK_SIZE;
    reload_inode_entry(1);
    reload_dir(0);
    strcpy(current_path,"[root@ /");  

    gdt.bg_block_bitmap=BLOCK_BITMAP;
    gdt.bg_inode_bitmap=INODE_BITMAP; 
    gdt.bg_inode_table=INODE_TABLE; 
    gdt.bg_free_blocks_count=DATA_BLOCK_COUNTS;  
    gdt.bg_free_inodes_count=INODE_TABLE_COUNTS; 
    gdt.bg_used_dirs_count=0; 
    update_group_desc();               

    reload_block_bitmap();
    reload_inode_bitmap();

    inode_area.i_mode=518;
    inode_area.i_blocks=0;
    inode_area.i_size=32;
    inode_area.i_block[0]=alloc_block();
    inode_area.i_blocks++;
    current_dir=get_inode();
    update_inode_entry(current_dir);
    //初始化根目录数据 
    dir[0].inode=dir[1].inode=current_dir;
    dir[0].name_len=0;
    dir[1].name_len=0;
    dir[0].file_type=dir[1].file_type=2;
    strcpy(dir[0].name,".");
    strcpy(dir[1].name,"..");
    time_t rawtime;
    time(&rawtime);
    struct tm * timeinfo;
    timeinfo = localtime(&rawtime);
    reload_inode_entry(dir[0].inode);
    strcpy(inode_area.LastWriteTime,asctime(timeinfo));
    strcpy(inode_area.CreateTime,asctime(timeinfo));
    strcpy(inode_area.LastAccessTime,asctime(timeinfo));
    update_inode_entry(dir[0].inode);
    update_dir(inode_area.i_block[0]);
    printf("The ext2 file system has been installed!\n");

    //初始化用户密码
    reload_group_desc();
    char password[12],usrname[12];
    printf("\nCreate Your account      \n\n");
    printf("Usrname: ");
    scanf("%s",usrname);
    printf("Password: ");
    scanf("%s",password);
    strcpy(gdt.bg_usrname,usrname);
    strcpy(gdt.bg_password,password);
    printf("\n");
    printf("Account creating successful!\n\n");
    update_group_desc();

    check_disk();
    fclose(fp);
}

void initialize_memory(void)
{
    int i=0;
    last_alloc_inode=1;
    last_alloc_block=0;
    for(i=0;i<16;i++)
    {
    	fopen_table[i]=0;
    }
    strcpy(current_path,"[root@ /");
    current_dir=1;
    fp=fopen("./Ext2","r+");
    if(fp==NULL)
    {
        printf("The File system does not exist!\n");
        initialize_disk();
        return ;
    }
    reload_group_desc();
    if(strcmp(gdt.bg_volume_name,VOLUME_NAME))
    {
    	printf("The File system [%s] is not suppoted yet!\n", gdt.bg_volume_name);
    	printf("The File system loaded error!\n");
    	initialize_disk();
    	return ;
    }
    login();
}

// 格式化
void format(void)
{
    initialize_disk();
    initialize_memory();
}

//进入目录
void cd(char tmp[9])
{
    unsigned short i,j,k,flag;
    flag=reserch_file(tmp,2,&i,&j,&k);
    strcpy(last_dir_name,tmp);
    if(flag)
    {
        current_dir=i;
        if(!strcmp(tmp,"..")&&dir[k-1].name_len){
            current_path[strlen(current_path)-dir[k-1].name_len-1]='\0';
            current_dirlen=dir[k].name_len; 
        }
        else if(!strcmp(tmp,"."))
        {
        	return ;
        }
        else if(strcmp(tmp,".."))
        {
            current_dirlen=strlen(tmp);
            strcat(current_path,tmp);
            strcat(current_path,"/");
            time_t rawtime;
            time(&rawtime);
            struct tm * timeinfo;
            timeinfo = localtime(&rawtime);
            reload_inode_entry(dir[k].inode);
            strcpy(inode_area.LastAccessTime,asctime(timeinfo));
            update_inode_entry(dir[k].inode);
        }
    }
    else
    {
    	printf("The directory %s not exists!\n",tmp);
    }
}

// 创建目录
void mkdir(char tmp[9],int type)
{
    unsigned short tmpno,i,j,k,flag;

    // 在当前目录下增加目录
    reload_inode_entry(current_dir);
    if(!reserch_file(tmp,type,&i,&j,&k)) // 未找到同名文件
    {
        if(inode_area.i_size==4096) //目录项已满   
        {
            printf("Directory has no room to be alloced!\n");
            return;
        }
        flag=1;
        if(inode_area.i_size!=inode_area.i_blocks*512) //目录中某些块中32个dir_entry未满
        {
            i=0;
            while(flag&&i<inode_area.i_blocks)
            {
                reload_dir(inode_area.i_block[i]);
                j=0;
                while(j<32)
                {
                    if(dir[j].inode==0)
                    {
                        flag=0; //找到未装满目录项的块
                        break;
                    }
                    j++;
                }
                i++;
            }
            tmpno=dir[j].inode=get_inode();

            dir[j].name_len=strlen(tmp);
            dir[j].file_type=type;
            strcpy(dir[j].name,tmp);
            update_dir(inode_area.i_block[inode_area.i_blocks-1]);
            inode_area.i_size+=16;
            update_inode_entry(current_dir);
            time_t rawtime;
            time(&rawtime);
            struct tm * timeinfo;
            timeinfo = localtime(&rawtime);
            reload_inode_entry(dir[j].inode);
            strcpy(inode_area.CreateTime,asctime(timeinfo));
            strcpy(inode_area.LastWriteTime,asctime(timeinfo));
            strcpy(inode_area.LastAccessTime,asctime(timeinfo));
            update_inode_entry(dir[j].inode);
            reload_inode_entry(dir[0].inode);
            strcpy(inode_area.LastWriteTime,asctime(timeinfo));
            update_inode_entry(dir[0].inode);
            reload_inode_entry(dir[1].inode);
            strcpy(inode_area.LastWriteTime,asctime(timeinfo));
            update_inode_entry(dir[1].inode);
           
        }
        else //全满，新增加块
        {
            inode_area.i_block[inode_area.i_blocks]=alloc_block();
            inode_area.i_blocks++;
            reload_dir(inode_area.i_block[inode_area.i_blocks-1]);
            tmpno=dir[0].inode=get_inode();
            dir[0].name_len=strlen(tmp);
            dir[0].file_type=type;
            strcpy(dir[0].name,tmp);
            for(flag=1;flag<32;flag++)
            {
            	dir[flag].inode=0;
            }
            update_dir(inode_area.i_block[inode_area.i_blocks-1]);
            inode_area.i_size+=16;
            update_inode_entry(current_dir);
            time_t rawtime;
            time(&rawtime);
            struct tm * timeinfo;
            timeinfo = localtime(&rawtime);
            reload_inode_entry(dir[0].inode);
            strcpy(inode_area.CreateTime,asctime(timeinfo));
            strcpy(inode_area.LastAccessTime,asctime(timeinfo));
            strcpy(inode_area.LastWriteTime,asctime(timeinfo));
            update_inode_entry(dir[0].inode);            
        }
        
        //为新增目录分配dir_entry
        dir_prepare(tmpno,strlen(tmp),type);
    }
    else  
    {
        printf("Directory has already existed!\n");
    }

}
//创建文件
void cat(char tmp[9],int type)
{
    unsigned short tmpno,i,j,k,flag;
    reload_inode_entry(current_dir);
    if(!reserch_file(tmp,type,&i,&j,&k))
    {
        if(inode_area.i_size==4096)
        {
            printf("Directory has no room to be alloced!\n");
            return;
        }
        flag=1;
        if(inode_area.i_size!=inode_area.i_blocks*512)
        {
            i=0;
            while(flag&&i<inode_area.i_blocks)
            {
                reload_dir(inode_area.i_block[i]);
                j=0;
                while(j<32)
                {
                    if(dir[j].inode==0)//找到未分配的目录项
                    {
                        flag=0;
                        break;
                    }
                    j++;
                }
                i++;
            }
            
            tmpno=dir[j].inode=get_inode();//分配索引节点  
            dir[j].name_len=strlen(tmp);
            dir[j].file_type=type;
            strcpy(dir[j].name,tmp);
            update_dir(inode_area.i_block[i-1]);
            inode_area.i_size+=16;
            update_inode_entry(current_dir);
            time_t rawtime;
            time(&rawtime);
            struct tm * timeinfo;
            timeinfo = localtime(&rawtime);
            reload_inode_entry(dir[j].inode);
            strcpy(inode_area.LastWriteTime,asctime(timeinfo));
            strcpy(inode_area.CreateTime,asctime(timeinfo));
            strcpy(inode_area.LastAccessTime,asctime(timeinfo));
            update_inode_entry(dir[j].inode);
            reload_inode_entry(dir[0].inode);
            strcpy(inode_area.LastWriteTime,asctime(timeinfo));
            update_inode_entry(dir[0].inode);
            reload_inode_entry(dir[1].inode);
            strcpy(inode_area.LastWriteTime,asctime(timeinfo));
            update_inode_entry(dir[1].inode);
        }
        else // 分配数据块      
        {
            inode_area.i_block[inode_area.i_blocks]=alloc_block();
            inode_area.i_blocks++;
            reload_dir(inode_area.i_block[inode_area.i_blocks-1]);
            tmpno=dir[0].inode=get_inode();
            dir[0].name_len=strlen(tmp);
            dir[0].file_type=type;
            strcpy(dir[0].name,tmp);
            for(flag=1;flag<32;flag++)
            {
                dir[flag].inode=0;
            }
            update_dir(inode_area.i_block[inode_area.i_blocks-1]);
            inode_area.i_size+=16;
            update_inode_entry(current_dir);
            time_t rawtime;
            time(&rawtime);
            struct tm * timeinfo;
            timeinfo = localtime(&rawtime);
            reload_inode_entry(dir[0].inode);
            strcpy(inode_area.LastWriteTime,asctime(timeinfo));
            strcpy(inode_area.LastAccessTime,asctime(timeinfo));
            strcpy(inode_area.CreateTime,asctime(timeinfo));
            update_inode_entry(dir[0].inode);
        }
        
        //初始化新增文件的inode节点
        dir_prepare(tmpno,strlen(tmp),type);

    }
    else
    {
        printf("File has already existed!\n");
    }
}

//删除目录
void rmdir(char tmp[9])
{
    unsigned short i,j,k,flag;
    unsigned short m,n;
    if(!strcmp(tmp,"..")||!strcmp(tmp,"."))
    {
        printf("The directory can not be deleted!\n");
        return;
    }
    flag=reserch_file(tmp,2,&i,&j,&k);
    if(flag)
    {
        reload_inode_entry(dir[k].inode); //加载要删除的节点
        if(inode_area.i_size==32) //目录为空
        {
            inode_area.i_size=0;
            inode_area.i_blocks=0;
            remove_block(inode_area.i_block[0]);
            reload_inode_entry(current_dir);
            reload_dir(inode_area.i_block[j]);
            remove_inode(dir[k].inode);
            dir[k].inode=0;
            update_dir(inode_area.i_block[j]);
            inode_area.i_size-=16;
            flag=0;
            m=1;
            //m从1开始，因为inode_area[0].i_block[0] 中有目录 . 和 ..所以这个数据块的非空 dir_entry 不可能为0*/
            while(flag<32&&m<inode_area.i_blocks)
            {
                flag=n=0;
                reload_dir(inode_area.i_block[m]);
                while(n<32)
                {
                    if(!dir[n].inode)
                    {
                    	flag++;
                    }
                    n++;
                }
                if(flag==32)//删除后数据块中目录项全部为空
                {
                    remove_block(inode_area.i_block[m]);
                    inode_area.i_blocks--;
                    while(m<inode_area.i_blocks)
                    {
                    	inode_area.i_block[m]=inode_area.i_block[m+1];
                    	++m;
                    }
                }
            }
            update_inode_entry(current_dir);
            time_t rawtime;
            time(&rawtime);
            struct tm * timeinfo;
            timeinfo = localtime(&rawtime);
            reload_inode_entry(dir[j].inode);
            strcpy(inode_area.LastWriteTime,asctime(timeinfo));
            update_inode_entry(dir[j].inode);
            if(dir[1].inode){
                reload_inode_entry(dir[1].inode);
                strcpy(inode_area.LastWriteTime,asctime(timeinfo));
                update_inode_entry(dir[1].inode);
            }
            return;
        }
        else
        {
            printf("Directory is not null!\n");
            return;
        }
    }
    else
    {
    	printf("Directory to be deleted not exists!\n");
    }
}

//ɾ   ļ 
void del(char tmp[9])
{
    unsigned short i,j,k,m,n,flag;
    m=0;
    flag=reserch_file(tmp,1,&i,&j,&k);
    if(flag)
    {
        time_t rawtime;
        time(&rawtime);
        struct tm * timeinfo;
        timeinfo = localtime(&rawtime);
        reload_inode_entry(dir[0].inode);
        strcpy(inode_area.LastWriteTime,asctime(timeinfo));
        update_inode_entry(dir[0].inode);
        reload_inode_entry(dir[1].inode);
        strcpy(inode_area.LastWriteTime,asctime(timeinfo));
        update_inode_entry(dir[1].inode);
        flag=0;
        // 若所删除文件处于打开状态，则将其设为关闭即对应位置清0
        while(fopen_table[flag]!=dir[k].inode&&flag<16)
        {
        	flag++;
        }
        if(flag<16)
        {
        	fopen_table[flag]=0;
        }
        reload_inode_entry(i); // 加载所删除文件的索引节点
        //删除文件对应数据块
        while(m<inode_area.i_blocks)
        {
        	remove_block(inode_area.i_block[m++]);
        }
        inode_area.i_blocks=0;
        reload_inode_entry(current_dir);
        reload_dir(inode_area.i_block[j]);
        dir[k].inode=0; //删除对应索引节点
        update_dir(inode_area.i_block[j]);
        inode_area.i_size-=16;
        m=1;
        //若整个数据块为空，则删除数据块
        while(m<inode_area.i_blocks)
        {
            flag=n=0;
            reload_dir(inode_area.i_block[m]);
            while(n<32)
            {
                if(!dir[n].inode)
                {
                	flag++;
                }
                n++;
            }
            if(flag==32)
            {
                remove_block(inode_area.i_block[m]);
                inode_area.i_blocks--;
                while(m<inode_area.i_blocks)
                {
                	inode_area.i_block[m]=inode_area.i_block[m+1];
                	++m;
                }
            }
        }
        update_inode_entry(current_dir);
    }
    else
    {
    	printf("The file %s not exists!\n",tmp);
    }
}

void attrib(char tmp[9]){
    unsigned short flag,i,j,k;
    flag = reserch_file(tmp,1,&i,&j,&k);
    char attr[5];
    scanf("%s",attr);
    if(flag){
        reload_inode_entry(dir[k].inode);
        if(!strcmp(attr,"r-"))
            inode_area.i_mode = inode_area.i_mode &4091;
        if(!strcmp(attr,"r+"))
            inode_area.i_mode = inode_area.i_mode |4;
        if(!strcmp(attr,"w-"))
            inode_area.i_mode = inode_area.i_mode &4093;
        if(!strcmp(attr,"w+"))
            inode_area.i_mode = inode_area.i_mode |2;
        if(!strcmp(attr,"x-"))
            inode_area.i_mode = inode_area.i_mode &7776;
        if(!strcmp(attr,"x+"))
            inode_area.i_mode = inode_area.i_mode |1;
        printf("The file %s has been %s\n",tmp,attr);
        update_inode_entry(dir[k].inode);
        return;
    }
    flag = reserch_file(tmp,2,&i,&j,&k);
    if(flag){
        reload_inode_entry(dir[k].inode);
        if(!strcmp(attr,"r-"))
            inode_area.i_mode = inode_area.i_mode &4091;
        if(!strcmp(attr,"r+"))
            inode_area.i_mode = inode_area.i_mode |4;
        if(!strcmp(attr,"w-"))
            inode_area.i_mode = inode_area.i_mode &4093;
        if(!strcmp(attr,"w+"))
            inode_area.i_mode = inode_area.i_mode |2;
        if(!strcmp(attr,"x-"))
            inode_area.i_mode = inode_area.i_mode &7776;
        if(!strcmp(attr,"x+"))
            inode_area.i_mode = inode_area.i_mode |1;
        printf("The file %s has been %s\n",tmp,attr);
        update_inode_entry(dir[k].inode);
        return;
    }    
    printf("The file %s does not exist!\n",tmp);
}

//打开文件
void open_file(char tmp[9])
{
    unsigned short flag,i,j,k;
    flag=reserch_file(tmp,1,&i,&j,&k);
    if(flag)
    {
        if(test_fd(dir[k].inode))
        {
        	printf("The file %s has opened!\n",tmp);
            time_t rawtime;
            time(&rawtime);
            struct tm * timeinfo;
            timeinfo = localtime(&rawtime);
            reload_inode_entry(dir[k].inode);
            strcpy(inode_area.LastAccessTime,asctime(timeinfo));
            update_inode_entry(dir[k].inode);
        }
        else
        {
            flag=0;
            while(fopen_table[flag])
            {
            	flag++;
            }
            fopen_table[flag]=dir[k].inode;
            printf("File %s opened!\n",tmp);
        }
    }
    else printf("The file %s does not exist!\n",tmp);
}

//关闭文件
void close_file(char tmp[9])
{
    unsigned short flag,i,j,k;
    flag=reserch_file(tmp,1,&i,&j,&k);

    if(flag)
    {
        if(test_fd(dir[k].inode))
        {
            flag=0;
            while(fopen_table[flag]!=dir[k].inode)
            {
            	flag++;
            }
            fopen_table[flag]=0;
            printf("File %s closed!\n",tmp);
            time_t rawtime;
            time(&rawtime);
            struct tm * timeinfo;
            timeinfo = localtime(&rawtime);
            reload_inode_entry(dir[k].inode);
            strcpy(inode_area.LastAccessTime,asctime(timeinfo));
            update_inode_entry(dir[k].inode);
        }
        else
        {
        	printf("The file %s has not been opened!\n",tmp);
        }
    }
    else
    {
    	printf("The file %s does not exist!\n",tmp);
    }
}

//读取文件
void read_file(char tmp[9])
{
    unsigned short flag,i,j,k,t;
    flag=reserch_file(tmp,1,&i,&j,&k);
    if(flag)
    {
        if(test_fd(dir[k].inode)) //检查文件是否打开
        {
            reload_inode_entry(dir[k].inode);
            // 判断读取权限
            if(!(inode_area.i_mode&4)) // 1xx表示可读
            {
                printf("The file %s can not be read!\n",tmp);
                return;
            }
            for(flag=0;flag<inode_area.i_blocks;flag++)
            {
                reload_block(inode_area.i_block[flag]);//将数据加载到缓冲区并输出
                for(t=0;t<inode_area.i_size-flag*512;++t)
                {
                	printf("%c",Buffer[t]);
                }
            }
            if(flag==0)
            {
            	printf("The file %s is empty!\n",tmp);
            }
            else
            {
            	printf("\n");
            }
        }
        else
        {
        	printf("The file %s has not been opened!\n",tmp);
        }
    }
    else printf("The file %s not exists!\n",tmp);
}

// 写入文件
void write_file(char tmp[9]) //写文件
{
    unsigned short flag,i,j,k,size=0,need_blocks,length;
    flag=reserch_file(tmp,1,&i,&j,&k);
    if(flag)
    {
        if(test_fd(dir[k].inode))
        {
            reload_inode_entry(dir[k].inode);
            if(!(inode_area.i_mode&2)) // x1x检测可写
            {
                printf("The file %s can not be writed!\n",tmp);
                return;
            }
            fflush(stdin);
            while(1)
            {
                tempbuf[size]=getchar();//将写入数据冲入缓冲区
                if(tempbuf[size]=='#')
                {
                    tempbuf[size]='\0';
                    break;
                }
                if(size>=4095)
                {
                    printf("Sorry,the max size of a file is 4KB!\n");
                    break;
                }
                size++;
            }
            if(size>=4095)
            {
            	length=4096;
            }
            else
            {
            	length=strlen(tempbuf);
            }
            //计算所需数据块个数
            need_blocks=length/512;
            if(length%512)
            {
            	need_blocks++;
            }
            if(need_blocks<9) //文件最大可占用8个数据块
            {
                //分配文件所需块数目
                //采用写覆盖，需判断原有数据块数目
                if(inode_area.i_blocks<=need_blocks)
                {
                    while(inode_area.i_blocks<need_blocks)
                    {
                        inode_area.i_block[inode_area.i_blocks]=alloc_block();
                        inode_area.i_blocks++;
                    }
                }
                else
                {
                    while(inode_area.i_blocks>need_blocks)
                    {
                        remove_block(inode_area.i_block[inode_area.i_blocks - 1]);
                        inode_area.i_blocks--;
                    }
                }
                j=0;
                while(j<need_blocks)
                {
                    if(j!=need_blocks-1)
                    {
                        reload_block(inode_area.i_block[j]);
                        memcpy(Buffer,tempbuf+j*BLOCK_SIZE,BLOCK_SIZE);
                        update_block(inode_area.i_block[j]);
                    }
                    else
                    {
                        reload_block(inode_area.i_block[j]);
                        memcpy(Buffer,tempbuf+j*BLOCK_SIZE,length-j*BLOCK_SIZE);
                        inode_area.i_size=length;
                        update_block(inode_area.i_block[j]);
                    }
                    j++;
                }
                update_inode_entry(dir[k].inode);
                time_t rawtime;
                time(&rawtime);
                struct tm * timeinfo;
                timeinfo = localtime(&rawtime);
                reload_inode_entry(dir[0].inode);
                strcpy(inode_area.LastWriteTime,asctime(timeinfo));
                update_inode_entry(dir[0].inode);
                reload_inode_entry(dir[1].inode);
                strcpy(inode_area.LastWriteTime,asctime(timeinfo));
                update_inode_entry(dir[1].inode);
                reload_inode_entry(dir[k].inode);
                strcpy(inode_area.LastWriteTime,asctime(timeinfo));
                update_inode_entry(dir[k].inode);
                
            }
            else
            {
            	printf("Sorry,the max size of a file is 4KB!\n");
            }
        }
        else
        {
        	printf("The file %s has not opened!\n",tmp);
        }
    }
    else
    {
    	printf("The file %s does not exist!\n",tmp);
    }
}

// 查看目录下内容
void ls(void)
{
    printf("items          type           mode           size           CreateTime                LastWriteTime             LastAccessTime\n"); /* 15*4 */
    unsigned short i,j,k,flag;
    i=0;
    reload_inode_entry(current_dir);
    while(i<inode_area.i_blocks)
    {
        k=0;
        reload_dir(inode_area.i_block[i]);
        while(k<32)
        {
            if(dir[k].inode)
            {
                printf("%s",dir[k].name);
                //文件名
                if(dir[k].file_type==2)
                {
                    j=0;
                    reload_inode_entry(dir[k].inode);
                    if(!strcmp(dir[k].name,".."))
                    {
                    	while(j++<13)
                    	{
                    		printf(" ");
                    	}
                    	flag=1;
                    }
                    else if(!strcmp(dir[k].name,"."))
                    {
                    	while(j++<14)
                    	{
                    		printf(" ");
                    	}
                    	flag=0;
                    }
                    else
                    {
                    	while(j++<15-dir[k].name_len)
                    	{
                    		printf(" ");
                    	}
                    	flag=2;
                    }
                    printf("<DIR>          ");
                    switch(inode_area.i_mode&7)
                    {
                        case 1:printf("____x");break;
                        case 2:printf("__w__");break;
                        case 3:printf("__w_x");break;
                        case 4:printf("r____");break;
                        case 5:printf("r___x");break;
                        case 6:printf("r_w__");break;
                        case 7:printf("r_w_x");break;
                    }
                    if(flag!=2)
                    {
                    	printf("          ----");
                    }
                    else
                    {
                    	printf("          ");
                    	printf("%4ld bytes",inode_area.i_size);
                    }
                }
                else if(dir[k].file_type==1)
                {
                    j=0;
                    reload_inode_entry(dir[k].inode);
                    while(j++<15-dir[k].name_len)printf(" ");
                    printf("<FILE>         ");
                    switch(inode_area.i_mode&7)
                    {
                        case 1:printf("____x");break;
                        case 2:printf("__w__");break;
                        case 3:printf("__w_x");break;
                        case 4:printf("r____");break;
                        case 5:printf("r___x");break;
                        case 6:printf("r_w__");break;
                        case 7:printf("r_w_x");break;
                    }
                    printf("          ");
                    printf("%4ld bytes",inode_area.i_size);
                }
                //打印文件类型以及根据文件名长度调整打印操作权限
                int p=0;
                reload_inode_entry(dir[k].inode);
                if(!strcmp(dir[k].name,".."))
                    {
                        printf("           ");
                        for(p=0;p<24;p++)
                            printf("%c",inode_area.CreateTime[p]);
                        printf("  ");
                        for(p=0;p<24;p++)
                            printf("%c",inode_area.LastWriteTime[p]);
                        printf("  ");
                        for(p=0;p<24;p++)
                            printf("%c",inode_area.LastAccessTime[p]);
                        printf("  ");
                    }
                    
                    else if(!strcmp(dir[k].name,"."))
                    {
                        printf("           ");
                        for(p=0;p<24;p++)
                            printf("%c",inode_area.CreateTime[p]);
                        printf("  ");
                        for(p=0;p<24;p++)
                            printf("%c",inode_area.LastWriteTime[p]);
                        printf("  ");
                        for(p=0;p<24;p++)
                            printf("%c",inode_area.LastAccessTime[p]);
                        printf("  ");
                    }
                    else
                    {
                        printf("     ");
                        for(p=0;p<24;p++)
                            printf("%c",inode_area.CreateTime[p]);
                        printf("  ");
                        for(p=0;p<24;p++)
                            printf("%c",inode_area.LastWriteTime[p]);
                        printf("  ");
                        for(p=0;p<24;p++)
                            printf("%c",inode_area.LastAccessTime[p]);
                        printf("  ");
                    }
                printf("\n");
                //打印最近写入时间
            }
            k++;
            reload_inode_entry(current_dir);
        }
        i++;
    }
}

//检查磁盘
void check_disk(void)
{
	reload_group_desc();
    printf("usr name          : %s\n",gdt.bg_usrname);
	printf("volume name       : %s\n", gdt.bg_volume_name);
	printf("disk size         : %d(blocks)\n", gdt.sb_disk_size);
	printf("free blocks       : %d(blocks)\n", gdt.bg_free_blocks_count);
	printf("ext2 file size    : %d(kb)\n", gdt.sb_disk_size*gdt.sb_size_per_block/1024);
	printf("block size        : %d(kb)\n", gdt.sb_size_per_block);
}

//登录
void login(void){
    char usrname[12],password[12];
    printf("\nLog in to the MYEXT2 file system        \n");
    printf("Or you can press q to exit\n\n");
    reload_group_desc();
    while(1){
        printf("Username: ");
        scanf("%s",usrname);
        if(!strcmp(usrname,"q"))
            exit(0);
        if(strcmp(usrname,gdt.bg_usrname))
            printf("Can't find the user \"%s\".\n",usrname);
        else
            break;
    }
    while(1){
        printf("Password: ");
        scanf("%s",password);
        if(!strcmp(password,"q"))
            exit(0);
        if(strcmp(password,gdt.bg_password))
            printf("Password not correct! \n");
        else{
            printf("\nLogin successful!\n");
            break;
        }
    }
    printf("\n");
    
}
//修改密码
void Newpassword(void){
    char Opassword[12],Npassword[12],Cpassword[12];
    reload_group_desc();
    while(1){
        printf("Please enter the original password: ");
        scanf("%s",Opassword);
        if(strcmp(Opassword,gdt.bg_password))
            printf("Password not correct! \n");
        else{
            break;
        }
    }
    printf("Please enter the new password: ");
    scanf("%s",Npassword);
    strcpy(gdt.bg_password,Npassword);
    update_group_desc();
}