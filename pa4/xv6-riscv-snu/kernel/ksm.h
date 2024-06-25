//----------------------------------------------------------------
//
//  4190.307 Operating Systems (Spring 2024)
//
//  Project #4: KSM (Kernel Samepage Merging)
//
//  May 7, 2024
//
//  Dept. of Computer Science and Engineering
//  Seoul National University
//
//----------------------------------------------------------------


#include "types.h"

extern int freemem;

struct metadata_node {
  uint64 pa;
  uint64 hash_data;
  pte_t *pte;
  struct metadata_node* next;
  int shared;
  pagetable_t *pagetable;
  int page_num;
};
