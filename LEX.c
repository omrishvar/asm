/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   LEX.c
 * Author: doron276
 * 
 * Created on 30 יולי 2018, 14:31
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "LEX.h"
#include "LINESTR.h"
#include "global.h"

BOOL LEX_Open(const char * szFileName){

    if(LINESTR_Open(szFileName) == FALSE){
        return FALSE;
    }
    return TRUE;
}




BOOL LEX_ReadNextToken(PLEX_TOKEN * pptToken){
    
}

//BOOL LEX_GetCurrentPosition(PLINESTR_LINE * pptLine){
    
//}

BOOL LEX_MoveToNextLine(){
    
}

void LEX_FreeToken(PLEX_TOKEN ptToken){
    
}

BOOL LEX_Close(){
    
}
