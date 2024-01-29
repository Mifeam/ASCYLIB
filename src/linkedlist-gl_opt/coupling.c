/*   
 *   File: coupling.c
 *   Author: Vincent Gramoli <vincent.gramoli@sydney.edu.au>, 
 *  	     Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>
 *   Description: hand-over-hand locking list
 *   (Details: Maurice Herlihy and Nir Shavit. The Art of 
 *    Multiprocessor Programming, Revised First Edition. 2012.)
 *   coupling.c is part of ASCYLIB
 *
 * Copyright (c) 2014 Vasileios Trigonakis <vasileios.trigonakis@epfl.ch>,
 * 	     	      Tudor David <tudor.david@epfl.ch>
 *	      	      Distributed Programming Lab (LPD), EPFL
 *
 * ASCYLIB is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 2
 * of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "coupling.h"

RETRY_STATS_VARS;
LOCK_LOCAL_DATA;

/* 
 * Similar algorithm for the delete, find, and insert:
 * Lock the first two elements (locking each before getting the copy of the element)
 * then unlock previous, keep ownership of the current, and lock next in a loop.
 */
sval_t
lockc_delete(intset_l_t *set, skey_t key)
{
  PARSE_TRY();
  UPDATE_TRY();

  node_l_t *curr, *next;
  sval_t res = 0;
	
  LOCK_A(set->lock);		/* when GL_[UN]LOCK is defined the [UN]LOCK is not ;-) */
  curr = set->head;
  next = curr->next;
	
  while (next->key < key) 
    {
      curr = next;
      next = next->next;
    }

  if (key == next->key)
    {
      res = next->val;
      curr->next = next->next;
#if GC == 1
      ssmem_free(alloc, (void*) next);
#endif
    } 

  UNLOCK_A(set->lock);

  return res;
}

sval_t
lockc_find(intset_l_t *set, skey_t key) 
{
  PARSE_TRY();

  node_l_t *curr, *next; 
  sval_t res = 0;
	
  curr = set->head;
  next = curr->next;
	
  while (next->key < key) 
    {
      curr = next;
      next = curr->next;
    }	
  if (key == next->key)
    {
      res = next->val;
    }

  return res;
}

int
lockc_insert(intset_l_t *set, skey_t key, sval_t val) 
{
  PARSE_TRY();
  UPDATE_TRY();

  node_l_t *curr, *next, *newnode;
  int found;
	
  LOCK_A(set->lock);
  curr = set->head;
  next = curr->next;
	
  while (next->key < key) 
    {
      curr = next;
      next = curr->next;
    }
  found = (key == next->key);
  if (!found) 
    {
      newnode = new_node_l(key, val, next, 1);
#ifdef __tile__
  MEM_BARRIER;
#endif
      curr->next = newnode;
    }
  UNLOCK_A(set->lock);
  return !found;
}
