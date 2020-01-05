//
//  sefl.h
//  MyMalloc
//
//  Created by Ishan Shah on 1/4/20.
//  Copyright © 2020 Ishan Shah. All rights reserved.
//

#ifndef sefl_h
#define sefl_h

#include <stdio.h>
#include "MyMalloc.h"

//sefl
block_t* sefl_add_block(block_t* h,block_t* const block);
block_t* sefl_rm_block(block_t* h,block_t* const block);

block_t* sefl_find_fit(const block_t* const h,const size_t req_block_size);
block_t* sefl_find_first_fit(const block_t* const h,const size_t req_block_size);

//Debug
bool sefl_validate(const block_t* const h);
bool sefl_check_block(const block_t* const h, const block_t* const block);
void sefl_print(const block_t* const h);
#endif /* sefl_h */
