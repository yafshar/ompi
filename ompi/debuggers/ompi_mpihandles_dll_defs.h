/*
 * Copyright (c) 2007-2015 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2004-2007 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
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

#ifndef OMPI_MPIHANDLES_DLL_DEFS_H
#define OMPI_MPIHANDLES_DLL_DEFS_H

#include "ompi_common_dll_defs.h"
#include "mpihandles_interface.h"

/*
 * Magic numbers for sanity checking
 */
enum {
    OMPI_DBG_COMM_MAGIC_NUMBER = 0x1122,
    OMPI_DBG_REQUEST_MAGIC_NUMBER = 0x2233,
    OMPI_DBG_STATUS_MAGIC_NUMBER = 0x3344,
    OMPI_DBG_ERRHANDLER_MAGIC_NUMBER = 0x4455
};

/*
 * OMPI's internal debugger communicator handle
 */
struct ompi_mpidbg_comm_handle_t {
    /* "base" class of the standard interface */
    struct mpidbg_comm_handle_t comm_handle;

    /* Magic number for sanity checking */
    uint16_t comm_magic_number;

    /* Name of the MPI_COMM */
    char comm_name[MPIDBG_MAX_OBJECT_NAME];

    /* Bit flags describing the communicator */
    enum mpidbg_comm_info_bitmap_t comm_bitflags;

    /* This process' rank within this communicator */
    int comm_rank;
    /* The communicator's size  */
    int comm_size;

    /* Number of processes in the local group */
    int comm_num_local_procs;
    /* Information about each process in the local group (in
       communicator rank order, length: comm_num_local_procs) */
    struct mpidbg_process_t *comm_local_procs;
    /* The OPAL process name of each local group process */
    opal_process_name_t *comm_local_proc_names;

    /* For intercommunicators, the number of processes in the remote
       group */
    int comm_num_remote_procs;
    /* For intercommunicators, information about each process in the
       remote group (in communicator rank order, length:
       comm_num_remote_procs) */
    struct mpidbg_process_t *comm_remote_procs;
    /* The OPAL process name of each remote group process */
    opal_process_name_t *comm_remote_proc_names;

    /* For cartesian communicators, the number of dimensions */
    int comm_cart_num_dims;
    /* For cartesian communicators, an array of dimension lengths
       (length: cart_comm_num_dims) */
    int *comm_cart_dims;
    /* For cartesian communicators, an array of boolean values
       indicating whether each dimension is periodic or not (length:
       cart_comm_num_dims) */
    int8_t *comm_cart_periods;
    /* For cartesian communicators, an array of coordinates of this
       process */
    int8_t *comm_cart_coords;

    /* For graph communicators, the number of nodes */
    int comm_graph_num_nodes;
    /* For graph communicators, an array of the node degrees (length:
       comm_graph_num_nodes) */
    int *comm_graph_index;
    /* For graph communicators, an array of the edges (length:
       comm_graph_num_nodes) */
    int *comm_graph_edges;

    /**** JMS NEED TO ADD FIELDS HERE FOR DIST GRAPH COMMUNICATORS ****/

    /* Fortran handle; will be MPIDBG_ERR_UNAVAILABLE if currently
       unavailable or MPIDBG_ERR_NOT_SUPPORTED if not supported */
    int comm_fortran_handle;
    /* C++ handle; will be MPIDBG_ERR_UNAVAILABLE if currently
       unavailable or MPIDBG_ERR_NOT_SUPPORTED if not supported */
    int comm_cxx_handle;

    /* Number of attributes defined on this communicator */
    int comm_num_attrs;
    /* Array of attribute keyval/value pairs defined on this
       communicator (length: comm_num_attrs) */
    struct mpidbg_attribute_pair_t *comm_attrs;

    /* Number of ongoing requests within this communicator, or
       MPIDBG_ERR_NOT_SUPPORTED */
    int comm_num_pending_requests;
    /* If comm_num_pending_requests != MPIDBG_ERR_NOT_SUPPORTED, an
       array of ongoing request handles attached on this
       communicator (length: comm_num_pending_requests) */
    mqs_taddr_t *comm_pending_requests;

    /* Number of MPI windows derived from this communicator, or
       MPIDBG_ERR_NOT_SUPPORTED  */
    int comm_num_derived_windows;
    /* If comm_num_derived_windows != MPIDBG_ERR_NOT_SUPPORTED, an
       array of window handles derived from this communicator (length:
       com_num_derived_windows) */
    mqs_taddr_t *comm_derived_windows;

    /* Number of MPI files derived from this communicator, or
       MPIDBG_ERR_NOT_SUPPORTED  */
    int comm_num_derived_files;
    /* If comm_num_derived_files != MPIDBG_ERR_NOT_SUPPORTED, an array
       of file handles derived from this communicator (length:
       comm_num_derived_files) */
    mqs_taddr_t *comm_derived_files;
};

/*
 * OMPI's internal debugger request handle
 */
struct ompi_mpidbg_request_handle_t {
    /* "base" class of the standard interface */
    struct mpidbg_request_handle_t req_handle;

    /* Magic number for sanity checking */
    uint16_t req_magic_number;

    /* Bit flags describing the error handler */
    enum mpidbg_request_info_bitmap_t req_bitflags;

    /* Fortran handle; will be MPIDBG_ERR_UNAVAILABLE if currently
       unavailable or MPIDBG_ERR_NOT_SUPPORTED if not supported */
    int req_fortran_handle;
    /* C++ handle; will be MPIDBG_ERR_UNAVAILABLE if currently
       unavailable or MPIDBG_ERR_NOT_SUPPORTED if not supported */
    int comm_cxx_handle;
};

/*
 * OMPI's internal debugger status handle
 */
struct ompi_mpidbg_status_handle_t {
    /* "base" class of the standard interface */
    struct mpidbg_status_handle_t req_handle;

    /* Magic number for sanity checking */
    uint16_t status_magic_number;

    /* Bit flags describing the status */
    enum mpidbg_status_info_bitmap_t status_bitflags;
};

/*
 * OMPI's internal debugger errhandler handle
 */
struct ompi_mpidbg_errhandler_handle_t {
    /* "base" class of the standard interface */
    struct mpidbg_status_handle_t req_handle;

    /* Magic number for sanity checking */
    uint16_t eh_magic_number;

    /* String name; only relevant for predefined errorhandlers.  If
       not a predefined errorhandler, eh_name[0] will be '\0'; */
    char eh_name[MPIDBG_MAX_OBJECT_NAME];

    /* Bit flags describing the error handler */
    enum mpidbg_errhandler_info_bitmap_t eh_bitflags;

    /* Fortran handle; will be MPIDBG_ERR_UNAVAILABLE if currently
       unavailable or MPIDBG_ERR_NOT_SUPPORTED if not supported */
    int eh_fortran_handle;
    /* C++ handle; will be MPIDBG_ERR_UNAVAILABLE if currently
       unavailable or MPIDBG_ERR_NOT_SUPPORTED if not supported */
    int comm_cxx_handle;

    /* Number of MPI handles that this error handler is attached to.
       MPIDBG_ERR_NOT_SUPPORTED means that this information is not
       supported by the DLL. */
    int16_t eh_refcount;
    /* If eh_refcount != MPIDBG_ERR_NOT_SUPPORTED, list of handles
       that are using this error handler (length: eh_refcount). */
    mqs_taddr_t *eh_handles;

    /* Address of the user-defined error handler.  Note that each of
       the 3 callback signatures contain an MPI handle; the debugger
       will need to figure out the appropriate size for these types
       depending on the platform and MPI implementation.  This value
       will be NULL if MPIDBG_ERRH_INFO_PREDEFINED is set on the
       flags. */
    mqs_taddr_t eh_callback_func;
};

#endif
