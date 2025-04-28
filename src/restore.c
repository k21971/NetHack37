/* NetHack 3.7	restore.c	$NHDT-Date: 1736530208 2025/01/10 09:30:08 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.234 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Michael Allison, 2009. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "tcap.h" /* for TERMLIB and ASCIIGRAPH */

#if defined(MICRO)
extern int dotcnt; /* shared with save */
extern int dotrow; /* shared with save */
#endif

staticfn void find_lev_obj(void);
staticfn void restlevchn(NHFILE *);
staticfn void restdamage(NHFILE *);
staticfn void restobj(NHFILE *, struct obj *);
staticfn struct obj *restobjchn(NHFILE *, boolean);
staticfn void restmon(NHFILE *, struct monst *);
staticfn struct monst *restmonchn(NHFILE *);
staticfn struct fruit *loadfruitchn(NHFILE *);
staticfn void freefruitchn(struct fruit *);
staticfn void rest_levl(NHFILE *);
staticfn void rest_stairs(NHFILE *);
staticfn void ghostfruit(struct obj *);
staticfn boolean restgamestate(NHFILE *);
staticfn void restlevelstate(void);
staticfn int restlevelfile(xint8);
staticfn void rest_bubbles(NHFILE *);
staticfn void restore_gamelog(NHFILE *);
staticfn void reset_oattached_mids(boolean);
staticfn boolean restgamestate(NHFILE *);
staticfn void rest_bubbles(NHFILE *);
staticfn void restore_gamelog(NHFILE *);
staticfn void restore_msghistory(NHFILE *);

/*
 * Save a mapping of IDs from ghost levels to the current level.  This
 * map is used by the timer routines when restoring ghost levels.
 */
#define N_PER_BUCKET 64
struct bucket {
    struct bucket *next;
    struct {
        unsigned gid; /* ghost ID */
        unsigned nid; /* new ID */
    } map[N_PER_BUCKET];
};

staticfn void clear_id_mapping(void);
staticfn void add_id_mapping(unsigned, unsigned);

#ifdef AMII_GRAPHICS
void amii_setpens(int); /* use colors from save file */
extern int amii_numcolors;
#endif

#include "display.h"

#define Is_IceBox(o) ((o)->otyp == ICE_BOX ? TRUE : FALSE)

/* third arg passed to mread() should be 'unsigned' but most calls use
   sizeof so are attempting to pass 'size_t'; mread()'s prototype results
   in an implicit conversion; this macro does it explicitly */
#define Mread(fd,adr,siz) mread((fd), (genericptr_t) (adr), (unsigned) (siz))

/* Recalculate svl.level.objects[x][y], since this info was not saved. */
staticfn void
find_lev_obj(void)
{
    struct obj *fobjtmp = (struct obj *) 0;
    struct obj *otmp;
    int x, y;

    for (x = 0; x < COLNO; x++)
        for (y = 0; y < ROWNO; y++)
            svl.level.objects[x][y] = (struct obj *) 0;

    /*
     * Reverse the entire fobj chain, which is necessary so that we can
     * place the objects in the proper order.  Make all obj in chain
     * OBJ_FREE so place_object will work correctly.
     */
    while ((otmp = fobj) != 0) {
        fobj = otmp->nobj;
        otmp->nobj = fobjtmp;
        otmp->where = OBJ_FREE;
        fobjtmp = otmp;
    }
    /* fobj should now be empty */

    /* Set svl.level.objects (as well as reversing the chain back again) */
    while ((otmp = fobjtmp) != 0) {
        fobjtmp = otmp->nobj;
        place_object(otmp, otmp->ox, otmp->oy);

        /* fixup(s) performed when restoring the level that the hero
           is on, rather than just an arbitrary one */
        if (u.uz.dlevel) { /* 0 during full restore until current level */
            /* handle uchain and uball when they're on the floor */
            if (otmp->owornmask & (W_BALL | W_CHAIN))
                setworn(otmp, otmp->owornmask);
        }
    }
}

/* Things that were marked "in_use" when the game was saved (ex. via the
 * infamous "HUP" cheat) get used up here.
 */
void
inven_inuse(boolean quietly)
{
    struct obj *otmp, *otmp2;

    for (otmp = gi.invent; otmp; otmp = otmp2) {
        otmp2 = otmp->nobj;
        if (otmp->in_use) {
            if (!quietly)
                pline("Finishing off %s...", xname(otmp));
            useup(otmp);
        }
    }
}

staticfn void
restlevchn(NHFILE *nhfp)
{
    int cnt = 0;
    s_level *tmplev, *x;

    svs.sp_levchn = (s_level *) 0;
    if (nhfp->structlevel) {
        Mread(nhfp->fd, &cnt, sizeof cnt);
    }
    for (; cnt > 0; cnt--) {
        tmplev = (s_level *) alloc(sizeof(s_level));
        if (nhfp->structlevel) {
            Mread(nhfp->fd, tmplev, sizeof *tmplev);
        }

        if (!svs.sp_levchn)
            svs.sp_levchn = tmplev;
        else {
            for (x = svs.sp_levchn; x->next; x = x->next)
                ;
            x->next = tmplev;
        }
        tmplev->next = (s_level *) 0;
    }
}

staticfn void
restdamage(NHFILE *nhfp)
{
    unsigned int dmgcount = 0;
    int counter;
    struct damage *tmp_dam;
    boolean ghostly = (nhfp->ftype == NHF_BONESFILE);

    if (nhfp->structlevel) {
        Mread(nhfp->fd, &dmgcount, sizeof dmgcount);
    }
    counter = (int) dmgcount;

    if (!counter)
        return;
    do {
        tmp_dam = (struct damage *) alloc(sizeof *tmp_dam);
        if (nhfp->structlevel) {
            Mread(nhfp->fd, tmp_dam, sizeof *tmp_dam);
        }

        if (ghostly)
            tmp_dam->when += (svm.moves - svo.omoves);

        tmp_dam->next = svl.level.damagelist;
        svl.level.damagelist = tmp_dam;
    } while (--counter > 0);
}

/* restore one object */
staticfn void
restobj(NHFILE *nhfp, struct obj *otmp)
{
    int buflen = 0;

    if (nhfp->structlevel) {
        Mread(nhfp->fd, otmp, sizeof *otmp);
    }

    otmp->lua_ref_cnt = 0;
    /* next object pointers are invalid; otmp->cobj needs to be left
       as is--being non-null is key to restoring container contents */
    otmp->nobj = otmp->nexthere = (struct obj *) 0;
    /* non-null oextra needs to be reconstructed */
    if (otmp->oextra) {
        otmp->oextra = newoextra();

        /* oname - object's name */
        if (nhfp->structlevel) {
            Mread(nhfp->fd, &buflen, sizeof buflen);
        }
        if (buflen > 0) { /* includes terminating '\0' */
            new_oname(otmp, buflen);
            if (nhfp->structlevel) {
                Mread(nhfp->fd, ONAME(otmp), buflen);
            }
        }

        /* omonst - corpse or statue might retain full monster details */
        if (nhfp->structlevel) {
            Mread(nhfp->fd, &buflen, sizeof buflen);
        }
        if (buflen > 0) {
            newomonst(otmp);
            /* this is actually a monst struct, so we
               can just defer to restmon() here */
            restmon(nhfp, OMONST(otmp));
        }

        /* omailcmd - feedback mechanism for scroll of mail */
        if (nhfp->structlevel) {
            Mread(nhfp->fd, &buflen, sizeof buflen);
        }
        if (buflen > 0) {
            char *omailcmd = (char *) alloc(buflen);

            if (nhfp->structlevel) {
                Mread(nhfp->fd, omailcmd, buflen);
            }
            new_omailcmd(otmp, omailcmd);
            free((genericptr_t) omailcmd);
        }

        /* omid - monster id number, connecting corpse to ghost */
        newomid(otmp); /* superfluous; we're already allocated otmp->oextra */
        if (nhfp->structlevel) {
            Mread(nhfp->fd, &OMID(otmp), sizeof OMID(otmp));
        }
    }
}

staticfn struct obj *
restobjchn(NHFILE *nhfp, boolean frozen)
{
    struct obj *otmp, *otmp2 = 0;
    struct obj *first = (struct obj *) 0;
    int buflen = 0;
    boolean ghostly = (nhfp->ftype == NHF_BONESFILE);

    while (1) {
        if (nhfp->structlevel) {
            Mread(nhfp->fd, &buflen, sizeof buflen);
        }
        if (buflen == -1)
            break;

        otmp = newobj();
        assert(otmp != 0);
        restobj(nhfp, otmp);
        if (!first)
            first = otmp;
        else
            otmp2->nobj = otmp;

        if (ghostly) {
            unsigned nid = next_ident();

            add_id_mapping(otmp->o_id, nid);
            otmp->o_id = nid;
        }
        if (ghostly && otmp->otyp == SLIME_MOLD)
            ghostfruit(otmp);
        /* Ghost levels get object age shifted from old player's clock
         * to new player's clock.  Assumption: new player arrived
         * immediately after old player died.
         */
        if (ghostly && !frozen && !age_is_relative(otmp))
            otmp->age = svm.moves - svo.omoves + otmp->age;

        /* get contents of a container or statue */
        if (Has_contents(otmp)) {
            struct obj *otmp3;

            otmp->cobj = restobjchn(nhfp, Is_IceBox(otmp));
            /* restore container back pointers */
            for (otmp3 = otmp->cobj; otmp3; otmp3 = otmp3->nobj)
                otmp3->ocontainer = otmp;
        }
        if (otmp->bypass)
            otmp->bypass = 0;
        if (!ghostly) {
            /* fix the pointers */
            if (otmp->o_id == svc.context.victual.o_id)
                svc.context.victual.piece = otmp;
            if (otmp->o_id == svc.context.tin.o_id)
                svc.context.tin.tin = otmp;
            if (otmp->o_id == svc.context.spbook.o_id)
                svc.context.spbook.book = otmp;
        }
        otmp2 = otmp;
    }
    if (first && otmp2->nobj) {
        impossible("Restobjchn: error reading objchn.");
        otmp2->nobj = 0;
    }

    return first;
}

/* restore one monster */
staticfn void
restmon(NHFILE *nhfp, struct monst *mtmp)
{
    int buflen = 0, mc = 0;

    if (nhfp->structlevel) {
        Mread(nhfp->fd, mtmp, sizeof *mtmp);
    }

    /* next monster pointer is invalid */
    mtmp->nmon = (struct monst *) 0;
    /* non-null mextra needs to be reconstructed */
    if (mtmp->mextra) {
        mtmp->mextra = newmextra();

        /* mgivenname - monster's name */
        if (nhfp->structlevel) {
            Mread(nhfp->fd, &buflen, sizeof buflen);
        }
        if (buflen > 0) { /* includes terminating '\0' */
            new_mgivenname(mtmp, buflen);
            if (nhfp->structlevel) {
                Mread(nhfp->fd, MGIVENNAME(mtmp), buflen);
            }
        }
        /* egd - vault guard */
        if (nhfp->structlevel) {
            Mread(nhfp->fd, &buflen, sizeof buflen);
        }
        if (buflen > 0) {
            newegd(mtmp);
            if (nhfp->structlevel) {
                Mread(nhfp->fd, EGD(mtmp), sizeof(struct egd));
            }
        }
        /* epri - temple priest */
        if (nhfp->structlevel) {
            Mread(nhfp->fd, &buflen, sizeof buflen);
        }
        if (buflen > 0) {
            newepri(mtmp);
            if (nhfp->structlevel) {
                Mread(nhfp->fd, EPRI(mtmp), sizeof(struct epri));
            }
        }
        /* eshk - shopkeeper */
        if (nhfp->structlevel) {
            Mread(nhfp->fd, &buflen, sizeof buflen);
        }
        if (buflen > 0) {
            neweshk(mtmp);
            if (nhfp->structlevel) {
                Mread(nhfp->fd, ESHK(mtmp), sizeof(struct eshk));
            }
        }
        /* emin - minion */
        if (nhfp->structlevel) {
            Mread(nhfp->fd, &buflen, sizeof buflen);
        }
        if (buflen > 0) {
            newemin(mtmp);
            if (nhfp->structlevel) {
                Mread(nhfp->fd, EMIN(mtmp), sizeof(struct emin));
            }
        }
        /* edog - pet */
        if (nhfp->structlevel) {
            Mread(nhfp->fd, &buflen, sizeof buflen);
        }
        if (buflen > 0) {
            newedog(mtmp);
            if (nhfp->structlevel) {
                Mread(nhfp->fd, EDOG(mtmp), sizeof(struct edog));
            }
            /* sanity check to prevent rn2(0) */
           if (EDOG(mtmp)->apport <= 0) {
               EDOG(mtmp)->apport = 1;
           }
        }
        /* ebones */
        if (nhfp->structlevel) {
            Mread(nhfp->fd, &buflen, sizeof buflen);
        }
        if (buflen > 0) {
            newebones(mtmp);
            if (nhfp->structlevel) {
                Mread(nhfp->fd, EBONES(mtmp), sizeof(struct ebones));
            }
        }
        /* mcorpsenm - obj->corpsenm for mimic posing as corpse or
           statue (inline int rather than pointer to something) */
        if (nhfp->structlevel) {
            Mread(nhfp->fd, &mc, sizeof mc);
        }
        MCORPSENM(mtmp) = mc;
    } /* mextra */
}

staticfn struct monst *
restmonchn(NHFILE *nhfp)
{
    struct monst *mtmp, *mtmp2 = 0;
    struct monst *first = (struct monst *) 0;
    int offset, buflen = 0;
    boolean ghostly = (nhfp->ftype == NHF_BONESFILE);

    while (1) {
        if (nhfp->structlevel) {
            Mread(nhfp->fd, &buflen, sizeof buflen);
        }
        if (buflen == -1)
            break;

        mtmp = newmonst();
        assert(mtmp != 0);
        restmon(nhfp, mtmp);
        if (!first)
            first = mtmp;
        else
            mtmp2->nmon = mtmp;

        if (ghostly) {
            unsigned nid = next_ident();

            add_id_mapping(mtmp->m_id, nid);
            mtmp->m_id = nid;
        }
        offset = mtmp->mnum;
        mtmp->data = &mons[offset];
        if (ghostly) {
            int mndx = (mtmp->cham == NON_PM) ? monsndx(mtmp->data)
                                              : mtmp->cham;

            if (propagate(mndx, TRUE, ghostly) == 0) {
                /* cookie to trigger purge in getbones() */
                mtmp->mhpmax = DEFUNCT_MONSTER;
            }
        }
        if (mtmp->minvent) {
            struct obj *obj;
            mtmp->minvent = restobjchn(nhfp, FALSE);
            /* restore monster back pointer */
            for (obj = mtmp->minvent; obj; obj = obj->nobj)
                obj->ocarry = mtmp;
        }
        if (mtmp->mw) {
            struct obj *obj;

            for (obj = mtmp->minvent; obj; obj = obj->nobj)
                if (obj->owornmask & W_WEP)
                    break;
            if (obj)
                mtmp->mw = obj;
            else {
                MON_NOWEP(mtmp);
                impossible("bad monster weapon restore");
            }
        }

        if (mtmp->isshk)
            restshk(mtmp, ghostly);
        if (mtmp->ispriest)
            restpriest(mtmp, ghostly);

        if (!ghostly) {
            if (mtmp->m_id == svc.context.polearm.m_id)
                svc.context.polearm.hitmon = mtmp;
        }
        mtmp2 = mtmp;
    }
    if (first && mtmp2->nmon) {
        impossible("Restmonchn: error reading monchn.");
        mtmp2->nmon = 0;
    }
    return first;
}

staticfn struct fruit *
loadfruitchn(NHFILE *nhfp)
{
    struct fruit *flist, *fnext;

    flist = 0;
    for (;;) {
        fnext = newfruit();
        if (nhfp->structlevel) {
            Mread(nhfp->fd, fnext, sizeof *fnext);
        }
        if (fnext->fid != 0) {
            fnext->nextf = flist;
            flist = fnext;
        } else
            break;
    }
    dealloc_fruit(fnext);
    return flist;
}

staticfn void
freefruitchn(struct fruit *flist)
{
    struct fruit *fnext;

    while (flist) {
        fnext = flist->nextf;
        dealloc_fruit(flist);
        flist = fnext;
    }
}

staticfn void
ghostfruit(struct obj *otmp)
{
    struct fruit *oldf;

    for (oldf = go.oldfruit; oldf; oldf = oldf->nextf)
        if (oldf->fid == otmp->spe)
            break;

    if (!oldf)
        impossible("no old fruit?");
    else
        otmp->spe = fruitadd(oldf->fname, (struct fruit *) 0);
}

#ifdef SYSCF
#define SYSOPT_CHECK_SAVE_UID sysopt.check_save_uid
#else
#define SYSOPT_CHECK_SAVE_UID TRUE
#endif

staticfn boolean
restgamestate(NHFILE *nhfp)
{
    int i;
    struct flag newgameflags;
    struct context_info newgamecontext; /* all 0, but has some pointers */
    struct obj *otmp;
    struct obj *bc_obj;
    char timebuf[15];
    unsigned long uid = 0;
    boolean defer_perm_invent, restoring_special;

    if (nhfp->structlevel) {
        Mread(nhfp->fd, &uid, sizeof uid);
    }

    if (SYSOPT_CHECK_SAVE_UID
        && uid != (unsigned long) getuid()) { /* strange ... */
        /* for wizard mode, issue a reminder; for others, treat it
           as an attempt to cheat and refuse to restore this file */
        pline("Saved game was not yours.");
        if (!wizard)
            return FALSE;
    }

    newgamecontext = svc.context; /* copy statically init'd context */
    if (nhfp->structlevel) {
        Mread(nhfp->fd, &svc.context, sizeof svc.context);
    }
    svc.context.warntype.species = (ismnum(svc.context.warntype.speciesidx))
                                  ? &mons[svc.context.warntype.speciesidx]
                                  : (struct permonst *) 0;
    /* context.victual.piece, .tin.tin, .spellbook.book, and .polearm.hitmon
       are pointers which get set to Null during save and will be recovered
       via corresponding o_id or m_id while objs or mons are being restored */

    /* we want to be able to revert to command line/environment/config
       file option values instead of keeping old save file option values
       if partial restore fails and we resort to starting a new game */
    newgameflags = flags;
    if (nhfp->structlevel) {
        Mread(nhfp->fd, &flags, sizeof flags);
    }

    /* avoid keeping permanent inventory window up to date during restore
       (setworn() calls update_inventory); attempting to include the cost
       of unpaid items before shopkeeper's bill is available is a no-no;
       named fruit names aren't accessible yet either
       [3.6.2: moved perm_invent from flags to iflags to keep it out of
       save files; retaining the override here is simpler than trying
       to figure out where it really belongs now] */
    defer_perm_invent = iflags.perm_invent;
    iflags.perm_invent = FALSE;
    /* wizard and discover are actually flags.debug and flags.explore;
       player might be overriding the save file values for them;
       in the discover case, we don't want to set that for a normal
       game until after the save file has been removed */
    iflags.deferred_X = (newgameflags.explore && !discover);
    restoring_special = (wizard || discover);
    if (newgameflags.debug) {
        /* authorized by startup code; wizard mode exists and is allowed */
        wizard = TRUE, discover = iflags.deferred_X = FALSE;
    } else if (restoring_special) {
        /* specified by save file; check authorization now. */
        set_playmode();
    }
    role_init(); /* Reset the initial role, race, gender, and alignment */
#ifdef AMII_GRAPHICS
    amii_setpens(amii_numcolors); /* use colors from save file */
#endif
    if (nhfp->structlevel) {
        Mread(nhfp->fd, &u, sizeof u);
    }
    gy.youmonst.cham = u.mcham;

    if (restoring_special && iflags.explore_error_flag) {
        /* savefile has wizard or explore mode, but player is no longer
           authorized to access either; can't downgrade mode any further, so
           fail restoration. */
        u.uhp = 0;
    }

    if (nhfp->structlevel) {
        Mread(nhfp->fd, timebuf, 14);
    }
    timebuf[14] = '\0';
    ubirthday = time_from_yyyymmddhhmmss(timebuf);
    if (nhfp->structlevel) {
        Mread(nhfp->fd, &urealtime.realtime, sizeof urealtime.realtime);
    }
    if (nhfp->structlevel) {
        Mread(nhfp->fd, timebuf, 14);
    }
    timebuf[14] = '\0';
    urealtime.start_timing = time_from_yyyymmddhhmmss(timebuf);

    /* current time is the time to use for next urealtime.realtime update */
    urealtime.start_timing = getnow();

    set_uasmon();
#ifdef CLIPPING
    cliparound(u.ux, u.uy);
#endif
    if (u.uhp <= 0 && (!Upolyd || u.mh <= 0)) {
        u.ux = u.uy = 0; /* affects pline() [hence You()] */
        You("were not healthy enough to survive restoration.");
        /* wiz1_level.dlevel is used by mklev.c to see if lots of stuff is
         * uninitialized, so we only have to set it and not the other stuff.
         */
        wiz1_level.dlevel = 0;
        u.uz.dnum = 0;
        u.uz.dlevel = 1;
        /* revert to pre-restore option settings */
        iflags.deferred_X = FALSE;
        iflags.perm_invent = defer_perm_invent;
        flags = newgameflags;
        svc.context = newgamecontext;
        gy.youmonst = cg.zeromonst;
        return FALSE;
    }
    /* in case hangup save occurred in midst of level change */
    assign_level(&u.uz0, &u.uz);

    /* this stuff comes after potential aborted restore attempts */
    restore_killers(nhfp);
    restore_timers(nhfp, RANGE_GLOBAL, 0L);
    restore_light_sources(nhfp);

    gi.invent = restobjchn(nhfp, FALSE);

    /* restore dangling (not on floor or in inventory) ball and/or chain */
    bc_obj = restobjchn(nhfp, FALSE);
    while (bc_obj) {
        struct obj *nobj = bc_obj->nobj;

        bc_obj->nobj = (struct obj *) 0;
        if (bc_obj->owornmask)
            setworn(bc_obj, bc_obj->owornmask);
        bc_obj = nobj;
    }
    gm.migrating_objs = restobjchn(nhfp, FALSE);
    gm.migrating_mons = restmonchn(nhfp);

    for (i = 0; i < NUMMONS; ++i) {
        if (nhfp->structlevel) {
            Mread(nhfp->fd, &svm.mvitals[i], sizeof (struct mvitals));
        }
    }
    /*
     * There are some things after this that can have unintended display
     * side-effects too early in the game.
     * Disable see_monsters() here, re-enable it at the top of moveloop()
     */
    gd.defer_see_monsters = TRUE;

    /* this comes after inventory has been loaded */
    for (otmp = gi.invent; otmp; otmp = otmp->nobj)
        if (otmp->owornmask)
            setworn(otmp, otmp->owornmask);

    /* reset weapon so that player will get a reminder about "bashing"
       during next fight when bare-handed or wielding an unconventional
       item; for pick-axe, we aren't able to distinguish between having
       applied or wielded it, so be conservative and assume the former */
    otmp = uwep;   /* `uwep' usually init'd by setworn() in loop above */
    uwep = 0;      /* clear it and have setuwep() reinit */
    setuwep(otmp); /* (don't need any null check here) */
    if (!uwep || uwep->otyp == PICK_AXE || uwep->otyp == GRAPPLING_HOOK)
        gu.unweapon = TRUE;

    restore_dungeon(nhfp);
    restlevchn(nhfp);
    if (nhfp->structlevel) {
        Mread(nhfp->fd, &svm.moves, sizeof svm.moves);
    }
    /* hero_seq isn't saved and restored because it can be recalculated */
    gh.hero_seq = svm.moves << 3; /* normally handled in moveloop() */
    if (nhfp->structlevel) {
        Mread(nhfp->fd, &svq.quest_status, sizeof svq.quest_status);
    }
    for (i = 0; i < (MAXSPELL + 1); ++i) {
        if (nhfp->structlevel) {
            Mread(nhfp->fd, &svs.spl_book[i],
                  sizeof (struct spell));
        }
    }
    restore_artifacts(nhfp);
    restore_oracles(nhfp);
    if (nhfp->structlevel) {
        Mread(nhfp->fd, svp.pl_character, sizeof svp.pl_character);
        Mread(nhfp->fd, svp.pl_fruit, sizeof svp.pl_fruit);
    }
    freefruitchn(gf.ffruit); /* clean up fruit(s) made by initoptions() */
    gf.ffruit = loadfruitchn(nhfp);

    restnames(nhfp);
    restore_msghistory(nhfp);
    restore_gamelog(nhfp);
    restore_luadata(nhfp);
    /* must come after all mons & objs are restored */
    relink_timers(FALSE);
    relink_light_sources(FALSE);
    adj_erinys(u.ualign.abuse);
#ifdef WHEREIS_FILE
    touch_whereis();
#endif
    /* inventory display is now viable */
    iflags.perm_invent = defer_perm_invent;
    return TRUE;
}

/* update game state pointers to those valid for the current level (so we
   don't dereference a wild u.ustuck when saving game state, for instance) */
staticfn void
restlevelstate(void)
{
    /*
     * Note: restoring steed and engulfer/holder/holdee is now handled
     * in getlev() and there's nothing left for restlevelstate() to do.
     */
    return;
}

/* after getlev(), put current level into a level/lock file;
   essential when splitting a save file into individual level files */
/*ARGSUSED*/
staticfn int
restlevelfile(xint8 ltmp)
{
    char whynot[BUFSZ];
    NHFILE *nhfp = (NHFILE *) 0;

    nhfp = create_levelfile(ltmp, whynot);
    if (!nhfp) {
        /* failed to create a new file; don't attempt to make a panic save */
        program_state.something_worth_saving = 0;
        panic("restlevelfile: %s", whynot);
    }
    bufon(nhfp->fd);
    nhfp->mode = WRITING | FREEING;
    savelev(nhfp, ltmp);
    close_nhfile(nhfp);
    return 2;
}

int
dorecover(NHFILE *nhfp)
{
    xint8 ltmp = 0;
    int rtmp;

    /* suppress map display if some part of the code tries to update that */
    program_state.restoring = REST_GSTATE;

    get_plname_from_file(nhfp, svp.plname, TRUE);
    getlev(nhfp, 0, (xint8) 0);
    if (!restgamestate(nhfp)) {
        NHFILE tnhfp;

        display_nhwindow(WIN_MESSAGE, TRUE);
        zero_nhfile(&tnhfp);
        tnhfp.mode = FREEING;
        tnhfp.fd = -1;
        savelev(&tnhfp, 0); /* discard current level */
        /* no need for close_nhfile(&tnhfp), which
           is not really affiliated with an open file */
        close_nhfile(nhfp);
        (void) delete_savefile();
        u.usteed_mid = u.ustuck_mid = 0;
        program_state.restoring = 0;
        return 0;
    }
    /* after restgamestate() -> restnames() so that 'bases[]' is populated */
    init_oclass_probs(); /* recompute go.oclass_prob_totals[] */

    restlevelstate();
#ifdef INSURANCE
    savestateinlock();
#endif
    rtmp = restlevelfile(ledger_no(&u.uz));
    if (rtmp < 2)
        return rtmp; /* dorecover called recursively */

    program_state.restoring = REST_LEVELS;

    /* these pointers won't be valid while we're processing the
     * other levels, but they'll be reset again by restlevelstate()
     * afterwards, and in the meantime at least u.usteed may mislead
     * place_monster() on other levels
     */
    u.ustuck = (struct monst *) 0;
    u.usteed = (struct monst *) 0;

#ifdef MICRO
#ifdef AMII_GRAPHICS
    {
        extern struct window_procs amii_procs;
        if (WINDOWPORT(amii)) {
            extern winid WIN_BASE;
            clear_nhwindow(WIN_BASE); /* hack until there's a hook for this */
        }
    }
#else
        clear_nhwindow(WIN_MAP);
#endif
    clear_nhwindow(WIN_MESSAGE);
    You("return to level %d in %s%s.", depth(&u.uz),
        svd.dungeons[u.uz.dnum].dname,
        flags.debug ? " while in debug mode"
                    : flags.explore ? " while in explore mode" : "");
    curs(WIN_MAP, 1, 1);
    dotcnt = 0;
    dotrow = 2;
    if (!WINDOWPORT(X11))
        putstr(WIN_MAP, 0, "Restoring:");
#endif
    restoreinfo.mread_flags = 1; /* return despite error */
    while (1) {
        if (nhfp->structlevel) {
            Mread(nhfp->fd, &ltmp, sizeof ltmp);
        }
        if (restoreinfo.mread_flags == -1)
            break;
        getlev(nhfp, 0, ltmp);
#ifdef MICRO
        curs(WIN_MAP, 1 + dotcnt++, dotrow);
        if (dotcnt >= (COLNO - 1)) {
            dotrow++;
            dotcnt = 0;
        }
        if (!WINDOWPORT(X11)) {
            putstr(WIN_MAP, 0, ".");
        }
        mark_synch();
#endif
        rtmp = restlevelfile(ltmp);
        if (rtmp < 2)
            return rtmp; /* dorecover called recursively */
    }
    restoreinfo.mread_flags = 0;

    rewind_nhfile(nhfp);        /* return to beginning of file */
    (void) validate(nhfp, (char *) 0, FALSE);
    get_plname_from_file(nhfp, svp.plname, TRUE);

    /* not 0 nor REST_GSTATE nor REST_LEVELS */
    program_state.restoring = REST_CURRENT_LEVEL;

    getlev(nhfp, 0, (xint8) 0);
    close_nhfile(nhfp);
    restlevelstate();
    program_state.something_worth_saving = 1; /* useful data now exists */

    if (!wizard && !discover)
        (void) delete_savefile();
    if (Is_rogue_level(&u.uz))
        assign_graphics(ROGUESET);
    reset_glyphmap(gm_levelchange);
    max_rank_sz(); /* to recompute gm.mrank_sz (botl.c) */

    if ((uball && !uchain) || (uchain && !uball)) {
        impossible("restgamestate: lost ball & chain");
        /* poor man's unpunish() */
        setworn((struct obj *) 0, W_CHAIN);
        setworn((struct obj *) 0, W_BALL);
    }

    /* in_use processing must be after:
     *    + The inventory has been read so that freeinv() works.
     *    + The current level has been restored so billing information
     *      is available.
     */
    inven_inuse(FALSE);

    /* Set up the vision internals, after levl[] data is loaded
       but before docrt(). */
    reglyph_darkroom();
    vision_reset();
    gv.vision_full_recalc = 1; /* recompute vision (not saved) */

    run_timers(); /* expire all timers that have gone off while away */
    program_state.restoring = 0; /* affects bot() so clear before docrt() */

    if (ge.early_raw_messages && !program_state.beyond_savefile_load) {
        /*
         * We're about to obliterate some potentially important
         * startup messages, so give the player a chance to see them.
         */
        ge.early_raw_messages = 0;
        wait_synch();
    }
    u.usteed_mid = u.ustuck_mid = 0;
    program_state.beyond_savefile_load = 1;

    docrt();
    clear_nhwindow(WIN_MESSAGE);

    /* Success! */
    welcome(FALSE);
    check_special_room(FALSE);
    return 1;
}

staticfn void
rest_stairs(NHFILE *nhfp)
{
    int buflen = 0;
    stairway stway = UNDEFINED_VALUES;
    stairway *newst;

    stairway_free_all();
    while (1) {
        if (nhfp->structlevel) {
            Mread(nhfp->fd, &buflen, sizeof buflen);
        }

        if (buflen == -1)
            break;

        if (nhfp->structlevel) {
            Mread(nhfp->fd, &stway, sizeof stway);
        }
        if (program_state.restoring != REST_GSTATE
            && stway.tolev.dnum == u.uz.dnum) {
            /* stairway dlevel is relative, make it absolute */
            stway.tolev.dlevel += u.uz.dlevel;
        }
        stairway_add(stway.sx, stway.sy, stway.up, stway.isladder,
                     &(stway.tolev));
        newst = stairway_at(stway.sx, stway.sy);
        if (newst)
            newst->u_traversed = stway.u_traversed;
    }
}

void
restcemetery(NHFILE *nhfp, struct cemetery **cemeteryaddr)
{
    struct cemetery *bonesinfo, **bonesaddr;
    int cflag = 0;

    if (nhfp->structlevel) {
        Mread(nhfp->fd, &cflag, sizeof cflag);
    }
    if (cflag == 0) {
        bonesaddr = cemeteryaddr;
        do {
            bonesinfo = (struct cemetery *) alloc(sizeof *bonesinfo);
            if (nhfp->structlevel) {
                Mread(nhfp->fd, bonesinfo, sizeof *bonesinfo);
            }
            *bonesaddr = bonesinfo;
            bonesaddr = &(*bonesaddr)->next;
        } while (*bonesaddr);
    } else {
        *cemeteryaddr = 0;
    }
}

/*ARGSUSED*/
staticfn void
rest_levl(NHFILE *nhfp)
{
    int c, r;

    for (c = 0; c < COLNO; ++c) {
        for (r = 0; r < ROWNO; ++r) {
            if (nhfp->structlevel) {
                Mread(nhfp->fd, &levl[c][r], sizeof (struct rm));
            }
        }
    }
}

void
trickery(char *reason)
{
    pline("Strange, this map is not as I remember it.");
    pline("Somebody is trying some trickery here...");
    pline("This game is void.");
    Strcpy(svk.killer.name, reason ? reason : "");
    done(TRICKED);
}

void
getlev(NHFILE *nhfp, int pid, xint8 lev)
{
    struct trap *trap;
    struct monst *mtmp;
    branch *br;
    int x, y;
    long elapsed = 0L;
    int hpid = 0;
    xint8 dlvl = 0;
    int i, c, r;
    boolean ghostly = (nhfp->ftype == NHF_BONESFILE);
    coord *tmpc = 0;
#ifdef TOS
    short tlev;
#endif

    program_state.in_getlev = TRUE;

    if (ghostly)
        clear_id_mapping();

#if defined(MSDOS) || defined(OS2)
    if (nhfp->structlevel)
        setmode(nhfp->fd, O_BINARY);
#endif
    /* Load the old fruit info.  We have to do it first, so the
     * information is available when restoring the objects.
     */
    if (ghostly)
        go.oldfruit = loadfruitchn(nhfp);

    /* First some sanity checks */
    if (nhfp->structlevel) {
        Mread(nhfp->fd, &hpid, sizeof hpid);
    }

    if (nhfp->structlevel) {
        Mread(nhfp->fd, &dlvl, sizeof dlvl);
    }
    if ((pid && pid != hpid) || (lev && dlvl != lev)) {
        char trickbuf[BUFSZ];

        if (pid && pid != hpid)
            Sprintf(trickbuf, "PID (%d) doesn't match saved PID (%d)!", hpid,
                    pid);
        else
            Sprintf(trickbuf, "This is level %d, not %d!", dlvl, lev);
        if (wizard)
            pline1(trickbuf);
        trickery(trickbuf);
    }
    restcemetery(nhfp, &svl.level.bonesinfo);
    rest_levl(nhfp);
    for (c = 0; c < COLNO; ++c)
        for (r = 0; r < ROWNO; ++r) {
            if (nhfp->structlevel) {
                Mread(nhfp->fd, &svl.lastseentyp[c][r], sizeof (schar));
            }
        }
    Mread(nhfp->fd, &svo.omoves, sizeof svo.omoves);
    elapsed = svm.moves - svo.omoves;

    rest_stairs(nhfp);
    if (nhfp->structlevel) {
        Mread(nhfp->fd, &svu.updest, sizeof svu.updest);
        Mread(nhfp->fd, &svd.dndest, sizeof svd.dndest);
        Mread(nhfp->fd, &svl.level.flags, sizeof svl.level.flags);
    }
    if (svd.doors) {
        free(svd.doors);
        svd.doors = 0;
    }
    if (nhfp->structlevel) {
        Mread(nhfp->fd, &svd.doors_alloc, sizeof svd.doors_alloc);
    }
    if (svd.doors_alloc) { /* avoid pointless alloc(0) */
        svd.doors = (coord *) alloc(svd.doors_alloc * sizeof(coord));
        tmpc = svd.doors;
        for (i = 0; i < svd.doors_alloc; ++i) {
            if (nhfp->structlevel) {
                Mread(nhfp->fd, tmpc, sizeof (coord));
            }
            tmpc++;
        }
    }
    rest_rooms(nhfp); /* No joke :-) */
    if (svn.nroom) {
        gd.doorindex = svr.rooms[svn.nroom - 1].fdoor
                       + svr.rooms[svn.nroom - 1].doorct;
    } else {
        gd.doorindex = 0;
    }

    restore_timers(nhfp, RANGE_LEVEL, elapsed);
    restore_light_sources(nhfp);
    fmon = restmonchn(nhfp);
    rest_worm(nhfp);    /* restore worm information */

    gf.ftrap = 0;
    for (;;) {
        trap = newtrap();
        if (nhfp->structlevel) {
            Mread(nhfp->fd, trap, sizeof *trap);
        }
        if (trap->tx != 0) {
            if (program_state.restoring != REST_GSTATE
                && trap->dst.dnum == u.uz.dnum) {
                /* convert relative destination to absolute */
                trap->dst.dlevel += u.uz.dlevel;
            }
            trap->ntrap = gf.ftrap;
            gf.ftrap = trap;
        } else
            break;
    }
    dealloc_trap(trap);

    fobj = restobjchn(nhfp, FALSE);
    find_lev_obj();
    /* restobjchn()'s `frozen' argument probably ought to be a callback
       routine so that we can check for objects being buried under ice */
    svl.level.buriedobjlist = restobjchn(nhfp, FALSE);
    gb.billobjs = restobjchn(nhfp, FALSE);
    rest_engravings(nhfp);

    /* reset level.monsters for new level */
    for (x = 0; x < COLNO; x++)
        for (y = 0; y < ROWNO; y++)
            svl.level.monsters[x][y] = (struct monst *) 0;
    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (mtmp->isshk)
            set_residency(mtmp, FALSE);
        if (mtmp->m_id == u.usteed_mid) {
            /* steed is kept on fmon list but off the map */
            u.usteed = mtmp;
            u.usteed_mid = 0;
        } else {
            if (mtmp->m_id == u.ustuck_mid) {
                set_ustuck(mtmp);
                u.ustuck_mid = 0;
            }
            place_monster(mtmp, mtmp->mx, mtmp->my);
            if (mtmp->wormno)
                place_wsegs(mtmp, NULL);
            if (hides_under(mtmp->data) && mtmp->mundetected)
                (void) hideunder(mtmp);
        }

        /* regenerate monsters while on another level */
        if (!u.uz.dlevel || program_state.restoring == REST_LEVELS)
            continue;
        if (ghostly) {
            /* reset peaceful/malign relative to new character;
               shopkeepers will reset based on name */
            if (!mtmp->isshk)
                mtmp->mpeaceful = (is_unicorn(mtmp->data)
                                   && (sgn(u.ualign.type)
                                       == sgn(mtmp->data->maligntyp))) ? 1
                                  : peace_minded(mtmp->data);
            set_malign(mtmp);
        } else if (elapsed > 0L) {
            mon_catchup_elapsed_time(mtmp, elapsed);
        }
        /* update shape-changers in case protection against
           them is different now than when the level was saved */
        restore_cham(mtmp);
        /* give hiders a chance to hide before their next move */
        if (ghostly || (elapsed > 00 && elapsed > (long) rnd(10)))
            hide_monst(mtmp);
    }
    restdamage(nhfp);
    rest_regions(nhfp);
    rest_bubbles(nhfp); /* for water and air; empty marker on other levels */
    load_exclusions(nhfp);
    rest_track(nhfp);

    if (ghostly) {
        stairway *stway = gs.stairs;
        while (stway) {
            if (!stway->isladder && !stway->up
                && stway->tolev.dnum == u.uz.dnum)
                break;
            stway = stway->next;
        }

        /* Now get rid of all the temp fruits... */
        freefruitchn(go.oldfruit), go.oldfruit = 0;

        if (lev > ledger_no(&medusa_level)
            && lev < ledger_no(&stronghold_level) && !stway) {
            coord cc;
            d_level dest;

            dest.dnum = u.uz.dnum;
            dest.dlevel = u.uz.dlevel + 1;

            mazexy(&cc);
            stairway_add(cc.x, cc.y, FALSE, FALSE, &dest);
            levl[cc.x][cc.y].typ = STAIRS;
        }

        br = Is_branchlev(&u.uz);
        if (br && u.uz.dlevel == 1) {
            d_level ltmp;

            if (on_level(&u.uz, &br->end1))
                assign_level(&ltmp, &br->end2);
            else
                assign_level(&ltmp, &br->end1);

            switch (br->type) {
            case BR_STAIR:
            case BR_NO_END1:
            case BR_NO_END2:
                stway = gs.stairs;
                while (stway) {
                    if (stway->tolev.dnum != u.uz.dnum)
                        break;
                    stway = stway->next;
                }
                if (stway)
                    assign_level(&(stway->tolev), &ltmp);
                break;
            case BR_PORTAL: /* max of 1 portal per level */
                for (trap = gf.ftrap; trap; trap = trap->ntrap)
                    if (trap->ttyp == MAGIC_PORTAL)
                        break;
                if (!trap)
                    panic("getlev: need portal but none found");
                assign_level(&trap->dst, &ltmp);
                break;
            }
        } else if (!br) {
            struct trap *ttmp = 0;

            /* Remove any dangling portals. */
            for (trap = gf.ftrap; trap; trap = ttmp) {
                ttmp = trap->ntrap;
                if (trap->ttyp == MAGIC_PORTAL)
                    deltrap(trap);
            }
        }
    }

    /* must come after all mons & objs are restored */
    relink_timers(ghostly);
    relink_light_sources(ghostly);
    reset_oattached_mids(ghostly);

    if (ghostly)
        clear_id_mapping();
    program_state.in_getlev = FALSE;
}

/* "name-role-race-gend-algn" occurs very early in a save file; sometimes we
   want the whole thing, other times just "name" (for svp.plname[]) */
void
get_plname_from_file(
    NHFILE *nhfp,
    char *outbuf, /* size must be at least [PL_NSIZ_PLUS] even if name_only */
    boolean name_only) /* True: just name; False: name-role-race-gend-algn */
{
    char plbuf[PL_NSIZ_PLUS];
    int pltmpsiz = 0;

    plbuf[0] = '\0';
    if (nhfp->structlevel) {
        (void) read(nhfp->fd, (genericptr_t) &pltmpsiz, sizeof pltmpsiz);
        /* pltmpsiz should now be PL_NSIZ_PLUS */
        (void) read(nhfp->fd, (genericptr_t) plbuf, pltmpsiz);
        /* plbuf[PL_NSIZ_PLUS-2] should be '\0';
           plbuf[PL_NSIZ_PLUS-1] should be '-' or 'X' or 'D' */
    }
    /* "-race-role-gend-algn" is already present except that it has been
       hidden by replacing the initial dash with NUL; if we want that
       information, replace the NUL with a dash */
    if (!name_only)
        *eos(plbuf) = '-';
    /* not simple strcpy(); playmode is in the last slot and could (probably
       will) be preceded by NULs */
    (void) memcpy((genericptr_t) outbuf, (genericptr_t) plbuf, PL_NSIZ_PLUS);
    return;
}

/* restore Plane of Water's air bubbles and Plane of Air's clouds */
staticfn void
rest_bubbles(NHFILE *nhfp)
{
    xint8 bbubbly;

    /* whether or not the Plane of Water's air bubbles or Plane of Air's
       clouds are present is recorded during save so that we don't have to
       know what level is being restored */
    bbubbly = 0;
    if (nhfp->structlevel) {
        Mread(nhfp->fd, &bbubbly, sizeof bbubbly);
    }

    if (bbubbly)
        restore_waterlevel(nhfp);
}

staticfn void
restore_gamelog(NHFILE *nhfp)
{
    int slen = 0;
    char msg[BUFSZ*2];
    struct gamelog_line tmp = { 0 };

    while (1) {
        if (nhfp->structlevel) {
            Mread(nhfp->fd, &slen, sizeof slen);
        }
        if (slen == -1)
            break;
        if (slen > ((BUFSZ*2) - 1))
            panic("restore_gamelog: msg too big (%d)", slen);
        if (nhfp->structlevel) {
            Mread(nhfp->fd, msg, slen);
        }
        msg[slen] = '\0';
        if (nhfp->structlevel) {
            Mread(nhfp->fd, &tmp, sizeof tmp);
        }
        gamelog_add(tmp.flags, tmp.turn, msg);
    }
}

staticfn void
restore_msghistory(NHFILE *nhfp)
{
    int msgsize = 0, msgcount = 0;
    char msg[BUFSZ];

    while (1) {
        if (nhfp->structlevel) {
            Mread(nhfp->fd, &msgsize, sizeof msgsize);
        }
        if (msgsize == -1)
            break;
        if (msgsize > BUFSZ - 1)
            panic("restore_msghistory: msg too big (%d)", msgsize);
        if (nhfp->structlevel) {
            Mread(nhfp->fd, msg, msgsize);
        }
        msg[msgsize] = '\0';
        putmsghistory(msg, TRUE);
        ++msgcount;
    }
    if (msgcount)
        putmsghistory((char *) 0, TRUE);
    debugpline1("Read %d messages from savefile.", msgcount);
}

/* Clear all structures for object and monster ID mapping. */
staticfn void
clear_id_mapping(void)
{
    struct bucket *curr;

    while ((curr = gi.id_map) != 0) {
        gi.id_map = curr->next;
        free((genericptr_t) curr);
    }
    gn.n_ids_mapped = 0;
}

/* Add a mapping to the ID map. */
staticfn void
add_id_mapping(unsigned int gid, unsigned int nid)
{
    int idx;

    idx = gn.n_ids_mapped % N_PER_BUCKET;
    /* idx is zero on first time through, as well as when a new bucket is */
    /* needed */
    if (idx == 0) {
        struct bucket *gnu = (struct bucket *) alloc(sizeof(struct bucket));
        gnu->next = gi.id_map;
        gi.id_map = gnu;
    }

    gi.id_map->map[idx].gid = gid;
    gi.id_map->map[idx].nid = nid;
    gn.n_ids_mapped++;
}

/*
 * Global routine to look up a mapping.  If found, return TRUE and fill
 * in the new ID value.  Otherwise, return false and return -1 in the new
 * ID.
 */
boolean
lookup_id_mapping(unsigned int gid, unsigned int *nidp)
{
    int i;
    struct bucket *curr;

    if (gn.n_ids_mapped)
        for (curr = gi.id_map; curr; curr = curr->next) {
            /* first bucket might not be totally full */
            if (curr == gi.id_map) {
                i = gn.n_ids_mapped % N_PER_BUCKET;
                if (i == 0)
                    i = N_PER_BUCKET;
            } else
                i = N_PER_BUCKET;

            while (--i >= 0)
                if (gid == curr->map[i].gid) {
                    *nidp = curr->map[i].nid;
                    return TRUE;
                }
        }

    return FALSE;
}

staticfn void
reset_oattached_mids(boolean ghostly)
{
    struct obj *otmp;
    unsigned oldid, nid;

    for (otmp = fobj; otmp; otmp = otmp->nobj) {
        if (ghostly && has_omonst(otmp)) {
            struct monst *mtmp = OMONST(otmp);

            mtmp->m_id = 0;
            mtmp->mpeaceful = mtmp->mtame = 0; /* pet's owner died! */
        }
        if (ghostly && has_omid(otmp)) {
            oldid = OMID(otmp);
            if (lookup_id_mapping(oldid, &nid))
                OMID(otmp) = nid;
            else
                free_omid(otmp);
        }
    }
}

#ifdef SELECTSAVED
/* put up a menu listing each character from this player's saved games;
   returns 1: use svp.plname[], 0: new game, -1: quit */
int
restore_menu(
    winid bannerwin) /* if not WIN_ERR, clear window
                      * and show copyright in menu */
{
    winid tmpwin;
    anything any;
    char **saved, *next, mode, menutext[BUFSZ];
    boolean all_normal;
    menu_item *chosen_game = (menu_item *) 0;
    int k, clet, ch = 0; /* ch: 0 => new game */
    int clr = NO_COLOR;

    *svp.plname = '\0';
    saved = get_saved_games(); /* array of character names */
    if (saved && *saved) {
        tmpwin = create_nhwindow(NHW_MENU);
        start_menu(tmpwin, MENU_BEHAVE_STANDARD);
        any = cg.zeroany; /* no selection */
        if (bannerwin != WIN_ERR) {
            /* for tty; erase copyright notice and redo it in the menu */
            clear_nhwindow(bannerwin);
            /* COPYRIGHT_BANNER_[ABCD] */
            for (k = 1; k <= 4; ++k)
                add_menu_str(tmpwin, copyright_banner_line(k));
            add_menu_str(tmpwin, "");
        }
        add_menu_str(tmpwin, "Select one of your saved games");
        /* if all the save files have a playmode of '-' then we'll just list
           their character name-role-race-gend-algn values, but if any are
           'X' or 'D', we'll list playmode along with name-role-&c values
           for every entry; first, figure out if they're all normal play */
        for (all_normal = TRUE, k = 0; all_normal && saved[k]; ++k) {
            next = saved[k];
            mode = next[PL_NSIZ_PLUS - 1]; /* fixed last char, beyond '\0' */
            if (mode != '-')
                all_normal = FALSE;
        }
        for (k = 0; saved[k]; ++k) {
            any.a_int = k + 1;
            next = saved[k];
            mode = next[PL_NSIZ_PLUS - 1];
            if (all_normal)
                Sprintf(menutext, "%.*s", PL_NSIZ_PLUS - 1, next);
            else
                Sprintf(menutext, "%c %.*s", mode, PL_NSIZ_PLUS - 1, next);
            add_menu(tmpwin, &nul_glyphinfo, &any, 0, 0, ATR_NONE, clr,
                     menutext, MENU_ITEMFLAGS_SKIPMENUCOLORS);
        }
        clet = (k <= 'n' - 'a') ? 'n'      /* new game */
               : (k <= 26 + 'N' - 'A') ? 'N' : 0;
        any.a_int = -1;                    /* not >= 0 */
        add_menu(tmpwin, &nul_glyphinfo, &any, clet, 'N', ATR_NONE, clr,
                 "Start a new character", MENU_ITEMFLAGS_NONE);
        clet = (k + 1 <= 'q' - 'a' && clet == 'n') ? 'q'  /* quit */
               : (k + 1 <= 26 + 'Q' - 'A' && clet == 'N') ? 'Q' : 0;
        any.a_int = -2;
        add_menu(tmpwin, &nul_glyphinfo, &any, clet, 'Q', ATR_NONE, clr,
                 "Never mind (quit)", MENU_ITEMFLAGS_SELECTED);
        /* no prompt on end_menu, as we've done our own at the top */
        end_menu(tmpwin, (char *) 0);
        if (select_menu(tmpwin, PICK_ONE, &chosen_game) > 0) {
            ch = chosen_game->item.a_int;
            if (ch > 0)
                Strcpy(svp.plname, saved[ch - 1]);
            else if (ch < 0)
                ++ch; /* -1 -> 0 (new game), -2 -> -1 (quit) */
            free((genericptr_t) chosen_game);
        } else {
            ch = -1; /* quit menu without making a selection => quit */
        }
        destroy_nhwindow(tmpwin);
        if (bannerwin != WIN_ERR) {
            /* for tty; clear the menu away and put subset of copyright back
             */
            clear_nhwindow(bannerwin);
            /* COPYRIGHT_BANNER_A, preceding "Who are you?" prompt */
            if (ch == 0)
                putstr(bannerwin, 0, copyright_banner_line(1));
        }
    }
    free_saved_games(saved);
    return (ch > 0) ? 1 : ch;
}
#endif /* SELECTSAVED */

int
validate(NHFILE *nhfp, const char *name, boolean without_waitsynch_perfile)
{
    unsigned long utdflags = 0L;
    boolean reslt = FALSE;

    if (nhfp->structlevel)
        utdflags |= UTD_CHECKSIZES;
    if (without_waitsynch_perfile)
        utdflags |= UTD_WITHOUT_WAITSYNCH_PERFILE;
    if (!(reslt = uptodate(nhfp, name, utdflags)))
        return 1;

    return 0;
}

#undef Mread

/*restore.c*/
