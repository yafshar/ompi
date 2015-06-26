# -*- shell-script -*-
#
# Copyright (c) 2015 Cisco Systems, Inc.  All rights reserved.
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

# MCA_ompi_cidalloc_lowestcid_CONFIG(action-if-can-compile,
#                        [action-if-cant-compile])
# ------------------------------------------------
# We can always build, unless we were explicitly disabled.
AC_DEFUN([MCA_ompi_cidalloc_lowestcid_CONFIG],[
    AC_CONFIG_FILES([ompi/mca/cidalloc/lowestcid/Makefile])
    [$1]
])dnl
