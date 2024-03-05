#include<stdio.h>
#include<stdlib.h>
#include <time.h>

typedef struct page{
	int num;
	int status;
	struct page *next;
}page;

page *head;
page *tail;

int repos=0;

int pagenum;
int blocknum;
int *page_seq;
int missingcount;

void init(){
	int i;
	head = (page *)malloc(sizeof(page));
	tail=head;
    head->num = 0;
    head->status = 0;
    page *Pp[blocknum];
    for(i =0; i < blocknum-1;i++){
    	Pp[i] = (page *)malloc(sizeof(page));
        Pp[i]->num = 0;
        Pp[i]->status = 0;
	}
	for(i =0;i<blocknum-2;i++){
		Pp[i]->next = Pp[i+1];
	}
	if(Pp[0])
		head->next =Pp[0];
	if(blocknum>=2)
		tail = Pp[blocknum-2];
	tail->next=NULL;
}

void print(){
	page *p = head;
    printf("Page £º");
    while (p != NULL) {
        printf("%d ", p->num);
        p = p->next;
    }
    printf("\n");
    printf("The poisition of head£º%d\n", head->num);
    printf("The poisition of tail£º%d\n", tail->num);
    printf("\n");
}

int search(int p) {
    page *q = head;
    while (q != NULL) {
        if (q->num == p) {
            return 1;
        }
        q = q->next;
    }
    return 0;
}

page *find_free() {
    page *q = head;
    while (q != NULL) {
        if (q->status == 0) {
            return q;
        }
        q = q->next;
    }
    return NULL;
}

void replace(int p) {
	int i;
	page *P =(page *)malloc(sizeof(page));
	P =head;
    for(i=0;i<repos;i++){
		P=P->next;
	}
	P->num = p;
	repos = (repos+1)%blocknum;
}

void simulate(){
	int i;
	for(i=0;i<pagenum;i++){
		printf("serch page: %d\n",page_seq[i]);
		if(search(page_seq[i])){
			printf("page hit! \n");
		}
		else{
			printf("page missing \n");
			missingcount++;
			page *fb = find_free();
			if(fb){
				printf("There is free block, fill it in\n");
				fb->num = page_seq[i];
                fb->status = 1;
			}
			else{
				printf("No free block, replace\n");
				replace(page_seq[i]);
			}
			print();
		}
	}
}

int main(){
	int i;int random=0;
	srand(time(NULL));
	printf("input block number£º\n");
    scanf("%d", &blocknum);
	printf("input page number£º\n");
    scanf("%d", &pagenum);
    page_seq = (int *)malloc(sizeof(int) * pagenum);
    printf("choose the way of creating page sequence\n");
	printf("input 1 for random sequence, 0 for your own sequence\n");
	
	scanf("%d",&random);
	if(random){
		printf("\nrandom page sequence:");
		for ( i = 0; i < pagenum; i++) {
	        page_seq[i] = rand() % ((blocknum) + 2)+1;
	        printf("%d ", page_seq[i]);
	    }
	    printf("\n");
	}
	else{
    	printf("input page sequence: ");
	    for ( i = 0; i < pagenum; i++) {
	        scanf("%d", &page_seq[i]);
	    }
	}
	
    init();
    simulate();
    printf("page missing number£º%d\n", missingcount);
    printf("page missing rate£º%.2f%%\n", (double)missingcount / pagenum * 100);
    return 0;
}
