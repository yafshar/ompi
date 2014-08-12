/*
 * Copyright (c) 2007-2014 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2004-2013 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2008      Sun Microsystems, Inc.  All rights reserved.
 * Copyright (c) 2012-2013 Inria.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

/**********************************************************************
 * Copyright (C) 2000-2004 by Etnus, LLC.
 * Copyright (C) 1999 by Etnus, Inc.
 * Copyright (C) 1997-1998 Dolphin Interconnect Solutions Inc.
 *
 * Permission is hereby granted to use, reproduce, prepare derivative
 * works, and to redistribute to others.
 *
 *				  DISCLAIMER
 *
 * Neither Dolphin Interconnect Solutions, Etnus LLC, nor any of their
 * employees, makes any warranty express or implied, or assumes any
 * legal liability or responsibility for the accuracy, completeness,
 * or usefulness of any information, apparatus, product, or process
 * disclosed, or represents that its use would not infringe privately
 * owned rights.
 *
 * This code was written by
 * James Cownie: Dolphin Interconnect Solutions. <jcownie@dolphinics.com>
 *               Etnus LLC <jcownie@etnus.com>
 **********************************************************************/

#include "ompi_config.h"

#include <string.h>
#include <stdlib.h>

#include "ompi/mca/pml/base/pml_base_request.h"
#include "mpihandles_interface.h"
#include "ompi_mpihandles_dll_defs.h"
#include "ompi/communicator/communicator.h"


#define OPAL_ALIGN(x,a,t) (((x)+((t)(a)-1)) & ~(((t)(a)-1)))

/* Globals that the debugger expects to find in the DLL */
#if defined(WORDS_BIGENDIAN)
uint8_t mpidbg_dll_is_big_endian = 1;
#else
uint8_t mpidbg_dll_is_big_endian = 0;
#endif
uint8_t mpidbg_dll_bitness = (char) (sizeof(void*) * 8);
enum mpidbg_comm_capabilities_t mpidbg_comm_capabilities = 0;
struct mpidbg_name_map_t mpidbg_comm_name_map[MPIDBG_COMM_MAX + 1];
enum mpidbg_errhandler_capabilities_t mpidbg_errhandler_capabilities = 0;
struct mpidbg_name_map_t mpidbg_errhandler_name_map[MPIDBG_ERRHANDLER_MAX + 1];
enum mpidbg_request_capabilities_t mpidbg_request_capabilities = 0;
struct mpidbg_name_map_t mpidbg_request_name_map[MPIDBG_REQUEST_MAX + 1];
enum mpidbg_status_capabilities_t mpidbg_status_capabilities = 0;
struct mpidbg_name_map_t mpidbg_status_name_map[MPIDBG_STATUS_MAX + 1];

#if defined(__SUNPRO_C)
/*
 * These symbols are defined here because of the different way compilers
 * may handle extern definitions. The particular case that is causing
 * problems is when there is an extern variable that is accessed in a
 * static inline function. For example, here is the code we often see in
 * a header file.
 *
 * extern int request_complete;
 * static inline check_request(void) {
 *    request_complete = 1;
 * }
 *
 * If this code exists in a header file and gets included in a source
 * file, then some compilers expect to have request_complete defined
 * somewhere even if request_complete is never referenced and
 * check_request is never called. Other compilers do not need them defined
 * if they are never referenced in the source file. Therefore, to handle
 * cases like the above with compilers that require the symbol (like
 * Sun Studio) we add in these definitions here.
 */
size_t ompi_request_completed;
opal_condition_t ompi_request_cond;
size_t ompi_request_waiting;
opal_mutex_t ompi_request_lock;
opal_mutex_t opal_event_lock;
int opal_progress_spin_count;
bool opal_mutex_check_locks;
bool opal_uses_threads;
#endif /* defined(__SUNPRO_C) */

/* Constants to convey semantic information to the debugger about some
   constants from the MPI implementation */

const int mpidbg_max_object_name = MPIDBG_MAX_OBJECT_NAME;
const int mpidbg_max_filename = MPIDBG_MAX_FILENAME;
const int mpidbg_interface_version = MPIDBG_INTERFACE_VERSION;

/*---------------------------------------------------------------------*/

/* Small helper function: look up a symbol, and if we find it, put it
   in a map entry */
static void fill_map(mqs_image *image,
                     char *public_name, char *private_name,
                     struct mpidbg_name_map_t *map)
{
    mqs_taddr_t value;
    mpi_image_info *i_info = (mpi_image_info *) mqs_get_image_info(image);

    if (NULL != i_info) {
        map->map_name = strdup(public_name);
        if (NULL != private_name) {
            if (mqs_ok == mqs_find_symbol(image, private_name, &value)) {
                map->map_handle = value;
                return;
            }
        } else {
            map->map_handle = 0;
            return;
        }
    }

    printf("OMPI MPI handles DLL: fill_map: Unable to find symbol: %s\n",
           private_name);
}

/* Helper function to lookup MPI attributes and fill an
   mpidbg_attribute_pair_t array with their keys/values */
static int fill_attributes(int *num_attrs,
                           struct mpidbg_attribute_pair_t **attrs,
                           mqs_taddr_t table)
{
    /* JMS fill me in */
    return mqs_ok;
}

/*---------------------------------------------------------------------*/

int mpidbg_init_once(const mqs_basic_callbacks *cb)
{
    mqs_basic_entrypoints = cb;
    printf("mpidbg_init_once\n");
    return MPIDBG_SUCCESS;
}

/*---------------------------------------------------------------------*/

/* Returns the fixed value */
int mpidbg_interface_version_compatibility(void)
{
    printf("mpidbg_interface_version_compatibility\n");
    return MPIDBG_INTERFACE_VERSION;
}


/* Returns a string specific to OMPI */
char *mpidbg_version_string(void)
{
    printf("mpidbg_version_string\n");
    return "Open MPI handle interpretation support for parallel"
           " debuggers compiled on " __DATE__;
}


/* So the debugger can tell what interface width the library was
   compiled with */
int mpidbg_dll_taddr_width(void)
{
    printf("mpidbg_dll_taddr_width\n");
    return sizeof(mqs_taddr_t);
}

/*---------------------------------------------------------------------*/

/* Once-per-image setup */
int mpidbg_init_per_image(mqs_image *image, const mqs_image_callbacks *icb,
                          struct mpidbg_handle_info_t *handle_types)
{
    char *message;
    mpi_image_info *i_info = (mpi_image_info *) mqs_malloc(sizeof(*i_info));
    printf("mpidbg_init_per_image\n");

    if (NULL == i_info) {
        printf("mpidbg_init_per_image: malloc failed!\n");
        return MPIDBG_ERR_NO_MEM;
    }

    memset((void *) i_info, 0, sizeof(*i_info));
    /* Before we do *ANYTHING* else */
    i_info->image_callbacks = icb;

    /* Nothing extra (yet) */
    i_info->extra = NULL;

    /* Save the info */
    mqs_put_image_info(image, (mqs_image_info *)i_info);

    /* Fill in the OMPI type information */
    if (mqs_ok != ompi_fill_in_type_info(image, &message)) {
        return MPIDBG_ERR_NOT_SUPPORTED;
    }

    /* Fill in the handle_types struct with our types */
    /* JMS: "MPI_Aint" is a typedef -- is that enough?  (the actual
       type is a #define, so it's not easy to put into the
       mqs_find_type call as a string) */
    handle_types->hi_c_aint = mqs_find_type(image, "MPI_Aint", mqs_lang_c);
    /* JMS: these ompi types are just the "foo" types; but OMPI MPI
       types are all "foo*"'s -- is this right?  If this is wrong, I
       *suspect* that something like the following may be right:

       handle_types->hi_c_comm =
         mqs_find_type(image, "ompi_communicator_t*", mqs_lang_c);

       Need to confirm this with the DDT guys...
    */
    handle_types->hi_c_comm = i_info->ompi_communicator_t.type;
    handle_types->hi_c_datatype = i_info->ompi_datatype_t.type;
    handle_types->hi_c_errhandler =
        mqs_find_type(image, "ompi_errhandler_t", mqs_lang_c);
    handle_types->hi_c_file =
        mqs_find_type(image, "ompi_file_t", mqs_lang_c);
    handle_types->hi_c_group = i_info->ompi_group_t.type;
    handle_types->hi_c_info =
        mqs_find_type(image, "ompi_info_t", mqs_lang_c);
    /* JMS: "MPI_Offset" is a typedef (see comment about MPI_Aint above) */
    handle_types->hi_c_offset =
        mqs_find_type(image, "MPI_Offset", mqs_lang_c);
    handle_types->hi_c_op =
        mqs_find_type(image, "ompi_op_t", mqs_lang_c);
    handle_types->hi_c_request = i_info->ompi_request_t.type;
    handle_types->hi_c_status = i_info->ompi_status_public_t.type;
    handle_types->hi_c_win =
        mqs_find_type(image, "ompi_win_t", mqs_lang_c);

    /* MPI::Aint is a typedef to MPI_Aint */
    handle_types->hi_cxx_aint = handle_types->hi_cxx_aint;
    handle_types->hi_cxx_comm =
        mqs_find_type(image, "MPI::Comm", mqs_lang_cplus);
    handle_types->hi_cxx_intracomm =
        mqs_find_type(image, "MPI::Intracomm", mqs_lang_cplus);
    handle_types->hi_cxx_intercomm =
        mqs_find_type(image, "MPI::Intercomm", mqs_lang_cplus);
    handle_types->hi_cxx_graphcomm =
        mqs_find_type(image, "MPI::Graphcomm", mqs_lang_cplus);
    handle_types->hi_cxx_cartcomm =
        mqs_find_type(image, "MPI::Cartcomm", mqs_lang_cplus);
    handle_types->hi_cxx_datatype =
        mqs_find_type(image, "MPI::Datatype", mqs_lang_cplus);
    handle_types->hi_cxx_errhandler =
        mqs_find_type(image, "MPI::Errhandler", mqs_lang_cplus);
    handle_types->hi_cxx_file =
        mqs_find_type(image, "MPI::File", mqs_lang_cplus);
    handle_types->hi_cxx_group =
        mqs_find_type(image, "MPI::Group", mqs_lang_cplus);
    handle_types->hi_cxx_info =
        mqs_find_type(image, "MPI::Info", mqs_lang_cplus);
    /* MPI::Offset is a typedef to MPI_Offset */
    handle_types->hi_cxx_offset = handle_types->hi_c_offset;
    handle_types->hi_cxx_op =
        mqs_find_type(image, "MPI::Op", mqs_lang_cplus);
    handle_types->hi_cxx_request =
        mqs_find_type(image, "MPI::Request", mqs_lang_cplus);
    handle_types->hi_cxx_prequest =
        mqs_find_type(image, "MPI::Prequest", mqs_lang_cplus);
    handle_types->hi_cxx_grequest =
        mqs_find_type(image, "MPI::Grequest", mqs_lang_cplus);
    handle_types->hi_cxx_status =
        mqs_find_type(image, "MPI::Status", mqs_lang_cplus);
    handle_types->hi_cxx_win =
        mqs_find_type(image, "MPI::Win", mqs_lang_cplus);

    /* Tell the debuger what capabilities we have */
    mpidbg_comm_capabilities =
        MPIDBG_COMM_CAP_BASIC |
        MPIDBG_COMM_CAP_STRING_NAMES |
        MPIDBG_COMM_CAP_FREED_HANDLE |
        MPIDBG_COMM_CAP_FREED_OBJECT;
    mpidbg_errhandler_capabilities =
        MPIDBG_ERRH_CAP_BASIC |
        MPIDBG_ERRH_CAP_STRING_NAMES |
        MPIDBG_ERRH_CAP_FREED_HANDLE |
        MPIDBG_ERRH_CAP_FREED_OBJECT;
    mpidbg_request_capabilities =
        MPIDBG_REQUEST_CAP_BASIC;
    mpidbg_status_capabilities =
        MPIDBG_STATUS_CAP_BASIC;

    /* All done */
    printf("mpidbg_init_per_image: init succeeded -- ready!\n");
    return MPIDBG_SUCCESS;
}


/* This image is now dead; free all the state associated with it */
void mpidbg_finalize_per_image(mqs_image *image, mqs_image_info *info)
{
    mpi_image_info *i_info = (mpi_image_info *)info;

    printf("mpidbg_finalize_per_image\n");
    if (NULL != i_info->extra) {
        mqs_free(i_info->extra);
    }
    mqs_free(info);
}

/*---------------------------------------------------------------------*/

/* Setup information needed for a specific process.  The debugger
 * assumes that this will hang something onto the process, if nothing
 * is attached to it, then TV will believe that this process has no
 * message queue information.
 */
int mpidbg_init_per_process(mqs_process *process,
                            const mqs_process_callbacks *pcb,
                            struct mpidbg_handle_info_t *handle_types)
{
    mqs_image *image;
    mpi_image_info *i_info;

    /* Extract the addresses of the global variables we need and save
       them away */
    mpi_process_info *p_info =
        (mpi_process_info *) mqs_malloc(sizeof(mpi_process_info));
    printf("mpidbg_init_per_process\n");

    if (NULL == p_info) {
        return MPIDBG_ERR_NO_MEM;
    }

    /* Setup the callbacks first */
    p_info->process_callbacks = pcb;

    /* Nothing extra (yet) */
    p_info->extra = NULL;

    /* Now we can get the rest of the info */
    image = mqs_get_image(process);
    i_info = (mpi_image_info *) mqs_get_image_info(image);

    /* Get process info sizes */
    mqs_get_type_sizes (process, &p_info->sizes);

    /* Save the info */
    mqs_put_process_info(process, (mqs_process_info *) p_info);

    /* Fill in pre-defined MPI handle name mappings (because OMPI uses
       #define's for the pre-defined names, such as "#define
       MPI_COMM_WORLD &ompi_mpi_comm_world"). */
    /* Communicators */
    if (NULL != mpidbg_comm_name_map) {
        fill_map(image, "MPI_COMM_WORLD", "ompi_mpi_comm_world",
                 &mpidbg_comm_name_map[MPIDBG_COMM_WORLD]);
        fill_map(image, "MPI_COMM_SELF", "ompi_mpi_comm_self",
                 &mpidbg_comm_name_map[MPIDBG_COMM_SELF]);
        fill_map(image, "MPI_COMM_SELF", "ompi_mpi_comm_parent",
                 &mpidbg_comm_name_map[MPIDBG_COMM_PARENT]);
        fill_map(image, "MPI_COMM_NULL", "ompi_mpi_comm_null",
                 &mpidbg_comm_name_map[MPIDBG_COMM_NULL]);

        /* Sentinel value */
        mpidbg_comm_name_map[MPIDBG_COMM_MAX].map_name = NULL;
    }

    /* Error handlers */
    if (NULL != mpidbg_errhandler_name_map) {
        fill_map(image, "MPI_ERRORS_ARE_FATAL", "ompi_mpi_errors_are_fatal",
                 &mpidbg_errhandler_name_map[MPIDBG_ERRHANDLER_ARE_FATAL]);
        fill_map(image, "MPI_ERRORS_RETURN", "ompi_mpi_errors_return",
                 &mpidbg_errhandler_name_map[MPIDBG_ERRHANDLER_RETURN]);
        fill_map(image, "MPI_ERRORS_RETURN", "ompi_mpi_errors_throw_exceptions",
                 &mpidbg_errhandler_name_map[MPIDBG_ERRHANDLER_THROW_EXCEPTIONS]);
        fill_map(image, "MPI_ERRHANDLER_NULL", "ompi_mpi_errhandler_null",
                 &mpidbg_errhandler_name_map[MPIDBG_ERRHANDLER_NULL]);

        /* Sentinel value */
        mpidbg_errhandler_name_map[MPIDBG_ERRHANDLER_MAX].map_name = NULL;
    }

    /* Requests */
    if (NULL != mpidbg_request_name_map) {
        fill_map(image, "MPI_REQUEST_NULL", "ompi_request_null",
                 &mpidbg_request_name_map[MPIDBG_REQUEST_NULL]);

        /* Sentinel value */
        mpidbg_request_name_map[MPIDBG_REQUEST_MAX].map_name = NULL;
    }

    /* Statuses */
    if (NULL != mpidbg_status_name_map) {
        fill_map(image, "MPI_STATUS_IGNORE", NULL,
                 &mpidbg_status_name_map[MPIDBG_STATUS_IGNORE]);
        fill_map(image, "MPI_STATUSES_IGNORE", NULL,
                 &mpidbg_status_name_map[MPIDBG_STATUS_IGNORES]);

        /* Sentinel value */
        mpidbg_status_name_map[MPIDBG_STATUS_MAX].map_name = NULL;
    }

    /* All done */
    return MPIDBG_SUCCESS;
}


/* This process is now done; free all the state associated with it */
void mpidbg_finalize_per_process(mqs_process *process, mqs_process_info *info)
{
    mpi_process_info *p_info = (mpi_process_info *)info;

    printf("mpidbg_finalize_per_process\n");
    if (NULL != p_info->extra) {
        mqs_free(p_info->extra);
    }
    mqs_free(info);
}


/*---------------------------------------------------------------------*/

int mpidbg_comm_query(mqs_image *image, mqs_image_info *image_info,
                      mqs_process *process, mqs_process_info *process_info,
                      mqs_taddr_t comm, struct mpidbg_comm_handle_t **ch)
{
    int flags, err;
    mpi_image_info *i_info = (mpi_image_info*) image_info;
    mpi_process_info *p_info = (mpi_process_info*) process_info;
    mqs_taddr_t group, topo, keyhash;
    struct ompi_mpidbg_comm_handle_t *handle;

    /* Set it to NULL until we have a full answer to return */
    (*ch) = NULL;

    /* Allocate an OMPI comm debugger handle */
    handle = mqs_malloc(sizeof(*handle));
    if (NULL == handle) {
        return MPIDBG_ERR_NO_MEM;
    }
    /* JMS temporarily zero everything out.  Remove this when we fill
       in all the fields */
    memset((void*) handle, 0, sizeof(*handle));
    handle->comm_handle.c_comm = comm;
    handle->comm_handle.image = image;
    handle->comm_handle.image_info = image_info;
    handle->comm_handle.process = process;
    handle->comm_handle.process_info = process_info;

    /* Set the magic number */
    handle->comm_magic_number = OMPI_DBG_COMM_MAGIC_NUMBER;

    mqs_fetch_data(process, comm + i_info->ompi_communicator_t.offset.c_name,
                   MPIDBG_MAX_OBJECT_NAME, handle->comm_name);

    /* Get this process' rank in the comm */
    handle->comm_rank = ompi_fetch_int(process,
                                        comm + i_info->ompi_communicator_t.offset.c_my_rank,
                                        p_info);

    /* Analyze the flags on the comm */
    flags = ompi_fetch_int(process,
                           comm + i_info->ompi_communicator_t.offset.c_flags,
                           p_info);
    handle->comm_bitflags = 0;
    if (MPI_PROC_NULL == handle->comm_rank) {
        /* This communicator is MPI_COMM_NULL */
        handle->comm_rank = handle->comm_size = 0;
        handle->comm_bitflags |= MPIDBG_COMM_INFO_COMM_NULL;
    } else if (0 != (flags & OMPI_COMM_INTER)) {
        handle->comm_bitflags |= MPIDBG_COMM_INFO_INTERCOMM;
    } else {
        if (0 != (flags & OMPI_COMM_CART)) {
            handle->comm_bitflags |= MPIDBG_COMM_INFO_CARTESIAN;
        } else if (0 != (flags & OMPI_COMM_GRAPH)) {
            handle->comm_bitflags |= MPIDBG_COMM_INFO_GRAPH;
        } else if (0 != (flags & OMPI_COMM_DIST_GRAPH)) {
            handle->comm_bitflags |= MPIDBG_COMM_INFO_DIST_GRAPH;
        }
    }
    if (0 != (flags & OMPI_COMM_ISFREED)) {
        handle->comm_bitflags |= MPIDBG_COMM_INFO_FREED_HANDLE;
    }
    if (0 != (flags & OMPI_COMM_INTRINSIC)) {
        handle->comm_bitflags |= MPIDBG_COMM_INFO_PREDEFINED;
    }
    if (0 != (flags & OMPI_COMM_INVALID)) {
        handle->comm_bitflags |= MPIDBG_COMM_INFO_FREED_OBJECT;
    }

    /* Look up the local group */
    group = ompi_fetch_pointer(process,
                               comm + i_info->ompi_communicator_t.offset.c_local_group,
                               p_info);
    handle->comm_rank = ompi_fetch_int(process,
                                        group + i_info->ompi_group_t.offset.grp_my_rank,
                                        p_info);
    handle->comm_num_local_procs = ompi_fetch_int(process,
                                                   group + i_info->ompi_group_t.offset.grp_proc_count,
                                                   p_info);

    /* Fill in the comm_size with the size of the local group.  We'll
       override below if this is an intercommunicator. */
    handle->comm_size = handle->comm_num_local_procs;

    /* JMS fill this in: waiting to decide between mpidbg_process_t
       and mqs_process_location */
    handle->comm_local_procs = NULL;

    /* Look up the remote group (if relevant) */
    if (0 != (flags & OMPI_COMM_INTER)) {
        group = ompi_fetch_pointer(process,
                                   comm + i_info->ompi_communicator_t.offset.c_remote_group,
                                   p_info);
        handle->comm_num_remote_procs = ompi_fetch_int(process,
                                                        group + i_info->ompi_group_t.offset.grp_proc_count,
                                                        p_info);
        handle->comm_size = handle->comm_num_remote_procs;

        /* JMS fill this in: waiting to decide between
           mpidbg_process_t and mqs_process_location */
        handle->comm_remote_procs = NULL;
    } else {
        handle->comm_num_remote_procs = 0;
        handle->comm_remote_procs = NULL;
    }

    /* Fill in cartesian/graph handle, if relevant.  The cartesian and
       graph data is just slightly different from each other; it's
       [slightly] easier (and less confusing!) to have separate
       retrieval code blocks. */
    topo = ompi_fetch_pointer(process,
                              comm + i_info->ompi_communicator_t.offset.c_topo,
                              p_info);
    if (0 != topo &&
        0 != (handle->comm_bitflags & MPIDBG_COMM_INFO_CARTESIAN)) {
        int i, ndims, tmp;
        mqs_taddr_t dims, periods, coords;

        /* Alloc space for copying arrays */
        handle->comm_cart_num_dims = ndims =
            ompi_fetch_int(process,
                           topo + i_info->mca_topo_base_module_t.offset.mtc_cart.ndims,
                           p_info);
        handle->comm_cart_dims = mqs_malloc(ndims * sizeof(int));
        if (NULL == handle->comm_cart_dims) {
            err = MPIDBG_ERR_NO_MEM;
            goto error;
        }
        handle->comm_cart_periods = mqs_malloc(ndims * sizeof(int8_t));
        if (NULL == handle->comm_cart_periods) {
            err = MPIDBG_ERR_NO_MEM;
            goto error;
        }
        handle->comm_cart_coords = mqs_malloc(ndims * sizeof(int8_t));
        if (NULL == handle->comm_cart_coords) {
            mqs_free(handle->comm_cart_periods); handle->comm_cart_periods = NULL;
            mqs_free(handle->comm_cart_dims);    handle->comm_cart_dims = NULL;
            return MPIDBG_ERR_NO_MEM;
        }

        /* Retrieve the dimension and periodic description data from
           the two arrays on the image's communicator */
        dims = ompi_fetch_pointer(process,
                                 topo + i_info->mca_topo_base_module_t.offset.mtc_cart.dims,
                                 p_info);
        periods = ompi_fetch_pointer(process,
                                 topo + i_info->mca_topo_base_module_t.offset.mtc_cart.periods,
                                 p_info);
        coords = ompi_fetch_pointer(process,
                                 topo + i_info->mca_topo_base_module_t.offset.mtc_cart.coords,
                                 p_info);

        for (i = 0; i < ndims; ++i) {
            handle->comm_cart_dims[i] =
                ompi_fetch_int(process, dims + (sizeof(int) * i), p_info);
            tmp = ompi_fetch_int(process, periods + (sizeof(int) * i), p_info);
            handle->comm_cart_periods[i] = (int8_t) tmp;
            printf("mpidbg: cart comm: dimension %d: (length %d, periodic: %d)\n", i, handle->comm_cart_dims[i], tmp);
        }
    } else if (0 != topo &&
               0 != (handle->comm_bitflags & MPIDBG_COMM_INFO_GRAPH)) {
        int i, nnodes;
        mqs_taddr_t index, edges;

        /* Alloc space for copying the indexes */
        handle->comm_graph_num_nodes = nnodes =
            ompi_fetch_int(process,
                           topo + i_info->mca_topo_base_module_t.offset.mtc_graph.nnodes,
                           p_info);
        handle->comm_graph_index = mqs_malloc(nnodes * sizeof(int));
        if (NULL == handle->comm_graph_index) {
            err = MPIDBG_ERR_NO_MEM;
            goto error;
        }

        /* Retrieve the index data */
        index = ompi_fetch_pointer(process,
                                 topo + i_info->mca_topo_base_module_t.offset.mtc_graph.index,
                                 p_info);
        for (i = 0; i < nnodes; ++i) {
            handle->comm_graph_index[i] =
                ompi_fetch_int(process, index + (sizeof(int) * i), p_info);
        }

        /* Allocate space for the edges */
        handle->comm_graph_edges = mqs_malloc(handle->comm_graph_index[handle->comm_graph_num_nodes - 1] * sizeof(int));
        if (NULL == handle->comm_graph_edges) {
            err = MPIDBG_ERR_NO_MEM;
            goto error;
        }

        /* Retrieve the edge data */
        edges = ompi_fetch_pointer(process,
                                 topo + i_info->mca_topo_base_module_t.offset.mtc_graph.edges,
                                 p_info);
        for (i = 0;
             i < handle->comm_graph_index[handle->comm_graph_num_nodes - 1];
             ++i) {
            handle->comm_graph_edges[i] =
                ompi_fetch_int(process, edges + (sizeof(int) * i), p_info);
        }
    } else if (0 != topo &&
               0 != (handle->comm_bitflags & MPIDBG_COMM_INFO_DIST_GRAPH)) {
        /* JMS TODO: Complete the info if the communicator has a distributed graph topology */
    }

    /* Fortran handle */
    handle->comm_fortran_handle =
        ompi_fetch_int(process,
                       comm + i_info->ompi_communicator_t.offset.c_f_to_c_index,
                       p_info);
    printf("mpdbg: comm fortran handle: %d\n", handle->comm_fortran_handle);

    /* CXX handle -- JMS fill me in */
    handle->comm_cxx_handle = MPIDBG_ERR_NOT_SUPPORTED;

    /* Fill in attributes */
    keyhash = ompi_fetch_pointer(process,
                                 comm + i_info->ompi_communicator_t.offset.c_keyhash,
                                 p_info);
    fill_attributes(&(handle->comm_num_attrs), &(handle->comm_attrs),
                    keyhash);

    /* JMS temporary */
    handle->comm_num_pending_requests = MPIDBG_ERR_NOT_SUPPORTED;
    handle->comm_pending_requests = NULL;
    handle->comm_num_derived_windows = MPIDBG_ERR_NOT_SUPPORTED;
    handle->comm_derived_windows = NULL;
    handle->comm_num_derived_files = MPIDBG_ERR_NOT_SUPPORTED;
    handle->comm_derived_files = NULL;

    /* We're happy -- set the return handle */
    (*ch) = &(handle->comm_handle);

    return MPIDBG_SUCCESS;

 error:
    if (NULL != handle->comm_cart_dims) {
        mqs_free(handle->comm_cart_dims);
    }
    if (NULL != handle->comm_cart_periods) {
        mqs_free(handle->comm_cart_periods);
    }
    if (NULL != handle->comm_graph_index) {
        mqs_free(handle->comm_graph_index);
    }
    if (NULL != handle->comm_graph_edges) {
        mqs_free(handle->comm_graph_edges);
    }

    return err;
}

/*
 * Free a handle returned by mpidbg_comm_query()
 */
int mpidbg_comm_handle_free(struct mpidbg_comm_handle_t *ch)
{
    /* Upcast to our augmented handle type */
    struct ompi_mpidbg_comm_handle_t *handle =
        (struct ompi_mpidbg_comm_handle_t*) ch;

    if (NULL == ch ||
        OMPI_DBG_COMM_MAGIC_NUMBER != handle->comm_magic_number) {
        return MPIDBG_ERR_NOT_FOUND;
    }

    /* Free Cartesian / Graph communicator information */
    if (0 != (handle->comm_bitflags & MPIDBG_COMM_INFO_CARTESIAN)) {
        if (NULL != handle->comm_cart_dims) {
            mqs_free(handle->comm_cart_dims);
        }
        if (NULL != handle->comm_cart_periods) {
            mqs_free(handle->comm_cart_periods);
        }
    } else if (0 != (handle->comm_bitflags & MPIDBG_COMM_INFO_GRAPH)) {
        if (NULL != handle->comm_graph_index) {
            mqs_free(handle->comm_graph_index);
        }
        if (NULL != handle->comm_graph_edges) {
            mqs_free(handle->comm_graph_edges);
        }
    }

    /* Free attributes */
    /* JMS fill me in */

    /* Zero it all out, just to be sure */
    memset((void*) handle, 0, sizeof(*handle));

    /* Free the struct itself; done */
    mqs_free(handle);

    return MPIDBG_SUCCESS;
}

/*
 * Return basic communicator information
 */
int mpidbg_comm_query_basic(struct mpidbg_comm_handle_t *ch,
                            char comm_name[MPIDBG_MAX_OBJECT_NAME],
                            enum mpidbg_comm_info_bitmap_t *comm_bitflags,
                            int *comm_rank,
                            int *comm_size,
                            int *comm_fortran_handle,
                            mqs_taddr_t *comm_cxx_handle,
                            struct mpidbg_keyvalue_pair_t ***comm_extra_info)
{
    /* Upcast to our augmented handle type */
    struct ompi_mpidbg_comm_handle_t *handle =
        (struct ompi_mpidbg_comm_handle_t*) ch;

    if (NULL == ch ||
        OMPI_DBG_COMM_MAGIC_NUMBER != handle->comm_magic_number) {
        return MPIDBG_ERR_NOT_FOUND;
    }

    /* Copy the values */
    memcpy(comm_name, handle->comm_name, MPIDBG_MAX_OBJECT_NAME);
    *comm_bitflags = handle->comm_bitflags;
    *comm_rank = handle->comm_rank;
    *comm_size = handle->comm_size;
    *comm_fortran_handle = handle->comm_fortran_handle;
    *comm_cxx_handle = handle->comm_cxx_handle;

    /* At the moment, we have nothing else to report */
    *comm_extra_info = NULL;

    return MPIDBG_SUCCESS;
}

/*
 * Return communicator process information
 */
int mpidbg_comm_query_procs(struct mpidbg_comm_handle_t *ch,
                            int *comm_num_local_procs,
                            struct mpidbg_process_t **comm_local_procs,
                            int *comm_num_remote_procs,
                            struct mpidbg_process_t **comm_remote_procs)
{
    /* Upcast to our augmented handle type */
    struct ompi_mpidbg_comm_handle_t *handle =
        (struct ompi_mpidbg_comm_handle_t*) ch;

    if (NULL == ch ||
        OMPI_DBG_COMM_MAGIC_NUMBER != handle->comm_magic_number) {
        return MPIDBG_ERR_NOT_FOUND;
    }

    /* Alloc and copy local procs */
    *comm_num_local_procs = handle->comm_num_local_procs;
    *comm_local_procs = mqs_malloc(sizeof(struct mpidbg_process_t) *
                                   *comm_num_local_procs);
    if (NULL == *comm_local_procs) {
        return MPIDBG_ERR_NO_MEM;
    }
    memcpy(*comm_local_procs, handle->comm_local_procs,
           sizeof(struct mpidbg_process_t) * *comm_num_local_procs);

    /* If there are remote procs, alloc and copy them, too */
    *comm_num_remote_procs = handle->comm_num_remote_procs;
    if (*comm_num_remote_procs > 0) {
        *comm_remote_procs = mqs_malloc(sizeof(struct mpidbg_process_t *) *
                                        *comm_num_remote_procs);
        memcpy(*comm_remote_procs, handle->comm_remote_procs,
               sizeof(struct mpidbg_process_t) * *comm_num_remote_procs);
    } else {
        *comm_remote_procs = NULL;
    }

    return MPIDBG_SUCCESS;
}

/*
 * Return communicator topology information
 */
int mpidbg_comm_query_topo(struct mpidbg_comm_handle_t *ch,
                           int *comm_out_length,
                           int **comm_cdorgi,
                           int **comm_cporge)
{
    /* Upcast to our augmented handle type */
    struct ompi_mpidbg_comm_handle_t *handle =
        (struct ompi_mpidbg_comm_handle_t*) ch;

    if (NULL == ch ||
        OMPI_DBG_COMM_MAGIC_NUMBER != handle->comm_magic_number ||
        0 == (handle->comm_bitflags & (MPIDBG_COMM_INFO_CARTESIAN |
                                       MPIDBG_COMM_INFO_GRAPH))) {
        return MPIDBG_ERR_NOT_FOUND;
    }

    /* Cartesian communicators */
    if (0 != (handle->comm_bitflags & MPIDBG_COMM_INFO_CARTESIAN)) {
        size_t size = sizeof(int) * handle->comm_cart_num_dims;
        *comm_out_length = handle->comm_cart_num_dims;

        /* Dimensions */
        *comm_cdorgi = mqs_malloc(size);
        if (NULL == *comm_cdorgi) {
            return MPIDBG_ERR_NO_MEM;
        }
        memcpy(*comm_cdorgi, handle->comm_cart_dims, size);

        /* Periods */
        *comm_cporge = mqs_malloc(size);
        if (NULL == *comm_cporge) {
            mqs_free(*comm_cdorgi);
            return MPIDBG_ERR_NO_MEM;
        }
        memcpy(*comm_cporge, handle->comm_cart_periods, size);
    }

    /* Graph communicators */
    else if (0 != (handle->comm_bitflags & MPIDBG_COMM_INFO_GRAPH)) {
        size_t size = sizeof(int) * handle->comm_graph_num_nodes;
        *comm_out_length = handle->comm_graph_num_nodes;

        /* Indexes */
        *comm_cdorgi = mqs_malloc(size);
        if (NULL == *comm_cdorgi) {
            return MPIDBG_ERR_NO_MEM;
        }
        memcpy(*comm_cdorgi, handle->comm_graph_index, size);

        /* Edges */
        size = sizeof(int) *
            handle->comm_graph_index[handle->comm_graph_num_nodes - 1];
        *comm_cporge = mqs_malloc(size);
        if (NULL == *comm_cporge) {
            mqs_free(*comm_cdorgi);
            return MPIDBG_ERR_NO_MEM;
        }
        memcpy(*comm_cporge, handle->comm_graph_edges, size);
    }

    /* Unknown -- shouldn't happen */
    else {
        return MPIDBG_ERR_NOT_FOUND;
    }

    return MPIDBG_SUCCESS;
}

/*
 * Return communicator attribute information
 */
int mpidbg_comm_query_attrs(struct mpidbg_comm_handle_t *ch,
                            int *comm_num_attrs,
                            struct mpidbg_attribute_pair_t *comm_attrs)
{
    /* Upcast to our augmented handle type */
    struct ompi_mpidbg_comm_handle_t *handle =
        (struct ompi_mpidbg_comm_handle_t*) ch;

    if (NULL == ch ||
        OMPI_DBG_COMM_MAGIC_NUMBER != handle->comm_magic_number) {
        return MPIDBG_ERR_NOT_FOUND;
    }

    /* JMS Fill me in someday */
    return MPIDBG_ERR_NOT_SUPPORTED;
}

/*
 * Return communicator request information
 */
int mpidbg_comm_query_requests(struct mpidbg_comm_handle_t *ch,
                               int *comm_num_requests,
                               mqs_taddr_t **comm_pending_requests)
{
    /* Upcast to our augmented handle type */
    struct ompi_mpidbg_comm_handle_t *handle =
        (struct ompi_mpidbg_comm_handle_t*) ch;

    if (NULL == ch ||
        OMPI_DBG_COMM_MAGIC_NUMBER != handle->comm_magic_number) {
        return MPIDBG_ERR_NOT_FOUND;
    }

    /* JMS Fill me in someday */
    return MPIDBG_ERR_NOT_SUPPORTED;
}

/*
 * Return communicator derived object information
 */
int mpidbg_comm_query_derived(struct mpidbg_comm_handle_t *ch,
                              int *comm_num_derived_files,
                              mqs_taddr_t **comm_derived_files,
                              int *comm_num_derived_windows,
                              mqs_taddr_t **comm_derived_windows)
{
    /* Upcast to our augmented handle type */
    struct ompi_mpidbg_comm_handle_t *handle =
        (struct ompi_mpidbg_comm_handle_t*) ch;

    if (NULL == ch ||
        OMPI_DBG_COMM_MAGIC_NUMBER != handle->comm_magic_number) {
        return MPIDBG_ERR_NOT_FOUND;
    }

    /* JMS Fill me in someday */
    return MPIDBG_ERR_NOT_SUPPORTED;
}

/*---------------------------------------------------------------------*/

int mpidbg_errhandler_query(mqs_image *image,
                            mqs_image_info *image_info,
                            mqs_process *process,
                            mqs_process_info *process_info,
                            mqs_taddr_t c_errhandler,
                            struct mpidbg_errhandler_handle_t **handle)
{
    printf("mpidbg_errhandler_query: %p\n", (void*) c_errhandler);
    printf("mpidbg_errhandler_query: not [yet] found\n");
    return MPIDBG_ERR_NOT_FOUND;
}

/*
 * Free a handle returned by mpidbg_errhandler_query()
 */
int mpidbg_errhandler_handle_free(struct mpidbg_errhandler_handle_t *eh)
{
    /* Upcast to our augmented handle type */
    struct ompi_mpidbg_errhandler_handle_t *handle =
        (struct ompi_mpidbg_errhandler_handle_t*) eh;

    if (NULL == eh ||
        OMPI_DBG_ERRHANDLER_MAGIC_NUMBER != handle->eh_magic_number) {
        return MPIDBG_ERR_NOT_FOUND;
    }

    if (NULL != handle->eh_handles) {
        mqs_free(handle->eh_handles);
    }

    /* Zero it all out, just to be sure */
    memset((void*) handle, 0, sizeof(*handle));

    /* Free the struct itself; done */
    mqs_free(handle);

    return MPIDBG_SUCCESS;
}

/*---------------------------------------------------------------------*/

int mpidbg_request_query(mqs_image *image,
                         mqs_image_info *image_info,
                         mqs_process *process,
                         mqs_process_info *process_info,
                         mqs_taddr_t c_request,
                         struct mpidbg_request_handle_t **handle)
{
    printf("mpidbg_request_query: %p\n", (void*) c_request);
    printf("mpidbg_request_query: not [yet] found\n");
    return MPIDBG_ERR_NOT_FOUND;
}

/*
 * Free a handle returned by mpidbg_request_query()
 */
int mpidbg_request_handle_free(struct mpidbg_request_handle_t *rh)
{
    /* Upcast to our augmented handle type */
    struct ompi_mpidbg_request_handle_t *handle =
        (struct ompi_mpidbg_request_handle_t*) rh;

    if (NULL == rh ||
        OMPI_DBG_REQUEST_MAGIC_NUMBER != handle->req_magic_number) {
        return MPIDBG_ERR_NOT_FOUND;
    }

    /* Zero it all out, just to be sure */
    memset((void*) handle, 0, sizeof(*handle));

    /* Free the struct itself; done */
    mqs_free(handle);

    return MPIDBG_SUCCESS;
}

/*---------------------------------------------------------------------*/

int mpidbg_status_query(mqs_image *image,
                        mqs_image_info *image_info,
                        mqs_process *process,
                        mqs_process_info *process_info,
                        mqs_taddr_t c_status,
                        struct mpidbg_status_handle_t **handle)
{
    printf("mpidbg_status_query: %p\n", (void*) c_status);
    printf("mpidbg_status_query: not [yet] found\n");
    return MPIDBG_ERR_NOT_FOUND;
}

/*
 * Free a handle returned by mpidbg_status_query()
 */
int mpidbg_status_handle_free(struct mpidbg_status_handle_t *sh)
{
    /* Upcast to our augmented handle type */
    struct ompi_mpidbg_status_handle_t *handle =
        (struct ompi_mpidbg_status_handle_t*) sh;

    if (NULL == sh ||
        OMPI_DBG_STATUS_MAGIC_NUMBER != handle->status_magic_number) {
        return MPIDBG_ERR_NOT_FOUND;
    }

    /* Zero it all out, just to be sure */
    memset((void*) handle, 0, sizeof(*handle));

    /* Free the struct itself; done */
    mqs_free(handle);

    return MPIDBG_SUCCESS;
}
