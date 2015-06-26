#
# Copyright (c) 2015 Cisco Systems, Inc.  All rights reserved.
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

lowestcid_sources  = \
	cidalloc_lowestcid.h \
	cidalloc_lowestcid_component.c \
	cidalloc_lowestcid_module.c

if MCA_BUILD_ompi_cidalloc_lowestcid_DSO
component_noinst =
component_install = mca_cidalloc_lowestcid.la
else
component_noinst = libmca_cidalloc_lowestcid.la
component_install =
endif

mcacomponentdir = $(ompilibdir)
mcacomponent_LTLIBRARIES = $(component_install)
mca_cidalloc_lowestcid_la_SOURCES = $(lowestcid_sources)
mca_cidalloc_lowestcid_la_LDFLAGS = -module -avoid-version

noinst_LTLIBRARIES = $(component_noinst)
libmca_cidalloc_lowestcid_la_SOURCES = $(lowestcid_sources)
libmca_cidalloc_lowestcid_la_LDFLAGS = -module -avoid-version
