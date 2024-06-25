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
#include "riscv.h"
#include "ksm.h"
#include "param.h"
#include "defs.h"
#include "spinlock.h"
#include "proc.h"
#include "memlayout.h"

extern struct proc proc[NPROC];

extern uint64 zero_page;

#define PTE_COW (1L << 8)

uint64
sys_ksm(void)
{
  struct metadata_node* metadata = 0;
  struct metadata_node* node = metadata;
  struct metadata_node* next;
  metadata = 0;
  uint64 scanned, merged;
  argaddr(0, &scanned);
  argaddr(1, &merged);
  int scanned_val = 0;
  int merged_val = 0;
  struct proc *p;
  pagetable_t pagetable;
  uint64 hash_value;
  for (p = proc; p < &proc[NPROC]; p++) {
    if (p->state == UNUSED || p->pid == 1 || p->pid == 2 || p->pid == myproc()->pid) {
      continue;
    }
    pagetable = p->pagetable;
    uint64 pa;
    for(int k=0; k<p->sz/PGSIZE; k++){
      pa = PTE2PA(*walk(pagetable, k*PGSIZE, 0));
      if(pa == 0){
        continue;
      }
      scanned_val++;
      hash_value = xxh64((void*)pa, PGSIZE);
      if(hash_value == xxh64((void*)zero_page, PGSIZE)){
        if(zero_page == PTE2PA(*walk(pagetable, k*PGSIZE, 0))){
          continue;
        }
        pte_t *pte = walk(pagetable, k*PGSIZE, 0);
        int perm = PTE_FLAGS(*pte);
        if((perm & PTE_W)!=0){
          perm &= ~PTE_W;
          perm |= PTE_COW;
        }
        uvmunmap(pagetable, k*PGSIZE, 1, 1);
        mappages(pagetable, k*PGSIZE, PGSIZE, zero_page, perm);
        merged_val++;
      }
      else{
        struct metadata_node* new_node = (struct metadata_node*)kalloc();
        new_node->shared = 0;
        new_node->pa = pa;
        new_node->pte = walk(pagetable, k*PGSIZE, 0);
        new_node->hash_data = hash_value;
        new_node->next = metadata;
        new_node->pagetable = &p->pagetable;
        new_node->page_num = k;
        metadata = new_node;
      }
    }
  }
  node = metadata;
  struct metadata_node *node_iter;
  while(node != 0){
    node_iter = metadata;
    while(node_iter != 0){
      if(node != node_iter && node->hash_data == node_iter->hash_data){
        if(node->pa == node_iter->pa){
          if(node->shared != -1 && node_iter->shared != -1){
            node->shared = 1;
            node_iter->shared = 1;
          }
        }
        else{
          if(node->shared == 1){
            node_iter->shared = -1;
            node_iter->pa = node->pa;
          }
          else if(node_iter->shared == 1){
            node->shared = -1;
            node->pa = node_iter->pa;
          }
          else if(node->shared == -1){
            node_iter->shared = -1;
            node_iter->pa = node->pa;
          }
          else if(node_iter->shared == -1){
            node->shared = -1;
            node->pa = node_iter->pa;
          }
          else{
            node->shared = -2;
            node_iter->shared = -2;
          }
        }
      }
      node_iter = node_iter->next;
    }
    node = node->next;
  }
  node = metadata;
  while(node != 0){
    if(node->shared == -1){
      pte_t *pte;
      pte = walk(*node->pagetable, node->page_num*PGSIZE, 0);
      int perm = PTE_FLAGS(*pte);
      if((perm & PTE_W)!=0){
        perm &= ~PTE_W;
        perm |= PTE_COW;
      }
      uvmunmap(*node->pagetable, node->page_num*PGSIZE, 1, 1);
      mappages(*node->pagetable, node->page_num*PGSIZE, PGSIZE, node->pa, perm);
      merged_val++;
      node->shared = 1;
    }
    node = node->next;
  }
  node = metadata;
  while (node != 0)
  {
    if(node->shared == -2){
      node_iter = metadata;
      while(node_iter != 0){
        if(node != node_iter && node->hash_data == node_iter->hash_data){
          if(node_iter->shared == 1){
            pte_t *pte;
            pte = walk(*node->pagetable, node->page_num*PGSIZE, 0);
            int perm = PTE_FLAGS(*pte);
            if((perm & PTE_W)!=0){
              perm &= ~PTE_W;
              perm |= PTE_COW;
            }
            uvmunmap(*node->pagetable, node->page_num*PGSIZE, 1, 1);
            mappages(*node->pagetable, node->page_num*PGSIZE, PGSIZE, node_iter->pa, perm);
            merged_val++;
            node->shared = 1;
            node_iter->shared = 1;
            node->pa = node_iter->pa;
            break;
          }
        }
        node_iter = node_iter->next;
      }
      node_iter = metadata;
      while(node->shared != 1 && node_iter != 0){
        if(node != node_iter && node->hash_data == node_iter->hash_data){
          if(node_iter->shared == -2){
            pte_t *pte;
            pte = walk(*node_iter->pagetable, node_iter->page_num*PGSIZE, 0);
            int perm = PTE_FLAGS(*pte);
            if((perm & PTE_W)!=0){
              perm &= ~PTE_W;
              perm |= PTE_COW;
            }
            uvmunmap(*node_iter->pagetable, node_iter->page_num*PGSIZE, 1, 1);
            mappages(*node_iter->pagetable, node_iter->page_num*PGSIZE, PGSIZE, node->pa, perm);
            if((*(node->pte) & PTE_W) != 0){
              *node->pte &= ~PTE_W;
              *node->pte |= PTE_COW;
            }
            merged_val++;
            node->shared = 1;
            node_iter->shared = 1;
            node_iter->pa = node->pa;
            break;
          }
        }
        node_iter = node_iter->next;
      }
    }
    node = node->next;
  }
  node = metadata;
  while(node != 0){
    next = node->next;
    kfree((void*)node);
    node = next;
  }
  copyout(myproc()->pagetable, scanned, (char*)&scanned_val, sizeof(scanned_val));
  copyout(myproc()->pagetable, merged, (char*)&merged_val, sizeof(merged_val));
  return freemem;
}

