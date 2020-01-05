//
//  efl.c
//  MyMalloc
//
//  Created by Ishan Shah on 12/30/19.
//  Copyright © 2019 Ishan Shah. All rights reserved.
//

#include "efl.h"
#include "MyMalloc.h"
#include "Invariants.h"
#include "sefl.h"

struct mbin
{
    block_t* head;
    size_t min_size;
    size_t max_size;
};

struct
{
    struct mbin bins[5];
    //0: size==32B
    //1: 48B <= size <=80B
    //2: 96B<= size <= 4064
    //3: 4080<= size <= 5712
    //4: 5728 <= size
}fl;

static const size_t bin_size[5][2]=
{
    {32LU,32LU},
    {48LU,80LU},
    {96LU,4064},
    {4080LU,5712LU},
    {5728LU,-1},
};


int efl_add_block(block_t* const block)
{
    ASSERT(efl_validate());
    ASSERT(isvalid_block(block));
    ASSERT(!get_block_alloc(block));
    ASSERT(!efl_check_block(block));
    
    const size_t block_size=get_block_size(block);
    for(size_t i=0;i<sizeof(fl.bins)/sizeof(struct mbin);i++)
    {
        if(fl.bins[i].min_size<= block_size && block_size<=fl.bins[i].max_size)
        {
            fl.bins[i].head=sefl_add_block(fl.bins[i].head,block);
        }
    }
    
    ASSERT(isvalid_block(block));
    ASSERT(!get_block_alloc(block));
    ASSERT(efl_validate());
    ASSERT(efl_check_block(block));
    return 0;
}

int efl_rm_block(block_t* const block)
{
    ASSERT(efl_validate());
    ASSERT(isvalid_block(block));
    ASSERT(!get_block_alloc(block));
    ASSERT(efl_check_block(block));
    
    const size_t block_size=get_block_size(block);
    for(size_t i=0;i<sizeof(fl.bins)/sizeof(struct mbin);i++)
    {
        if(fl.bins[i].min_size<=block_size && block_size<=fl.bins[i].max_size)
        {
            ASSERT(sefl_check_block(fl.bins[i].head,block));
            fl.bins[i].head=sefl_rm_block(fl.bins[i].head,block);
        }
    }
    
    ASSERT(efl_validate());
    ASSERT(isvalid_block(block));
    ASSERT(!get_block_alloc(block));
    ASSERT(!efl_check_block(block));
    return 0;
}

block_t* efl_find_fit(const size_t req_block_size)
{
    ASSERT(efl_validate());
    ASSERT(isvalid_block_size(req_block_size));
    
    size_t si=-1;
    for(size_t i=0;i<sizeof(fl.bins)/sizeof(struct mbin);i++)
    {
        if(fl.bins[i].min_size<=req_block_size && req_block_size<=fl.bins[i].max_size)
        {
            si=i;
            break;
        }
    }
    
    for(size_t i=si;i<sizeof(fl.bins)/sizeof(struct mbin);i++)
    {
        block_t* block=sefl_find_fit(fl.bins[i].head,req_block_size);
        if(block)
            return block;
    }
    
    return NULL;
}

void fl_set_bin(block_t* const head,const size_t min_size,const size_t max_size,const unsigned long i)
{
    ASSERT(i<sizeof(fl.bins)/sizeof(struct mbin));
    ASSERT(isvalid_block_size(min_size));
    ASSERT(max_size==-1 || isvalid_block_size(max_size));
    ASSERT(!head);
    
    fl.bins[i].head=head;
    fl.bins[i].min_size=min_size;
    fl.bins[i].max_size=max_size;
}

void efl_init(void)
{
    for(size_t i=0;i<sizeof(fl.bins)/sizeof(struct mbin);i++)
    {
        fl_set_bin(NULL,bin_size[i][0],bin_size[i][1],i);
    }
    ASSERT(efl_validate());
}



bool efl_validate(void)
{
    ASSERT(sizeof(fl.bins)/sizeof(struct mbin)>0);
    for(size_t i=0;i<sizeof(fl.bins)/sizeof(struct mbin);i++)
    {
        if(i==0)
        {
            if(fl.bins[0].min_size!=MIN_BLOCK_SIZE)
                return false;
        }
        else
        {
            if(fl.bins[i].min_size<=fl.bins[i-1].max_size)
                return false;
            if(fl.bins[i].min_size!=block_round(fl.bins[i-1].max_size+1))
                return false;
        }
        
        
        if(i==sizeof(fl.bins)/sizeof(struct mbin)-1)
        {
            if(fl.bins[i].max_size!=-1)
                return false;
        }
        
        
        if(!isvalid_block_size(fl.bins[i].min_size) || (!isvalid_block_size(fl.bins[i].max_size) && fl.bins[i].max_size!=-1 ))
            return false;
        if(fl.bins[i].min_size>fl.bins[i].max_size)
            return false;
        if(!sefl_validate(fl.bins[i].head))
            return false;
        
        for(const block_t* t=fl.bins[i].head;t;t=t->next_block)
        {
            ASSERT(isvalid_block(t));
            ASSERT(!get_block_alloc(t));
            const size_t block_size=get_block_size(t);
            if(block_size<fl.bins[i].min_size || fl.bins[i].max_size<block_size)
            {
                return false;
            }
        }
    }
    return true;
}




bool efl_check_block(const block_t* const block)
{
    ASSERT(efl_validate());
    ASSERT(isvalid_block(block));
    ASSERT(!get_block_alloc(block));
    
    const size_t block_size=get_block_size(block);
    
    for(size_t i=0;i<sizeof(fl.bins)/sizeof(struct mbin);i++)
    {
        if(fl.bins[i].min_size<=block_size && block_size<=fl.bins[i].max_size)
        {
            return sefl_check_block(fl.bins[i].head,block);
        }
    }
    
    return false;
}

void efl_print(void)
{
    ASSERT(efl_validate());
    
    for(size_t i=0;i<sizeof(fl.bins)/sizeof(struct mbin);i++)
    {
        printf("Bin: %lu - %lu:\n",fl.bins[i].min_size,fl.bins[i].max_size);
        sefl_print(fl.bins[i].head);
        puts("\n\n");
    }
}
