/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   list.h
 * Author: boazgildor
 *
 * Created on July 27, 2018, 10:11 AM
 */
#if 0
#ifndef LIST_H
#define LIST_H

#include <stdio.h>
#include <stdlib.h>

typedef struct LIST_HEADER *HLIST;

HLIST LIST_CreateList();
void * LIST_CreateElementAsFirst(HLIST hList, size_t cbElementSize);
void * LIST_CreateElementAsLast(HLIST hList, size_t cbElementSize);
void * LIST_GetFirstElement(HLIST hList);
void * LIST_GetNextElement(void * pElement);
void LIST_Free();

#endif /* LIST_H */
#endif