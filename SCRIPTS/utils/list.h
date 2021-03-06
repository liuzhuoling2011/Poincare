/* Copyright (C) 2002-2014 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@redhat.com>, 2002.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

#ifndef _LIST_H
# define _LIST_H

#define atomic_write_barrier() __asm ("" ::: "memory")

/* The definitions of this file are adopted from those which can be
   found in the Linux kernel headers to enable people familiar with
   the latter find their way in these sources as well.  */


/* Basic type for the double-link list.  */
typedef struct list_head
{
  struct list_head *next;
  struct list_head *prev;
} list_t;


/* Define a variable with the head and tail of the list.  */
# define LIST_HEAD(name) \
  list_t name = { &(name), &(name) }

/* Initialize a new list head.  */
# define INIT_LIST_HEAD(ptr) \
  (ptr)->next = (ptr)->prev = (ptr)


/* Add new element at the head of the list.  */
static void
list_add_after (list_t *newp, list_t *head)
{
  newp->next = head->next;
  newp->prev = head;
  head->next->prev = newp;
//  atomic_write_barrier ();
  head->next = newp;
}


static void
list_add_before (list_t *newp, list_t *head)
{
	newp->prev = head->prev;
	newp->next = head;

	head->prev->next = newp;
//	atomic_write_barrier ();
	head->prev = newp;
}

static int 
list_empty(const list_t *head)
{
	return head->next == head;
}

/* Remove element from list.  */
static void
list_del (list_t *elem)
{
  elem->next->prev = elem->prev;
  elem->prev->next = elem->next;
}


/* Join two lists.  */
static void
list_splice (list_t *add, list_t *head)
{
  /* Do nothing if the list which gets added is empty.  */
  if (add != add->next)
    {
      add->next->prev = head;
      add->prev->next = head->next;
      head->next->prev = add->prev;
      head->next = add->next;
    }
}


/* Get typed element from list at a given position.  */
# define list_entry(ptr, type, member) \
  ((type *) ((char *) (ptr) - (unsigned long long) (&((type *) 0)->member)))


/* Iterate forward over the elements of the list.  */
# define list_for_each(pos, head) \
  for (pos = (head)->next; pos != (head); pos = pos->next)


/* Iterate forward over the elements of the list.  */
# define list_for_each_prev(pos, head) \
  for (pos = (head)->prev; pos != (head); pos = pos->prev)

# define list_for_each(pos, head) \
  for (pos = (head)->next; pos != (head); pos = pos->next)

/* Iterate backwards over the elements list.  The list elements can be
   removed from the list while doing this.  */
# define list_for_each_prev_safe(pos, p, head) \
  for (pos = (head)->prev, p = pos->prev; \
       pos != (head); \
       pos = p, p = pos->prev)

# define list_for_each_safe(pos, p, head) \
  for (pos = (head)->next, p = pos->next; \
       pos != (head); \
       pos = p, p = pos->next)

#endif /* _LIST_H */
