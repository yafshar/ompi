/*
 * Copyright (c) 2015 Cisco Systems, Inc. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "ompi_config.h"

#include "ompi/mca/cidalloc/lowestcid/cidalloc_lowestcid.h"

static int mca_cidalloc_lowestcid_component_register(void);

mca_bml_base_component_2_0_0_t mca_cidalloc_lowestcid_component = {

    /* First, the mca_base_component_t struct containing meta
       information about the component itself */

    .bml_version = {
        MCA_CIDALLOC_BASE_VERSION_2_0_0,

        .mca_component_name = "lowestcid",
        MCA_BASE_MAKE_VERSION(component, OMPI_MAJOR_VERSION, OMPI_MINOR_VERSION,
                              OMPI_RELEASE_VERSION),
        .mca_open_component = mca_cidalloc_lowestcid_component_open,
        .mca_close_component = mca_cidalloc_lowestcid_component_close,
        .mca_register_component_params = mca_cidalloc_lowestcid_component_register,
    },
    .cidalloc_data = {
        /* The component is checkpoint ready */
        MCA_BASE_METADATA_PARAM_CHECKPOINT
    },

    .cidalloc_init = mca_cidalloc_lowestcid_component_init,
};

static int mca_cidalloc_lowestcid_component_register(void)
{
    return OMPI_SUCCESS;
}

int mca_cidalloc_lowestcid_component_open(void)
{
    return OMPI_SUCCESS;
}


int mca_cidalloc_lowestcid_component_close(void)
{
    return OMPI_SUCCESS;
}


mca_cidalloc_base_module_t*
mca_cidalloc_lowestcid_component_init(int* priority,
                                      bool enable_mpi_threads)
{
    /* initialize BTLs */

    if(OMPI_SUCCESS != mca_btl_base_select(enable_progress_threads,enable_mpi_threads))
        return NULL;

    *priority = 100;
    mca_cidalloc_lowestcid.btls_added = false;
    return &mca_cidalloc_lowestcid.super;
}
