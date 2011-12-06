/**
 ** @file tree.c
 ** A mini tree library.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup util
 **
 ** @date  Started on: Mon Mar  3 18:06:33 2003
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
#include "tree.h"

tree_t *
new_tree_node(size_t childs, size_t grow)
{
  tree_t *tree;

  tree = Xzmalloc(sizeof (tree_t));
  tree->child_sz = childs;
  tree->grow = grow;

  if (childs)
    tree->child = Xmalloc(childs * sizeof (tree_t *));

  return (tree);
}

void
free_tree_node(tree_t *node)
{
  if (node->child_sz)
    Xfree(node->child);
  Xfree(node);
}

void
tree_recursive_deep_pre_traverse(tree_t *root, treewalk_t fnct, void *param)
{
  int i;

  if (fnct)
    fnct(root->data, param);

  for (i = 0; i < root->childs; ++i)
    tree_recursive_deep_pre_traverse(root->child[i], fnct, param);
}

void tree_walk_test(tree_t *root);

void
tree_walk_test(tree_t *root)
{
  int i;

  for (i = 0; i < root->childs; ++i)
    tree_walk_test(root->child[i]);

  printf("node type %i\n", root->type);
}

void
tree_recursive_deep_post_traverse(tree_t *root, int (*fnct)(void *, void *), void *param)
{
  int i;

  for (i = 0; i < root->childs; ++i)
    tree_recursive_deep_post_traverse(root->child[i], fnct, param);

  if (fnct)
    fnct(root->data, param);
}

void
tree_addchild(tree_t *parent, tree_t *child)
{
  /* XXX -- param check ?? */
  if (child == NULL)
    return ;

  if (parent->childs == parent->child_sz)
    {
      if (parent->grow == 0)
        return ;

      parent->child_sz += parent->grow;
      parent->child = Xrealloc(parent->child, parent->child_sz * sizeof (tree_t *));
    }

  parent->child[ parent->childs++ ] = child;
  child->parent = parent;
}


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
