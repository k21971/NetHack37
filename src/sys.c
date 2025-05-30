/* NetHack 3.7	sys.c	$NHDT-Date: 1717449153 2024/06/03 21:12:33 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.64 $ */
/* Copyright (c) Kenneth Lorber, Kensington, Maryland, 2008. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#ifndef SYSCF
/* !SYSCF configurations need '#define DEBUGFILES "foo.c bar.c"'
 * to enable debugging feedback for source files foo.c and bar.c;
 * to activate debugpline(), set an appropriate value and uncomment
 */
/* # define DEBUGFILES "*" */

/* note: DEBUGFILES value here or in sysconf.DEBUGFILES can be overridden
   at runtime by setting up a value for "DEBUGFILES" in the environment */
#endif

struct sysopt_s sysopt;

void
sys_early_init(void)
{
    const char *p;

    /* Don't assume that these are not already set, and that it is
     * safe to dupstr() without orphaning any pointers. Check them. */

    sysopt.support = (char *) 0;
    sysopt.recover = (char *) 0;
    sysopt.livelog = 0;
#ifdef SYSCF
    sysopt.wizards = (char *) 0;
#else
    if (sysopt.wizards)
        free((genericptr_t) sysopt.wizards);
    sysopt.wizards = dupstr(WIZARD_NAME);
#endif

    if ((p = getenv("DEBUGFILES")) != 0) {
        if (sysopt.debugfiles)
            free((genericptr_t) sysopt.debugfiles);
        sysopt.debugfiles = dupstr(p);
        sysopt.env_dbgfl = 1; /* prevent sysconf processing from overriding */
    } else {
#if defined(SYSCF) || !defined(DEBUGFILES)
        sysopt.debugfiles = (char *) 0;
#else
        if (sysopt.debugfiles)
            free((genericptr_t) sysopt.debugfiles);
        sysopt.debugfiles = dupstr(DEBUGFILES);
#endif
        sysopt.env_dbgfl = 0;
    }

#ifdef DUMPLOG
    sysopt.dumplogfile = (char *) 0;
    sysopt.dumplogurl = (char *) 0;
#endif
#ifdef DUMPHTML
    sysopt.dumphtmlfile = (char *) 0;
#endif
    sysopt.shellers = (char *) 0;
    sysopt.explorers = (char *) 0;
    sysopt.genericusers = (char *) 0;
    sysopt.msghandler = (char *) 0;
    sysopt.maxplayers = 0; /* XXX eventually replace MAX_NR_OF_PLAYERS */
    sysopt.bones_pools = 0;
    sysopt.livelog = LL_NONE;

    /* record file */
    sysopt.persmax = max(PERSMAX, 1);
    sysopt.entrymax = max(ENTRYMAX, 10);
    sysopt.pointsmin = max(POINTSMIN, 1);
    sysopt.pers_is_uid = PERS_IS_UID;
    sysopt.tt_oname_maxrank = 10;

    /* sanity checks */
    if (sysopt.pers_is_uid != 0 && sysopt.pers_is_uid != 1)
        panic("config error: PERS_IS_UID must be either 0 or 1");

#ifdef PANICTRACE
    /* panic options */
    if (sysopt.gdbpath)
        free((genericptr_t) sysopt.gdbpath);
    sysopt.gdbpath = dupstr(GDBPATH);
    if (sysopt.greppath)
        free((genericptr_t) sysopt.greppath);
    sysopt.greppath = dupstr(GREPPATH);
#if (NH_DEVEL_STATUS != NH_STATUS_RELEASED)
    sysopt.panictrace_gdb = 1;
#ifdef PANICTRACE_LIBC
    sysopt.panictrace_libc = 2;
#endif
#else
    sysopt.panictrace_gdb = 0;
#ifdef PANICTRACE_LIBC
    sysopt.panictrace_libc = 0;
#endif
#endif
#endif
    sysopt.crashreporturl = NULL;

    sysopt.check_save_uid = 1;
    sysopt.check_plname = 0;
    sysopt.seduce = 1; /* if it's compiled in, default to on */
    sysopt_seduce_set(sysopt.seduce);
    sysopt.saveformat[0] = sysopt.bonesformat[0] = historical;
    sysopt.accessibility = 0;
#ifdef WIN32
    sysopt.portable_device_paths = 0;
#endif

    /* help menu */
    sysopt.hideusage = 0;

    return;
}

void
sysopt_release(void)
{
    if (sysopt.support)
        free((genericptr_t) sysopt.support), sysopt.support = (char *) 0;
    if (sysopt.recover)
        free((genericptr_t) sysopt.recover), sysopt.recover = (char *) 0;
    if (sysopt.wizards)
        free((genericptr_t) sysopt.wizards), sysopt.wizards = (char *) 0;
    if (sysopt.explorers)
        free((genericptr_t) sysopt.explorers), sysopt.explorers = (char *) 0;
    if (sysopt.shellers)
        free((genericptr_t) sysopt.shellers), sysopt.shellers = (char *) 0;
    if (sysopt.debugfiles)
        free((genericptr_t) sysopt.debugfiles),
        sysopt.debugfiles = (char *) 0;
    sysopt.env_dbgfl = 0;
    if (sysopt.msghandler)
        free((genericptr_t) sysopt.msghandler), sysopt.msghandler = (char *) 0;
#ifdef DUMPLOG
    if (sysopt.dumplogfile)
        free((genericptr_t) sysopt.dumplogfile), sysopt.dumplogfile=(char *) 0;
#endif
#ifdef DUMPHTML
    if (sysopt.dumphtmlfile)
        free((genericptr_t)sysopt.dumphtmlfile), sysopt.dumphtmlfile=(char *)0;
#endif
    if (sysopt.genericusers)
        free((genericptr_t) sysopt.genericusers),
            sysopt.genericusers = (char *) 0;
    if (sysopt.gdbpath)
        free((genericptr_t) sysopt.gdbpath), sysopt.gdbpath = (char *) 0;
    if (sysopt.greppath)
        free((genericptr_t) sysopt.greppath), sysopt.greppath = (char *) 0;

#ifdef CRASHREPORT
    if (gc.crash_email)
        free((genericptr_t) gc.crash_email), gc.crash_email = (char *) NULL;
    if (gc.crash_name)
        free((genericptr_t) gc.crash_name), gc.crash_name = (char *) NULL;
#endif

    /* this one's last because it might be used in panic feedback, although
       none of the preceding ones are likely to trigger a controlled panic */
    if (sysopt.fmtd_wizard_list)
        free((genericptr_t) sysopt.fmtd_wizard_list),
            sysopt.fmtd_wizard_list = (char *) 0;
    return;
}

extern const struct attack c_sa_yes[NATTK];
extern const struct attack c_sa_no[NATTK];

void
sysopt_seduce_set(
#if 0
int val)
{
/*
 * Attack substitution is now done on the fly in getmattk(mhitu.c).
 */
    struct attack *setval = val ? c_sa_yes : c_sa_no;
    int x;

    for (x = 0; x < NATTK; x++) {
        mons[PM_INCUBUS].mattk[x] = setval[x];
        mons[PM_SUCCUBUS].mattk[x] = setval[x];
    }
#else
int val UNUSED)
{
#endif
    return;
}

/*sys.c*/
