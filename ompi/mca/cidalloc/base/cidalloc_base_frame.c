/*
 * Copyright (c) 2015 Cisco Systems, Inc.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */


#include "ompi_config.h"

#include <stdio.h>

#include <string.h>
#include <unistd.h>

#include "ompi/mca/mca.h"
#include "opal/util/output.h"
#include "opal/mca/base/base.h"

#include "ompi/constants.h"
#include "ompi/mca/cidalloc/cidalloc.h"
#include "ompi/mca/cidalloc/base/base.h"

/*
 * The following file was created by configure.  It contains extern
 * statements and the definition of an array of pointers to each
 * component's public mca_base_component_t struct.
 */

#include "ompi/mca/cidalloc/base/static-components.h"

mca_cidalloc_base_component_t mca_cidalloc_base_selected_component = {{0}};
opal_pointer_array_t mca_cidalloc_base_cidalloc = {{0}};
char *ompi_cidalloc_base_bsend_allocator_name = NULL;

static int mca_cidalloc_base_register(mca_base_register_flag_t flags)
{
    return OMPI_SUCCESS;
}

int mca_cidalloc_base_finalize(void)
{
    if (NULL != mca_cidalloc_base_selected_component.cidallocm_finalize) {
        return mca_cidalloc_base_selected_component.cidallocm_finalize();
    }
    return OMPI_SUCCESS;
}


static int mca_cidalloc_base_close(void)
{
    /* Close all remaining available components */
    return mca_base_framework_components_close(&ompi_cidalloc_base_framework, NULL);
}

/**
 * Function for finding and opening either all MCA components, or the one
 * that was specifically requested via a MCA parameter.
 */
static int mca_cidalloc_base_open(mca_base_open_flag_t flags)
{
    /* Open up all available components */

    if (OPAL_SUCCESS !=
        mca_base_framework_components_open(&ompi_cidalloc_base_framework, flags)) {
        return OMPI_ERROR;
    }

    return OMPI_SUCCESS;

}

MCA_BASE_FRAMEWORK_DECLARE(ompi, cidalloc,
                           "OMPI CID allocator",
                           mca_cidalloc_base_register,
                           mca_cidalloc_base_open,
                           mca_cidalloc_base_close,
                           mca_cidalloc_base_static_components,
                           0);
