#
# Copyright (c) 2015 Cisco Systems, Inc.  All rights reserved.
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

# main library setup
noinst_LTLIBRARIES = libmca_cidalloc.la
libmca_cidalloc_la_SOURCES =

# local files
headers = cidalloc.h cidalloc_constants.h
libmca_cidalloc_la_SOURCES += $(headers) $(nodist_headers)

# Conditionally install the header files
if WANT_INSTALL_HEADERS
ompidir = $(ompiincludedir)/$(subdir)
nobase_ompi_HEADERS = $(headers)
nobase_nodist_ompi_HEADERS = $(nodist_headers)
endif

include base/Makefile.am

distclean-local:
	rm -f base/static-components.h
