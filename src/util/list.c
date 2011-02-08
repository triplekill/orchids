/**
 ** @file list.c
 ** list util function.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup util
 **
 ** @date  Started on: Mon Jan 20 11:03:49 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>

#include "safelib.h"
#include "list.h"

void
list_add(list_t **l, void *value)
{
  list_t *t;

  t = Xmalloc(sizeof (list_t));
  t->value = value;
  t->next = *l;
  *l = t;
}

int
list_length(list_t *l)
{
  int i;
  
  i = 0;
  for (;l;l = l->next)
    i++;

  return (i);
}

void
list_reverse(list_t **l)
{
  list_t *tail;
  list_t *elmt;
  list_t *next;
  
  elmt = *l;
  tail = 0;
  while (elmt)
    {
      next = elmt->next;
      elmt->next = tail;
      tail = elmt;
      elmt = next;
    }
  *l = tail;
}

void *
list_at(list_t *l, int idx)
{
  int i;

  for (i = 0; l; l = l->next)
    {
      if (i == idx)
        return (l->value);
      i++;
    }

  return (NULL);
}

/*
** 
*/

/* XXX -- Warning, DO NOT use on huge lists (> 10k elements) */
void
list_sort(list_t **pc, int (*compar)(void *x, void *y))
{
  list_t *head;
  list_t *maxi;
  list_t **pnxt;
  list_t *cell;
  list_t *pred;

  head = *pc;
  for (*pc = 0; 0 != head; *pc = maxi)
    {
      pnxt = &head;
      maxi = pred = head;
      cell = head->next;
      while (cell)
        {
          if (compar(cell->value, maxi->value) > 0)
            {
              maxi = cell;
              pnxt = &(pred->next);
            }
          pred = cell;
          cell = cell->next;
        }
      *pnxt = maxi->next;
      maxi->next = *pc;
    }
}

void
list_free(list_t **l, void (*elmt_free)(void *e))
{
  list_t *t;

  t = *l;
  while (t)
    {
      list_t *t2;
      
      t2 = t->next;
      if (elmt_free)
        elmt_free(t->value);
      Xfree(t);
      t = t2;
    }
  *l = NULL;
}

#if 0
void
list_del_elmt(list_t **list, list_t *elmt)
{
  list_t *l;
  list_t *prev;

  prev = NULL;
  for (l = *list; l; l = l->next)
    {
      if (!compar(l->value, ref))
        {
          if (prev)
            prev->next = l->next;
          else
            *list = l->next;
          Xfree(l);
          return ;
        }
      prev = l;
    }
}
#endif

void *
list_find_elmt(list_t *l, int (*compar)(void *x, void *y), void *ref)
{
  for (; l; l = l->next)
    if (!compar(l->value, ref))
      return (l->value);

  return (NULL);
}

#if 0
int
test_compar(void *x, void *y)
{
  return ((int)x - (int)y);
}

void
test_list()
{
  list_t *l, *k;
  int i;

  l = NULL;
  for (i = 0; i < 100; i++)
    list_add(&l, (void *)rand());

  fprintf(stderr, "initial list\n");
  printf("---\n");
  for (k = l; k; k = k->next)
    printf("val = %d\n", (int)k->value);

  list_reverse(&l);

  fprintf(stderr, "reversed  list\n");
  printf("---\n");
  for (k = l; k; k = k->next)
    printf("val = %d\n", (int)k->value);

  list_sort(&l, test_compar);

  fprintf(stderr, "sorted list\n");
  printf("---\n");
  for (k = l; k; k = k->next)
    printf("val = %d\n", (int)k->value);

  list_free(&l, NULL);

  exit(1);
}
#endif



/*
** Copyright (c) 2002-2005 by Julien OLIVAIN, Laboratoire Spécification
** et Vérification (LSV), CNRS UMR 8643 & ENS Cachan.
**
** Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
**
** This software is a computer program whose purpose is to detect intrusions
** in a computer network.
**
** This software is governed by the CeCILL license under French law and
** abiding by the rules of distribution of free software.  You can use,
** modify and/or redistribute the software under the terms of the CeCILL
** license as circulated by CEA, CNRS and INRIA at the following URL
** "http://www.cecill.info".
**
** As a counterpart to the access to the source code and rights to copy,
** modify and redistribute granted by the license, users are provided
** only with a limited warranty and the software's author, the holder of
** the economic rights, and the successive licensors have only limited
** liability.
**
** In this respect, the user's attention is drawn to the risks associated
** with loading, using, modifying and/or developing or reproducing the
** software by the user in light of its specific status of free software,
** that may mean that it is complicated to manipulate, and that also
** therefore means that it is reserved for developers and experienced
** professionals having in-depth computer knowledge. Users are therefore
** encouraged to load and test the software's suitability as regards
** their requirements in conditions enabling the security of their
** systems and/or data to be ensured and, more generally, to use and
** operate it in the same conditions as regards security.
**
** The fact that you are presently reading this means that you have had
** knowledge of the CeCILL license and that you accept its terms.
*/

/* End-of-file */
