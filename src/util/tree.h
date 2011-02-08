/**
 ** @file tree.h
 ** Generic tree lib header.
 **
 ** @author Julien OLIVAIN <julien.olivain@lsv.ens-cachan.fr>
 **
 ** @version 0.1
 ** @ingroup util
 **
 ** @date  Started on: Mon Mar  3 18:03:26 2003
 **/

/*
 * See end of file for LICENSE and COPYRIGHT informations.
 */

#ifndef TREE_H
#define TREE_H

typedef int (*treewalk_t)(void *data, void *param);

typedef struct tree_s tree_t;
struct tree_s
{
  void *data;
  int type;
  tree_t *parent;
  size_t childs;
  size_t child_sz;
  size_t grow;
  tree_t **child;
};

tree_t *new_tree_node(size_t childs, size_t grow);
void free_tree_node(tree_t *node);
void tree_recursive_deep_pre_traverse(tree_t *root, treewalk_t fnct,
                                      void *param);
void tree_recursive_deep_post_traverse(tree_t *root, treewalk_t fnct,
                                       void *param);
void tree_addchild(tree_t *parent, tree_t *child);


#endif /* TREE_H */



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
