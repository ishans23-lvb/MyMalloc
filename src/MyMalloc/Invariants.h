//
//  Invariants.h
//  MyMalloc
//
//  Created by Ishan Shah on 12/14/19.
//  Copyright © 2019 Ishan Shah. All rights reserved.
//

#ifndef Invariants_h
#define Invariants_h

#include "MyMalloc.h"

//Block
bool isvalid_null_block(const block_t* const block);
bool isvalid_block(const block_t* const block);

//Page
bool isvalid_page(void* page);
bool isvalid_page_header(const ph_t* const page_header);
bool isvalid_page_footer(const pf_t* const page_footer);


//Basic Block
bool isvalid_block_size(const size_t block_size);
bool isvalid_block_addr(const block_t* const block);

//Basic Page
bool isvalid_page_size(const size_t page_size);
bool isvalid_page_addr(void* page);



#endif /* Invariants_h */
