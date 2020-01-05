//
//  MyMalloc.c
//  MyMalloc
//
//  Created by Ishan Shah on 12/14/19.
//  Copyright © 2019 Ishan Shah. All rights reserved.
//

#include "MyMalloc.h"
#include "efl.h"
#include "Invariants.h"
#include "Error.h"
#include <stdio.h>
#include <stdlib.h>


static void* malloc_first_page=NULL_PAGE;
static void* malloc_last_page=NULL_PAGE;
static bool smalloc=false;

#ifdef DEBUG
static size_t tmap_size=0;
size_t fl_tsize=0;
#endif

//malloc
void* my_malloc(const size_t size)
{
    ASSERT(isvalid_block_size(MIN_BLOCK_SIZE));
    ASSERT(sizeof(block_t)+sizeof(size_t)==MIN_BLOCK_SIZE);
    ASSERT(isvalid_block_size(sizeof(pf_t)+sizeof(ph_t)));
    ASSERT(sizeof(block_t)+sizeof(size_t)==MIN_BLOCK_SIZE);
    ASSERT(offsetof(block_t,header)<MIN_BLOCK_SIZE);
    ASSERT(offsetof(block_t,header)+sizeof(size_t)<=MIN_BLOCK_SIZE);
    ASSERT(offsetof(block_t,payload)<MIN_BLOCK_SIZE);
    ASSERT(offsetof(block_t,prev_block)<MIN_BLOCK_SIZE);
    ASSERT(offsetof(block_t,prev_block)+sizeof(block_t*)<=MIN_BLOCK_SIZE);
    ASSERT(offsetof(block_t,next_block)<MIN_BLOCK_SIZE);
    ASSERT(offsetof(block_t,next_block)+sizeof(block_t*)<=MIN_BLOCK_SIZE);
    ASSERT(validate_pages());
    ASSERT(!smalloc || efl_validate());
    
    if(!smalloc)
    {
        if(atexit(malloc_terminate)<0)
            print_error("atexit failed");
        efl_init();
        smalloc=true;
    }
    
    if(size==0)
        return NULL;
    
    size_t block_size=calc_block_size(size);
    ASSERT(isvalid_block_size(block_size));
    
    block_t* block=find_fit(block_size);
    
    if(!block)
    {
        size_t page_size=calc_page_size(block_size);
        page(page_size);
        block=find_fit(block_size);
    }
    
    ASSERT(block);
    ASSERT(isvalid_block(block));
    ASSERT(get_block_size(block)>=block_size);
    ASSERT(!get_block_alloc(block));
    
    alloc_block(block,block_size);
    
    ASSERT(isvalid_block(block));
    ASSERT(get_block_size(block)>=block_size);
    ASSERT(get_block_alloc(block));
    ASSERT(validate_pages());
    ASSERT(efl_validate());
    return block->payload;
}

void my_free(void* ptr)
{
    ASSERT(validate_pages());
    if(!ptr)
        return;
    
    block_t* block=get_block_from_payload(ptr);
    ASSERT(isvalid_block(block));
    ASSERT(get_block_alloc(block));
    
    block=free_block(block);
    ASSERT(isvalid_block(block));
    ASSERT(!get_block_alloc(block));
    ASSERT(validate_pages());
}

void malloc_terminate(void)
{
    ASSERT(validate_pages());
    
    while(malloc_first_page!=NULL_PAGE)
        unpage(malloc_first_page);
    
    ASSERT(malloc_first_page==NULL_PAGE && malloc_last_page==NULL_PAGE);
    ASSERT(validate_pages());
}

//Find Fit
block_t* find_fit(const size_t req_block_size)
{
    ASSERT(validate_pages());
    ASSERT(isvalid_block_size(req_block_size));
    return efl_find_fit(req_block_size);
}
block_t* first_fit_ifl(const size_t req_block_size)
{
    ASSERT(validate_pages());
    ASSERT(isvalid_block_size(req_block_size));
    
    for(void* page=malloc_first_page;page!=NULL_PAGE;page=get_next_page(page))
    {
        ASSERT(isvalid_page(page));
        ASSERT(validate_page(page));
        
        for(const block_t* block=get_page_first_block(page);!is_null_block(block);block=get_next_block(block))
        {
            ASSERT(isvalid_block(block));
            
            if(!get_block_alloc(block) && get_block_size(block)>=req_block_size)
                return (block_t*)block;
        }
    }
    return NULL;
}

//Block Manager
int alloc_block(block_t* const block,const size_t req_block_size)
{
    ASSERT(validate_pages());
    ASSERT(isvalid_block(block));
    ASSERT(isvalid_block_size(req_block_size));
    ASSERT(get_block_size(block)>=req_block_size);
    ASSERT(!get_block_alloc(block));
    
    ASSERT(efl_validate());
    ASSERT(efl_check_block(block));
    efl_rm_block(block);
    ASSERT(efl_validate());
    ASSERT(!efl_check_block(block));
    
    split_block(block,req_block_size);
    ASSERT(isvalid_block(block));
    ASSERT(get_block_size(block)>=req_block_size);
    ASSERT(!get_block_alloc(block));
    
    block_t* next_block=get_next_block(block);
    
    set_block_alloc(block,true);
    
    if(is_null_block(next_block))
    {
        ASSERT(isvalid_null_block(next_block));
        ASSERT(isvalid_page_footer((pf_t*)next_block));
        
        set_page_footer_palloc((pf_t*)next_block,true);
        
        ASSERT(isvalid_null_block(next_block));
        ASSERT(isvalid_page_footer((pf_t*)next_block));
    }
    else
    {
        ASSERT(isvalid_block(next_block));
        
        set_block_palloc(next_block,true);
        
        ASSERT(isvalid_block(next_block));
    }
    
    ASSERT(isvalid_block(block));
    ASSERT(get_block_size(block)>=req_block_size);
    ASSERT(get_block_alloc(block));
    ASSERT(validate_pages());
    return 0;
}



int split_block(block_t* const block,const size_t req_block_size)
{
    ASSERT(validate_pages());
    ASSERT(isvalid_block(block));
    ASSERT(isvalid_block_size(req_block_size));
    ASSERT(get_block_size(block)>=req_block_size);
    ASSERT(!get_block_alloc(block));
    ASSERT(efl_validate());
    ASSERT(!efl_check_block(block));
    
    const size_t block_size=get_block_size(block);
    
    if(block_size<req_block_size)
        return -1;
    
    if((block_size-req_block_size)>=MIN_BLOCK_SIZE)
    {
        block_t* const new_block=(block_t*)(((void*)block)+req_block_size);ASSERT(isvalid_block_addr(new_block));
        const size_t new_block_size=block_size-req_block_size;ASSERT(isvalid_block_size(new_block_size));
        
        write_block(block,req_block_size,false,get_block_palloc(block),get_block_fb(block));
        write_block(new_block,new_block_size,false,false,false);
        
        
        ASSERT(isvalid_block(block));
        ASSERT(isvalid_block(new_block));
        ASSERT(!get_block_alloc(block) && !get_block_alloc(new_block));
        
        efl_add_block(new_block);
        
        ASSERT(isvalid_block(block));
        ASSERT(isvalid_block(new_block));
        ASSERT(!get_block_alloc(block) && !get_block_alloc(new_block));
        ASSERT(efl_check_block(new_block));
    }
    
    ASSERT(isvalid_block(block));
    ASSERT(get_block_size(block)>=req_block_size);
    ASSERT(!get_block_alloc(block));
    ASSERT(validate_pages());
    ASSERT(efl_validate());
    ASSERT(!efl_check_block(block));
    return 0;
}

block_t* free_block(block_t* block)
{
    ASSERT(validate_pages());
    ASSERT(isvalid_block(block));
    ASSERT(get_block_alloc(block));
    
    block_t* next_block=get_next_block(block);
    set_block_alloc(block,false);
    
    if(!is_null_block(next_block))
    {
        ASSERT(isvalid_block(next_block));
        ASSERT(get_block_palloc(next_block));
        
        set_block_palloc(next_block,false);
        
        ASSERT(isvalid_block(next_block));
        ASSERT(!get_block_palloc(next_block));
    }
    else
    {
        ASSERT(isvalid_page_footer((pf_t*)next_block));
        ASSERT(get_page_footer_palloc((pf_t*)next_block));
        
        set_page_footer_palloc((pf_t*)next_block,false);
        
        ASSERT(isvalid_page_footer((pf_t*)next_block));
        ASSERT(!get_page_footer_palloc((pf_t*)next_block));
    }
    
    ASSERT(validate_pages());
    
    block_t* new_block=coalesce_block(block);
    ASSERT(isvalid_block(new_block));
    ASSERT(!get_block_alloc(new_block));
    ASSERT(validate_pages());
    
    ASSERT(efl_validate());
    ASSERT(!efl_check_block(new_block));
    efl_add_block(new_block);
    ASSERT(efl_validate());
    ASSERT(efl_check_block(new_block));
    
    ASSERT(isvalid_block(new_block));
    ASSERT(!get_block_alloc(new_block));
    ASSERT(validate_pages());
    return new_block;
}

block_t* coalesce_block(block_t* const block)
{
    ASSERT(validate_pages());
    ASSERT(isvalid_block(block));
    ASSERT(!get_block_alloc(block));
    ASSERT(efl_validate());
    ASSERT(!efl_check_block(block));
    
    block_t* new_block=block;
    block_t* prev_block=get_prev_block(block);
    block_t* next_block=get_next_block(block);
    
    if(!is_null_block(next_block) && !get_block_alloc(next_block))
    {
        ASSERT(isvalid_block(next_block));
        ASSERT(!get_block_palloc(next_block));
        ASSERT(!get_block_fb(next_block));
        
        efl_rm_block(next_block);
        
        size_t new_block_size=get_block_size(block)+get_block_size(next_block);
        bool new_block_fb=get_block_fb(block);
        bool new_block_palloc=get_block_palloc(block);
        
        ASSERT(isvalid_block_size(new_block_size));
        write_block(block,new_block_size,false,new_block_palloc,new_block_fb);
        ASSERT(isvalid_block(block));
        ASSERT(get_block_size(block)==new_block_size);
        ASSERT(!get_block_alloc(block));
        ASSERT(get_block_palloc(block)==new_block_palloc);
        ASSERT(get_block_fb(block)==new_block_fb);
    }
    if(prev_block)
    {
        ASSERT(isvalid_block(prev_block));
        ASSERT(isvalid_block(block));
        ASSERT(!get_block_alloc(prev_block));
        ASSERT(!get_block_alloc(block));
        ASSERT(!get_block_palloc(block));
        ASSERT(!get_block_fb(block));
        
        efl_rm_block(prev_block);
        
        size_t new_block_size=get_block_size(prev_block)+get_block_size(block);
        bool new_block_palloc=get_block_palloc(prev_block);
        bool new_block_fb=get_block_fb(prev_block);
        
        ASSERT(isvalid_block_size(new_block_size));
        write_block(prev_block,new_block_size,false,new_block_palloc,new_block_fb);
        new_block=prev_block;
        ASSERT(isvalid_block(new_block));
        ASSERT(get_block_size(new_block)==new_block_size);
        ASSERT(!get_block_alloc(new_block));
        ASSERT(get_block_palloc(new_block)==new_block_palloc);
        ASSERT(get_block_fb(new_block)==new_block_fb);
        
    }
    
    ASSERT(isvalid_block(new_block));
    ASSERT(!get_block_alloc(new_block));
    ASSERT(validate_pages());
    return new_block;
}

//Page Manager
void* page(const size_t page_size)
{
    ASSERT(isvalid_page_size(page_size));
    ASSERT(validate_pages());
    
    void* new_page=mmap(malloc_last_page==NULL_PAGE ? NULL : malloc_last_page+get_page_size(malloc_last_page),page_size,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,-1,0);
    if(new_page==NULL_PAGE)
    {
        print_error("mmap failed");
    }
    
    ASSERT((tmap_size+=page_size)>=page_size);
    
    ASSERT(isvalid_page_addr(new_page));
    
    write_page(new_page,page_size,malloc_last_page,NULL_PAGE);
    ASSERT(isvalid_page(new_page));
    
    if(malloc_last_page!=NULL_PAGE)
        set_next_page(malloc_last_page,new_page);
    else
        malloc_first_page=new_page;
    malloc_last_page=new_page;
    
    ASSERT(validate_page(new_page));
    ASSERT(validate_pages());
    ASSERT(malloc_last_page==new_page);
    
    new_page=coalesce_page(new_page);
    
    
    ASSERT(validate_page(new_page));
    ASSERT(validate_pages());
    return new_page;
}

//Only Use At Exit
int unpage(void* const page)
{
    ASSERT(isvalid_page(page));
    ASSERT(validate_page(page));
    ASSERT(validate_pages());
    
    void* const prev_page=get_prev_page(page);
    void* const next_page=get_next_page(page);
    const size_t page_size=get_page_size(page);
    
    if(prev_page!=NULL_PAGE)
    {
        ASSERT(validate_page(prev_page));
        ASSERT(get_next_page(prev_page)==page);
        
        set_next_page(prev_page,next_page);
        
        ASSERT(validate_page(prev_page));
    }
    else
    {
        ASSERT(malloc_first_page==page);
        malloc_first_page=next_page;
    }
    
    if(next_page!=NULL_PAGE)
    {
        ASSERT(validate_page(next_page));
        ASSERT(get_prev_page(next_page)==page);
        
        set_prev_page(next_page,prev_page);
        
        ASSERT(validate_page(next_page));
    }
    else
    {
        ASSERT(malloc_last_page==page);
        malloc_last_page=prev_page;
    }
    
    if(munmap(page,page_size)<0)
    {
        print_error("munmap(%p,%lu) failed",page,page_size);
    }
    
    ASSERT((tmap_size-=page_size)>=0);
    
    ASSERT(validate_pages());
    return 0;
}

void* coalesce_page(void* page)
{
    ASSERT(validate_pages());
    ASSERT(isvalid_page(page));
    ASSERT(validate_page(page));
    
    void* next_page=get_next_page(page);
    void* prev_page=get_prev_page(page);
    
    if(next_page!=NULL_PAGE && page+get_page_size(page)==next_page)
    {
        ASSERT(validate_page(page));
        ASSERT(validate_page(next_page));
        ASSERT(get_next_page(page)==next_page);
        ASSERT(get_prev_page(next_page)==page);
        ASSERT(malloc_first_page!=next_page);
        ASSERT(malloc_last_page!=page);
        ASSERT(false);
        
        
        size_t npage_size=get_page_size(page)+get_page_size(next_page);
        ph_t* npage_header=get_page_header(page);
        pf_t* npage_footer=get_page_footer(next_page);
        bool npf_palloc=get_page_footer_palloc(npage_footer);
        
        
        block_t* next_page_first_block=get_page_first_block(next_page);
        
        block_t* block=(block_t*)get_page_footer(page);
        ASSERT(isvalid_block_addr(block));ASSERT(is_null_block(block));ASSERT(isvalid_block_size(sizeof(pf_t)+sizeof(ph_t)));
        ASSERT(isvalid_page_footer((pf_t*)block));
        ASSERT(((void*)block)+(sizeof(pf_t)+sizeof(ph_t))==(void*)next_page_first_block);
        
        if(malloc_last_page==next_page)
            malloc_last_page=page;
        
        write_page_header(npage_header,npage_size,npage_header->prev_page);
        write_page_footer(npage_footer,npage_size,npage_footer->next_page);
        
        write_block(block,sizeof(pf_t)+sizeof(ph_t),false,get_page_footer_palloc((pf_t*)block),false);
        ASSERT(isvalid_block(block));
        ASSERT(((void*)block)+get_block_size(block)==(void*)next_page_first_block);
        
        set_page_footer_palloc(npage_footer,npf_palloc);
        set_block_fb(next_page_first_block,false);
        
        
        
        ASSERT(validate_page(page));
        ASSERT(validate_pages());
        ASSERT(get_next_block(block)==next_page_first_block);
        ASSERT(efl_validate());
        ASSERT(!efl_check_block(block));
        
        block=coalesce_block(block);
        ASSERT(isvalid_block(block));
        ASSERT(!get_block_alloc(block));
        ASSERT(get_block_size(block)>=(sizeof(pf_t)+sizeof(ph_t)));
        ASSERT(validate_page(page));
        ASSERT(validate_pages());
        ASSERT(efl_validate());
        ASSERT(!efl_check_block(block));
        
        efl_add_block(block);
        ASSERT(isvalid_block(block));
        ASSERT(!get_block_alloc(block));
        ASSERT(get_block_size(block)>=(sizeof(pf_t)+sizeof(ph_t)));
        ASSERT(validate_page(page));
        ASSERT(validate_pages());
        ASSERT(efl_validate());
        ASSERT(efl_check_block(block));
    }
    if(prev_page!=NULL_PAGE && prev_page+get_page_size(prev_page)==page)
    {
        ASSERT(validate_page(prev_page));
        ASSERT(validate_page(page));
        ASSERT(get_next_page(prev_page)==page);
        ASSERT(get_prev_page(page)==prev_page);
        ASSERT(malloc_last_page!=prev_page);
        ASSERT(malloc_first_page!=page);
        
        
        size_t npage_size=get_page_size(prev_page)+get_page_size(page);
        ph_t* npage_header=get_page_header(prev_page);
        pf_t* npage_footer=get_page_footer(page);
        bool npf_palloc=get_page_footer_palloc(npage_footer);
        block_t* next_page_first_block=get_page_first_block(page);
        
        
        block_t* block=(block_t*)get_page_footer(prev_page);
        ASSERT(isvalid_block_addr(block));ASSERT(is_null_block(block));ASSERT(isvalid_block_size(sizeof(pf_t)+sizeof(ph_t)));
        ASSERT(isvalid_page_footer((pf_t*)block));
        ASSERT(((void*)block)+(sizeof(pf_t)+sizeof(ph_t))==(void*)next_page_first_block);
        
        if(malloc_last_page==page)
            malloc_last_page=prev_page;
        
        write_page_header(npage_header,npage_size,npage_header->prev_page);
        write_page_footer(npage_footer,npage_size,npage_footer->next_page);
        page=prev_page;
        
        write_block(block,sizeof(pf_t)+sizeof(ph_t),false,get_page_footer_palloc((pf_t*)block),false);
        ASSERT(isvalid_block(block));
        ASSERT(((void*)block)+get_block_size(block)==(void*)next_page_first_block);
        
        set_page_footer_palloc(npage_footer,npf_palloc);
        set_block_fb(next_page_first_block,false);
        
        
        ASSERT(validate_page(page));
        ASSERT(validate_pages());
        ASSERT(get_next_block(block)==next_page_first_block);
        ASSERT(efl_validate());
        ASSERT(!efl_check_block(block));
        
        block=coalesce_block(block);
        ASSERT(isvalid_block(block));
        ASSERT(!get_block_alloc(block));
        ASSERT(get_block_size(block)>=(sizeof(pf_t)+sizeof(ph_t)));
        ASSERT(validate_page(page));
        ASSERT(validate_pages());
        ASSERT(efl_validate());
        ASSERT(!efl_check_block(block));
        
        efl_add_block(block);
        ASSERT(isvalid_block(block));
        ASSERT(!get_block_alloc(block));
        ASSERT(get_block_size(block)>=(sizeof(pf_t)+sizeof(ph_t)));
        ASSERT(validate_page(page));
        ASSERT(validate_pages());
        ASSERT(efl_validate());
        ASSERT(efl_check_block(block));
    }
    
    
    ASSERT(validate_page(page));
    ASSERT(validate_pages());
    return page;
}


//Block Opps
size_t get_block_size(const block_t* const block)
{
    ASSERT(isvalid_block(block));
    ASSERT(isvalid_block_size(BGetSize(block->header)));
    return BGetSize(block->header);
}

bool get_block_alloc(const block_t* const block)
{
    ASSERT(isvalid_block(block));
    return GetAlloc(block->header);
}

bool get_block_palloc(const block_t* const block)
{
    ASSERT(isvalid_block(block));
    return GetPAlloc(block->header);
}

bool get_block_fb(const block_t* const block)
{
    ASSERT(isvalid_block(block));
    return GetFB(block->header);
}

int set_block_alloc(block_t* const block,const bool alloc)
{
    ASSERT(isvalid_block(block));
    
    block->header= alloc ? SetAlloc(block->header) : SetFree(block->header);
    
    if(!alloc)
    {
        ASSERT(isvalid_block_size(BGetSize(block->header)));
        void* block_footer_addr=((void*)block)+(BGetSize(block->header)-BLOCK_FOOTER_SIZE);ASSERT(isvalid_block_addr(block_footer_addr+BLOCK_FOOTER_SIZE));
        *(size_t*)block_footer_addr=block->header;
    }
    
    ASSERT(isvalid_block(block));
    ASSERT(get_block_alloc(block)==alloc);
    return 0;
}

int set_block_palloc(block_t* const block,const bool palloc)
{
    ASSERT(isvalid_block(block));
    
    block->header= palloc ? SetPAlloc(block->header) : SetPFree(block->header);
    
    if(!GetAlloc(block->header))
    {
        ASSERT(isvalid_block_size(BGetSize(block->header)));
        void* block_footer_addr=((void*)block)+(BGetSize(block->header)-BLOCK_FOOTER_SIZE);ASSERT(isvalid_block_addr(block_footer_addr+BLOCK_FOOTER_SIZE));
        *(size_t*)block_footer_addr=block->header;
    }
    
    ASSERT(isvalid_block(block));
    ASSERT(get_block_palloc(block)==palloc);
    return 0;
}

int set_block_fb(block_t* const block,const bool fb)
{
    ASSERT(isvalid_block(block));
    
    block->header= fb ? SetFB(block->header) : UnSetFB(block->header);
    
    if(!GetAlloc(block->header))
    {
        ASSERT(isvalid_block_size(BGetSize(block->header)));
        void* block_footer_addr=((void*)block)+(BGetSize(block->header)-BLOCK_FOOTER_SIZE);ASSERT(isvalid_block_addr(block_footer_addr+BLOCK_FOOTER_SIZE));
        *(size_t*)block_footer_addr=block->header;
    }
    
    ASSERT(isvalid_block(block));
    ASSERT(get_block_fb(block)==fb);
    return 0;
}

block_t* get_next_block(const block_t* const block)
{
    ASSERT(isvalid_block(block));
    
    block_t* next_block=(block_t*)(((void*)block)+get_block_size(block));
    
    ASSERT(isvalid_null_block(next_block) || isvalid_block(next_block));
    return next_block;
}
block_t* get_prev_block(const block_t* const block)
{
    ASSERT(isvalid_block(block));
    
    if(get_block_palloc(block) || get_block_fb(block))
        return NULL;
    
    void* prev_block_footer_addr=((void*)block)-BLOCK_HEADER_SIZE;
    size_t prev_block_size=BGetSize(*(size_t*)prev_block_footer_addr);
    block_t* prev_block=((void*)block)-prev_block_size;
    
    ASSERT(isvalid_block(prev_block));
    ASSERT(!get_block_alloc(prev_block));
    return prev_block;
}
    
block_t* get_block_from_payload(void* const payload)
{
    block_t* block=(block_t*)(payload-BLOCK_HEADER_SIZE);
    ASSERT(isvalid_block(block));
    return block;
}

bool is_null_block(const block_t* const block)
{
    ASSERT(isvalid_null_block(block) || isvalid_block(block));
    return GetLB(block->header);
}

int write_block(block_t* const block,const size_t block_size,const bool alloc,const bool palloc,const bool fb)
{
    ASSERT(isvalid_block_addr(block));
    ASSERT(isvalid_block_size(block_size));
    ASSERT(!alloc);
    ASSERT(!palloc || !fb);
    ASSERT(!fb || !palloc);
    
    size_t block_header=calc_block_header(block_size,alloc,palloc,fb);
    void* block_header_addr=(void*)block;
    void* block_footer_addr=((void*)block)+(block_size-BLOCK_FOOTER_SIZE);
    
    *(size_t*)block_header_addr=block_header;
    *(size_t*)block_footer_addr=block_header;
    
    ASSERT(isvalid_block(block));
    ASSERT(BGetSize(block->header)==block_size);
    ASSERT(GetAlloc(block->header)==alloc);
    ASSERT(GetPAlloc(block->header)==palloc);
    ASSERT(GetFB(block->header)==fb);
    return 0;
}


//Page Opps
size_t get_page_size(void* page)
{
    ASSERT(isvalid_page(page));
    
    ph_t* page_header=(ph_t*)page;
    ASSERT(isvalid_page_header(page_header));
    
    return PGetSize(page_header->header);
}

void* get_prev_page(void* page)
{
    ASSERT(isvalid_page(page));
    
    ph_t* page_header=(ph_t*)page;
    ASSERT(isvalid_page_header(page_header));
    
    ASSERT(page_header->prev_page==NULL_PAGE || isvalid_page(page_header->prev_page));
    return page_header->prev_page;
}

void* get_next_page(void* page)
{
    ASSERT(isvalid_page(page));
    
    pf_t* page_footer=(pf_t*)(page+(get_page_size(page)-sizeof(pf_t)));
    ASSERT(isvalid_page_footer(page_footer));
    
    ASSERT(page_footer->next_page==NULL_PAGE || isvalid_page(page_footer->next_page));
    return page_footer->next_page;
}

int set_prev_page(void* const page,void* const prev_page)
{
    ASSERT(isvalid_page(page));
    ASSERT(prev_page==NULL_PAGE || isvalid_page(prev_page));
    
    ph_t* page_header=(ph_t*)page;
    ASSERT(isvalid_page_header(page_header));
    
    page_header->prev_page=prev_page;
    
    ASSERT(isvalid_page_header(page_header));
    ASSERT(isvalid_page(page));
    return 0;
}

int set_next_page(void* const page,void* const next_page)
{
    ASSERT(isvalid_page(page));
    ASSERT(next_page==NULL_PAGE || isvalid_page(next_page));
    
    pf_t* page_footer=(pf_t*)(page+(get_page_size(page)-sizeof(pf_t)));
    page_footer->next_page=next_page;
    
    ASSERT(isvalid_page_footer(page_footer));
    ASSERT(isvalid_page(page));
    return 0;
}

bool get_page_footer_palloc(const pf_t* const page_footer)
{
    ASSERT(isvalid_page_footer(page_footer));
    return GetPAlloc(page_footer->footer);
}

int set_page_footer_palloc(pf_t* const page_footer,const bool palloc)
{
    ASSERT(isvalid_page_footer(page_footer));
    page_footer->footer= palloc ? SetPAlloc(page_footer->footer) : SetPFree(page_footer->footer);
    ASSERT(isvalid_page_footer(page_footer));
    return 0;
}

int write_page(void* const page,const size_t page_size,void* const prev_page,void* const next_page)
{
    ASSERT(isvalid_page_addr(page));
    ASSERT(isvalid_page_size(page_size));
    ASSERT(prev_page==NULL_PAGE || isvalid_page_addr(prev_page));
    ASSERT(next_page==NULL_PAGE || isvalid_page_addr(next_page));
    
    ph_t* page_header=(ph_t*)page;
    pf_t* page_footer=(pf_t*)(page+(page_size-sizeof(pf_t)));
    
    write_page_header(page_header,page_size,prev_page);
    write_page_footer(page_footer,page_size,next_page);
    
    block_t* block=page+sizeof(ph_t);
    size_t block_size=page_size-PAGE_META_DATA_SIZE;
    
    ASSERT(isvalid_block_addr(block));
    ASSERT(isvalid_block_size(block_size));
    
    write_block(block,block_size,false,false,true);
    efl_add_block(block);
    ASSERT(isvalid_page(page));
    return 0;
}
int write_page_header(ph_t* const page_header,const size_t page_size,void* const prev_page)
{
    ASSERT(isvalid_page_addr((void*)page_header));
    ASSERT(isvalid_page_size(page_size));
    ASSERT(prev_page==NULL_PAGE || isvalid_page_addr(prev_page));
    
    page_header->header=page_size;
    page_header->prev_page=prev_page;
    
    ASSERT(isvalid_page_header(page_header));
    return 0;
}
int write_page_footer(pf_t* const page_footer,const size_t page_size,void* const next_page)
{
    ASSERT(isvalid_page_addr(((void*)page_footer)+sizeof(pf_t)));
    ASSERT(isvalid_page_size(page_size));
    ASSERT(next_page==NULL_PAGE || isvalid_page_addr(next_page));
    
    page_footer->footer=SetLB(page_size);
    page_footer->next_page=next_page;
    
    ASSERT(isvalid_page_footer(page_footer));
    return 0;
}

block_t* get_page_first_block(void* const page)
{
    ASSERT(isvalid_page(page));
    ASSERT(validate_page(page));
    
    ASSERT(isvalid_block((block_t*)(page+sizeof(ph_t))));
    return (block_t*)(page+sizeof(ph_t));
}


ph_t* get_page_header(void* const page)
{
    ASSERT(isvalid_page(page));
    ASSERT(isvalid_page_header((ph_t*)page));
    return (ph_t*)page;
}
pf_t* get_page_footer(void* const page)
{
    ASSERT(isvalid_page(page));
    
    ph_t* page_header=(ph_t*)page;
    ASSERT(isvalid_page_header(page_header));
    
    pf_t* page_footer=(pf_t*)(page+(PGetSize(page_header->header)-sizeof(pf_t)));
    ASSERT(isvalid_page_footer(page_footer));
    return page_footer;
}

//Bloc Calc
size_t calc_block_header(const size_t block_size,const bool alloc,const bool palloc,const bool fb)
{
    ASSERT(isvalid_block_size(block_size));
    
    size_t header=block_size;
    header= alloc ? SetAlloc(header) : header;
    header= palloc ? SetPAlloc(header) : header;
    header= fb ? SetFB(header) : header;
    
    ASSERT(BGetSize(header)==block_size);
    ASSERT(GetAlloc(header)==alloc);
    ASSERT(GetPAlloc(header)==palloc);
    ASSERT(GetFB(header)==fb);
    ASSERT(!GetLB(header));
    return header;
}

//Block Round
size_t block_round(const size_t block_size)
{
    return block_size<=MIN_BLOCK_SIZE ? MIN_BLOCK_SIZE : ( (block_size&(0xfLU)) ? ((block_size&(~(0xfLU)))+0x10) : block_size );
}

size_t calc_block_size(const size_t payload_size)
{
    ASSERT(isvalid_block_size(block_round(payload_size+BLOCK_HEADER_SIZE)));
    ASSERT(block_round(payload_size+BLOCK_HEADER_SIZE)>=(payload_size+BLOCK_HEADER_SIZE));
    return block_round(payload_size+BLOCK_HEADER_SIZE);
}


//Page Round
size_t page_round(const size_t page_size)
{
    ASSERT(isvalid_page_size(page_size<=MIN_PAGE_SIZE ? MIN_PAGE_SIZE : ( (page_size&PAGE_OFFSET_MASK) ? ((page_size&PAGE_SIZE_MASK)+MIN_PAGE_SIZE) : page_size )));
    return page_size<=MIN_PAGE_SIZE ? MIN_PAGE_SIZE : ( (page_size&PAGE_OFFSET_MASK) ? ((page_size&PAGE_SIZE_MASK)+MIN_PAGE_SIZE) : page_size );
}

size_t calc_page_size(const size_t block_size)
{
    ASSERT(isvalid_page_size(page_round(block_size+PAGE_META_DATA_SIZE)));
    ASSERT(page_round(block_size+PAGE_META_DATA_SIZE)>=(block_size+PAGE_META_DATA_SIZE));
    return page_round(block_size+PAGE_META_DATA_SIZE);
}

//Debug
bool validate_page(void* const page)
{
    if(!isvalid_page(page))
        return false;
    
    block_t* block=NULL;
    block_t* prev_block=NULL;
    size_t block_size_accumulator=0;
    
    for(block=(block_t*)(page+sizeof(ph_t));!GetLB(block->header);block=get_next_block(block))
    {
        if(!isvalid_block(block))
            return false;
        
        if(get_block_fb(block) && get_block_palloc(block))
            return false;
        
        if(prev_block)
        {
            if(get_block_fb(block))
                return false;
            if(get_block_alloc(prev_block)!=get_block_palloc(block))
                return false;
        }
        else
        {
            if(!get_block_fb(block))
                return false;
        }
        
        block_t* next_block=get_next_block(block);
        ASSERT(is_null_block(next_block) || isvalid_block(next_block));
        
        block_size_accumulator+=get_block_size(block);
        prev_block=block;
    }
    
    if(!is_null_block(block) || !isvalid_page_footer((pf_t*)block))
        return false;
    if(get_page_footer_palloc((pf_t*)block)!=get_block_alloc(prev_block))
        return false;
    
    if(get_page_size(page)!=(block_size_accumulator+(sizeof(ph_t)+sizeof(pf_t))))
    {
        return false;
    }
    
    return true;
}


bool validate_pages(void)
{
    if( (malloc_first_page!=NULL_PAGE && malloc_last_page==NULL_PAGE) || (malloc_last_page!=NULL_PAGE && malloc_first_page==NULL_PAGE))
    {
        return false;
    }
    
    size_t page_size_accumulator=0;
    
    for(void* page=malloc_first_page;page!=NULL_PAGE;page=get_next_page(page))
    {
        if(!validate_page(page))
        {
            return false;
        }
        
        if(get_prev_page(page)!=NULL_PAGE)
        {
            if(get_next_page(get_prev_page(page))!=page)
            {
                return false;
            }
        }
        else
        {
            if(page!=malloc_first_page)
            {
                return false;
            }
        }
        
        if(get_next_page(page)!=NULL_PAGE)
        {
            if(get_prev_page(get_next_page(page))!=page)
            {
                return false;
            }
        }
        else
        {
            if(page!=malloc_last_page)
            {
                return false;
            }
        }
        
        page_size_accumulator+=get_page_size(page);
    }
    
#ifdef DEBUG
    if(page_size_accumulator!=tmap_size)
        return false;
#endif
    return true;
}

void print_page(void* const page)
{
    ASSERT(isvalid_page(page));
    ASSERT(validate_page(page));
    
    ph_t* page_header=get_page_header(page);
    pf_t* page_footer=get_page_footer(page);
    
    ASSERT(isvalid_page_header(page_header));
    ASSERT(isvalid_page_footer(page_footer));
    
    printf("Page: %p\nHeader: %p\nSize: %lu\nPAlloc: %s\nLB: %s\nPrev Page: %p\n\n",
           page,
           page_header,
           PGetSize(page_header->header),
           GetPAlloc(page_header->header) ? "Alloc" : "Free",
           GetLB(page_header->header) ? "T" : "F",
           page_header->prev_page);
    
    
    const block_t* block=NULL;
    unsigned long num_block=0;
    
    for(block=get_page_first_block(page);!is_null_block(block);block=get_next_block(block))
    {
        ASSERT(isvalid_block(block));
        
        printf("block: %lu\nSize: %lu\nAlloc: %s\nPAlloc: %s\nFB: %s\nLB: %s\n\n",
               ++num_block,
               BGetSize(block->header),
               GetAlloc(block->header) ? "Alloc" : "Free",
               GetPAlloc(block->header) ? "Alloc" : "Free",
               GetFB(block->header) ? "T" : "F",
               GetLB(block->header) ? "T" : "F");
        
    }
    
    
    
    printf("Footer: %p\nSize: %lu\nPAlloc: %s\nLB: %s\nNext Page: %p\n\n\n\n",
           page_footer,
           PGetSize(page_footer->footer),
           GetPAlloc(page_footer->footer) ? "Alloc" : "Free",
           GetLB(page_footer->footer) ? "T" : "F",
           page_footer->next_page);
    
    ASSERT(isvalid_page(page));
    ASSERT(validate_page(page));
}

void print_pages(void)
{
    ASSERT(validate_pages());
    
    for(void* page=malloc_first_page;page!=NULL_PAGE;page=get_next_page(page))
    {
        ASSERT(isvalid_page(page));
        ASSERT(validate_page(page));
        ASSERT( get_next_page(page)==NULL_PAGE || get_prev_page(get_next_page(page))==page );
        ASSERT( get_prev_page(page)==NULL_PAGE || get_next_page(get_prev_page(page))==page );
        ASSERT( malloc_first_page!=page || get_prev_page(page)==NULL_PAGE );
        ASSERT( malloc_last_page!=page || get_next_page(page)==NULL_PAGE );
        print_page(page);
    }
    
    ASSERT(validate_pages());
}
