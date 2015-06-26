/*
 * Copyright (c) 2015 Cisco Systems, Inc.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "ompi_config.h"

#include "ompi/mca/cidalloc/lowestcid/cidalloc_lowestcid.h"

extern mca_cidalloc_base_component_t mca_cidalloc_lowestcid_component;


/* JMS moar here */

int mca_cidalloc_lowestcid_component_fini(void)
{
    return OMPI_SUCCESS;
}

mca_cidalloc_lowestcid_module_t mca_cidalloc_lowestcid = {
    .super = {
        /* JMS use C99 initialization */
        &mca_cidalloc_lowestcid_component,
        mca_cidalloc_lowestcid_add_procs,
        mca_cidalloc_lowestcid_del_procs,
        mca_cidalloc_lowestcid_add_btl,
        mca_cidalloc_lowestcid_del_btl,
        mca_cidalloc_lowestcid_del_proc_btl,
        mca_cidalloc_lowestcid_register,
        mca_cidalloc_lowestcid_register_error,
        mca_cidalloc_lowestcid_finalize,
        mca_cidalloc_lowestcid_ft_event
    }

};
