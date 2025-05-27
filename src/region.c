/* NetHack 3.7	region.c	$NHDT-Date: 1727251269 2024/09/25 08:01:09 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.104 $ */
/* Copyright (c) 1996 by Jean-Christophe Collet  */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

/*
 * This should really go into the level structure, but
 * I'll start here for ease. It *WILL* move into the level
 * structure eventually.
 */

#define NO_CALLBACK (-1)

void free_region(NhRegion *);
#ifndef SFCTOOL
boolean inside_gas_cloud(genericptr, genericptr);
boolean expire_gas_cloud(genericptr, genericptr);
boolean inside_rect(NhRect *, int, int);
NhRegion *create_region(NhRect *, int);
void add_rect_to_reg(NhRegion *, NhRect *);
void add_mon_to_reg(NhRegion *, struct monst *);
void remove_mon_from_reg(NhRegion *, struct monst *);
boolean mon_in_region(NhRegion *, struct monst *);

#if 0
NhRegion *clone_region(NhRegion *);
#endif
void add_region(NhRegion *);
void remove_region(NhRegion *);

#if 0
void replace_mon_regions(struct monst *,struct monst *);
void remove_mon_from_regions(struct monst *);
NhRegion *create_msg_region(coordxy,coordxy,coordxy,coordxy, const char *,
                           const char *);
boolean enter_force_field(genericptr,genericptr);
NhRegion *create_force_field(coordxy,coordxy,int,long);
#endif

staticfn void reset_region_mids(NhRegion *);
staticfn boolean is_hero_inside_gas_cloud(void);
staticfn void make_gas_cloud(NhRegion *, int, boolean) NONNULLARG1;

static const callback_proc callbacks[] = {
#define INSIDE_GAS_CLOUD 0
    inside_gas_cloud,
#define EXPIRE_GAS_CLOUD 1
    expire_gas_cloud
};

/* Should be inlined. */
boolean
inside_rect(NhRect *r, int x, int y)
{
    return (boolean) (x >= r->lx && x <= r->hx && y >= r->ly && y <= r->hy);
}

/*
 * Check if a point is inside a region.
 */
boolean
inside_region(NhRegion *reg, int x, int y)
{
    int i;

    if (reg == (NhRegion *) 0 || !inside_rect(&(reg->bounding_box), x, y))
        return FALSE;
    for (i = 0; i < reg->nrects; i++)
        if (inside_rect(&(reg->rects[i]), x, y))
            return TRUE;
    return FALSE;
}

/*
 * Create a region. It does not activate it.
 */
NhRegion *
create_region(NhRect *rects, int nrect)
{
    int i;
    NhRegion *reg;

    reg = (NhRegion *) alloc(sizeof(NhRegion));
    (void) memset((genericptr_t) reg, 0, sizeof(NhRegion));
    /* Determines bounding box */
    if (nrect > 0) {
        reg->bounding_box = rects[0];
    } else {
        reg->bounding_box.lx = COLNO;
        reg->bounding_box.ly = ROWNO;
        reg->bounding_box.hx = 0; /* 1 */
        reg->bounding_box.hy = 0;
    }
    reg->nrects = nrect;
    reg->rects = (nrect > 0) ? (NhRect *) alloc(nrect * sizeof (NhRect)) : 0;
    for (i = 0; i < nrect; i++) {
        if (rects[i].lx < reg->bounding_box.lx)
            reg->bounding_box.lx = rects[i].lx;
        if (rects[i].ly < reg->bounding_box.ly)
            reg->bounding_box.ly = rects[i].ly;
        if (rects[i].hx > reg->bounding_box.hx)
            reg->bounding_box.hx = rects[i].hx;
        if (rects[i].hy > reg->bounding_box.hy)
            reg->bounding_box.hy = rects[i].hy;
        reg->rects[i] = rects[i];
    }
    reg->ttl = -1L; /* Defaults */
    reg->attach_2_u = FALSE;
    reg->attach_2_m = 0;
    /* reg->attach_2_o = NULL; */
    reg->enter_msg = (const char *) 0;
    reg->leave_msg = (const char *) 0;
    reg->expire_f = NO_CALLBACK;
    reg->enter_f = NO_CALLBACK;
    reg->can_enter_f = NO_CALLBACK;
    reg->leave_f = NO_CALLBACK;
    reg->can_leave_f = NO_CALLBACK;
    reg->inside_f = NO_CALLBACK;
    clear_hero_inside(reg);
    clear_heros_fault(reg);
    reg->n_monst = 0;
    reg->max_monst = 0;
    reg->monsters = (unsigned *) 0;
    reg->arg = cg.zeroany;
    return reg;
}

/*
 * Add rectangle to region.
 */
void
add_rect_to_reg(NhRegion *reg, NhRect *rect)
{
    NhRect *tmp_rect;

    tmp_rect = (NhRect *) alloc((reg->nrects + 1) * sizeof (NhRect));
    if (reg->nrects > 0) {
        (void) memcpy((genericptr_t) tmp_rect, (genericptr_t) reg->rects,
                      reg->nrects * sizeof (NhRect));
        free((genericptr_t) reg->rects);
    }
    tmp_rect[reg->nrects] = *rect;
    reg->nrects++;
    reg->rects = tmp_rect;
    /* Update bounding box if needed */
    if (reg->bounding_box.lx > rect->lx)
        reg->bounding_box.lx = rect->lx;
    if (reg->bounding_box.ly > rect->ly)
        reg->bounding_box.ly = rect->ly;
    if (reg->bounding_box.hx < rect->hx)
        reg->bounding_box.hx = rect->hx;
    if (reg->bounding_box.hy < rect->hy)
        reg->bounding_box.hy = rect->hy;
}

/*
 * Add a monster to the region
 */
void
add_mon_to_reg(NhRegion *reg, struct monst *mon)
{
    int i;
    unsigned *tmp_m;

    /* if this is a long worm, it might already be present in the region;
       only include it once no matter how segments the region contains */
    if (mon_in_region(reg, mon)) {
        if (mon->data != &mons[PM_LONG_WORM])
            impossible("add_mon_to_reg: %s [#%u] already in region.",
                       m_monnam(mon), mon->m_id);
        return;
    }
    if (reg->max_monst <= reg->n_monst) {
        tmp_m = (unsigned *) alloc(sizeof (unsigned)
                                   * (reg->max_monst + MONST_INC));
        if (reg->max_monst > 0) {
            for (i = 0; i < reg->max_monst; i++)
                tmp_m[i] = reg->monsters[i];
            free((genericptr_t) reg->monsters);
        }
        reg->monsters = tmp_m;
        reg->max_monst += MONST_INC;
    }
    reg->monsters[reg->n_monst++] = mon->m_id;
}

/*
 * Remove a monster from the region list (it left or died...)
 */
void
remove_mon_from_reg(NhRegion *reg, struct monst *mon)
{
    int i;

    for (i = 0; i < reg->n_monst; i++)
        if (reg->monsters[i] == mon->m_id) {
            reg->n_monst--;
            reg->monsters[i] = reg->monsters[reg->n_monst];
            return;
        }
}

/*
 * Check if a monster is inside the region.
 * It's probably quicker to check with the region internal list
 * than to check for coordinates.
 */
boolean
mon_in_region(NhRegion *reg, struct monst *mon)
{
    int i;

    for (i = 0; i < reg->n_monst; i++)
        if (reg->monsters[i] == mon->m_id)
            return TRUE;
    return FALSE;
}

#if 0
/* not yet used */

/*
 * Clone (make a standalone copy) the region.
 */
NhRegion *
clone_region(NhRegion *reg)
{
    NhRegion *ret_reg;
    unsigned *m_id_list;
    short i;

    ret_reg = create_region(reg->rects, reg->nrects);
    ret_reg->ttl = reg->ttl;
    ret_reg->attach_2_u = reg->attach_2_u;
    ret_reg->attach_2_m = reg->attach_2_m;
 /* ret_reg->attach_2_o = reg->attach_2_o; */
    ret_reg->expire_f = reg->expire_f;
    ret_reg->enter_f = reg->enter_f;
    ret_reg->can_enter_f = reg->can_enter_f;
    ret_reg->leave_f = reg->leave_f;
    ret_reg->can_leave_f = reg->can_leave_f;
    ret_reg->player_flags = reg->player_flags; /* set/clear_hero_inside,&c*/
    ret_reg->n_monst = reg->n_monst;
    ret_reg->max_monst = reg->max_monst;
    if (reg->max_monst > 0) {
        m_id_list = (unsigned *) alloc(reg->max_monst * sizeof (unsigned));
        for (i = 0; i < reg->max_monst; ++i)
            m_id_list[i] = reg->monsters[i];
    } else
        m_id_list = (unsigned *) 0;
    ret_reg->monsters = m_id_list;
    return ret_reg;
}

#endif /*0*/
#endif /* !SFCTOOL */

/*
 * Free mem from region.
 */
void
free_region(NhRegion *reg)
{
    if (reg) {
        if (reg->rects)
            free((genericptr_t) reg->rects);
        if (reg->monsters)
            free((genericptr_t) reg->monsters);
        if (reg->enter_msg)
            free((genericptr_t) reg->enter_msg);
        if (reg->leave_msg)
            free((genericptr_t) reg->leave_msg);
        free((genericptr_t) reg);
    }
}

#ifndef SFCTOOL
/*
 * Add a region to the list.
 * This actually activates the region.
 */
void
add_region(NhRegion *reg)
{
    NhRegion **tmp_reg;
    int i, j;

    if (gm.max_regions <= svn.n_regions) {
        tmp_reg = gr.regions;
        gr.regions =
            (NhRegion **) alloc((gm.max_regions + 10) * sizeof (NhRegion *));
        if (gm.max_regions > 0) {
            (void) memcpy((genericptr_t) gr.regions, (genericptr_t) tmp_reg,
                          gm.max_regions * sizeof (NhRegion *));
            free((genericptr_t) tmp_reg);
        }
        gm.max_regions += 10;
    }
    gr.regions[svn.n_regions] = reg;
    svn.n_regions++;
    /* Check for monsters inside the region */
    for (i = reg->bounding_box.lx; i <= reg->bounding_box.hx; i++)
        for (j = reg->bounding_box.ly; j <= reg->bounding_box.hy; j++) {
            struct monst *mtmp;
            boolean is_inside = FALSE;

            /* Some regions can cross the level boundaries */
            if (!isok(i, j))
                continue;
            if (inside_region(reg, i, j)) {
                is_inside = TRUE;
                /* if there's a monster here, add it to the region */
                if ((mtmp = m_at(i, j)) != 0
#if 0
                    /* leave this bit (to exclude long worm tails) out;
                       assume that worms use "cutaneous respiration" (they
                       breath through their skin rather than nose/gills/&c)
                       so their tails are susceptible to poison gas */
                    && mtmp->mx == i && mtmp->my == j
#endif
                    ) {
                    add_mon_to_reg(reg, mtmp);
                }
            }
            if (reg->visible) {
                if (is_inside)
                    block_point(i, j);
                if (cansee(i, j))
                    newsym(i, j);
            }
        }
    /* Check for player now... */
    if (inside_region(reg, u.ux, u.uy))
        set_hero_inside(reg);
    else
        clear_hero_inside(reg);
}

/*
 * Remove a region from the list & free it.
 */
void
remove_region(NhRegion *reg)
{
    int i, x, y;

    for (i = 0; i < svn.n_regions; i++)
        if (gr.regions[i] == reg)
            break;
    if (i == svn.n_regions)
        return;

    /* remove region before potential newsym() calls, but don't free it yet */
    if (--svn.n_regions != i)
        gr.regions[i] = gr.regions[svn.n_regions];
    gr.regions[svn.n_regions] = (NhRegion *) 0;

    /* Update screen if necessary */
    reg->ttl = -2L; /* for visible_region_at */
    if (reg->visible) {
        int pass;
        boolean tmp_uinwater = u.uinwater;

        /* need to process the region's spots twice, first unblocking all
           locations which no longer block line-of-sight, then redrawing
           spots within revised line-of-sight; skip second pass if blind */
        for (pass = 1; pass <= (Blind ? 1 : 2); ++pass) {
            u.uinwater = (pass == 1) ? 0 : tmp_uinwater;

            for (x = reg->bounding_box.lx; x <= reg->bounding_box.hx; x++)
                for (y = reg->bounding_box.ly; y <= reg->bounding_box.hy; y++)
                    if (isok(x, y) && inside_region(reg, x, y)) {
                        if (pass == 1) {
                            if (!does_block(x, y, &levl[x][y]))
                                unblock_point(x, y);
                        } else { /* pass==2 */
                            if (cansee(x, y))
                                newsym(x, y);
                        }
                    }
        }
        u.uinwater = tmp_uinwater;
    }
    free_region(reg);
}
#endif /* !SFCTOOL */

/*
 * Remove all regions and clear all related data.  This must be done
 * when changing level, for instance.
 */
void
clear_regions(void)
{
    int i;

    for (i = 0; i < svn.n_regions; i++)
        free_region(gr.regions[i]);
    svn.n_regions = 0;
    if (gm.max_regions > 0)
        free((genericptr_t) gr.regions);
    gm.max_regions = 0;
    gr.regions = (NhRegion **) 0;
}

#ifndef SFCTOOL
/*
 * This function is called every turn.
 * It makes the regions age, if necessary and calls the appropriate
 * callbacks when needed.
 */
void
run_regions(void)
{
    int i, j, k;
    int f_indx;

    /* reset some messaging variables */
    gg.gas_cloud_diss_within = FALSE;
    gg.gas_cloud_diss_seen = 0;

    /* End of life ? */
    /* Do it backward because the array will be modified */
    for (i = svn.n_regions - 1; i >= 0; i--) {
        if (gr.regions[i]->ttl == 0L) {
            if ((f_indx = gr.regions[i]->expire_f) == NO_CALLBACK
                || (*callbacks[f_indx])(gr.regions[i], (genericptr_t) 0))
                remove_region(gr.regions[i]);
        }
    }

    /* Process remaining regions */
    for (i = 0; i < svn.n_regions; i++) {
        /* Make the region age */
        if (gr.regions[i]->ttl > 0L)
            gr.regions[i]->ttl--;
        /* Check if player is inside region */
        f_indx = gr.regions[i]->inside_f;
        if (f_indx != NO_CALLBACK && hero_inside(gr.regions[i]))
            (void) (*callbacks[f_indx])(gr.regions[i], (genericptr_t) 0);
        /* Check if any monster is inside region */
        if (f_indx != NO_CALLBACK) {
            for (j = 0; j < gr.regions[i]->n_monst; j++) {
                struct monst *mtmp =
                    find_mid(gr.regions[i]->monsters[j], FM_FMON);

                if (!mtmp || DEADMONSTER(mtmp)
                    || (*callbacks[f_indx])(gr.regions[i], mtmp)) {
                    /* The monster died, remove it from list */
                    k = (gr.regions[i]->n_monst -= 1);
                    gr.regions[i]->monsters[j] = gr.regions[i]->monsters[k];
                    gr.regions[i]->monsters[k] = 0;
                    --j; /* current slot has been reused; recheck it next */
                }
            }
        }
    }

    if (gg.gas_cloud_diss_within) {
        pline_The("gas cloud around you dissipates.");
        /* normally won't see additional dissipation when within */
        /* FIXME? this assumes that additional dissipation is close by */
        if (u.xray_range <= 1)
            gg.gas_cloud_diss_seen = 0;
        gg.gas_cloud_diss_within = FALSE;
    }
    if (gg.gas_cloud_diss_seen) {
        You_see("%s gas cloud%s dissipate.",
                (gg.gas_cloud_diss_seen == 1) ? "a" : "some",
                plur(gg.gas_cloud_diss_seen));
        gg.gas_cloud_diss_seen = 0;
    }
}

/*
 * check whether player enters/leaves one or more regions.
 */
boolean
in_out_region(coordxy x, coordxy y)
{
    int i, f_indx = 0;

    /* First check if hero can do the move */
    for (i = 0; i < svn.n_regions; i++) {
        if (gr.regions[i]->attach_2_u)
            continue;
        if (inside_region(gr.regions[i], x, y)
            ? (!hero_inside(gr.regions[i])
               && (f_indx = gr.regions[i]->can_enter_f) != NO_CALLBACK)
            : (hero_inside(gr.regions[i])
               && (f_indx = gr.regions[i]->can_leave_f) != NO_CALLBACK)) {
            if (!(*callbacks[f_indx])(gr.regions[i], (genericptr_t) 0))
                return FALSE;
        }
    }

    /* Callbacks for the regions hero does leave */
    for (i = 0; i < svn.n_regions; i++) {
        if (gr.regions[i]->attach_2_u)
            continue;
        if (hero_inside(gr.regions[i])
            && !inside_region(gr.regions[i], x, y)) {
            clear_hero_inside(gr.regions[i]);
            if (gr.regions[i]->leave_msg != (const char *) 0)
                pline1(gr.regions[i]->leave_msg);
            if ((f_indx = gr.regions[i]->leave_f) != NO_CALLBACK)
                (void) (*callbacks[f_indx])(gr.regions[i], (genericptr_t) 0);
        }
    }

    /* Callbacks for the regions hero does enter */
    for (i = 0; i < svn.n_regions; i++) {
        if (gr.regions[i]->attach_2_u)
            continue;
        if (!hero_inside(gr.regions[i])
            && inside_region(gr.regions[i], x, y)) {
            set_hero_inside(gr.regions[i]);
            if (gr.regions[i]->enter_msg != (const char *) 0)
                pline1(gr.regions[i]->enter_msg);
            if ((f_indx = gr.regions[i]->enter_f) != NO_CALLBACK)
                (void) (*callbacks[f_indx])(gr.regions[i], (genericptr_t) 0);
        }
    }

    return TRUE;
}

/*
 * check whether a monster enters/leaves one or more regions.
 */
boolean
m_in_out_region(struct monst *mon, coordxy x, coordxy y)
{
    int i, f_indx = 0;

    /* First check if mon can do the move */
    for (i = 0; i < svn.n_regions; i++) {
        if (gr.regions[i]->attach_2_m == mon->m_id)
            continue;
        if (inside_region(gr.regions[i], x, y)
            ? (!mon_in_region(gr.regions[i], mon)
               && (f_indx = gr.regions[i]->can_enter_f) != NO_CALLBACK)
            : (mon_in_region(gr.regions[i], mon)
               && (f_indx = gr.regions[i]->can_leave_f) != NO_CALLBACK)) {
            if (!(*callbacks[f_indx])(gr.regions[i], mon))
                return FALSE;
        }
    }

    /* Callbacks for the regions mon does leave */
    for (i = 0; i < svn.n_regions; i++) {
        if (gr.regions[i]->attach_2_m == mon->m_id)
            continue;
        if (mon_in_region(gr.regions[i], mon)
            && !inside_region(gr.regions[i], x, y)) {
            remove_mon_from_reg(gr.regions[i], mon);
            if ((f_indx = gr.regions[i]->leave_f) != NO_CALLBACK)
                (void) (*callbacks[f_indx])(gr.regions[i], mon);
        }
    }

    /* Callbacks for the regions mon does enter */
    for (i = 0; i < svn.n_regions; i++) {
        if (gr.regions[i]->attach_2_m == mon->m_id)
            continue;
        if (!mon_in_region(gr.regions[i], mon)
            && inside_region(gr.regions[i], x, y)) {
            add_mon_to_reg(gr.regions[i], mon);
            if ((f_indx = gr.regions[i]->enter_f) != NO_CALLBACK)
                (void) (*callbacks[f_indx])(gr.regions[i], mon);
        }
    }

    return TRUE;
}

/*
 * Checks player's regions after a teleport for instance.
 */
void
update_player_regions(void)
{
    int i;

    for (i = 0; i < svn.n_regions; i++)
        if (!gr.regions[i]->attach_2_u
            && inside_region(gr.regions[i], u.ux, u.uy))
            set_hero_inside(gr.regions[i]);
        else
            clear_hero_inside(gr.regions[i]);
}

/*
 * Ditto for a specified monster.
 */
void
update_monster_region(struct monst *mon)
{
    int i;

    for (i = 0; i < svn.n_regions; i++) {
        if (inside_region(gr.regions[i], mon->mx, mon->my)) {
            if (!mon_in_region(gr.regions[i], mon))
                add_mon_to_reg(gr.regions[i], mon);
        } else {
            if (mon_in_region(gr.regions[i], mon))
                remove_mon_from_reg(gr.regions[i], mon);
        }
    }
}

#if 0
/* not yet used */

/*
 * Change monster pointer in gr.regions
 * This happens, for instance, when a monster grows and
 * need a new structure (internally that is).
 */
void
replace_mon_regions(monold, monnew)
struct monst *monold, *monnew;
{
    int i;

    for (i = 0; i < svn.n_regions; i++)
        if (mon_in_region(gr.regions[i], monold)) {
            remove_mon_from_reg(gr.regions[i], monold);
            add_mon_to_reg(gr.regions[i], monnew);
        }
}

/*
 * Remove monster from all regions it was in (ie monster just died)
 */
void
remove_mon_from_regions(struct monst *mon)
{
    int i;

    for (i = 0; i < svn.n_regions; i++)
        if (mon_in_region(gr.regions[i], mon))
            remove_mon_from_reg(gr.regions[i], mon);
}

#endif /*0*/

/* per-turn damage inflicted by visible region; hides details from caller */
int
reg_damg(NhRegion *reg)
{
    int damg = (!reg->visible || reg->ttl == -2L) ? 0 : reg->arg.a_int;

    return damg;
}

/* check whether current level has any visible regions */
boolean
any_visible_region(void)
{
    int i;

    for (i = 0; i < svn.n_regions; i++) {
        if (!gr.regions[i]->visible || gr.regions[i]->ttl == -2L)
            continue;
        return TRUE;
    }
    return FALSE;
}

/* for the wizard mode #timeout command */
void
visible_region_summary(winid win)
{
    NhRegion *reg;
    char buf[BUFSZ], typbuf[QBUFSZ];
    int i, damg, hdr_done = 0;
    const char *fldsep = iflags.menu_tab_sep ? "\t" : "  ";

    for (i = 0; i < svn.n_regions; i++) {
        reg = gr.regions[i];
        if (!reg->visible || reg->ttl == -2L)
            continue;

        if (!hdr_done++) {
            putstr(win, 0, "");
            putstr(win, 0, "Visible regions");
        }
        /*
         * TODO? sort the regions by time-to-live or by bounding box.
         */

        /* we display relative time (turns left) rather than absolute
           (the turn when region will go away);
           since time-to-live has already been decremented, regions
           which are due to timeout on the next turn have ttl==0;
           adding 1 is intended to make the display be less confusing */
        Sprintf(buf, "%5ld", reg->ttl + 1L);
        damg = reg->arg.a_int;
        if (damg)
            Sprintf(typbuf, "poison gas (%d)", damg);
        else
            Strcpy(typbuf, "vapor");
        Sprintf(eos(buf), "%s%-16s", fldsep, typbuf);
        Sprintf(eos(buf), "%s@[%d,%d..%d,%d]", fldsep,
                reg->bounding_box.lx, reg->bounding_box.ly,
                reg->bounding_box.hx, reg->bounding_box.hy);
        putstr(win, 0, buf);
    }
}

/*
 * Check if a spot is under a visible region (eg: gas cloud).
 * Returns NULL if not, otherwise returns region.
 */
NhRegion *
visible_region_at(coordxy x, coordxy y)
{
    int i;

    for (i = 0; i < svn.n_regions; i++) {
        if (!gr.regions[i]->visible || gr.regions[i]->ttl == -2L)
            continue;
        if (inside_region(gr.regions[i], x, y))
            return gr.regions[i];
    }
    return (NhRegion *) 0;
}

void
show_region(NhRegion *reg, coordxy x, coordxy y)
{
    show_glyph(x, y, reg->glyph);
}

/**
 * save_regions :
 */
void
save_regions(NHFILE *nhfp)
{
    NhRegion *r;
    int i, j;
    unsigned n;

    if (!update_file(nhfp))
        goto skip_lots;
    /* timestamp */
    Sfo_long(nhfp, &svm.moves, "region-tmstamp");
    Sfo_int(nhfp, &svn.n_regions, "region-region_count");

    for (i = 0; i < svn.n_regions; i++) {
        r = gr.regions[i];
        Sfo_nhrect(nhfp, &r->bounding_box, "region-bounding_box");
        Sfo_short(nhfp, &r->nrects, "region-nrects");
        for (j = 0; j < r->nrects; j++) {
            Sfo_nhrect(nhfp, &r->rects[j], "region-rect");
        }
        Sfo_boolean(nhfp, &r->attach_2_u, "region-attach_2_u");
        Sfo_unsigned(nhfp, &r->attach_2_m, "region-attach_2_m");
        n = 0;
        n = !r->enter_msg ? 0U : (unsigned) strlen(r->enter_msg);
        Sfo_unsigned(nhfp, &n, "region-enter_msg_length");
        if (n > 0) {
            Sfo_char(nhfp, (char *) r->enter_msg,
                     "region-enter_msg", (int) n);
        }
        n = !r->leave_msg ? 0U : (unsigned) strlen(r->leave_msg);
        Sfo_unsigned(nhfp, &n, "region-leave_msg_length");
        if (n > 0) {
            Sfo_char(nhfp, (char *) r->leave_msg, "region-leave_msg", (int) n);
        }
        Sfo_long(nhfp, &r->ttl, "region-ttl");
        Sfo_short(nhfp, &r->expire_f, "region-expire_f");
        Sfo_short(nhfp, &r->can_enter_f, "region-can_enter_f");
        Sfo_short(nhfp, &r->enter_f, "region-enter_f");
        Sfo_short(nhfp, &r->can_leave_f, "region-can_leave_f");
        Sfo_short(nhfp, &r->leave_f, "region-leave_f");
        Sfo_short(nhfp, &r->inside_f, "region-inside_f");
        Sfo_unsigned(nhfp, &r->player_flags, "region-player_flags");
        Sfo_short(nhfp, &r->n_monst, "region-monster_count");

        for (j = 0; j < r->n_monst; j++) {
            Sfo_unsigned(nhfp, &r->monsters[j], "region-monster");
        }
        Sfo_boolean(nhfp, &r->visible, "region-visible");
        Sfo_int(nhfp, &r->glyph, "region-glyph");
        Sfo_any(nhfp, &r->arg, "region-arg");
    }

 skip_lots:
    if (release_data(nhfp))
        clear_regions();
}
#endif /* !SFCTOOL */

void
rest_regions(NHFILE *nhfp)
{
    NhRegion *r;
    int i, j;
    unsigned n = 0;
    long tmstamp = 0L;
    char *msg_buf;
    boolean ghostly = (nhfp->ftype == NHF_BONESFILE);

    clear_regions(); /* Just for security */
    Sfi_long(nhfp, &tmstamp, "region-tmstamp");
    if (ghostly)
        tmstamp = 0;
    else
        tmstamp = (svm.moves - tmstamp);
    Sfi_int(nhfp, &svn.n_regions, "region-region_count");
    gm.max_regions = svn.n_regions;
    if (svn.n_regions > 0)
        gr.regions = (NhRegion **) alloc(svn.n_regions * sizeof (NhRegion *));
    for (i = 0; i < svn.n_regions; i++) {
        r = gr.regions[i] = (NhRegion *) alloc(sizeof (NhRegion));
        Sfi_nhrect(nhfp, &r->bounding_box, "region-bounding box");
        Sfi_short(nhfp, &r->nrects, "region-nrects");
        if (r->nrects > 0)
            r->rects = (NhRect *) alloc(r->nrects * sizeof (NhRect));
        else
            r->rects = (NhRect *) 0;
        for (j = 0; j < r->nrects; j++) {
           Sfi_nhrect(nhfp, &r->rects[j], "region-rect");
        }

        Sfi_boolean(nhfp, &r->attach_2_u, "region-attach_2_u");
        Sfi_unsigned(nhfp, &r->attach_2_m, "region-attach_2_m");
        Sfi_unsigned(nhfp, &n, "region-enter_msg_length");
        if (n > 0) {
            msg_buf = (char *) alloc(n + 1);
            Sfi_char(nhfp, msg_buf, "region-enter_msg", n);
            msg_buf[n] = '\0';
        } else {
            msg_buf = (char *) 0;
        }
        r->enter_msg = (const char *) msg_buf;

        Sfi_unsigned(nhfp, &n, "region-leave_msg_length");
        if (n > 0) {
            msg_buf = (char *) alloc(n + 1);
            Sfi_char(nhfp, msg_buf, "region-leave_msg", n);
            msg_buf[n] = '\0';
            r->leave_msg = (const char *) msg_buf;
        } else {
            msg_buf = (char *) 0;
        }
        r->leave_msg = (const char *) msg_buf;

        Sfi_long(nhfp, &r->ttl, "region-ttl");
        /* check for expired region */
        if (r->ttl >= 0L)
            r->ttl = (r->ttl > tmstamp) ? r->ttl - tmstamp : 0L;
        Sfi_short(nhfp, &r->expire_f, "region-expire_f");
        Sfi_short(nhfp, &r->can_enter_f, "region-can_enter_f");
        Sfi_short(nhfp, &r->enter_f, "region-enter_f");
        Sfi_short(nhfp, &r->can_leave_f, "region-can_leave_f");
        Sfi_short(nhfp, &r->leave_f, "region-leave_f");
        Sfi_short(nhfp, &r->inside_f, "region-inside_f");
        Sfi_unsigned(nhfp, &r->player_flags, "region-player_flags");
        if (ghostly) { /* settings pertained to old player */
            clear_hero_inside(r);
            clear_heros_fault(r);
        }
        Sfi_short(nhfp, &r->n_monst, "region-monster_count");
        if (r->n_monst > 0)
            r->monsters = (unsigned *) alloc(r->n_monst * sizeof (unsigned));
        else
            r->monsters = (unsigned *) 0;
        r->max_monst = r->n_monst;
        for (j = 0; j < r->n_monst; j++) {
            Sfi_unsigned(nhfp, &r->monsters[j], "region-monster");
        }
        Sfi_boolean(nhfp, &r->visible, "region-visible");
        Sfi_int(nhfp, &r->glyph, "region-glyph");
        Sfi_any(nhfp, &r->arg, "region-arg");
    }
#ifndef SFCTOOL
    /* remove expired regions, do not trigger the expire_f callback (yet!);
       also update monster lists if this data is coming from a bones file */
    for (i = svn.n_regions - 1; i >= 0; i--) {
        r = gr.regions[i];
        if (r->ttl == 0L)
            remove_region(r);
        else if (ghostly && r->n_monst > 0)
            reset_region_mids(r);
    }
#endif /* !SFCTOOL */
}

#ifndef SFCTOOL
DISABLE_WARNING_FORMAT_NONLITERAL

/* to support '#stats' wizard-mode command */
void
region_stats(
    const char *hdrfmt,
    char *hdrbuf,
    long *count,
    long *size)
{
    NhRegion *rg;
    int i;

    /* other stats formats take one parameter; this takes two */
    Sprintf(hdrbuf, hdrfmt, (long) sizeof (NhRegion), (long) sizeof (NhRect));
    *count = (long) svn.n_regions; /* might be 0 even tho max_regions isn't */
    *size = (long) gm.max_regions * (long) sizeof (NhRegion);
    for (i = 0; i < svn.n_regions; ++i) {
        rg = gr.regions[i];
        *size += (long) rg->nrects * (long) sizeof (NhRect);
        if (rg->enter_msg)
            *size += (long) (strlen(rg->enter_msg) + 1);
        if (rg->leave_msg)
            *size += (long) (strlen(rg->leave_msg) + 1);
        *size += (long) rg->max_monst * (long) sizeof *rg->monsters;
    }
    /* ? */
}

RESTORE_WARNING_FORMAT_NONLITERAL

/* update monster IDs for region being loaded from bones; `ghostly' implied */
staticfn void
reset_region_mids(NhRegion *reg)
{
    int i = 0, n = reg->n_monst;
    unsigned *mid_list = reg->monsters;

    while (i < n)
        if (!lookup_id_mapping(mid_list[i], &mid_list[i])) {
            /* shrink list to remove missing monster; order doesn't matter */
            mid_list[i] = mid_list[--n];
        } else {
            /* move on to next monster */
            ++i;
        }
    reg->n_monst = n;
    return;
}

#if 0
/* not yet used */

/*--------------------------------------------------------------*
 *                                                              *
 *                      Create Region with just a message       *
 *                                                              *
 *--------------------------------------------------------------*/

NhRegion *
create_msg_region(
    coordxy x, coordxy y, coordxy w, coordxy h,
    const char *msg_enter, const char *msg_leave)
{
    NhRect tmprect;
    NhRegion *reg = create_region((NhRect *) 0, 0);

    if (msg_enter)
        reg->enter_msg = dupstr(msg_enter);
    if (msg_leave)
        reg->leave_msg = dupstr(msg_leave);
    tmprect.lx = x;
    tmprect.ly = y;
    tmprect.hx = x + w;
    tmprect.hy = y + h;
    add_rect_to_reg(reg, &tmprect);
    reg->ttl = -1L;
    return reg;
}


/*--------------------------------------------------------------*
 *                                                              *
 *                      Force Field Related Cod                 *
 *                      (unused yet)                            *
 *--------------------------------------------------------------*/

boolean
enter_force_field(genericptr_t p1, genericptr_t p2)
{
    struct monst *mtmp;

    if (p2 == (genericptr_t) 0) { /* That means the player */
        if (!Blind)
            You("bump into %s.  Ouch!",
                Hallucination ? "an invisible tree"
                              : "some kind of invisible wall");
        else
            pline("Ouch!");
    } else {
        mtmp = (struct monst *) p2;
        if (canseemon(mtmp))
            pline("%s bumps into %s!", Monnam(mtmp), something);
    }
    return FALSE;
}

NhRegion *
create_force_field(coordxy x, coordxy y, int radius, long ttl)
{
    int i;
    NhRegion *ff;
    int nrect;
    NhRect tmprect;

    ff = create_region((NhRect *) 0, 0);
    nrect = radius;
    tmprect.lx = x;
    tmprect.hx = x;
    tmprect.ly = y - (radius - 1);
    tmprect.hy = y + (radius - 1);
    for (i = 0; i < nrect; i++) {
        add_rect_to_reg(ff, &tmprect);
        tmprect.lx--;
        tmprect.hx++;
        tmprect.ly++;
        tmprect.hy--;
    }
    ff->ttl = ttl;
    if (!gi.in_mklev && !svc.context.mon_moving)
        set_heros_fault(ff); /* assume player has created it */
 /* ff->can_enter_f = enter_force_field; */
 /* ff->can_leave_f = enter_force_field; */
    add_region(ff);
    return ff;
}

#endif /*0*/

/*--------------------------------------------------------------*
 *                                                              *
 *                      Gas cloud related code                  *
 *                                                              *
 *--------------------------------------------------------------*/

/*
 * Here is an example of an expire function that may prolong
 * region life after some mods...
 */
/*ARGSUSED*/
boolean
expire_gas_cloud(genericptr_t p1, genericptr_t p2 UNUSED)
{
    NhRegion *reg;
    int damage, pass;
    coordxy x, y;

    reg = (NhRegion *) p1;
    damage = reg->arg.a_int;

    /* If it was a thick cloud, it dissipates a little first */
    if (damage >= 5) {
        damage /= 2; /* It dissipates, let's do less damage */
        reg->arg = cg.zeroany;
        reg->arg.a_int = damage;
        reg->ttl = 2L; /* Here's the trick : reset ttl */
        return FALSE;  /* THEN return FALSE, means "still there" */
    }

    /* The cloud no longer blocks vision.  cansee() checks shouldn't be made
       until all blocked spots have been unblocked, so we need two passes */
    for (pass = 1; pass <= (Blind ? 1 : 2); ++pass) {
        for (x = reg->bounding_box.lx; x <= reg->bounding_box.hx; x++) {
            for (y = reg->bounding_box.ly; y <= reg->bounding_box.hy; y++) {
                if (inside_region(reg, x, y)) {
                    if (pass == 1) {
                        if (!does_block(x, y, &levl[x][y]))
                            unblock_point(x, y);
                    } else { /* pass==2 */
                        if (!u.uswallow) {
                            if (u_at(x, y))
                                gg.gas_cloud_diss_within = TRUE;
                            else if (cansee(x, y))
                                gg.gas_cloud_diss_seen++;
                        }
                    }
                }
            }
        }
    }

    return TRUE; /* OK, it's gone, you can free it! */
}

/* returns True if p2 is killed by region p1, False otherwise */
boolean
inside_gas_cloud(genericptr_t p1, genericptr_t p2)
{
    NhRegion *reg = (NhRegion *) p1;
    struct monst *mtmp = (struct monst *) p2;
    struct monst *umon = mtmp ? mtmp : &gy.youmonst;
    int dam = reg->arg.a_int;

    /*
     * Gas clouds can't be targeted at water locations, but they can
     * start next to water and spread over it.
     */

    /* fog clouds maintain gas clouds, even poisonous ones */
    if (reg->ttl < 20 && umon && umon->data == &mons[PM_FOG_CLOUD])
        reg->ttl += 5;

    if (dam < 1)
        return FALSE; /* if no damage then there's nothing to do here... */

    if (!mtmp) { /* hero is indicated by Null rather than by &youmonst */
        if (m_poisongas_ok(&gy.youmonst) == M_POISONGAS_OK)
            return FALSE;
        if (!Blind) {
            Your("%s sting.", makeplural(body_part(EYE)));
            make_blinded(1L, FALSE);
        }
        if (!Poison_resistance) {
            pline("%s is burning your %s!", Something,
                  makeplural(body_part(LUNG)));
            You("cough and spit blood!");
            wake_nearto(u.ux, u.uy, 2);
            dam = Maybe_Half_Phys(rnd(dam) + 5);
            if (Half_gas_damage) /* worn towel */
                dam = (dam + 1) / 2;
            losehp(dam, "gas cloud", KILLED_BY_AN);
            monstunseesu(M_SEEN_POISON);
            return FALSE;
        } else {
            You("cough!");
            wake_nearto(u.ux, u.uy, 2);
            monstseesu(M_SEEN_POISON);
            return FALSE;
        }
    } else { /* A monster is inside the cloud */
        mtmp = (struct monst *) p2;

        if (m_poisongas_ok(mtmp) != M_POISONGAS_OK) {
            if (!is_silent(mtmp->data)) {
                if (cansee(mtmp->mx, mtmp->my)
                    || (distu(mtmp->mx, mtmp->my) < 8))
                    pline("%s coughs!", Monnam(mtmp));
                wake_nearto(mtmp->mx, mtmp->my, 2);
            }
            if (heros_fault(reg))
                setmangry(mtmp, TRUE);
            if (haseyes(mtmp->data) && mtmp->mcansee) {
                mtmp->mblinded = 1;
                mtmp->mcansee = 0;
            }
            if (resists_poison(mtmp))
                return FALSE;
            mtmp->mhp -= rnd(dam) + 5;
            if (DEADMONSTER(mtmp)) {
                if (heros_fault(reg))
                    killed(mtmp);
                else
                    monkilled(mtmp, "gas cloud", AD_DRST);
                if (DEADMONSTER(mtmp)) { /* not lifesaved */
                    return TRUE;
                }
            }
        }
    }
    return FALSE; /* Monster is still alive */
}

staticfn boolean
is_hero_inside_gas_cloud(void)
{
    int i;

    for (i = 0; i < svn.n_regions; i++)
        if (hero_inside(gr.regions[i])
            && gr.regions[i]->inside_f == INSIDE_GAS_CLOUD)
            return TRUE;
    return FALSE;
}

/* details of gas cloud creation which are common to create_gas_cloud()
   and create_gas_cloud_selection() */
staticfn void
make_gas_cloud(
    NhRegion *cloud,
    int damage,
    boolean inside_cloud)
{
    if (!gi.in_mklev && !svc.context.mon_moving)
        set_heros_fault(cloud); /* assume player has created it */
    cloud->inside_f = INSIDE_GAS_CLOUD;
    cloud->expire_f = EXPIRE_GAS_CLOUD;
    cloud->arg = cg.zeroany;
    cloud->arg.a_int = damage;
    cloud->visible = TRUE;
    cloud->glyph = cmap_to_glyph(damage ? S_poisoncloud : S_cloud);
    add_region(cloud);

    if (!gi.in_mklev && !inside_cloud && is_hero_inside_gas_cloud()) {
        You("are enveloped in a cloud of %s!",
            /* FIXME: "steam" is wrong if this cloud is just the trail of
               a fog cloud's movement; changing to "vapor" would handle
               that but seems a step backward when it really is steam */
            damage ? "noxious gas" : "steam");
        iflags.last_msg = PLNMSG_ENVELOPED_IN_GAS;
    }
}

/* Create a gas cloud which starts at (x,y) and grows outward from it via
 * breadth-first search.
 * cloudsize is the number of squares the cloud will attempt to fill.
 * damage is how much it deals to afflicted creatures. */
#define MAX_CLOUD_SIZE 150
NhRegion *
create_gas_cloud(
    coordxy x, coordxy y,
    int cloudsize,
    int damage)
{
    NhRegion *cloud;
    int i, j;
    NhRect tmprect;

    /* store visited coords */
    coordxy xcoords[MAX_CLOUD_SIZE];
    coordxy ycoords[MAX_CLOUD_SIZE];
    xcoords[0] = x;
    ycoords[0] = y;
    int curridx;
    int newidx = 1; /* initial spot is already taken */
    boolean inside_cloud = is_hero_inside_gas_cloud();

    /* a single-point cloud on hero and it deals no damage.
       probably a natural cause of being polyed. don't message about it */
    if (!svc.context.mon_moving && u_at(x, y) && cloudsize == 1
        && (!damage
            || (damage && m_poisongas_ok(&gy.youmonst) == M_POISONGAS_OK)))
        inside_cloud = TRUE;

    if (cloudsize > MAX_CLOUD_SIZE) {
        impossible("create_gas_cloud: cloud too large (%d)!", cloudsize);
        cloudsize = MAX_CLOUD_SIZE;
    }

    for (curridx = 0; curridx < newidx; curridx++) {
        if (newidx >= cloudsize)
            break;
        int xx = xcoords[curridx];
        int yy = ycoords[curridx];
        /* Do NOT check for if there is already a gas cloud created at some
         * other time at this position. They can overlap. */

        /* Primitive Fisher-Yates-Knuth shuffle to randomize the order of
         * directions chosen. */
        coord dirs[4] = { {0, -1}, {0, 1}, {-1, 0}, {1, 0} };
        for (i = 4; i > 0; --i) {
            coordxy swapidx = rn2(i);
            coord tmp = dirs[swapidx];

            dirs[swapidx] = dirs[i - 1];
            dirs[i - 1] = tmp;
        }
        int nvalid = 0; /* # of valid adjacent spots */
        for (i = 0; i < 4; ++i) {
            /* try all 4 cardinal directions */
            int dx = dirs[i].x, dy = dirs[i].y;
            boolean isunpicked = TRUE;

            if (valid_cloud_pos(xx + dx, yy + dy)) {
                nvalid++;
                /* don't pick a location we've already picked */
                for (j = 0; j < newidx; ++j) {
                    if (xcoords[j] == xx + dx && ycoords[j] == yy + dy) {
                        isunpicked = FALSE;
                        break;
                    }
                }
                /* randomly disrupt the natural breadth-first search, so that
                 * clouds released in open spaces don't always tend towards a
                 * rhombus shape */
                if (nvalid == 4 && !rn2(2))
                    continue;

                if (isunpicked) {
                    xcoords[newidx] = xx + dx;
                    ycoords[newidx] = yy + dy;
                    newidx++;
                }
            }
            if (newidx >= cloudsize) {
                /* don't try further directions */
                break;
            }
        }
    }
    /* We have now either filled up xcoord and ycoord entirely or run out
       of space.  In either case, newidx is the correct total number of
       coordinates inserted. */
    cloud = create_region((NhRect *) 0, 0);
    for (i = 0; i < newidx; ++i) {
        tmprect.lx = tmprect.hx = xcoords[i];
        tmprect.ly = tmprect.hy = ycoords[i];
        add_rect_to_reg(cloud, &tmprect);
    }
    cloud->ttl = rn1(3, 4);
    /* If cloud was constrained in small space, give it more time to live. */
    cloud->ttl = (cloud->ttl * cloudsize) / newidx;

    make_gas_cloud(cloud, damage, inside_cloud);
    return cloud;
}

/* create a single gas cloud from selection */
NhRegion *
create_gas_cloud_selection(
    struct selectionvar *sel,
    int damage)
{
    NhRegion *cloud;
    NhRect tmprect;
    coordxy x, y;
    NhRect r = cg.zeroNhRect;
    boolean inside_cloud = is_hero_inside_gas_cloud();

    selection_getbounds(sel, &r);

    cloud = create_region((NhRect *) 0, 0);
    for (x = r.lx; x <= r.hx; x++)
        for (y = r.ly; y <= r.hy; y++)
            if (selection_getpoint(x, y, sel)) {
                tmprect.lx = tmprect.hx = x;
                tmprect.ly = tmprect.hy = y;
                add_rect_to_reg(cloud, &tmprect);
            }

    make_gas_cloud(cloud, damage, inside_cloud);
    return cloud;
}


/* for checking troubles during prayer; is hero at risk? */
boolean
region_danger(void)
{
    int i, f_indx, n = 0;

    for (i = 0; i < svn.n_regions; i++) {
        /* only care about regions that hero is in */
        if (!hero_inside(gr.regions[i]))
            continue;
        f_indx = gr.regions[i]->inside_f;
        /* the only type of region we understand is gas_cloud */
        if (f_indx == INSIDE_GAS_CLOUD) {
            /* completely harmless if you don't need to breathe */
            if (nonliving(gy.youmonst.data) || Breathless)
                continue;
            /* minor inconvenience if you're poison resistant;
               not harmful enough to be a prayer-level trouble */
            if (Poison_resistance)
                continue;
            ++n;
        }
    }
    return n ? TRUE : FALSE;
}

/* for fixing trouble at end of prayer;
   danger detected at start of prayer might have expired by now */
void
region_safety(void)
{
    NhRegion *r = 0;
    int i, f_indx, n = 0;

    for (i = 0; i < svn.n_regions; i++) {
        /* only care about regions that hero is in */
        if (!hero_inside(gr.regions[i]))
            continue;
        f_indx = gr.regions[i]->inside_f;
        /* the only type of region we understand is gas_cloud */
        if (f_indx == INSIDE_GAS_CLOUD) {
            if (!n++ && gr.regions[i]->ttl >= 0)
                r = gr.regions[i];
        }
    }

    if (n > 1 || (n == 1 && !r)) {
        /* multiple overlapping cloud regions or non-expiring one */
        (void) safe_teleds(TELEDS_NO_FLAGS);
        /* maybe there's no safe place available; must get hero out of danger
           or prayer's "fix all troubles" result will get stuck in a loop */
        if (region_danger()) {
            set_itimeout(&HMagical_breathing, (long) (d(4, 4) + 4));
            /* not already Breathless or wouldn't be in region danger */
            You_feel("able to breathe.");
        }
    } else if (r) {
        remove_region(r);
        pline_The("gas cloud enveloping you dissipates.");
    } else {
        /* cloud dissipated on its own, so nothing needs to be done */
        pline_The("gas cloud has dissipated.");
    }
    /* maybe cure blindness too */
    if (BlindedTimeout == 1L)
        make_blinded(0L, TRUE);
}
#endif /* !SFCTOOL */

/*region.c*/
