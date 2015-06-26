/*
 * Copyright (c) 2015 Cisco Systems, Inc.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef OMPI_MCA_CIDALLOC_CIDLOWEST_H
#define OMPI_MCA_CIDALLOC_CIDLOWEST_H

#include "ompi_config.h"

#include "ompi/mca/cidalloc/cidalloc.h"

BEGIN_C_DECLS

typedef struct mca_cidalloc_lowestcid_module_t
    mca_cidalloc_lowestcid_module_t;

OMPI_DECLSPEC extern mca_bml_base_component_2_0_0_t
    mca_cidalloc_lowestcid_component;
extern mca_cidalloc_lowestcid_module_t mca_cidalloc_lowestcid;

mca_bml_base_module_t*
mca_cidalloc_lowestcid_component_init(int *priority,
                                      bool enable_mpi_threads);

int mca_cidalloc_lowestcid_finalize(void);

END_C_DECLS

#endif // OMPI_MCA_CIDALLOC_LOWESTCID_H
