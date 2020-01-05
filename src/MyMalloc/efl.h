//
//  efl.h
//  MyMalloc
//
//  Created by Ishan Shah on 12/30/19.
//  Copyright © 2019 Ishan Shah. All rights reserved.
//

#ifndef efl_h
#define efl_h

#include "MyMalloc.h"

#ifdef __cplusplus
extern "C" {
#endif

int efl_add_block(block_t* const block);
int efl_rm_block(block_t* const block);

block_t* efl_find_fit(const size_t req_block_size);
void efl_init(void);



//Debug
bool efl_validate(void);
bool efl_check_block(const block_t* const block);
void efl_print(void);

#ifdef __cplusplus
}
#endif

#endif /* efl_h */
