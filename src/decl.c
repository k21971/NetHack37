/* NetHack 3.7	decl.c	$NHDT-Date: 1736530208 2025/01/10 09:30:08 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.341 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Michael Allison, 2009. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

const char * const nhcb_name[NUM_NHCB] = {
    "cmd_before",
    "level_enter",
    "level_leave",
    "end_turn",
};

int nhcb_counts[NUM_NHCB] = DUMMY;
NEARDATA const struct c_color_names c_color_names = {
    "black",  "amber", "golden", "light blue", "red",   "green",
    "silver", "blue",  "purple", "white",      "orange"
};
const char *c_obj_colors[] = {
    "black",          /* CLR_BLACK */
    "red",            /* CLR_RED */
    "green",          /* CLR_GREEN */
    "brown",          /* CLR_BROWN */
    "blue",           /* CLR_BLUE */
    "magenta",        /* CLR_MAGENTA */
    "cyan",           /* CLR_CYAN */
    "gray",           /* CLR_GRAY */
    "transparent",    /* no_color */
    "orange",         /* CLR_ORANGE */
    "bright green",   /* CLR_BRIGHT_GREEN */
    "yellow",         /* CLR_YELLOW */
    "bright blue",    /* CLR_BRIGHT_BLUE */
    "bright magenta", /* CLR_BRIGHT_MAGENTA */
    "bright cyan",    /* CLR_BRIGHT_CYAN */
    "white",          /* CLR_WHITE */
};

const struct c_common_strings c_common_strings =
    { "Nothing happens.",
      "Nothing seems to happen.",
      "That's enough tries!",
      "That is a silly thing to %s.",
      "shudder for a moment.",
      "something",
      "Something",
      "You can move again.",
      "Never mind.",
      "vision quickly clears.",
      { "the", "your" },
      { "mon", "you" }
};

const char disclosure_options[] = "iavgco";
char emptystr[] = {0};       /* non-const */

NEARDATA struct flag flags;  /* extern declaration is in flag.h, not decl.h */

/* Global windowing data, defined here for multi-window-system support */
#ifdef WIN32
boolean fqn_prefix_locked[PREFIX_COUNT] = { FALSE, FALSE, FALSE,
                                            FALSE, FALSE, FALSE,
                                            FALSE, FALSE, FALSE,
                                            FALSE };
#endif
#ifdef PREFIXES_IN_USE
const char *fqn_prefix_names[PREFIX_COUNT] = {
    "hackdir",  "leveldir", "savedir",    "bonesdir",  "datadir",
    "scoredir", "lockdir",  "sysconfdir", "configdir", "troubledir"
};
#endif

/* used by coloratt.c, options.c, utf8map.c, windows.c */
const char hexdd[33] = "00112233445566778899aAbBcCdDeEfF";

/* x/y/z deltas for the 10 movement directions (8 compass pts, 2 down/up) */
const schar xdir[N_DIRS_Z] = { -1, -1,  0,  1,  1,  1,  0, -1, 0,  0 };
const schar ydir[N_DIRS_Z] = {  0, -1, -1, -1,  0,  1,  1,  1, 0,  0 };
const schar zdir[N_DIRS_Z] = {  0,  0,  0,  0,  0,  0,  0,  0, 1, -1 };
/* reordered directions, cardinals first */
const schar dirs_ord[N_DIRS] =
    { DIR_W, DIR_N, DIR_E, DIR_S, DIR_NW, DIR_NE, DIR_SE, DIR_SW };

NEARDATA boolean has_strong_rngseed = FALSE;
struct engr *head_engr;
NEARDATA struct instance_flags iflags;
NEARDATA struct accessibility_data a11y;
/* NOTE: the order of these words exactly corresponds to the
   order of oc_material values #define'd in objclass.h. */
const char *materialnm[] = { "mysterious", "liquid",  "wax",        "organic",
                             "flesh",      "paper",   "cloth",      "leather",
                             "wooden",     "bone",    "dragonhide", "iron",
                             "metal",      "copper",  "silver",     "gold",
                             "platinum",   "mithril", "plastic",    "glass",
                             "gemstone",   "stone" };
const char quitchars[] = " \r\n\033";
const int shield_static[SHIELD_COUNT] = {
    S_ss1, S_ss2, S_ss3, S_ss2, S_ss1, S_ss2, S_ss4, /* 7 per row */
    S_ss1, S_ss2, S_ss3, S_ss2, S_ss1, S_ss2, S_ss4,
    S_ss1, S_ss2, S_ss3, S_ss2, S_ss1, S_ss2, S_ss4,
};
NEARDATA struct you u;
NEARDATA time_t ubirthday;
NEARDATA struct u_realtime urealtime;
NEARDATA struct obj *uwep, *uarm, *uswapwep,
    *uquiver, /* quiver */
    *uarmu, /* under-wear, so to speak */
    *uskin, /* dragon armor, if a dragon */
    *uarmc, *uarmh, *uarms, *uarmg,*uarmf, *uamul,
    *uright, *uleft, *ublindf, *uchain, *uball;
const char vowels[] = "aeiouAEIOU";
NEARDATA winid WIN_MESSAGE, WIN_STATUS, WIN_MAP, WIN_INVEN;
const char ynchars[] = "yn";
const char ynqchars[] = "ynq";
const char ynaqchars[] = "ynaq";
const char ynNaqchars[] = "yn#aq";
const char rightleftchars[] = "rl";
const char hidespinchars[] = "hsq";
NEARDATA long yn_number = 0L;
#ifdef PANICTRACE
const char *ARGV0;
#endif

static const struct Role urole_init_data = {
    { "Undefined", 0 },
    { { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 },
      { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
    "L", "N", "C",
    "Xxx", "home", "locate",
    NON_PM, NON_PM, NON_PM, NON_PM, NON_PM, NON_PM, NON_PM,
    0, 0, 0, 0,
    /* Str Int Wis Dex Con Cha */
    { 7, 7, 7, 7, 7, 7 },
    { 20, 15, 15, 20, 20, 10 },
    /* Init   Lower  Higher */
    { 10, 0, 0, 8, 1, 0 }, /* Hit points */
    { 2, 0, 0, 2, 0, 3 },
    14, /* Energy */
     0,
    10,
     0,
     0,
     4,
    A_INT,
     0,
    -3
};

static const struct Race urace_init_data = {
    "something",
    "undefined",
    "something",
    "Xxx",
    { 0, 0 },
    NON_PM,
    NON_PM,
    NON_PM,
    0,
    0,
    0,
    0,
    /*    Str     Int Wis Dex Con Cha */
    { 3, 3, 3, 3, 3, 3 },
    { STR18(100), 18, 18, 18, 18, 18 },
    /* Init   Lower  Higher */
    { 2, 0, 0, 2, 1, 0 }, /* Hit points */
    { 1, 0, 2, 0, 2, 0 }  /* Energy */
};

struct display_hints disp = { 0 };

static const struct instance_globals_a g_init_a = {
    /* artifact.c */
    /* decl.c */
    UNDEFINED_PTR,  /* afternmv */
    /* detect.c */
    0,              /* already_found_flag */
    /* do.c */
    FALSE,          /* at_ladder */
    /* dog.c */
    UNDEFINED_PTR, /* apelist */
    /* end.c */
    { UNDEFINED_VALUES }, /* amulets */
    /* mon.c */
    UNDEFINED_PTR, /* animal_list */
    UNDEFINED_VALUE, /* animal_list_count */
#ifdef CHANGE_COLOR
    /* options.c */
    { 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U,
      0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U },  /* altpalette[CLR_MAX] */
#endif
    /* pickup.c */
    0, /* A_first_hint */
    0, /* A_second_hint */
    UNDEFINED_VALUE, /* abort_looting */
    /* shk.c */
    FALSE, /* auto_credit */
    /* sounds.c */
    soundlib_nosound, /* enum soundlib_ids active_soundlib */

    /* trap.c */
    { 0, 0, FALSE }, /* acid_ctx */
    TRUE, /* havestate*/
};

static const struct instance_globals_b g_init_b = {
    /* botl.c */
    { { { NULL, NULL, 0L, FALSE, FALSE, 0, ANY_INVALID, { 0 }, { 0 }, NULL, 0, 0, 0
#ifdef STATUS_HILITES
            , UNDEFINED_PTR, UNDEFINED_PTR
#endif
    } }
    }, /* blstats */
    FALSE, /* blinit */
#ifdef STATUS_HILITES
    0L, /* bl_hilite_moves */
#endif
    /* decl.c */
    { 0, 0 }, /* bhitpos */
    UNDEFINED_PTR, /* billobjs */
    /* files.c */
    BONESINIT, /* bones */
    /* hack.c */
    0U, /* bldrpush_oid - last boulder pushed */
    0L, /* bldrpushtime - turn message was given about pushing that boulder */
    /* mkmaze.c */
    { {COLNO, ROWNO, 0, 0}, {COLNO, ROWNO, 0, 0},
            FALSE, FALSE, 0, 0, { 0 } }, /* bughack */
    /* pickup.c */
    FALSE, /* bucx_filter */
    /* zap.c */
    NULL, /* buzzer -- monst that zapped/cast/breathed to initiate buzz() */
    FALSE, /* bot_disabled */

    TRUE, /* havestate*/
};

static const struct instance_globals_c g_init_c = {
    UNDEFINED_VALUES, /* command_queue */
    /* botl.c */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0 }, /* cond_hilites */
    0, /* condmenu_sortorder */
    /* cmd.c */
    UNDEFINED_VALUES, /* Cmd */
    { 0, 0 }, /* clicklook_cc */
    /* decl.c */
    UNDEFINED_VALUES, /* chosen_windowtype */
    0, /* cmd_key */
    0L, /* command_count */
    UNDEFINED_PTR, /* current_wand */
#ifdef DEF_PAGER
    NULL, /* catmore */
#endif
    /* dog.c */
    DUMMY, /* catname */
    /* end.c */
    NULL, /* crash_email */
    NULL, /* crash_name */
    -1, /* crash_urlmax */
    /* symbols.c */
    0,     /* currentgraphics */
    /* files.c */
    NULL, /* cmdline_rcfile */
    NULL, /* config_section_chosen */
    NULL, /* config_section_current */
    FALSE, /* chosen_symset_start */
    FALSE, /* chosen_symset_end */
    /* invent.c */
    WIN_ERR, /* cached_pickinv_win */
    0,       /* core_invent_state */
    /* options.c */
    NULL, /* cmdline_windowsys */
    (struct menucoloring *) 0, /* color_colorings */
    /* pickup.c */
    (struct obj *) 0, /* current_container */
    FALSE, /* class_filter */
    /* questpgr.c */
    UNDEFINED_VALUES, /* cvt_buf */
    /* sounds.c */
    soundlib_nosound, /* chosen_soundlib */
    UNDEFINED_PTR, /* coder */
    /* uhitm.c */
    NON_PM, /* corpsenm_digested */
    FALSE,  /* converted_savefile_loaded */
    TRUE, /* havestate*/
};

static const struct instance_globals_d g_init_d = {
    /* decl.c */
    0, /* doorindex */
    0L, /* done_money */
    0L, /* domove_attempting */
    0L, /* domove_succeeded */
    FALSE, /* defer_see_monsters */
    /* dig.c */
    UNDEFINED_VALUE, /* did_dig_msg */
    /* do.c */
    NULL, /* dfr_pre_msg */
    NULL, /* dfr_post_msg */
    0, /* did_nothing_flag */
    /* dog.c */
    DUMMY, /* dogname */
    /* end.c */
    0L, /* done_seq */
    /* mon.c */
    FALSE, /* disintegested */
    /* objname.c */
    0, /* distantname */
#ifdef DUMPHTML
    FALSE, /* dumping_list */
#endif
    /* pickup.c */
    FALSE, /* decor_fumble_override */
    FALSE, /* decor_levitate_override */
    FALSE, /* deferred_showpaths */
    NULL,  /* deferred_showpaths_dir  */
    TRUE, /* havestate*/
};

static const struct instance_globals_e g_init_e = {
    /* cmd.c */
    WIN_ERR, /* en_win */
    FALSE, /* en_via_menu */
    UNDEFINED_VALUE, /* ext_tlist */
    /* eat.c */
    NULL, /* eatmbuf */
    /* mkmaze.c */
    UNDEFINED_PTR, /* ebubbles */
    /* new */
    0,      /* early_raw_messages */
    TRUE, /* havestate*/
};

static const struct instance_globals_f g_init_f = {
    /* decl.c */
    UNDEFINED_PTR, /* ftrap */
    { NULL }, /* fqn_prefix */
    NULL, /* ffruit */
    /* eat.c */
    FALSE, /* force_save_hs */
    /* mhitm.c */
     FALSE, /* far_noise */
    /* rumors.c */
    0L, /* false_rumor_size */
    0UL, /* false_rumor_start*/
    0L, /* false_rumor_end */
    /* shk.c */
    0L, /* followmsg */
    TRUE, /* havestate*/
};

static const struct instance_globals_g g_init_g = {
    /* display.c */
    { { { 0 } } }, /* gbuf */
    UNDEFINED_VALUES, /* gbuf_start */
    UNDEFINED_VALUES, /* gbug_stop */

    /* do_name.c */
    0, 0, /* getposx, getposy */
    UNDEFINED_PTR, /* gloc_filter_map */
    UNDEFINED_VALUE, /* gloc_filter_floodfill_match_glyph */
    /* dog.c */
    UNDEFINED_VALUE, /* gtyp */
    0, /* gx */
    0, /* gy */
    /* dokick.c */
    NULL, /* gate_str */
    /* end.c */
    { UNDEFINED_VALUES }, /* gems */
    /* invent.c */
    0L,      /* glyph_reset_timestamp */
    /* nhlua.c */
    FALSE, /* gmst_stored */
    0L, /* gmst_moves */
    NULL, /* gmst_invent */
    NULL, NULL, NULL, /* gmst_ubak, gmst_disco, gmst_mvitals */
    { DUMMY }, /* gmst_spl_book */
    /* pline.c */
    UNDEFINED_PTR, /* gamelog */
    /* region.c */
    FALSE, /* gas_cloud_diss_within */
    0, /* gas_cloud_diss_seen */
    /* new */
    /* per-level glyph mapping flags */
    0L,     /* glyphmap_perlevel_flags */
    TRUE, /* havestate*/
};

static const struct instance_globals_h g_init_h = {
    /* decl.c */
    NULL, /* hname */
#if defined(MICRO) || defined(WIN32)
    UNDEFINED_VALUES, /* hackdir */
#endif /* MICRO || WIN32 */
    1L << 3, /* hero_seq: sequence number for hero movement, 'moves*8 + n'
              * where n is usually 1, sometimes 2 when Fast/Very_fast, maybe
              * higher if polymorphed into something that's even faster */
    /* dog.c */
    DUMMY, /* horsename */
    /* mhitu.c */
    0U, /* hitmsg_mid */
    NULL, /* hitmsg_prev */
    /* save.c */
    TRUE, /* havestate*/
};

static const struct instance_globals_i g_init_i = {
    /* decl.c */
    0, /* in_doagain */
    FALSE, /* in_mklev */
    FALSE, /* in_steed_dismounting */
    UNDEFINED_PTR, /* invent */
    /* do_wear.c */
    FALSE, /* initial_don */
    /* invent.c */
    NULL, /* invbuf */
    0U, /* invbufsize */
    0,       /* in_sync_perminvent */
    /* mon.c */
    NULL, /* itermonarr */
    /* restore.c */
    UNDEFINED_PTR, /* id_map */
    /* sp_lev.c */
    FALSE, /* in_mk_themerooms */

    TRUE, /* havestate*/
};

static const struct instance_globals_j g_init_j = {
    /* apply.c */
    0,  /* jumping_is_magic */
    TRUE, /* havestate*/
};

static const struct instance_globals_k g_init_k = {
    { 0, 0 }, /* kickedloc */
    /* decl.c */
    UNDEFINED_PTR, /* kickedobj */
    /* read.c */
    UNDEFINED_VALUE, /* known */
    TRUE, /* havestate*/
};

static const struct instance_globals_l g_init_l = {
    /* cmd.c */
    UNDEFINED_VALUE, /* last_command_count */
    /* decl.c */
#if defined(UNIX) || defined(VMS)
    0, /* locknum */
#endif
#ifdef MICRO
    UNDEFINED_VALUES, /* levels */
#endif /* MICRO */
    /* files.c */
    UNDEFINED_VALUE, /* lockptr */
    LOCKNAMEINIT, /* lock */
    /* invent.c */
    51, /* lastinvr */
    /* light.c */
    UNDEFINED_PTR, /* light_base */
    /* mklev.c */
    { UNDEFINED_PTR }, /* luathemes[] */
    /* mon.c */
    0U, /* last_hider */
    /* nhlan.c */
#ifdef MAX_LAN_USERNAME
    UNDEFINED_VALUES, /* lusername */
    MAX_LAN_USERNAME, /* lusername_size */
#endif /* MAX_LAN_USERNAME */
    /* nhlua.c */
    UNDEFINED_VALUE, /* luacore */
    DUMMY, /* lua_warnbuf[] */
    0, /* loglua */
    0, /* lua_sid */
    /* options.c */
    FALSE, /* loot_reset_justpicked */
    /* save.c */
    (struct obj *) 0, /* looseball */
    (struct obj *) 0, /* loosechain */
    /* sp_lev.c */
    NULL, /* lev_message */
    UNDEFINED_PTR, /* lregions */
    /* trap.c */
    { UNDEFINED_PTR, 0, 0 }, /* launchplace */
    /* windows.c */
    UNDEFINED_PTR, /* last_winchoice */
    /* new */
    DUMMY,   /* lua_ver[LUA_VER_BUFSIZ] */
    DUMMY,   /* lua_copyright[LUA_COPYRIGHT_BUFSIZ] */
    TRUE, /* havestate*/
};

static const struct instance_globals_m g_init_m = {
    /* apply.c */
    0, /* mkot_trap_warn_count */
    /* botl.c */
    0,  /* mrank_sz */
    /* decl.c */
    0, /* multi */
    NULL, /* multi_reason */
    /* multi_reason usually points to a string literal (when not Null)
       but multireasonbuf[] is available for when it needs to be dynamic */
    DUMMY, /* multireasonbuf[] */
    { 0, 0, STRANGE_OBJECT, FALSE }, /* m_shot */
    FALSE, /* mrg_to_wielded */
    UNDEFINED_PTR, /* menu_colorings */
    UNDEFINED_PTR, /* migrating_objs */
    /* dog.c */
    UNDEFINED_PTR, /* mydogs */
    UNDEFINED_PTR, /* migrating_mons */
    /* dokick.c */
    UNDEFINED_PTR, /* maploc */
    /* mhitu.c */
    UNDEFINED_VALUE, /* mhitu_dieroll */
    /* mklev.c */
    FALSE, /* made_branch */
    /* mkmap.c */
    UNDEFINED_VALUE, /* min_rx */
    UNDEFINED_VALUE, /* max_rx */
    UNDEFINED_VALUE, /* min_ry */
    UNDEFINED_VALUE, /* max_ry */
    /* mkobj.c */
    FALSE, /* mkcorpstat_norevive */
    /* mthrowu.c */
    UNDEFINED_VALUE, /* mesg_given */
    UNDEFINED_PTR, /* mtarget */
    UNDEFINED_PTR, /* marcher */
    /* muse.c */
    FALSE, /* m_using */
    UNDEFINED_VALUES, /* m */
    /* options.c */
    UNDEFINED_VALUES, /* mapped_menu_cmds */
    UNDEFINED_VALUES, /* mapped_menu_op */
    /* region.c */
    0, /* max_regions */
    /* trap.c */
    FALSE, /* mentioned_water */
    TRUE, /* havestate*/
};

static const struct instance_globals_n g_init_n = {
    /* botl.c */
    0, /* now_or_before_idx */
    /* decl.c */
    NULL, /* nomovemsg */
    0, /* nsubroom */
    /* dokick.c */
    UNDEFINED_VALUES, /* nowhere */
    /* files.c */
    0, /* nesting */
    0, /* no_sound_notified */
    /* mhitm.c */
    0L, /* noisetime */
    /* mkmap.c */
    UNDEFINED_PTR, /* new_locations */
    UNDEFINED_VALUE, /* n_loc_filled */
    /* options.c */
    0, /* n_menu_mapped */
    /* potion.c */
    FALSE, /* notonhead */
    /* questpgr.c */
    UNDEFINED_VALUES, /* nambuf */
    /* restore.c */
    0, /* n_ids_mapped */
    /* sp_lev.c */
    0, /* num_lregions */
    /* u_init.c */
    STRANGE_OBJECT, /* nocreate */
    STRANGE_OBJECT, /* nocreate2 */
    STRANGE_OBJECT, /* nocreate3 */
    STRANGE_OBJECT, /* nocreate4 */
    TRUE, /* havestate*/
};

static const struct instance_globals_o g_init_o = {
    NULL, /* objs_deleted */
    /* dbridge.c */
    { { 0 } }, /* occupants */
    /* decl.c */
    UNDEFINED_PTR, /* occupation */
    0, /* occtime */
    UNDEFINED_VALUE, /* otg_temp */
    NULL, /* otg_otmp */
    NULL, /* occtxt */
    /* symbols.c */
    DUMMY, /* ov_primary_syms */
    DUMMY, /* ov_rogue_syms */
    /* invent.c */
    UNDEFINED_VALUES, /* only (coord) */
    /* o_init.c */
    DUMMY, /* oclass_prob_totals */
    /* options.c */
    0, /* opt_phase */
    FALSE, /* opt_initial */
    FALSE, /* opt_from_file */
    FALSE, /* opt_need_redraw */
    FALSE, /* opt_need_glyph_reset */
    FALSE, /* opt_need_promptstyle */
    FALSE, /* opt_reset_customcolors */
    FALSE, /* opt_reset_customsymbols */
    FALSE, /* opt_update_basic_palette */
    FALSE, /* opt_symset_changed */
    /* pickup.c */
    0,  /* oldcap */
    /* restore.c */
    UNDEFINED_PTR, /* oldfruit */
    /* rumors.c */
    0, /* oracle_flag */
    /* uhitm.c */
    FALSE, /* override_confirmation */
    /* zap.c */
    FALSE,  /* obj_zapped */
    TRUE, /* havestate*/
};

static const struct instance_globals_p g_init_p = {
    /* apply.c */
    -1, /* polearm_range_min */
    -1, /* polearm_range_max  */
    /* decl.c */
    0, /* plnamelen */
    '\0', /* pl_race */
    UNDEFINED_PTR, /* plinemsg_types */
    /* dog.c */
    0,  /* petname_used */
    UNDEFINED_VALUE, /* preferred_pet */
    /* symbols.c */
    DUMMY, /* primary_syms */
    /* invent.c */
    0,       /* perm_invent_toggling_direction */
    /* pickup.c */
    FALSE, /* picked_filter */
    0, /* pickup_encumbrance */
    /* pline.c */
    0U, /* pline_flags */
    UNDEFINED_VALUES, /* prevmsg */
    /* potion.c */
    UNDEFINED_VALUE, /* potion_nothing */
    UNDEFINED_VALUE, /* potion_unkn */
    /* pray.c */
    UNDEFINED_VALUE, /* p_aligntyp */
    UNDEFINED_VALUE, /* p_trouble */
    UNDEFINED_VALUE, /* p_type */
    /* weapon.c */
    UNDEFINED_PTR, /* propellor */
    /* zap.c */
    UNDEFINED_VALUE, /* poly_zap */
    TRUE, /* havestate*/
};

static const struct instance_globals_q g_init_q = {
    TRUE, /* havestate*/
};

static const struct instance_globals_r g_init_r = {
    /* symbols.c */
    DUMMY, /* rogue_syms */
    /* extralev.c */
    { { UNDEFINED_VALUES } }, /* r */
    /* mkmaze.c */
    FALSE, /* ransacked */
    /* region.c */
    UNDEFINED_PTR, /* regions */
    /* rip.c */
    UNDEFINED_PTR, /* rip */
    /* role.c */
    UNDEFINED_VALUES, /* role_pa */
    UNDEFINED_VALUE, /* role_post_attrib */
    { { 0 }, 0 }, /* rfilter */
    /* shk.c */
    UNDEFINED_VALUES, /* repo */
    TRUE, /* havestate*/
};

static const struct instance_globals_s g_init_s = {
    /* artifact.c */
    0,  /* spec_dbon_applies */
    /* decl.c */
    UNDEFINED_PTR, /* stairs */
    DUMMY, /* smeq */
    FALSE, /* stoned */
    UNDEFINED_PTR, /* subrooms */
    /* do.c */
    { 0, 0 }, /* save_dlevel */
    /* symbols.c */
    { DUMMY }, /* symset */
    { { { 0 } }, { { 0 } } }, /* symset_customizations */
    DUMMY, /* showsyms */
    /* files.c */
    0, /* symset_count */
    0, /* symset_which_set */
    DUMMY, /* SAVEF */
#ifdef MICRO
    DUMMY, /* SAVEP */
#endif
    /* invent.c */
    0, /* sortloogmode */
    /* mhitm.c */
    FALSE, /* skipdrin */
    /* mon.c */
    FALSE, /* somebody_can_move */
    /* options.c */
    (struct symsetentry *) 0, /* symset_list */
    FALSE, /* save_menucolors */
    (struct menucoloring *) 0, /* save_colorings */
    FALSE, /* simple_options_help */
    /* pickup.c */
    FALSE, /* sellobj_first */
    FALSE, /* shop_filter */
    /* pline.c */
#ifdef DUMPLOG_CORE
    0U, /* saved_pline_index */
    { NULL }, /* saved_plines */
#endif
    /* polyself.c */
    0, /* sex_change_ok */
    /* shk.c */
    'a', /* sell_response */
    SELL_NORMAL, /* sell_how */
    /* spells.c */
    0, /* spl_sortmode */
    UNDEFINED_PTR, /* spl_orderindx */
    /* steal.c */
    0U, /* stealoid */
    0U, /* stealmid */
    /* vision.c */
    0, /* seethru */
    TRUE, /* havestate*/
};

static const struct instance_globals_t g_init_t = {
    /* apply.c */
    UNDEFINED_VALUES, /* trapinfo */
    /* decl.c */
    0, /* tbx */
    0, /* tby */
    UNDEFINED_VALUES, /* toplines */
    UNDEFINED_PTR, /* thrownobj */
    DUMMY, /* tc_gbl_data */
    /* hack.c */
    UNDEFINED_VALUES, /* tmp_anything */
    UNDEFINED_PTR, /* travelmap */
    /* invent.c */
    0, /* this_type */
    NULL, /* this_title */
    /* muse.c */
    UNDEFINED_VALUE, /* trapx */
    UNDEFINED_VALUE, /* trapy */
    /* rumors.c */
    0L, /* true_rumor_size */
    0UL, /* true_rumor_start*/
    0L, /* true_rumor_end */
    /* sp_lev.c */
    FALSE, /* themeroom_failed */
    /* timeout.c */
    UNDEFINED_PTR, /* timer_base */
    /* topten.c */
    WIN_ERR, /* toptenwin */
    /* uhitm.c */
    0, /* twohits */
    /**/
    TRUE, /* havestate*/
};

static const struct instance_globals_u g_init_u = {
    /* botl.c */
    FALSE, /* update_all */
    /* decl.c */
    FALSE, /* unweapon */
    /* role.c */
    UNDEFINED_ROLE, /* urole */
    UNDEFINED_RACE, /* urace */
    /* save.c */
    { 0, 0 }, /* uz_save */
    TRUE, /* havestate*/
};

static const struct instance_globals_v g_init_v = {
    /* botl.c */
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, /* valset */
    /* end.c */
    { UNDEFINED_VALUES }, /* valuables */
    /* mhitm.c */
    FALSE, /* vis */
    /* mklev.c */
    UNDEFINED_VALUE, /* vault_x */
    UNDEFINED_VALUE, /* vault_y */
    /* mon.c */
    FALSE, /* vamp_rise_msg */
    /* pickup.c */
    0L, /* val_for_n_or_more */
    UNDEFINED_VALUES, /* valid_menu_classes */
    /* vision.c */
    UNDEFINED_PTR, /* viz_array */
    UNDEFINED_PTR, /* viz_rmin */
    UNDEFINED_PTR, /* viz_rmax */
    FALSE, /* vision_full_recalc */
    UNDEFINED_VALUES,  /* voice */
    TRUE, /* havestate*/
};

static const struct instance_globals_w g_init_w = {
    /* decl.c */
    0, /* warn_obj_cnt */
    0L, /* wailmsg */
    /* do_wear.c */
    0U, /* wasinwater */
    /* symbols.c */
    DUMMY, /* warnsyms */
    /* files.c */
    UNDEFINED_VALUES, /* wizkit */
    /* hack.c */
    UNDEFINED_VALUE, /* wc */
    /* mkmaze.c */
    UNDEFINED_PTR, /* wportal */
    /* new */
    { wdmode_traditional, NO_COLOR },       /* wsettings */
    0L,                                     /* were.c, allmain.c */
    TRUE, /* havestate*/
};

static const struct instance_globals_x g_init_x = {
    /* decl.c */
    (COLNO - 1) & ~1, /* x_maze_max */
    /* lock.c */
    UNDEFINED_VALUES,  /* xlock */
    /* objnam.c */
    NULL, /* xnamep */
    /* sp_lev.c */
    UNDEFINED_VALUE, /* xstart */
    UNDEFINED_VALUE, /* xsize */
    TRUE, /* havestate*/
};

static const struct instance_globals_y g_init_y = {
    /* decl.c */
    (ROWNO - 1) & ~1, /* y_maze_max */
    DUMMY, /* youmonst */
    /* pline.c */
    NULL, /* you_buf */
    0, /* you_buf_siz */
    /* sp_lev.c */
    UNDEFINED_VALUE, /* ystart */
    UNDEFINED_VALUE, /* ysize */
    TRUE, /* havestate*/
};

static const struct instance_globals_z g_init_z = {
    /* mon.c */
    FALSE, /* zombify */
    /* muse.c */
    FALSE, /* zap_oseen */
    TRUE, /* havestate*/
};

static const struct instance_globals_saved_b init_svb = {
    /* dungeon.c */
    UNDEFINED_PTR,                       /* branches */
    /* mkmaze.c */
    UNDEFINED_PTR,                       /* bbubbles */
    DUMMY                                /* bases */
};

static const struct instance_globals_saved_c init_svc = {
    /* decl.c */
    DUMMY,                               /* context */
};

static const struct instance_globals_saved_d init_svd = {
    /* dungeon.c */
    { { {0},{0},{0},{0}, 0, {0}, 0, 0, 0, 0, 0 } }, /* dungeons */
    { {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0},
    {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0},
    0, 0, 0, 0, 0,
    {0}, {0}, {0},
    {0}, {0}, {0} },                     /* dungeon_topology */
    /* decl.c */
    { 0, 0, 0, 0, 0, 0, 0, 0 },          /* dndest */
    NULL,                                /* doors */
    0,                                   /* doors_alloc */
    /* o_init.c */
    DUMMY,                               /* disco */
};

static const struct instance_globals_saved_e init_sve = {
    /* decl.c */
    NULL                                 /* exclusion_zones */
};

static const struct instance_globals_saved_h init_svh = {
    /* decl.c */
    0                                    /* hackpid */
};

static const struct instance_globals_saved_i init_svi = {
    /* decl.c */
    { 0, 0 }                             /* inv_pos */
};

static const struct instance_globals_saved_k init_svk = {
    /* decl.c */
    DUMMY                                /* killer */
};

static const struct instance_globals_saved_l init_svl = {
    /* decl.c */
    { { 0 } },                             /* lastseentyp */
    { { { UNDEFINED_VALUES } },            /* level.locations */
      { { UNDEFINED_PTR } },               /* level.objects   */
      { { UNDEFINED_PTR } },               /* level.monsters  */
      NULL, NULL, NULL, NULL, NULL, {0} }, /* level */
    { UNDEFINED_VALUES }                   /* level_info */
};

static const struct instance_globals_saved_m init_svm = {
    /* dungeon.c */
    UNDEFINED_PTR,                       /* mapseenchn */
    /* decl.c */
    0L,                                  /* moves; misnamed turn counter */
    { UNDEFINED_VALUES }                 /* mvitals */
};

static const struct instance_globals_saved_n init_svn = {
    /* dungeon.c */
    0,                                   /* n_dgns */
    /* mkroom.c */
    0,                                   /* nroom */
    /* region.c */
    0                                    /* n_regions */
};

static const struct instance_globals_saved_o init_svo = {
    /* rumors.c */
    0U,                                  /* oracle_cnt */
    UNDEFINED_PTR,                       /* oracle_loc */

    /* other */
    0L                                   /* omoves */
};

static const struct instance_globals_saved_p init_svp = {
    /* decl.c */
    DUMMY,                               /* plname */
    DUMMY,                               /* pl_character */
    DUMMY,                               /* pl_fruit */
};

static const struct instance_globals_saved_q init_svq = {
    /* quest.c */
    DUMMY                                /* quest_status */
};

static const struct instance_globals_saved_r init_svr = {
    /* mkroom.c */
    { DUMMY },                           /* rooms */
};

static const struct instance_globals_saved_s init_svs = {
    /* decl.c */
    { DUMMY },                           /* spl_book */
    UNDEFINED_PTR                        /* sp_levchn */
};

static const struct instance_globals_saved_t init_svt = {
    /* decl.c */
    DUMMY,                               /* tune */
    /* timeout.c */
    1UL,                                 /* timer_id */
};

static const struct instance_globals_saved_u init_svu = {
    /* decl.c */
    { 0, 0, 0, 0, 0, 0, 0, 0 },          /* updest */
};

static const struct instance_globals_saved_x init_svx = {
    /* mkmaze.c */
    UNDEFINED_VALUE,                     /* xmin */
    UNDEFINED_VALUE                      /* xmax */
};

static const struct instance_globals_saved_y init_svy = {
    /* mkmaze.c */
    UNDEFINED_VALUE,                     /* ymin */
    UNDEFINED_VALUE                      /* ymax */
};

static const struct sinfo init_program_state = { 0 };

#if 0
struct instance_globals g;
#endif /* 0 */

struct instance_globals_a ga;
struct instance_globals_b gb;
struct instance_globals_c gc;
struct instance_globals_d gd;
struct instance_globals_e ge;
struct instance_globals_f gf;
struct instance_globals_g gg;
struct instance_globals_h gh;
struct instance_globals_i gi;
struct instance_globals_j gj;
struct instance_globals_k gk;
struct instance_globals_l gl;
struct instance_globals_m gm;
struct instance_globals_n gn;
struct instance_globals_o go;
struct instance_globals_p gp;
struct instance_globals_q gq;
struct instance_globals_r gr;
struct instance_globals_s gs;
struct instance_globals_t gt;
struct instance_globals_u gu;
struct instance_globals_v gv;
struct instance_globals_w gw;
struct instance_globals_x gx;
struct instance_globals_y gy;
struct instance_globals_z gz;
struct instance_globals_saved_b svb;
struct instance_globals_saved_c svc;
struct instance_globals_saved_d svd;
struct instance_globals_saved_e sve;
struct instance_globals_saved_h svh;
struct instance_globals_saved_i svi;
struct instance_globals_saved_k svk;
struct instance_globals_saved_l svl;
struct instance_globals_saved_m svm;
struct instance_globals_saved_n svn;
struct instance_globals_saved_o svo;
struct instance_globals_saved_p svp;
struct instance_globals_saved_q svq;
struct instance_globals_saved_r svr;
struct instance_globals_saved_s svs;
struct instance_globals_saved_t svt;
struct instance_globals_saved_u svu;
struct instance_globals_saved_x svx;
struct instance_globals_saved_y svy;
struct sinfo program_state;

const struct const_globals cg = {
    DUMMY, /* zeroobj */
    DUMMY, /* zeromonst */
    DUMMY, /* zeroany */
    DUMMY, /* zeroNhRect */
};

#define ZERO(x) memset(&x, 0, sizeof(x))

#define MAGICCHECK(xx) \
    do {                                                                   \
        if ((xx).havestate != TRUE) {                                      \
            raw_printf(                                                    \
                 "decl_globals_init: %s.havestate not True.", #xx);        \
            exit(1);                                                       \
        }                                                                  \
    } while(0);

void
decl_globals_init(void)
{
#if 0
    g = g_init;
#endif
    ga = g_init_a;
    gb = g_init_b;
    gc = g_init_c;
    gd = g_init_d;
    ge = g_init_e;
    gf = g_init_f;
    gg = g_init_g;
    gh = g_init_h;
    gi = g_init_i;
    gj = g_init_j;
    gk = g_init_k;
    gl = g_init_l;
    gm = g_init_m;
    gn = g_init_n;
    go = g_init_o;
    gp = g_init_p;
    gq = g_init_q;
    gr = g_init_r;
    gs = g_init_s;
    gt = g_init_t;
    gu = g_init_u;
    gv = g_init_v;
    gw = g_init_w;
    gx = g_init_x;
    gy = g_init_y;
    gz = g_init_z;
    svb = init_svb;
    svc = init_svc;
    svd = init_svd;
    sve = init_sve;
    svh = init_svh;
    svi = init_svi;
    svk = init_svk;
    svl = init_svl;
    svm = init_svm;
    svn = init_svn;
    svo = init_svo;
    svp = init_svp;
    svq = init_svq;
    svr = init_svr;
    svs = init_svs;
    svt = init_svt;
    svu = init_svu;
    svx = init_svx;
    svy = init_svy;
    program_state = init_program_state;

    gv.valuables[0].list = gg.gems;
    gv.valuables[0].size = SIZE(gg.gems);
    gv.valuables[1].list = ga.amulets;
    gv.valuables[1].size = SIZE(ga.amulets);
    gv.valuables[2].list = NULL;
    gv.valuables[2].size = 0;

#if 0
    MAGICCHECK(g_init);
#endif
    MAGICCHECK(g_init_a);
    MAGICCHECK(g_init_b);
    MAGICCHECK(g_init_c);
    MAGICCHECK(g_init_d);
    MAGICCHECK(g_init_e);
    MAGICCHECK(g_init_f);
    MAGICCHECK(g_init_g);
    MAGICCHECK(g_init_h);
    MAGICCHECK(g_init_i);
    MAGICCHECK(g_init_j);
    MAGICCHECK(g_init_k);
    MAGICCHECK(g_init_l);
    MAGICCHECK(g_init_m);
    MAGICCHECK(g_init_n);
    MAGICCHECK(g_init_o);
    MAGICCHECK(g_init_p);
    MAGICCHECK(g_init_q);
    MAGICCHECK(g_init_r);
    MAGICCHECK(g_init_s);
    MAGICCHECK(g_init_t);
    MAGICCHECK(g_init_u);
    MAGICCHECK(g_init_v);
    MAGICCHECK(g_init_w);
    MAGICCHECK(g_init_x);
    MAGICCHECK(g_init_y);
    MAGICCHECK(g_init_z);

    gs.subrooms = &svr.rooms[MAXNROFROOMS + 1];

    ZERO(flags);
    ZERO(iflags);
    ZERO(a11y);
    ZERO(disp);
    ZERO(u);
    ZERO(ubirthday);
    ZERO(urealtime);

    uwep = uarm = uswapwep = uquiver = uarmu = uskin = uarmc = NULL;
    uarmh = uarms = uarmg = uarmf = uamul = uright = uleft = NULL;
    ublindf = uchain = uball = NULL;

    WIN_MESSAGE =  WIN_STATUS =  WIN_MAP = WIN_INVEN = WIN_ERR;

    gu.urole = urole_init_data;
    gu.urace = urace_init_data;
}

/* fields in 'hands_obj' don't matter, just its distinct address */
struct obj hands_obj = DUMMY;

/* gcc 12.2's static analyzer thinks that some fields of svc.context.victual
   are uninitialized when compiling 'bite(eat.c)' but that's impossible;
   it is defined at global scope so guaranteed to be given implicit
   initialization for fields that aren't explicitly initialized (all of
   'context'); having bite() pass &svc.context.victual to this no-op
   eliminates the analyzer's very verbose complaint */
void
sa_victual(
    volatile struct victual_info *context_victual UNUSED)
{
    return;
}

/*decl.c*/
