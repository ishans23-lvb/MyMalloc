//
//  Error.c
//  MyMalloc2
//
//  Created by Ishan Shah on 12/1/19.
//  Copyright © 2019 Ishan Shah. All rights reserved.
//

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <assert.h>
#include "Error.h"


void error_out(void)
{
#ifdef DEBUG
    abort();
#else
    exit(-1);
#endif
}

void print_error(const char* format,...)
{
    const char* err_msg=strerror(errno);
    size_t new_format_size=strlen(format)+strlen(": ")+strlen(err_msg)+1;
    char* new_format=alloca(new_format_size);
    
    strcpy(new_format,format);
    strcat(new_format,": ");
    strcat(new_format,err_msg);
    
    va_list lst;
    va_start(lst,format);
    
    vfprintf(stderr,new_format,lst);
    
    error_out();
}

void print_bits(const unsigned long val,const int bits)
{
    assert(bits>0 && bits<=64);
    int bits_printed=0;
    for(unsigned long mask=(1LU<<(bits-1));mask;mask>>=1)
    {
        if(bits_printed>=4)
        {
            bits_printed=0;
            fprintf(stderr," ");
        }
        fprintf(stderr,"%d",val&mask ? 1 : 0);
        bits_printed++;
    }
}
