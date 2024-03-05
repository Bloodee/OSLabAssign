# Linux的EXT2文件系统与类EXT2的设计

## 程序结构

**底层**

*加载与更新部分*

当需要读取文件系统中信息时，需通过fseek函数将文件指针指向对应位置，在利用fread函数讲信息读入缓冲区，这是reload函数统一的逻辑

当需向文件系统中写入信息是，首先是先调用reload函数加载信息，再修改缓冲区中信息，然后通过fseek函数将文件指针指向正确位置，通过fwrite将修改后缓冲区的信息写入文件系统中，再及时将信息冲入文件中，这是update函数统一的逻辑

```c
static void update_group_desc(void)//更新组描述符
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

```
*具体底层函数实现策略*

分配数据块：顺着上一次经过分配的数据块向后分配一个数据块

```c
static int alloc_block(void) 
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
```

分配索引节点：与分配数据块逻辑相同，顺着上一次分配的数据块向后分配一块

```c

static int get_inode(void)
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
```

寻找文件：加载当前目录，找到所寻文件在当前目录的块号，再加载所寻文件，找到其在当前目录的目录项序号，通过目录项找到其inode序号，返回这三个序号。

```c   
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

```

准备文件：若为目录项，添加目录.与..分别代表当前目录及上一级目录；若为普通文件，将其初始化

```c
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
```

删除块：根据删除块的序号，将块位图对应位置处赋为0

```c
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

```

删除inode：根据所删除inode的序号，将inode块位图对应位置赋为0

```c
static void remove_inode(unsigned short del_num)
{
    unsigned short tmp;
    tmp=(del_num-1)/8;
    reload_inode_bitmap();
    switch((del_num-1)%8)
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
```

检测文件状况：遍历文件打开表，查询文件是否打开。

```c
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
```

**初始化文件系统**

初始化磁盘：清空所有缓冲区，清空文件，并对根目录进行初始化操作等等。

```c
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
```

初始化内存：若文件系统还未建立，则建立文件系统，若已经建立，直接登录文件系统

```c
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
```

**命令层**

格式化硬盘：调用initialize_disk和initialize_memory初始化磁盘和内存

运行结果:https://gitee.com/riac00/oslab_3/raw/master/img/format.jpg

```c
void format(void)
{
    initialize_disk();
    initialize_memory();
}
```

进入目录：实际上就是改变当前路径，调用reserch_file判断是否存在文件，若存在，将路径改为相应路径

运行结果：https://gitee.com/riac00/oslab_3/raw/master/img/cd.jpg

```c
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
            //当进入目录文件时更新访问时间
        }
    }
    else
    {
    	printf("The directory %s not exists!\n",tmp);
    }
}
```

创建目录：先寻找是否存在同名目录，若不存在，获取一个inode节点并分配dir_entry

运行结果：https://gitee.com/riac00/oslab_3/raw/master/img/mkdir.jpg

```c
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
```

创建文件：实现同创建目录文件类似

运行结果：https://gitee.com/riac00/oslab_3/raw/master/img/touch.jpg

```c
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
```

删除目录文件:调用reserch_file检索文件所在当前目录下的目录号，并加载其inode_entry，将各信息初始化，若目录文件中无其他文件，则调用remove_block和remove_inode,并检查指定 inode 的目录项，如果发现某个数据块中的所有目录项都为空，就删除该数据块并更新 inode 的相关信息，以清理数据块。

运行结果:https://gitee.com/riac00/oslab_3/raw/master/img/rmdir.jpg

```c
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
            //m从1开始，因为inode_area.i_block[0] 中有目录 . 和 ..所以这个数据块的非空 dir_entry 不可能为0
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
```

删除文件：具体实现与rmdir函数类似，若文件此时处于打开状态，需索引文件打开表并将对应位置清0

运行结果：https://gitee.com/riac00/oslab_3/raw/master/img/rm.jpg

```c
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
```

修改权限：寻找文件所在目录项位置，加载其inode_entry，对所修改权限位按要求取与或取或操作

运行结果：https://gitee.com/riac00/oslab_3/raw/master/img/attrib.jpg

```c
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
```

打开文件：调用reserch_file找到文件所在当前目录的位置，若寻找成功，检测该文件是否处于打开状态，若不是，则打开该文件

运行结果：https://gitee.com/riac00/oslab_3/raw/master/img/open.jpg

```c
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
```

关闭文件：调用reserch_file找到文件所在当前目录的位置，若寻找成功，检测该文件是否处于打开状态，若是，则关闭该文件

运行结果:https://gitee.com/riac00/oslab_3/raw/master/img/close.jpg

```c
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
```

读取文件：调用reserch_file找到文件所在当前目录的位置，若寻找成功，检测该文件是否处于打开状态，若是，则读取文件信息到缓冲区，并打印缓冲区内容

运行结果：https://gitee.com/riac00/oslab_3/raw/master/img/read.jpg

```c
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
```

写入文件：调用reserch_file找到文件所在当前目录的位置，若寻找成功，检测该文件是否处于打开状态，若是，则向缓冲区处写入数据，在调用memcpy函数将缓冲区数据冲入数据块，实现文件的覆写

运行结果：https://gitee.com/riac00/oslab_3/raw/master/img/write.jpg

```c
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
```

展示当前目录内容：加载当前路径目录的inode_entry，顺次检索目录下文件的各项属性，并按照一定格式进行打印

运行结果：https://gitee.com/riac00/oslab_3/raw/master/img/ls.jpg

```c
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
```

检查磁盘：打印磁盘各项属性

运行结果：https://gitee.com/riac00/oslab_3/raw/master/img/ckdisk.jpg

```c
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
```

登入文件系统:匹配用户名与密码，若正确则进入系统，错误可选择重新输入或退出系统

运行结果：https://gitee.com/riac00/oslab_3/raw/master/img/login.jpg

```c
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
```

修改密码：加载组描述符，输入旧密码和新密码来修改密码

运行结果：https://gitee.com/riac00/oslab_3/raw/master/img/pwd.jpg

```c
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
```


## 实现过程中的问题

**在设计文件系统时数据结构遇到的问题**

1.inode中的访问、修改和创建时间的获取难以用一4字节无符号数表示，于是我将其类型修改为字符串，并调整inode大小
2.在合并superblock和group_desc内容时，需对一些重复信息进行筛选
3.斟酌选用哪种数据结构来表示数据快位图和inode块位图，结果应采取简单易索引的数组来表示

**实现底层函数时遇到的挑战问题**

1.在创建文件时应对文件采取相应的初始化操作，若在其余函数中都实现相同的重复操作会使代码非常繁琐，因而加入函数dir_prepare来实现这一操作
2.在初始化磁盘和内存时，对于各结构的加载和更新需要有一个清晰的操作过程，不然很容易出现难以发现的问题。

**调试代码时遇到的挑战问题**

问题：

1.使用mkdir建立文件夹时会清除当前文件中其他所有的文件，且所期待建立的文件夹也不会建立。
2.Format格式化硬盘时，存储空间等信息恢复原样，但所创建过的文件内容和读写控制被情况，但文件仍旧存在。
3.在修改文件的LastWritetime时，无法对文件的上一级目录的该信息进行同步修改。
4.在进入二级目录时，无法成功创建文件。
5.利用rmdir删除目录时，若该目录中含有其他文件，会删除失败且进入一个错误的路径。

原因及解决的方法：

1.mkdir函数调用时将当前文件夹的所有文件包括其自身在内全部被初始化，经修改后功能正常。
2.在initializedisk函数中没有格式化索引节点的的各项信息，导致又一些信息残存在系统又无法使用，经过正确填充后问题解决。
3.添加上级目录名称变量以及索引节点变量，以在修改时可以实现对上一级目录信息的修改。
4.我在创建文件函数中修改时间时加载了新的inode_entry，导致原用于创建文件所加载到缓冲区的inode_entry被覆盖，导致创建出现问题，经调整后问题解决。
在rmdir中条件判断出现问题，导致不断递归的进行rmdir，出现一些难以调试的问题，于是将rmdir函数调整为：若目录中含有其他文件，则不能进行删除操作。
