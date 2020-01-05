//
//  Error.h
//  MyMalloc2
//
//  Created by Ishan Shah on 12/1/19.
//  Copyright © 2019 Ishan Shah. All rights reserved.
//

#ifndef Error_h
#define Error_h

#ifdef __cplusplus
extern "C"{
#endif

void error_out(void) __attribute__((noreturn));
void print_error(const char* format,...) __attribute__((format(printf,1,2))) __attribute__((noreturn));
void print_bits(const unsigned long val,const int bits);



#ifdef __cplusplus
}
#endif

#endif /* Error_h */
