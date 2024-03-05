#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#define PROCESS_NAME_LEN 32
#define MIN_SLICE 10 
#define DEFAULT_MEM_SIZE 1024 
#define DEFAULT_MEM_START 0 

#define MA_FF 1
#define MA_BF 2
#define MA_WF 3
int mem_size=DEFAULT_MEM_SIZE; 
int ma_algorithm = MA_FF; 
static int pid = 0; 
int flag = 0;

struct free_block_type{
 int size;
 int start_addr;
 struct free_block_type *next;
}; 

struct free_block_type *free_block;

struct allocated_block{
 int pid; int size;
 int start_addr;
 char process_name[PROCESS_NAME_LEN];
 struct allocated_block *next;
 };

struct allocated_block *allocated_block_head = NULL;

struct free_block_type *init_free_block(int mem_size){
    struct free_block_type *fb;
    fb=(struct free_block_type *)malloc(sizeof(struct free_block_type));
    if(fb==NULL){
        printf("No mem\n");
        return NULL;
    }
    fb->size = mem_size;
    fb->start_addr = DEFAULT_MEM_START;
    fb->next = NULL;
    return fb;
}
void do_exit(){
	while(free_block->next!= NULL&&free_block !=NULL){
		struct free_block_type *temp = free_block;
		free_block = free_block->next;
		free(temp);
	}
	if(free_block)
		free(free_block);
}

void display_menu(){
    printf("\n");
    printf("1 - Set memory size (default=%d)\n", DEFAULT_MEM_SIZE);
    printf("2 - Select memory allocation algorithm\n");
    printf("3 - New process \n");
    printf("4 - Terminate a process \n");
    printf("5 - Display memory usage \n");
    printf("0 - Exit\n");
}
int set_mem_size(){
    int size;
    if(flag!=0){ 
    printf("Cannot set memory size again\n");
    return 0;
    }
    printf("Total memory size =");
    scanf("%d", &size);
    if(size>0) {
        mem_size = size;
        free_block->size = mem_size;
    }
    flag=1; return 1;
}

int rearrange_FF(){ 
    struct free_block_type *fbt = free_block;
    struct free_block_type *cfb;
     while(fbt->next != NULL){
        cfb = fbt->next;
        while(cfb != NULL){
            if(fbt->start_addr > cfb->start_addr){
            
                int temp_size = fbt->size;
                int temp_start = fbt->start_addr;
                fbt->size = cfb->size;
                fbt->start_addr = cfb->start_addr;
                cfb->size = temp_size;
                cfb->start_addr = temp_start;
            }
            cfb = cfb->next;
        }
        fbt = fbt->next;
     }
     return 1;
}
    
int rearrange_BF(){
     struct free_block_type *fbt = free_block;
     struct free_block_type *cfb;
     while(fbt->next != NULL){
         cfb = fbt->next;
         while(cfb != NULL){
             if(fbt->size > cfb->size){
               
                 int temp_size = fbt->size;
                 int temp_start = fbt->start_addr;
                 fbt->size = cfb->size;
                 fbt->start_addr = cfb->start_addr;
                 cfb->size = temp_size;
                 cfb->start_addr = temp_start;
             }
             cfb = cfb->next;
         }
         fbt = fbt->next;
     }
	return 1;
}

int rearrange_WF(){
     struct free_block_type *fbt ;
	 fbt = free_block;
     struct free_block_type *cfb;
     while(fbt->next != NULL){
         cfb = fbt->next;
         while(cfb != NULL){
             if(fbt->size < cfb->size){

                 int temp_size = fbt->size;
                 int temp_start = fbt->start_addr;
                 fbt->size = cfb->size;
                 fbt->start_addr = cfb->start_addr;
                 cfb->size = temp_size;
                 cfb->start_addr = temp_start;
             }
             cfb = cfb->next;
         }
         fbt = fbt->next;
     }
}

int rearrange(int algorithm){
    switch(algorithm){
    case MA_FF: return rearrange_FF(); break;
    case MA_BF: return rearrange_BF(); break;
    case MA_WF: return rearrange_WF(); break;
 }
}

void set_algorithm(){
    int algorithm;
    printf("\t1 - First Fit\n");
    printf("\t2 - Best Fit \n");
    printf("\t3 - Worst Fit \n");
    scanf("%d", &algorithm);
    if(algorithm>=1 && algorithm <=3) 
        ma_algorithm=algorithm;

    rearrange(ma_algorithm); 
}

int allocate_mem(struct allocated_block *ab){
 	struct free_block_type *fbt, *pre;
 	int request_size=ab->size;
 	fbt = pre = free_block;
	while(fbt != NULL){
		if(fbt->size >= request_size){ 
			if(fbt->size - request_size <= MIN_SLICE){ 
		  		ab->start_addr = fbt->start_addr;
		 		ab->size = fbt->size;
				if(fbt == free_block){ 
					free_block = fbt->next; 
				}
				else{
					 pre->next = fbt->next; 
				}
				
			    free(fbt); 

		}
			else{ 
			    ab->start_addr = fbt->start_addr;
			    ab->size = request_size;
			    fbt->start_addr += request_size; 
			    fbt->size -= request_size; 
		    }
		rearrange(ma_algorithm);
	    return 1; 
	   }
    pre = fbt; 
	fbt = fbt->next; 
 	}
	if(rearrange(ma_algorithm) == 1){ 
		pre = fbt = free_block; 
		while(fbt != NULL){
			if(fbt->size >= request_size){ 
			if(fbt->size - request_size <= MIN_SLICE){ 
		  		ab->start_addr = fbt->start_addr;
		 		ab->size = fbt->size;
				if(fbt == free_block){ 
				    
					    free_block = fbt->next; 
					}
					else{
					    pre->next = fbt->next; 
					}
			    free(fbt);
			    }
			 else{ 
				    ab->start_addr = fbt->start_addr;
				    ab->size = request_size;
				    fbt->start_addr += request_size; 
				    fbt->size -= request_size; 
			    }
			    rearrange(ma_algorithm);
			    return 1;
		    }
             else if(fbt->size < MIN_SLICE){ 
                return -1; 
	}
		    pre = fbt; 
		    fbt = fbt->next; 
	  	}	
 	}
 return -1; 
}


int new_process(){
    struct allocated_block *ab;
    int size; int ret;
    ab=(struct allocated_block *)malloc(sizeof(struct allocated_block));
    if(!ab) exit(-5);
    ab->next = NULL;
    pid++;
    sprintf(ab->process_name, "PROCESS-%02d", pid);
    ab->pid = pid; 
    printf("Memory for %s:", ab->process_name);
    scanf("%d", &size);
    if(size>0) ab->size=size;
    ret = allocate_mem(ab);
    if((ret==1) &&(allocated_block_head == NULL)){ 
        allocated_block_head=ab;
        return 1; }

    else if (ret==1) {
        ab->next=allocated_block_head;
        allocated_block_head=ab;
        return 2; }
    else if(ret==-1){ 
    	pid--;
        printf("Allocation fail\n");
        free(ab);
        return -1; 
    }
    return 3;
}


struct allocated_block *find_process(int pid){
    struct allocated_block *ab = allocated_block_head; 
    while(ab != NULL){ 
        if(ab->pid == pid){ 
            return ab; 
        }
        ab = ab->next; 
    }
    return NULL; 
}



int free_mem(struct allocated_block *ab){
    int algorithm = ma_algorithm;
    struct free_block_type *fbt, *pre;
    fbt=(struct free_block_type*) malloc(sizeof(struct free_block_type));
    if(!fbt) return -1;

    fbt->size=ab->size;
    fbt->start_addr=ab->start_addr;
    fbt->next=free_block;
    free_block = fbt;
    
	rearrange_FF();
	pre = free_block;
	struct free_block_type *next = pre->next;
    
    while(next != NULL){  // Modify this line
        if(pre->start_addr + pre->size == next->start_addr){ 
            pre->size += next->size; 
            struct free_block_type *temp = next;
            pre->next = next->next; 
            free(temp); 
            next = pre->next;  // Add this line
        }
        else{ 
            pre = pre->next; 
            next = next->next;  // Add this line
        }
    }
    rearrange(ma_algorithm);
    return 0;
}

int dispose(struct allocated_block *free_ab){
    struct allocated_block *pre, *ab;
    if(free_ab == allocated_block_head) { 
    allocated_block_head = allocated_block_head->next;
    free(free_ab);
    return 1;
    }
    pre = allocated_block_head; 
    ab = allocated_block_head->next;
    while(ab!=free_ab){ pre = ab; ab = ab->next; }
    pre->next = ab->next;
    free(ab);
    return 2;
 }

void kill_process(){
    struct allocated_block *ab;
    int pid;
    printf("Kill Process, pid=");
    scanf("%d", &pid);
	ab = find_process(pid);
	if(ab!=NULL){
        free_mem(ab); 
        dispose(ab);
        printf("\nProcess %d has been killed\n", pid); 
    }
    else
        printf("\nThere is no Process %d\n",pid);
}

int display_mem_usage(){
    struct free_block_type *fbt= free_block;
    struct allocated_block *ab=allocated_block_head;
    printf("----------------------------------------------------------\n");
    printf("Free Memory:\n");
    printf("%20s %20s\n", " start_addr", " size");
    while(fbt!=NULL){
        printf("%20d %20d\n", fbt->start_addr, fbt->size);
        fbt=fbt->next;
 } 

 printf("\nUsed Memory:\n");
 printf("%10s %20s %10s %10s\n", "PID", "ProcessName", "start_addr", " size");
 while(ab!=NULL){
    printf("%10d %20s %10d %10d\n", ab->pid, ab->process_name, 
    ab->start_addr, ab->size);
    ab=ab->next;
 }
 printf("----------------------------------------------------------\n");
 return 0;
 }

int main(){
    char choice; pid=0;
    free_block = init_free_block(mem_size); 
    while(1) {
    display_menu(); 
        fflush(stdin);
        choice=getchar(); 
        switch(choice){
            case '1': set_mem_size(); break; 
            case '2': set_algorithm();flag=1; break;
            case '3': new_process(); flag=1;break;
            case '4': kill_process(); flag=1; break;
            case '5': display_mem_usage(); flag=1; break; 
            case '0': do_exit(); exit(0); 
            default: break; 
            } 
        } 
    return 0;
}