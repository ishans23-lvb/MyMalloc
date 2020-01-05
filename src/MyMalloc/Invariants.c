//
//  Invariants.c
//  MyMalloc
//
//  Created by Ishan Shah on 12/14/19.
//  Copyright © 2019 Ishan Shah. All rights reserved.
//

#include "Invariants.h"

//Block
bool isvalid_null_block(const block_t* const block)
{
    
    return isvalid_block_addr(block) &&
           isvalid_page_footer((pf_t*)block) &&
           GetLB(block->header);
}
bool isvalid_block(const block_t* const block)
{
    if(!isvalid_block_addr(block) || !isvalid_block_size(BGetSize(block->header)))
        return false;
    
    if(GetFB(block->header) && GetPAlloc(block->header))
        return false;
    
    if(GetLB(block->header))
        return false;
    
    if(!GetAlloc(block->header))
    {
        void* block_footer_addr=((void*)block)+(BGetSize(block->header)-BLOCK_FOOTER_SIZE);
        if(block->header!=*(size_t*)block_footer_addr)
            return false;
    }
    
    return true;
}

//Page
bool isvalid_page(void* page)
{
    ph_t* page_header=(ph_t*)page;
    
    if(!isvalid_page_header(page_header))
        return false;
    
    pf_t* page_footer=page+(PGetSize(page_header->header)-sizeof(pf_t));
    
    if(!isvalid_page_footer(page_footer))
        return false;
    
    if(PGetSize(page_header->header)!=PGetSize(page_footer->footer))
        return false;
    
    return true;
}

bool isvalid_page_header(const ph_t* const page_header)
{
    return isvalid_page_addr((void*)page_header) &&
           isvalid_page_size(PGetSize(page_header->header)) &&
           !GetAlloc(page_header->header) &&
           !GetPAlloc(page_header->header) &&
           !GetLB(page_header->header) &&
           ((page_header->header)&PAGE_OFFSET_MASK)==0 &&
           (page_header->prev_page==NULL_PAGE || isvalid_page_addr(page_header->prev_page));
}

bool isvalid_page_footer(const pf_t* const page_footer)
{
    return isvalid_page_addr(((void*)page_footer)+sizeof(pf_t)) &&
           isvalid_page_size(PGetSize(page_footer->footer)) &&
           !GetAlloc(page_footer->footer) &&
           GetLB(page_footer->footer) &&
           UnSetLB(SetPFree(((page_footer->footer)&PAGE_OFFSET_MASK)))==0 &&
           (page_footer->next_page==NULL_PAGE || isvalid_page_addr(page_footer->next_page));
}


//Basic Block
bool isvalid_block_size(const size_t block_size)
{
    return !(block_size&0xf) && block_size>=MIN_BLOCK_SIZE;
}

bool isvalid_block_addr(const block_t* const block)
{
    return !(((size_t)block)&0xf);
}

//Basic Page
bool isvalid_page_size(const size_t page_size)
{
    return ( (!(page_size&PAGE_OFFSET_MASK)) && page_size>=MIN_PAGE_SIZE );
}

bool isvalid_page_addr(void* page)
{
    return !(((size_t)page)&PAGE_OFFSET_MASK);
}
