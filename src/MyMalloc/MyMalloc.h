//
//  MyMalloc.h
//  MyMalloc
//
//  Created by Ishan Shah on 12/14/19.
//  Copyright © 2019 Ishan Shah. All rights reserved.
//

#ifndef MyMalloc_h
#define MyMalloc_h

#include <sys/mman.h>
#include <stdbool.h>
#include <stddef.h>
#include <assert.h>


#ifndef DEBUG
#error
#endif

#define BLOCK_HEADER_SIZE sizeof(size_t)
#define BLOCK_FOOTER_SIZE sizeof(size_t)
#define BLOCK_META_DATA_SIZE (BLOCK_HEADER_SIZE + BLOCK_FOOTER_SIZE)
#define MIN_BLOCK_SIZE (32LU)
#define MIN_PAGE_SIZE (4096LU)
#define PAGE_OFFSET_MASK (MIN_PAGE_SIZE-1)
#define PAGE_SIZE_MASK (~PAGE_OFFSET_MASK)
#define PAGE_META_DATA_SIZE (sizeof(ph_t)+sizeof(pf_t))
#define NULL_PAGE ((void*)(-1))

#define SetAlloc(x) ((x)|0x1LU)
#define SetFree(x) ((x)&(~(0x1LU)))
#define GetAlloc(x) ((bool)((x)&0x1LU))

#define SetPAlloc(x) ((x)|0x2LU)
#define SetPFree(x) ((x)&(~(0x2LU)))
#define GetPAlloc(x) ((bool)((x)&0x2LU))

#define SetFB(x) ((x)|(0x4LU))
#define UnSetFB(x) ((x)&(~(0x4LU)))
#define GetFB(x) ((bool)((x)&(0x4LU)))

#define SetLB(x) ((x)|(0x8LU))
#define UnSetLB(x) ((x)&(~(0x8LU)))
#define GetLB(x) ((bool)((x)&(0x8LU)))

#define BGetSize(x) ((x)&(~(0xfLU)))
#define PGetSize(x) ((x)&PAGE_SIZE_MASK)


#ifdef DEBUG
#define ASSERT(x) assert(x)
#else
#define ASSERT(x)
#endif

#ifdef __cplusplus
extern "C"{
#endif


typedef struct block block_t;

typedef struct page_header
{
    size_t header;
    void* prev_page;
}ph_t;

typedef struct page_footer
{
    size_t footer;
    void* next_page;
}pf_t;

struct block
{
    size_t header;
    char payload[0];
    block_t* prev_block;
    block_t* next_block;
};


//Malloc
void* my_malloc(const size_t size);
void my_free(void* const ptr);
void malloc_terminate(void);

//Find Fit
block_t* find_fit(const size_t req_block_size);
block_t* first_fit_ifl(const size_t req_block_size);

//Block Manger
int alloc_block(block_t* block,const size_t req_block_size);
int split_block(block_t* block,const size_t req_block_size);
block_t* free_block(block_t* block);
block_t* coalesce_block(block_t* block);

//Page Manager
void* page(const size_t page_size);
void* coalesce_page(void* const page);
int unpage(void* const page);

//Block Opps
size_t get_block_size(const block_t* const block);
bool get_block_alloc(const block_t* const block);
bool get_block_palloc(const block_t* const block);
bool get_block_fb(const block_t* const block);
    
int set_block_alloc(block_t* const block,bool alloc);
int set_block_palloc(block_t* const block,bool palloc);
int set_block_fb(block_t* const block,bool fb);
    
block_t* get_next_block(const block_t* const block);
block_t* get_prev_block(const block_t* const block);
    
block_t* get_block_from_payload(void* const payload);

bool is_null_block(const block_t* const block);

int write_block(block_t* const block,const size_t block_size,const bool alloc,const bool palloc,const bool fb);

//Page Opps
size_t get_page_size(void* const page);

void* get_prev_page(void* const page);
void* get_next_page(void* const page);
    
int set_prev_page(void* const page,void* const prev_page);
int set_next_page(void* const page,void* const next_page);

bool get_page_footer_palloc(const pf_t* const page_footer);
int set_page_footer_palloc(pf_t* const page_footer,bool palloc);

ph_t* get_page_header(void* const page);
pf_t* get_page_footer(void* const page);

int write_page(void* const page,const size_t page_size,void* const prev_page,void* const next_page);
int write_page_header(ph_t* page_header,const size_t page_size,void* const prev_page);
int write_page_footer(pf_t* page_footer,const size_t page_size,void* const next_page);

block_t* get_page_first_block(void* const page);

//Block Round
size_t calc_block_size(const size_t payload_size);
size_t block_round(const size_t block_size);

//Page Round
size_t calc_page_size(const size_t block_size);

//Block Calc
size_t calc_block_header(const size_t block_size,const bool alloc,const bool palloc,const bool fb);

//Debug
bool validate_page(void* const page);
bool validate_pages(void);
   
void print_pages(void);
    
void* get_malloc_first_page(void);

#ifdef DEBUG
extern size_t fl_tsize;
#endif

#ifdef __cplusplus
}
#endif

#endif /* MyMalloc_h */
