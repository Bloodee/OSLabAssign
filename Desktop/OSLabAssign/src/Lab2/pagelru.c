#include <stdio.h>
#include <stdlib.h>
#include <time.h>
typedef struct Node {
    int data;
    struct Node* next;
} Node;

int cpn=0;
int pageMissCount = 0;

Node* createNode(int data) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    newNode->data = data;
    newNode->next = NULL;
    return newNode;
}

void insertAtHead(Node** head, int data) {
    Node* newNode = createNode(data);
    newNode->next = *head;
    *head = newNode;
}

Node* findNode(Node* head, int data) {
    Node* current = head;
    while (current != NULL) {
        if (current->data == data) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void deleteNode(Node** head, int data) {
    Node* current = *head;
    Node* prev = NULL;

    while (current != NULL && current->data != data) {
        prev = current;
        current = current->next;
    }

    if (current != NULL) {
        if (prev == NULL) {
            *head = current->next;
        } else {
            prev->next = current->next;
        }
        free(current);
    }
}

void deleteTail(Node** head) {
    Node* current = *head;
    Node* prev = NULL;

    while (current != NULL && current->next != NULL) {
        prev = current;
        current = current->next;
    }

    if (current != NULL) {
        if (prev == NULL) {
            *head = NULL;
        } else {
            prev->next = NULL;
        }
        free(current);
    }
}

void printList(Node* head) {
    Node* current = head;
    while (current != NULL) {
        printf("%d ", current->data);
        current = current->next;
    }
    printf("\n");
}

int main() {
    int i;
    int generate=0;
    int memorySize, pageSize;
    printf("input block number: ");
    scanf("%d", &memorySize);
	srand(time(NULL));
    printf("input page number: ");
    scanf("%d", &pageSize);

    int* pages = (int*)malloc(sizeof(int) * pageSize);
	printf("choose the way of creating page sequence\n");
	printf("input 1 for random sequence, 0 for your own sequence\n");
	
	scanf("%d",&generate);
	
	if(generate){
		printf("\nrandom page sequence:");
		for ( i = 0; i < pageSize; i++) {
	        pages[i] = rand() % ((memorySize) + 2)+1;
	        printf("%d ", pages[i]);
	    }
	    printf("\n");
	}
	else{
    	printf("input page sequence: ");
	    for ( i = 0; i < pageSize; i++) {
	        scanf("%d", &pages[i]);
	    }
	}
    printf("\n");

    Node* memory = NULL;
    Node* pageList = NULL;

    for (i = 0; i < pageSize; i++) {
        int currentPage = pages[i];
		printf("serch for page %d\n", currentPage);
        if (findNode(memory, currentPage) != NULL) {
        	printf("page hit\n");
            deleteNode(&memory, currentPage);
            insertAtHead(&memory, currentPage);
        } else {
        	
		printf("page missing\n");	
        if (cpn>=memorySize) {
    		printf("page evict\n");       	
            deleteTail(&memory);
            cpn--;
        }
			
            insertAtHead(&memory, currentPage);
            cpn++;
            pageMissCount++;
        }

        printf("current page situation£º");
        printList(memory);
        printf("\n");
    }

    printf("missing number: %d\n", pageMissCount);
    printf("missing rate: %.2f%%\n", (float)pageMissCount / pageSize * 100);

    free(pages);
    Node* current = memory;
    while (current != NULL) {
        Node* next = current->next;
        free(current);
        current = next;
    }
    current = pageList;
    while (current != NULL) {
        Node* next = current->next;
        free(current);
        current = next;
    }

    return 0;
}

