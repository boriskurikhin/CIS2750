#include "LinkedListAPI.h"

#define DEBUG_LIST false

/*
    This linked list library was not created by me.
*/

List * initializeList(char* (*printFunction)(void* toBePrinted),void (*deleteFunction)(void* toBeDeleted),int (*compareFunction)(const void* first,const void* second)){
    List * newList = malloc(sizeof(List));
    newList->head = NULL;
    newList->tail = NULL;
    newList->length = 0;
    newList->deleteData = deleteFunction;
    newList->compare = compareFunction;
    newList->printData = printFunction;
    return newList;
}

Node* initializeNode(void* data){
    Node* newNode = malloc(sizeof(Node));
    newNode->data = data;
    newNode->next = NULL;
    newNode->previous = NULL;
    return newNode;
}

void insertFront(List* list, void* toBeAdded){
    
    if(list == NULL || toBeAdded == NULL){
        return;
    }
    
    Node* newNode = initializeNode(toBeAdded);
    
    if(list->head == NULL){
        
        list->head = newNode;
        list->tail = newNode;
    }else{
        list->head->previous = newNode;
        newNode->next = list->head;
        list->head = newNode;
    }
    list->length++;
}

void insertBack(List* list, void* toBeAdded){
    
    if(list == NULL || toBeAdded == NULL){
        return;
    }
    
    Node* newNode = initializeNode(toBeAdded);
    
    if(list->tail == NULL){
        
        list->head = newNode;
        list->tail = newNode;
    }else{
        list->tail->next = newNode;
        newNode->previous = list->tail;
        list->tail = newNode;
    }
    list->length++;
}

void clearList(List* list){
    
    if(list == NULL){
        return;
    }
    
    Node* tempNode;
    Node* currentNode = list->head;
    
    while(currentNode != NULL){
        tempNode = currentNode;
        currentNode = currentNode->next;
        
        list->deleteData(tempNode->data);
        free(tempNode);
    }
    
    list->head = NULL;
    list->tail = NULL;
    list->length = 0;
}

void insertSorted(List* list, void* toBeAdded){
    
    if(list == NULL || toBeAdded == NULL){
        return;
    }
    
    Node* newNode = initializeNode(toBeAdded);
    Node* currentNode = currentNode = list->head;
    
    if(list->head == NULL && list->tail == NULL){
        list->head = newNode;
        list->tail = newNode;
        list->length++;
        return;
    }
    
    while(currentNode != NULL){
        
        if (list->compare(newNode->data, currentNode->data) < 0) {
            list->head = newNode;
            newNode->next = currentNode;
            currentNode->previous = newNode;
            list->length++;
            return;
        }
        
        if (currentNode->next == NULL) {
            list->tail = newNode;
            newNode->previous = currentNode;
            currentNode->next = newNode;
            list->length++;
            return;
        }
        
        if (list->compare(newNode->data, currentNode->data) >= 0 && list->compare(newNode->data, currentNode->next->data) < 0) {
            newNode->previous = currentNode;
            newNode->next = currentNode->next;
            currentNode->next = currentNode->next->previous = newNode;
            list->length++;
            return;
        }
        
        currentNode = currentNode->next;
    }
}

void* deleteDataFromList(List* list, void* toBeDeleted){
    
    if(list == NULL || toBeDeleted == NULL){
        return NULL;
    }
    
    void* nodeToBeReturned;
    Node* nodeToBeDeleted;
    Node* currentNode = list->head;
    
    while(currentNode != NULL){
        
        if(list->compare(currentNode->data, toBeDeleted) == 0){
            
            if(currentNode->previous == NULL){
                nodeToBeDeleted = currentNode;
                list->head = currentNode->next;
                list->head->previous = NULL;
                
                nodeToBeReturned = currentNode->data;
                free(nodeToBeDeleted);
                list->length--;
                return nodeToBeReturned;
            }
            
            if(currentNode->next == NULL){
                nodeToBeDeleted = currentNode;
                list->tail = currentNode->previous;
                list->tail->next = NULL;
                
                nodeToBeReturned = currentNode->data;
                free(nodeToBeDeleted);
                list->length--;
                return nodeToBeReturned;
            }
            
            if(currentNode->next != NULL && currentNode->previous != NULL){
                nodeToBeDeleted = currentNode;
                currentNode->previous->next = currentNode->next;
                currentNode->next->previous = currentNode->previous;
                
                nodeToBeReturned = currentNode->data;
                free(nodeToBeDeleted);
                list->length--;
                return nodeToBeReturned;
            }
        }
        
        currentNode = currentNode->next;
    }
    return NULL;
}

void* getFromFront(List *l){
    List list = *l;
    
    if(list.head != NULL){
        return list.head->data;
    }else{
        return NULL;
    }
}

void* getFromBack(List *l){
    List list = *l;
    
    if(list.tail != NULL){
        return list.tail->data;
    }else{
        return NULL;
    }
}

char* toString(List *l){
    List list = *l;
    
    if(list.head == NULL || list.tail == NULL){
        char* feedback = calloc(1, sizeof(char) + strlen("No List\n")+9);
        strcpy(feedback, "No list\n"); 
        return feedback;
    }
    
    char *string = calloc(1, sizeof(string)+9);
    int total = 0;
    Node* currentNode;
    
    currentNode = list.head;
    
    while (currentNode != NULL) {
        char* stringDataSize = list.printData(currentNode->data);
        total = total + strlen(stringDataSize) + 99;
        string = realloc(string, sizeof(char) + total);
        strcat(string, stringDataSize);
        free(stringDataSize);
        currentNode = currentNode->next;
    }
    strcat(string, "\0");

    if(DEBUG_LIST)printf("toString head: %s\n", list.printData(list.head->data));
    if(DEBUG_LIST)printf("toString next: %s\n", list.printData(list.head->next->data));
    if(DEBUG_LIST)printf("toString tail: %s\n", list.printData(list.tail->data));
    if(DEBUG_LIST)printf("toString prev: %s\n", list.printData(list.tail->previous->data));
    if(DEBUG_LIST)printf("toString return: %s\n", string);
    return string;
}

ListIterator createIterator(List *l){
    List list = *l;
    ListIterator iterator;
    iterator.current = list.head;
    return iterator;
}

void* nextElement(ListIterator* iter){
    void* dataToBeReturned;
    
    if(iter->current != NULL){
        dataToBeReturned = iter->current->data;
        iter->current = iter->current->next;
        return dataToBeReturned;
    }else{
        return NULL;
    }
}

int getLength(List *l){
    List list = *l;
    if(list.head == NULL || list.tail == NULL){
        list.length = 0;
        return 0;
    }
    
    int countNodes = 0;
    Node* currentNode = list.head;
    
    while(currentNode != NULL){
        countNodes++;
        list.length = countNodes;
        currentNode = currentNode->next;
    }
    list.length = countNodes;
    return countNodes;
}

void* findElement(List *l, bool (*customCompare)(const void* first,const void* second), const void* searchRecord){ 
    List list = *l;

    if(list.head == NULL) {
        return NULL;
    }
    
    Node* currentNode = list.head;
    
    while(currentNode != NULL){
        if(customCompare(currentNode->data, searchRecord) == true){
            return currentNode->data;
        }
        currentNode = currentNode->next;
    }
    return NULL;
}
