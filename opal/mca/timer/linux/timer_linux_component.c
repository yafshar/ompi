/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2016 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * Copyright (c) 2015-2017 Los Alamos National Security, LLC. All rights
 *                         reserved.
 * Copyright (c) 2015-2017 Cisco Systems, Inc.  All rights reserved
 * Copyright (c) 2016 Broadcom Limited. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "opal_config.h"

#include <string.h>

#include "opal/mca/timer/timer.h"
#include "opal/mca/timer/base/base.h"
#include "opal/mca/timer/linux/timer_linux.h"
#include "opal/constants.h"
#include "opal/util/show_help.h"
#include "opal/util/proc.h"

static opal_timer_t opal_timer_linux_get_cycles_sys_timer(void);
static opal_timer_t opal_timer_linux_get_usec_sys_timer(void);

/**
 * Define some sane defaults until we call the _init function.
 */
#if OPAL_HAVE_CLOCK_GETTIME
static opal_timer_t opal_timer_linux_get_cycles_clock_gettime(void);
static opal_timer_t opal_timer_linux_get_usec_clock_gettime(void);

opal_timer_t (*opal_timer_base_get_cycles)(void) =
    opal_timer_linux_get_cycles_clock_gettime;
opal_timer_t (*opal_timer_base_get_usec)(void) =
    opal_timer_linux_get_usec_clock_gettime;
#else
opal_timer_t (*opal_timer_base_get_cycles)(void) =
    opal_timer_linux_get_cycles_sys_timer;
opal_timer_t (*opal_timer_base_get_usec)(void) =
    opal_timer_linux_get_usec_sys_timer;
#endif  /* OPAL_HAVE_CLOCK_GETTIME */

static opal_timer_t opal_timer_linux_freq = {0};

static int opal_timer_linux_open(void);

const opal_timer_base_component_2_0_0_t mca_timer_linux_component = {
    /* First, the mca_component_t struct containing meta information
       about the component itself */
    .timerc_version = {
        OPAL_TIMER_BASE_VERSION_2_0_0,

        /* Component name and version */
        .mca_component_name = "linux",
        MCA_BASE_MAKE_VERSION(component, OPAL_MAJOR_VERSION, OPAL_MINOR_VERSION,
                              OPAL_RELEASE_VERSION),

        /* Component open and close functions */
        .mca_open_component = opal_timer_linux_open,
    },
    .timerc_data = {
        /* The component is checkpoint ready */
        MCA_BASE_METADATA_PARAM_CHECKPOINT
    },
};

static char *
find_info(FILE* fp, char *str, char *buf, size_t buflen)
{
    char *tmp;

    rewind(fp);
    while (NULL != fgets(buf, buflen, fp)) {
        if (strncmp(buf, str, strlen(str)) == 0) {
            /* we found the line.  Now eat everything up to,
               including, and one past the : */
            for (tmp = buf ; (*tmp != '\0') && (*tmp != ':') ; ++tmp) ;
            if (*tmp == '\0') {
                continue;
            }
            for ( ++tmp ; *tmp == ' ' ; ++tmp);
            if ('\0' != *tmp) {
                return tmp;
            }
        }
    }

    return NULL;
}

static int opal_timer_linux_find_freq(bool* constant_tsc)
{
    FILE *fp;
    char *loc;
    float cpu_f;
    int ret;
    char buf[1024];

    *constant_tsc = false;

    fp = fopen("/proc/cpuinfo", "r");
    if (NULL == fp) {
        return OPAL_ERR_IN_ERRNO;
    }

    opal_timer_linux_freq = 0;

#if OPAL_ASSEMBLY_ARCH == OPAL_ARM64
    opal_timer_linux_freq = opal_sys_timer_freq();
#endif

    loc = find_info(fp, "flags", buf, 1024);
    if (NULL != loc) {
        if (NULL != strstr(loc, "constant_tsc")) {
            *constant_tsc = true;
        }
    }
    if (0 == opal_timer_linux_freq) {
        /* first, look for a timebase field.  probably only on PPC,
           but one never knows */
        loc = find_info(fp, "timebase", buf, 1024);
        if (NULL != loc) {
            int freq;
            ret = sscanf(loc, "%d", &freq);
            if (1 == ret) {
                opal_timer_linux_freq = freq;
            }
        }
    }

    if (0 == opal_timer_linux_freq) {
        /* find the CPU speed - most timers are 1:1 with CPU speed */
        loc = find_info(fp, "cpu MHz", buf, 1024);
        if (NULL != loc) {
            ret = sscanf(loc, "%f", &cpu_f);
            if (1 == ret) {
                /* numer is in MHz - convert to Hz and make an integer */
                opal_timer_linux_freq = (opal_timer_t) (cpu_f * 1000000);
            }
        }
    }

    if (0 == opal_timer_linux_freq) {
        /* look for the sparc way of getting cpu frequency */
        loc = find_info(fp, "Cpu0ClkTck", buf, 1024);
        if (NULL != loc) {
            unsigned int freq;
            ret = sscanf(loc, "%x", &freq);
            if (1 == ret) {
                opal_timer_linux_freq = freq;
            }
        }
    }

    fclose(fp);

    /* convert the timer frequency to MHz to avoid an extra operation when
     * converting from cycles to usec */
    opal_timer_linux_freq /= 1000000;

    return OPAL_SUCCESS;
}

int opal_timer_linux_open(void)
{
    bool constant_tsc = false;
    bool want_clock_gettime = false;
    bool want_sys_timer = false;

    // Find out the system clock frequency, and if it is constant.
    opal_timer_linux_find_freq(&constant_tsc);
    opal_output_verbose(5, opal_timer_base_framework.framework_output,
                        "timer:linux: constant TSC: %d",
                        (int) constant_tsc);

    // The logic block below could likely be reduced to a simpler
    // expression.  It is kept expanded to explicitly show all 4
    // cases, just for simplicity / ease of reading the code.
    // Additionally, this code is executed during startup; it's not
    // worth optimizing down to a minimal set of comparisons.

    // Has the user requested (via MCA param) a monontonic timer?
    if (mca_timer_base_monotonic) {
        opal_output_verbose(5, opal_timer_base_framework.framework_output,
                            "timer:linux: want monontonic timer");

        // If the system timer is monotonic and its frequency is
        // constant, use it.
        if (opal_sys_timer_is_monotonic() && constant_tsc) {
            want_sys_timer = true;
        }
        // Otherwise, fall back to clock_gettime().
        else {
            want_clock_gettime = true;
        }
    }
    // If the user did not request a monotonic timer...
    else {
        opal_output_verbose(5, opal_timer_base_framework.framework_output,
                            "timer:linux: don't care about monontonic timer");

        // Only use the system timer if it is constant
        if (constant_tsc) {
            want_sys_timer = true;
        }
        // Otherwise, fall back to clock_gettime()
        else {
            want_clock_gettime = true;
        }
    }

    // It's not possible to fall through the above without one of
    // want_sys_timer or want_clock_gettime being true.

    // Setup clock_gettime (if possible)
    if (want_clock_gettime) {
        opal_output_verbose(5, opal_timer_base_framework.framework_output,
                            "timer:linux: setting up clock_gettime");

#if OPAL_HAVE_CLOCK_GETTIME
        // Ensure that clock_gettime() is usable.  We only use
        // CLOCK_MONOTONIC (even if mca_timer_base_monotonic is
        // false).
        struct timespec res;
        if (0 == clock_getres(CLOCK_MONOTONIC, &res)) {
            opal_timer_linux_freq = 1.e3;
            opal_timer_base_get_cycles =
                opal_timer_linux_get_cycles_clock_gettime;
            opal_timer_base_get_usec =
                opal_timer_linux_get_usec_clock_gettime;
            return OPAL_SUCCESS;
        } else {
            opal_output_verbose(5, opal_timer_base_framework.framework_output,
                                "timer:linux: clock_getres() failed");
        }
#else
        opal_output_verbose(5, opal_timer_base_framework.framework_output,
                            "timer:linux: no clock_gettime() on this platform");
#endif
    }

    // Setup the system timer
    else if (want_sys_timer) {
        opal_output_verbose(5, opal_timer_base_framework.framework_output,
                            "timer:linux: setting up system timer");

        opal_timer_base_get_cycles =
            opal_timer_linux_get_cycles_sys_timer;
        opal_timer_base_get_usec =
            opal_timer_linux_get_usec_sys_timer;
        return OPAL_SUCCESS;
    }

    // If we get here, then no timer was set.
    opal_show_help("help-opal-timer-linux.txt",
                   "unable to setup a timer", true,
                   opal_process_info.nodename);
    return OPAL_ERR_NOT_SUPPORTED;
}

#if OPAL_HAVE_CLOCK_GETTIME
opal_timer_t opal_timer_linux_get_usec_clock_gettime(void)
{
    struct timespec tp = {.tv_sec = 0, .tv_nsec = 0};

    (void) clock_gettime (CLOCK_MONOTONIC, &tp);

    return (tp.tv_sec * 1e6 + tp.tv_nsec/1000);
}

opal_timer_t opal_timer_linux_get_cycles_clock_gettime(void)
{
    struct timespec tp = {.tv_sec = 0, .tv_nsec = 0};

    (void) clock_gettime(CLOCK_MONOTONIC, &tp);

    return (tp.tv_sec * 1e9 + tp.tv_nsec);
}
#endif  /* OPAL_HAVE_CLOCK_GETTIME */

opal_timer_t opal_timer_linux_get_cycles_sys_timer(void)
{
#if OPAL_HAVE_SYS_TIMER_GET_CYCLES
    return opal_sys_timer_get_cycles();
#else
    return 0;
#endif
}


opal_timer_t opal_timer_linux_get_usec_sys_timer(void)
{
#if OPAL_HAVE_SYS_TIMER_GET_CYCLES
    /* freq is in MHz, so this gives usec */
    return opal_sys_timer_get_cycles()  / opal_timer_linux_freq;
#else
    return 0;
#endif
}

opal_timer_t opal_timer_base_get_freq(void)
{
    return opal_timer_linux_freq * 1000000;
}
