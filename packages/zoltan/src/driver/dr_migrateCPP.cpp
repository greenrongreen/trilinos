/*****************************************************************************
 * Zoltan Library for Parallel Applications                                  *
 * Copyright (c) 2000,2001,2002, Sandia National Laboratories.               *
 * For more info, see the README file in the top-level Zoltan directory.     *  
 *****************************************************************************/
/*****************************************************************************
 * CVS File Information :
 *    $RCSfile$
 *    $Author$
 *    $Date$
 *    $Revision$
 ****************************************************************************/

#include <mpi.h>
#include <iostream>

#include "dr_const.h"
#include "dr_err_const.h"
#include "dr_loadbal_const.h"
#include "dr_par_util_const.h"
#include "dr_util_const.h"
#include "dr_output_const.h"
#include "dr_elem_util_const.h"
#include "dr_maps_const.h"
#include "dr_dd.h"

#include "zoltanCPP.h"

using namespace Zoltan;
using namespace std;
using namespace MPI;

/*
 *  PROTOTYPES for load-balancer interface functions.
 */
ZOLTAN_PRE_MIGRATE_PP_FN migrate_pre_process;
ZOLTAN_POST_MIGRATE_PP_FN migrate_post_process;

/* Object-based migration callbacks; only one of these or the list-based
 * callbacks are actually needed. */
ZOLTAN_PACK_OBJ_FN migrate_pack_elem;
ZOLTAN_UNPACK_OBJ_FN migrate_unpack_elem;

/* List-based migration callbacks; only one of these or the object-based
 * callbacks are actually needed. */
ZOLTAN_PACK_OBJ_MULTI_FN migrate_pack_elem_multi;
ZOLTAN_UNPACK_OBJ_MULTI_FN migrate_unpack_elem_multi;

/*****************************************************************************/
/*
 *  Static global variables to help with migration.
 */
static int *New_Elem_Index = NULL;    /* Array containing globalIDs of 
                                         elements in the new decomposition,
                                         ordered in the same order as the
                                         elements array.
                                         Built in migrate_pre_process; used
                                         in migrate_pre_process to adjust
                                         element adjacencies; used in 
                                         migrate_unpack_elem to store 
                                         imported elements.                  */
static int New_Elem_Index_Size = 0;   /* Number of integers allocated in
                                         New_Elem_Index.                     */
static int Use_Edge_Wgts = 0;         /* Flag indicating whether elements
                                         store edge weights.                 */

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
int migrate_elements(
  int Proc,
  MESH_INFO_PTR mesh,
  Zoltan::Zoltan_Object &zz,
  int num_gid_entries, 
  int num_lid_entries,
  int num_imp,
  ZOLTAN_ID_PTR imp_gids,
  ZOLTAN_ID_PTR imp_lids,
  int *imp_procs,
  int *imp_to_part,
  int num_exp,
  ZOLTAN_ID_PTR exp_gids,
  ZOLTAN_ID_PTR exp_lids,
  int *exp_procs,
  int *exp_to_part)
{
/* Local declarations. */
char *yo = "migrate_elements";

/***************************** BEGIN EXECUTION ******************************/
  DEBUG_TRACE_START(Proc, yo);

  /*
   * register migration functions
   */
  if (!Test.Null_Lists) {
    /* If not passing NULL lists, let Help_Migrate call the
     * pre-processing and post-processing routines.
     */
    if (zz.Set_Pre_Migrate_PP_Fn(migrate_pre_process,
                      (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, "fatal:  error returned from zz.Set_Pre_Migrate_PP_Fn()\n");
      return 0;
    }

    if (zz.Set_Post_Migrate_PP_Fn(migrate_post_process,
                      (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, "fatal:  error returned from zz.Set_Post_Migrate_PP_Fn()\n");
      return 0;
    }
  }

  if (Test.Multi_Callbacks) {
    if (zz.Set_Obj_Size_Multi_Fn(migrate_elem_size_multi,
                      (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, "fatal:  error returned from zz.Set_Obj_Size_Multi_Fn()\n");
      return 0;
    }

    if (zz.Set_Pack_Obj_Multi_Fn(migrate_pack_elem_multi,
                      (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, "fatal:  error returned from zz.Set_Pack_Obj_Multi_Fn()\n");
      return 0;
    }
  
    if (zz.Set_Unpack_Obj_Multi_Fn(migrate_unpack_elem_multi,
                      (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, "fatal:  error returned from zz.Set_Unpack_Obj_Multi_Fn()\n");
      return 0;
    }
  }
  else {
    if (zz.Set_Obj_Size_Fn(migrate_elem_size,
                      (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, "fatal:  error returned from zz.Set_Obj_Size_Fn()\n");
      return 0;
    }

    if (zz.Set_Pack_Obj_Fn(migrate_pack_elem,
                      (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, "fatal:  error returned from zz.Set_Pack_Obj_Fn()\n");
      return 0;
    }

    if (zz.Set_Unpack_Obj_Fn(migrate_unpack_elem,
                      (void *) mesh) == ZOLTAN_FATAL) {
      Gen_Error(0, "fatal:  error returned from zz.Set_Unpack_Obj_Fn()\n");
      return 0;
    }
  }


  if (Test.Null_Lists == NONE) {
    if (zz.Migrate(num_imp, imp_gids, imp_lids, imp_procs, imp_to_part,
                   num_exp, exp_gids, exp_lids, exp_procs, exp_to_part)
                   == ZOLTAN_FATAL) {
      Gen_Error(0, "fatal:  error returned from zz.Migrate()\n");
      return 0;
    }
  }
  else {
    /* Call zz.Help_Migrate with empty import lists. */
    /* Have to "manually" call migrate_pre_process and migrate_post_process. */
    int ierr = 0;
    migrate_pre_process((void *) mesh, 1, 1,
                        num_imp, imp_gids, imp_lids, imp_procs, imp_to_part,
                        num_exp, exp_gids, exp_lids, exp_procs, exp_to_part,
                        &ierr);
    if (Test.Null_Lists == IMPORT_LISTS) {
      if (zz.Migrate(-1, NULL, NULL, NULL, NULL,
                     num_exp, exp_gids, exp_lids, exp_procs, exp_to_part)
                     == ZOLTAN_FATAL) {
        Gen_Error(0, "fatal:  error returned from zz.Migrate()\n");
        return 0;
      }
    }
    else {
      if (zz.Migrate(num_imp, imp_gids, imp_lids, imp_procs, imp_to_part,
                    -1, NULL, NULL, NULL, NULL)
                    == ZOLTAN_FATAL) {
        Gen_Error(0, "fatal:  error returned from zz.Migrate()\n");
        return 0;
      }
    }
    migrate_post_process((void *) mesh, 1, 1,  
                         num_imp, imp_gids, imp_lids, imp_procs, imp_to_part,
                         num_exp, exp_gids, exp_lids, exp_procs, exp_to_part,
                         &ierr);
  }

  DEBUG_TRACE_END(Proc, yo);
  return 1;

}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
void migrate_pre_process(void *data, int num_gid_entries, int num_lid_entries, 
                         int num_import, 
                         ZOLTAN_ID_PTR import_global_ids,
                         ZOLTAN_ID_PTR import_local_ids, int *import_procs,
                         int *import_to_part,
                         int num_export, ZOLTAN_ID_PTR export_global_ids,
                         ZOLTAN_ID_PTR export_local_ids, int *export_procs,
                         int *export_to_part,
                         int *ierr)
{
int lid = num_lid_entries-1;
int gid = num_gid_entries-1;
char msg[256];

  *ierr = ZOLTAN_OK;

  if (data == NULL) {
    *ierr = ZOLTAN_FATAL;
    return;
  }
  MESH_INFO_PTR mesh = (MESH_INFO_PTR) data;
  ELEM_INFO_PTR elements = mesh->elements;

  /*
   *  Set some flags. Assume if true for one element, true for all elements.
   *  Note that some procs may have no elements. 
   */

  int k = 0;

  if (elements[0].edge_wgt != NULL)
    k = 1;

  /* Make sure all procs have the same value */

  MPI_Allreduce(&k, &Use_Edge_Wgts, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);

  /*
   *  For all elements, update adjacent elements' processor information.
   *  That way, when perform migration, will be migrating updated adjacency
   *  information.  
   */
  
  int proc = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &proc);

  /*
   *  Build New_Elem_Index array and list of processor assignments.
   */

  New_Elem_Index_Size = mesh->num_elems + num_import - num_export;
  if (mesh->elem_array_len > New_Elem_Index_Size) 
    New_Elem_Index_Size = mesh->elem_array_len;

  New_Elem_Index = new int [New_Elem_Index_Size];

  int *proc_ids = NULL;
  char *change = NULL;

  if (mesh->num_elems > 0) {

    proc_ids = new int [mesh->num_elems];
    change   = new char [mesh->num_elems];

    if (New_Elem_Index == NULL || proc_ids == NULL || change == NULL) {
      Gen_Error(0, "fatal: insufficient memory");
      *ierr = ZOLTAN_MEMERR;
      if (proc_ids) delete [] proc_ids;
      if (change) delete [] change;
      if (New_Elem_Index)
        {
        delete [] New_Elem_Index;
        New_Elem_Index = NULL;
        }
      return;
    }

    for (int i = 0; i < mesh->num_elems; i++) {
      New_Elem_Index[i] = elements[i].globalID;
      proc_ids[i] = proc;
      change[i] = 0;
    }
  }

  for (int i = mesh->num_elems; i < New_Elem_Index_Size; i++) {
    New_Elem_Index[i] = -1;
  }

  for (int i = 0; i < num_export; i++) {

    int exp_elem = 0;

    if (num_lid_entries)
      exp_elem = export_local_ids[lid+i*num_lid_entries];
    else  /* testing num_lid_entries == 0 */
      search_by_global_id(mesh, export_global_ids[gid+i*num_gid_entries], 
                          &exp_elem);

    if (export_procs[i] != proc) {
      /* Export is moving to a new processor */
      New_Elem_Index[exp_elem] = -1;
      proc_ids[exp_elem] = export_procs[i];
    }
  }

  for (int i = 0; i < num_import; i++) {
    if (import_procs[i] != proc) {
      /* Import is moving from a new processor, not just from a new partition */
      /* search for first free location */
      int j=0;
      for (j = 0; j < New_Elem_Index_Size; j++) 
        if (New_Elem_Index[j] == -1) break;

      New_Elem_Index[j] = import_global_ids[gid+i*num_gid_entries];
    }
  }

  /* 
   * Update local information 
   */

  /* Set change flag for elements whose adjacent elements are being exported */

  for (int i = 0; i < num_export; i++) {

    int exp_elem = 0;

    if (num_lid_entries)
      exp_elem = export_local_ids[lid+i*num_lid_entries];
    else  /* testing num_lid_entries == 0 */
      search_by_global_id(mesh, export_global_ids[gid+i*num_gid_entries], 
                          &exp_elem);

    elements[exp_elem].my_part = export_to_part[i];

    if (export_procs[i] == proc) 
      continue;  /* No adjacency changes needed if export is changing
                    only partition, not processor. */

    for (int j = 0; j < elements[exp_elem].adj_len; j++) {

      /* Skip NULL adjacencies (sides that are not adjacent to another elem). */
      if (elements[exp_elem].adj[j] == -1) continue;

      /* Set change flag for adjacent local elements. */
      if (elements[exp_elem].adj_proc[j] == proc) {
        change[elements[exp_elem].adj[j]] = 1;
      }
    }
  }

  /* Change adjacency information in marked elements */
  for (int i = 0; i < mesh->num_elems; i++) {
    if (change[i] == 0) continue;

    /* loop over marked element's adjacencies; look for ones that are moving */
    for (int j = 0; j < elements[i].adj_len; j++) {

      /* Skip NULL adjacencies (sides that are not adjacent to another elem). */
      if (elements[i].adj[j] == -1) continue;

      if (elements[i].adj_proc[j] == proc) {
        /* adjacent element is local; check whether it is moving. */
        int new_proc = proc_ids[elements[i].adj[j]];
        if (new_proc != proc) {
          /* Adjacent element is being exported; update this adjacency entry */
          elements[i].adj[j] = elements[elements[i].adj[j]].globalID;
          elements[i].adj_proc[j] = new_proc;
        }
      }
    }
  }
  delete [] change;

  /*
   * Update off-processor information 
   */

  int maxlen = 0;
  int *send_vec = NULL;

  for (int i = 0; i < mesh->necmap; i++) 
    maxlen += mesh->ecmap_cnt[i];

  if (maxlen > 0) {
    send_vec = new int [maxlen];
    if (send_vec == NULL) {
      Gen_Error(0, "fatal: insufficient memory");
      *ierr = ZOLTAN_MEMERR;
      delete [] proc_ids;
      delete [] change;
      return;
    }

    /* Load send vector */

    for (int i = 0; i < maxlen; i++)
      send_vec[i] = proc_ids[mesh->ecmap_elemids[i]];
  }

  delete [] proc_ids;

  int *recv_vec = NULL;

  if (maxlen > 0)
    recv_vec = new int [maxlen];

  /*  Perform boundary exchange */

  boundary_exchange(mesh, 1, send_vec, recv_vec);
  
  /* Unload receive vector */

  int offset = 0;
  for (int i = 0; i < mesh->necmap; i++) {
    for (int j = 0; j < mesh->ecmap_cnt[i]; j++, offset++) {
      if (recv_vec[offset] == mesh->ecmap_id[i]) {
        /* off-processor element is not changing processors.  */
        /* no changes are needed in the local data structure. */
        continue;
      }
      /* Change processor assignment in local element's adjacency list */
      int bor_elem = mesh->ecmap_elemids[offset];
      for (k = 0; k < elements[bor_elem].adj_len; k++) {

        /* Skip NULL adjacencies (sides that are not adj to another elem). */
        if (elements[bor_elem].adj[k] == -1) continue;

        if (elements[bor_elem].adj[k] == mesh->ecmap_neighids[offset] &&
            elements[bor_elem].adj_proc[k] == mesh->ecmap_id[i]) {
          elements[bor_elem].adj_proc[k] = recv_vec[offset];
          if (recv_vec[offset] == proc) {
            /* element is moving to this processor; */
            /* convert adj from global to local ID. */
            int idx = in_list(mesh->ecmap_neighids[offset],New_Elem_Index_Size,
                              New_Elem_Index);
            if (idx == -1) {
              sprintf(msg, "fatal: unable to locate element %d in "
                           "New_Elem_Index", mesh->ecmap_neighids[offset]);
              Gen_Error(0, msg);
              *ierr = ZOLTAN_FATAL;
              if (send_vec) delete [] send_vec;
              if (recv_vec) delete [] recv_vec;
              return;
            }
            elements[bor_elem].adj[k] = idx;
          }
          break;  /* from k loop */
        }
      }
    }
  }

  if (recv_vec) delete [] recv_vec;
  if (send_vec) delete [] send_vec;

  /*
   * Allocate space (if needed) for the new element data.
   */

  if (mesh->elem_array_len < New_Elem_Index_Size) {
    mesh->elem_array_len = New_Elem_Index_Size;

    // We don't use C++ new/delete here, because this was malloc'd
    // in some C code.

    mesh->elements = (ELEM_INFO_PTR) realloc (mesh->elements,
                                     mesh->elem_array_len * sizeof(ELEM_INFO));
    if (mesh->elements == NULL) {
      Gen_Error(0, "fatal: insufficient memory");
      return;
    }

    /* initialize the new spots */
    for (int i = mesh->num_elems; i < mesh->elem_array_len; i++)
      initialize_element(&(mesh->elements[i]));
  }
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
void migrate_post_process(void *data, int num_gid_entries, int num_lid_entries,
                          int num_import, 
                          ZOLTAN_ID_PTR import_global_ids,
                          ZOLTAN_ID_PTR import_local_ids, int *import_procs,
                          int *import_to_part,
                          int num_export, ZOLTAN_ID_PTR export_global_ids,
                          ZOLTAN_ID_PTR export_local_ids, int *export_procs,
                          int *export_to_part,
                          int *ierr)
{
  if (data == NULL) {
    *ierr = ZOLTAN_FATAL;
    return;
  }
  MESH_INFO_PTR mesh = (MESH_INFO_PTR) data;
  ELEM_INFO *elements = mesh->elements;

  int proc = 0, num_proc = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &proc);
  MPI_Comm_size(MPI_COMM_WORLD, &num_proc);

  /* compact elements array, as the application expects the array to be dense */
  for (int i = 0; i < New_Elem_Index_Size; i++) {
    if (New_Elem_Index[i] != -1) continue;

    /* Don't want to shift all elements down one position to fill the  */
    /* blank spot -- too much work to adjust adjacencies!  So find the */
    /* last element in the array and move it to the blank spot.        */

    int last = 0;

    for (last = New_Elem_Index_Size-1; last >= 0; last--)
      if (New_Elem_Index[last] != -1) break;

    /* If (last < i), array is already dense; i is just in some blank spots  */
    /* at the end of the array.  Quit the compacting.                     */
    if (last < i) break;

    /* Copy elements[last] to elements[i]. */
    elements[i] = elements[last];

    /* Adjust adjacencies for local elements.  Off-processor adjacencies */
    /* don't matter here.                                                */

    for (int j = 0; j < elements[i].adj_len; j++) {

      /* Skip NULL adjacencies (sides that are not adjacent to another elem). */
      if (elements[i].adj[j] == -1) continue;

      int adj_elem = elements[i].adj[j];

      /* See whether adjacent element is local; if so, adjust its entry */
      /* for local element i.                                           */
      if (elements[i].adj_proc[j] == proc) {
        for (int k = 0; k < elements[adj_elem].adj_len; k++) {
          if (elements[adj_elem].adj[k] == last &&
              elements[adj_elem].adj_proc[k] == proc) {
            /* found adjacency entry for element last; change it to i */
            elements[adj_elem].adj[k] = i;
            break;
          }
        }
      }
    }

    /* Update New_Elem_Index */
    New_Elem_Index[i] = New_Elem_Index[last];
    New_Elem_Index[last] = -1;

    /* clear elements[last] */
    elements[last].globalID = -1;
    elements[last].border = 0;
    elements[last].my_part = -1;
    elements[last].perm_value = -1;
    elements[last].invperm_value = -1;
    elements[last].nadj = 0;
    elements[last].adj_len = 0;
    elements[last].elem_blk = -1;
    for (int k=0; k<MAX_CPU_WGTS; k++)
      elements[last].cpu_wgt[k] = 0;
    elements[last].mem_wgt = 0;
    elements[last].avg_coord[0] = elements[last].avg_coord[1] 
                                = elements[last].avg_coord[2] = 0.;
    elements[last].coord = NULL;
    elements[last].connect = NULL;
    elements[last].adj = NULL;
    elements[last].adj_proc = NULL;
    elements[last].edge_wgt = NULL;
  }

  if (New_Elem_Index != NULL) {
    delete [] New_Elem_Index;
    New_Elem_Index = NULL;
   } 

  New_Elem_Index_Size = 0;

  if (!build_elem_comm_maps(proc, mesh)) {
    Gen_Error(0, "Fatal: error rebuilding elem comm maps");
  }

  if (mesh->data_type == HYPERGRAPH && !update_elem_dd(mesh)) {
    Gen_Error(0, "Fatal: error updating element dd");
  }

  if (mesh->data_type == HYPERGRAPH && !update_hvertex_proc(mesh)) {
    Gen_Error(0, "Fatal: error updating hyperedges");
  }

  if (Debug_Driver > 3)
    print_distributed_mesh(proc, num_proc, mesh);
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
int migrate_elem_size(void *data, int num_gid_entries, int num_lid_entries,
    ZOLTAN_ID_PTR elem_gid, ZOLTAN_ID_PTR elem_lid, int *ierr)
/*
 * Function to return size of element information for a single element.
 */
{
int gid = num_gid_entries-1;
int lid = num_lid_entries-1;
int idx = 0;

  *ierr = ZOLTAN_OK;

  if (data == NULL) {
    *ierr = ZOLTAN_FATAL;
    return 0;
  }
  MESH_INFO_PTR mesh = (MESH_INFO_PTR) data;
  ELEM_INFO *current_elem = (num_lid_entries 
                   ? &(mesh->elements[elem_lid[lid]])
                   : search_by_global_id(mesh, elem_gid[gid], &idx));
  int num_nodes = mesh->eb_nnodes[current_elem->elem_blk];

  /*
   * Compute size of one element's data.
   */

  int size = sizeof(ELEM_INFO);
 
  /* Add space to correct alignment so casts work in (un)packing. */
  size = Zoltan_Object::Align(size);

  /* Add space for connect table. */
  if (mesh->num_dims > 0)
    size += num_nodes * sizeof(int);

  /* Add space for adjacency info (elements[].adj and elements[].adj_proc). */
  size += current_elem->adj_len * 2 * sizeof(int);

  /* Assume if one element has edge wgts, all elements have edge wgts. */
  if (Use_Edge_Wgts) {
    /* Add space to correct alignment so casts work in (un)packing. */
    size = Zoltan_Object::Align(size);
    size += current_elem->adj_len * sizeof(float);
  }

  /* Add space for coordinate info */
  size = Zoltan_Object::Align(size);
  size += num_nodes * mesh->num_dims * sizeof(float);
  
  return (size);
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
void migrate_pack_elem(void *data, int num_gid_entries, int num_lid_entries,
                       ZOLTAN_ID_PTR elem_gid, ZOLTAN_ID_PTR elem_lid,
                       int mig_part, int elem_data_size, char *buf, int *ierr)
{
  int gid = num_gid_entries-1;
  int lid = num_lid_entries-1;

  if (data == NULL) {
    *ierr = ZOLTAN_FATAL;
    return;
  }
  MESH_INFO_PTR mesh = (MESH_INFO_PTR) data;
  ELEM_INFO *elem = mesh->elements;

  int proc = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &proc);

  int idx;

  ELEM_INFO *current_elem = (num_lid_entries 
                   ? &(elem[elem_lid[lid]])
                   : search_by_global_id(mesh, elem_gid[gid], &idx));

  int num_nodes = mesh->eb_nnodes[current_elem->elem_blk];

  ELEM_INFO *elem_mig = (ELEM_INFO *) buf;

  /*
   * copy the ELEM_INFO structure
   */
  *elem_mig = *current_elem;
  int size = sizeof(ELEM_INFO);

  /*
   * copy the allocated integer fields for this element.
   */

  /* Pad the buffer so the following casts will work.  */
  
  size = Zoltan_Object::Align(size);

  int *buf_int = (int *) (buf + size);

  /* copy the connect table */
  if (mesh->num_dims > 0) {
    for (int i = 0; i < num_nodes; i++) {
      *buf_int = current_elem->connect[i];
      buf_int++;
    }
    size += num_nodes * sizeof(int);
  }

  /* copy the adjacency info */
  /* send globalID for all adjacencies */
  for (int i =  0; i < current_elem->adj_len; i++) {
    if (current_elem->adj[i] != -1 && current_elem->adj_proc[i] == proc) 
      *buf_int = New_Elem_Index[current_elem->adj[i]];
    else
      *buf_int = current_elem->adj[i];
    buf_int++;
    *buf_int = current_elem->adj_proc[i];
    buf_int++;
  }
  size += current_elem->adj_len * 2 * sizeof(int);

  /*
   * copy the allocated float fields for this element.
   */

  /* copy the edge_wgt data */

  float *buf_float = NULL;
  if (Use_Edge_Wgts) {

    /* Pad the buffer so the following casts will work.  */
    size = Zoltan_Object::Align(size);
    buf_float = (float *) (buf + size);

    for (int i = 0; i < current_elem->adj_len; i++) {
      *buf_float = current_elem->edge_wgt[i];
      buf_float++;
    }
    size += current_elem->adj_len * sizeof(float);
  }

  /* Pad the buffer so the following casts will work.  */
  size = Zoltan_Object::Align(size);
  buf_float = (float *) (buf + size);

  /* copy coordinate data */
  for (int i = 0; i < num_nodes; i++) {
    for (int j = 0; j < mesh->num_dims; j++) {
      *buf_float = current_elem->coord[i][j];
      buf_float++;
    }
  }
  size += num_nodes * mesh->num_dims * sizeof(float);

  /*
   * need to update the Mesh struct to reflect this element
   * being gone
   */
  mesh->num_elems--;
  mesh->eb_cnts[current_elem->elem_blk]--;

  /*
   * need to remove this entry from this procs list of elements
   * do so by setting the globalID to -1
   */
  current_elem->globalID = -1;
  free_element_arrays(current_elem, mesh);

  /*
   * NOTE: it is not worth the effort to determine the change in the
   * number of nodes on this processor until all of the migration is
   * completed.
   */
  if (size > elem_data_size) 
    *ierr = ZOLTAN_WARN;
  else
    *ierr = ZOLTAN_OK;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
void migrate_unpack_elem(void *data, int num_gid_entries, ZOLTAN_ID_PTR elem_gid, 
                         int elem_data_size, char *buf, int *ierr)
{
  int gid = num_gid_entries-1;

  if (data == NULL) {
    *ierr = ZOLTAN_FATAL;
    return;
  }
  MESH_INFO_PTR mesh = (MESH_INFO_PTR) data;
  ELEM_INFO *elem = mesh->elements;
  ELEM_INFO *elem_mig = (ELEM_INFO *) buf;

  int proc = 0;
  MPI_Comm_rank(MPI_COMM_WORLD, &proc);

  int idx = 0;

  if ((idx = in_list(elem_gid[gid], New_Elem_Index_Size, New_Elem_Index)) == -1) {
    Gen_Error(0, "fatal: Unable to locate position for element");
    *ierr = ZOLTAN_FATAL;
    return;
  }

  ELEM_INFO *current_elem = &(elem[idx]);
  /* now put the migrated information into the array */
  *current_elem = *elem_mig;
  int num_nodes = mesh->eb_nnodes[current_elem->elem_blk];

  int size = sizeof(ELEM_INFO);

  /*
   * copy the allocated integer fields for this element.
   */

  /* Pad the buffer so the following casts will work.  */
  size = Zoltan_Object::Align(size);
  int *buf_int = (int *) (buf + size);

  /* copy the connect table */
  if (mesh->num_dims > 0) {

    // Don't use C++ new/delete here becuase these items are
    // malloc'd/free'd elsewhere in C code

    current_elem->connect = (int *) malloc(num_nodes * sizeof(int));
    if (current_elem->connect == NULL) {
      Gen_Error(0, "fatal: insufficient memory");
      *ierr = ZOLTAN_MEMERR;
      return;
    }
    for (int i = 0; i < num_nodes; i++) {
      current_elem->connect[i] = *buf_int;
      buf_int++;
    }
    size += num_nodes * sizeof(int);
  }

  /* copy the adjacency info */
  /* globalIDs are received; convert to local IDs when adj elem is local */

  float *buf_float = NULL;

  if (current_elem->adj_len > 0) {
    current_elem->adj      = (int *)malloc(current_elem->adj_len * sizeof(int));
    current_elem->adj_proc = (int *)malloc(current_elem->adj_len * sizeof(int));
    if (current_elem->adj == NULL || current_elem->adj_proc == NULL) {
      Gen_Error(0, "fatal: insufficient memory");
      *ierr = ZOLTAN_MEMERR;
      return;
    }
    for (int i =  0; i < current_elem->adj_len; i++) {
      current_elem->adj[i] = *buf_int;
      buf_int++;
      current_elem->adj_proc[i] = *buf_int;
      buf_int++;
      if (current_elem->adj[i] != -1 && current_elem->adj_proc[i] == proc) 
        current_elem->adj[i] = in_list(current_elem->adj[i], 
                                       New_Elem_Index_Size, New_Elem_Index);
    }
    size += current_elem->adj_len * 2 * sizeof(int);


    /* copy the edge_wgt data */
    if (Use_Edge_Wgts) {

      /* Pad the buffer so the following casts will work.  */
      size = Zoltan_Object::Align(size);
      buf_float = (float *) (buf + size);

      current_elem->edge_wgt = (float *) malloc(current_elem->adj_len 
                                              * sizeof(float));
      if (current_elem->edge_wgt == NULL) {
        Gen_Error(0, "fatal: insufficient memory");
        *ierr = ZOLTAN_MEMERR;
        return;
      }
      for (int i = 0; i < current_elem->adj_len; i++) {
        current_elem->edge_wgt[i] = *buf_float;
        buf_float++;
      }
      size += current_elem->adj_len * sizeof(float);
    }
  }

  /* copy coordinate data */
  if (num_nodes > 0) {

    /* Pad the buffer so the following casts will work.  */
    size = Zoltan_Object::Align(size);
    buf_float = (float *) (buf + size);

    current_elem->coord = (float **) malloc(num_nodes * sizeof(float *));
    if (current_elem->coord == NULL) {
      Gen_Error(0, "fatal: insufficient memory");
      *ierr = ZOLTAN_MEMERR;
      return;
    }
    for (int i = 0; i < num_nodes; i++) {
      current_elem->coord[i] = (float *) malloc(mesh->num_dims * sizeof(float));
      if (current_elem->coord[i] == NULL) {
        Gen_Error(0, "fatal: insufficient memory");
        *ierr = ZOLTAN_MEMERR;
        return;
      }
      for (int j = 0; j < mesh->num_dims; j++) {
        current_elem->coord[i][j] = *buf_float;
        buf_float++;
      }
    }
    size += num_nodes * mesh->num_dims * sizeof(float);
  }


  /* and update the Mesh struct */
  mesh->num_elems++;
  mesh->eb_cnts[current_elem->elem_blk]++;

  if (size > elem_data_size) 
    *ierr = ZOLTAN_WARN;
  else
    *ierr = ZOLTAN_OK;
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

void migrate_elem_size_multi(
  void *data,
  int num_gid_entries,
  int num_lid_entries,
  int num_ids,
  ZOLTAN_ID_PTR global_ids,
  ZOLTAN_ID_PTR local_ids,
  int *num_bytes,
  int *ierr
)
{
  *ierr = ZOLTAN_OK;
  for (int i = 0; i < num_ids; i++) {
    num_bytes[i] = migrate_elem_size(data, num_gid_entries, num_lid_entries,
                  &(global_ids[i*num_gid_entries]),
                  (num_lid_entries!=0 ? &(local_ids[i*num_lid_entries]) : NULL),
                  ierr);
    if (*ierr != ZOLTAN_OK)
      return;
  }
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

void migrate_pack_elem_multi(
  void *data,
  int num_gid_entries,
  int num_lid_entries,
  int num_ids,
  ZOLTAN_ID_PTR global_ids,
  ZOLTAN_ID_PTR local_ids,
  int *dest_proc,
  int *size,
  int *index,
  char *buffer,
  int *ierr
)
{
  *ierr = ZOLTAN_OK;
  for (int i = 0; i < num_ids; i++) {
    migrate_pack_elem(data, num_gid_entries, num_lid_entries,
            &(global_ids[i*num_gid_entries]),
            (num_lid_entries!=0 ? &(local_ids[i*num_lid_entries]) : NULL),
            dest_proc[i], size[i], &(buffer[index[i]]), ierr);
    if (*ierr != ZOLTAN_OK)
      return;
  }
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/

void migrate_unpack_elem_multi(
  void *data,
  int num_gid_entries,
  int num_ids,
  ZOLTAN_ID_PTR global_ids,
  int *size,
  int *index,
  char *buffer,
  int *ierr
)
{
  *ierr = ZOLTAN_OK;
  for (int i = 0; i < num_ids; i++) {
    migrate_unpack_elem(data, num_gid_entries,
                        &(global_ids[i*num_gid_entries]),
                        size[i], &(buffer[index[i]]), ierr);
    if (*ierr != ZOLTAN_OK)
      return;
  }
}

/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
