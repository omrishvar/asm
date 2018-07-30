/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   list.c
 * Author: boazgildor
 * 
 * Created on July 27, 2018, 10:11 AM
 */
#if 0
#include "global.h"
#include "list.h"

#define HEADER_TO_CONTENT(header)  (header + sizeof(LIST_ELEMENT))
#define CONTENT_TO_HEADER(content)  (content - sizeof(LIST_ELEMENT))

typedef struct LIST_ELEMENT {
    struct LIST_ELEMENT *ptNext;
} LIST_ELEMENT, *PLIST_ELEMENT;

typedef struct LIST_HEADER {
    int iNumberOfElements;
    void *pHead;
    void *pTail;
} LIST_HEADER;

HLIST LIST_CreateList() {
    HLIST hList = NULL;
    hList = (HLIST)malloc(sizeof(*hList));
    if (NULL == hList) {
        return NULL;
    }
    hList->iNumberOfElements = 0;
    hList->pHead = NULL;
    hList->pTail = NULL;
    return hList;
}

void * LIST_CreateElementAsFirst(HLIST hList, size_t cbElementSize) {
    PLIST_ELEMENT pElementHeader = NULL;
    void * pElementContent = NULL;
    if (NULL == hList) {
        return NULL;
    }
    pElementHeader = malloc(sizeof(*pElementHeader) + cbElementSize);
    if (NULL == pElementHeader) {
        return NULL;
    }
    pElementContent = HEADER_TO_CONTENT(pElementHeader);
    pElementHeader->ptNext = hList->pHead;
    if (NULL == hList->pTail) {
        hList->pTail = pElementContent;
    }
    hList->pHead = pElementContent;
    return pElementContent;
}
    
#endif
