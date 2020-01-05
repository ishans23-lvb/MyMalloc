//
//  sefl.c
//  MyMalloc
//
//  Created by Ishan Shah on 1/4/20.
//  Copyright © 2020 Ishan Shah. All rights reserved.
//

#include "sefl.h"
#include "MyMalloc.h"
#include "Invariants.h"

block_t* sefl_find_fit(const block_t* const h,const size_t req_block_size)
{
    ASSERT(isvalid_block_size(req_block_size));
    ASSERT(sefl_validate(h));
    return sefl_find_first_fit(h,req_block_size);
}

block_t* sefl_find_first_fit(const block_t* const h,const size_t req_block_size)
{
    ASSERT(isvalid_block_size(req_block_size));
    ASSERT(sefl_validate(h));
    for(const block_t* block=h;block;block=block->next_block)
    {
        ASSERT(isvalid_block(block));
        ASSERT(!get_block_alloc(block));
        
        if(get_block_size(block)>=req_block_size)
            return (block_t*)block;
    }
    return NULL;
}

block_t* sefl_add_block(block_t* h,block_t* const block)
{
    ASSERT(block);
    ASSERT(isvalid_block(block));
    ASSERT(!get_block_alloc(block));
    ASSERT( h!=block || !(block->prev_block));
    ASSERT(sefl_validate(h));
    ASSERT(!sefl_check_block(h,block));
    if(h)
    {
        ASSERT(!(h->prev_block));
        h->prev_block=block;
    }
    
    block->next_block=h;
    block->prev_block=NULL;
    h=block;
    
    ASSERT(isvalid_block(block));
    ASSERT(!get_block_alloc(block));
    ASSERT(!(block->prev_block) || (block->prev_block)->next_block==block);
    ASSERT(!(block->next_block) || (block->next_block)->prev_block==block);
    ASSERT((block->prev_block) || h==block);
    ASSERT(sefl_validate(h));
    ASSERT(sefl_check_block(h,block));
    return h;
}

block_t* sefl_rm_block(block_t* h,block_t* const block)
{
    ASSERT(block);
    ASSERT(isvalid_block(block));
    ASSERT(!get_block_alloc(block));
    ASSERT(!(block->prev_block) || (block->prev_block)->next_block==block);
    ASSERT(!(block->next_block) || (block->next_block)->prev_block==block);
    ASSERT(sefl_validate(h));
    ASSERT(sefl_check_block(h,block));
    if(block->prev_block)
    {
        ASSERT(isvalid_block(block->prev_block));
        ASSERT(!get_block_alloc(block->prev_block));
        ASSERT((block->prev_block)->next_block==block);
        (block->prev_block)->next_block=block->next_block;
    }
    if(block->next_block)
    {
        ASSERT(isvalid_block(block->next_block));
        ASSERT(!get_block_alloc(block->next_block));
        ASSERT((block->next_block)->prev_block==block);
        (block->next_block)->prev_block=block->prev_block;
    }
    
    if(h==block)
    {
        ASSERT(!(block->prev_block));
        h=block->next_block;
    }
    
    block->next_block=NULL;
    block->prev_block=NULL;
    
    ASSERT(isvalid_block(block));
    ASSERT(!get_block_alloc(block));
    ASSERT(sefl_validate(h));
    ASSERT(!sefl_check_block(h,block));
    return h;
}

bool sefl_validate(const block_t* const h)
{
    for(const block_t* t=h;t;t=t->next_block)
    {
        ASSERT(isvalid_block(t));
        ASSERT(!get_block_alloc(t));
        
        if(t->next_block && (t->next_block)->prev_block!=t)
            return false;
        if(t->prev_block && (t->prev_block)->next_block!=t)
            return false;
        if(!(t->prev_block) && t!=h)
            return false;
        if(t==h && t->prev_block)
            return false;
    }
    
    return true;
}

bool sefl_check_block(const block_t* const h, const block_t* const block)
{
    ASSERT(isvalid_block(block));
    ASSERT(!get_block_alloc(block));
    for(const block_t* t=h;t;t=t->next_block)
    {
        ASSERT(isvalid_block(block));
        if(t==block)
            return true;
    }
    return false;
}

void sefl_print(const block_t* const h)
{
    ASSERT(sefl_validate(h));
    for(const block_t* t=h;t;t=t->next_block)
    {
        printf("Block: %p\nSize: %lu\nPrev: %p\nNext: %p\n",t,get_block_size(t),t->prev_block,t->next_block);
    }
}
