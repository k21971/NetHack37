/* NetHack 3.7  decl.h  $NHDT-Date: 1725653004 2024/09/06 20:03:24 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.377 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Michael Allison, 2007. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef DECL_H
#define DECL_H

/* The names of the colors used for gems, etc. */
extern const char *c_obj_colors[];

/* lua callback queue names */
extern const char * const nhcb_name[];
extern int nhcb_counts[];

extern NEARDATA const struct c_color_names c_color_names;
#define NH_BLACK c_color_names.c_black
#define NH_AMBER c_color_names.c_amber
#define NH_GOLDEN c_color_names.c_golden
#define NH_LIGHT_BLUE c_color_names.c_light_blue
#define NH_RED c_color_names.c_red
#define NH_GREEN c_color_names.c_green
#define NH_SILVER c_color_names.c_silver
#define NH_BLUE c_color_names.c_blue
#define NH_PURPLE c_color_names.c_purple
#define NH_WHITE c_color_names.c_white
#define NH_ORANGE c_color_names.c_orange

/* common_strings */
extern const struct c_common_strings c_common_strings;
#define nothing_happens c_common_strings.c_nothing_happens
#define nothing_seems_to_happen c_common_strings.c_nothing_seems_to_happen
#define thats_enough_tries c_common_strings.c_thats_enough_tries
#define silly_thing_to c_common_strings.c_silly_thing_to
#define shudder_for_moment c_common_strings.c_shudder_for_moment
#define something c_common_strings.c_something
#define Something c_common_strings.c_Something
#define You_can_move_again c_common_strings.c_You_can_move_again
#define Never_mind c_common_strings.c_Never_mind
#define vision_clears c_common_strings.c_vision_clears
#define the_your c_common_strings.c_the_your
/* fakename[] used occasionally so vtense() won't be fooled by an assigned
   name ending in 's' */
#define fakename c_common_strings.c_fakename

/* default object class symbols */
extern const struct class_sym def_oc_syms[MAXOCLASSES];

/* default mon class symbols */
extern const struct class_sym def_monsyms[MAXMCLASSES];

extern const char disclosure_options[];

/* empty string that is non-const for parameter use */
extern char emptystr[];

#ifdef WIN32
extern boolean fqn_prefix_locked[PREFIX_COUNT];
#endif
#ifdef PREFIXES_IN_USE
extern const char *fqn_prefix_names[PREFIX_COUNT];
#endif

extern NEARDATA boolean has_strong_rngseed;
extern struct engr *head_engr;

/* used by coloratt.c, options.c, utf8map.c, windows.c */
extern const char hexdd[33];

/* material strings */
extern const char *materialnm[];

/* current mon class symbols */
extern uchar monsyms[MAXMCLASSES];

/* current object class symbols */
extern uchar oc_syms[MAXOCLASSES];

extern const char quitchars[];
extern NEARDATA char tune[6];
extern const schar xdir[], ydir[], zdir[], dirs_ord[];
extern const char vowels[];
extern const char ynchars[];
extern const char ynqchars[];
extern const char ynaqchars[];
extern const char ynNaqchars[];
extern const char rightleftchars[];
extern const char hidespinchars[];
extern NEARDATA long yn_number;
extern struct restore_info restoreinfo;
extern NEARDATA struct savefile_info sfcap, sfrestinfo, sfsaveinfo;
extern const int shield_static[];

extern NEARDATA struct obj *uarm, *uarmc, *uarmh, *uarms, *uarmg, *uarmf,
    *uarmu, /* under-wear, so to speak */
    *uskin, *uamul, *uleft, *uright, *ublindf, *uwep, *uswapwep, *uquiver;
extern NEARDATA struct obj *uchain; /* defined only when punished */
extern NEARDATA struct obj *uball;
extern NEARDATA struct you u;
extern NEARDATA time_t ubirthday;
extern NEARDATA struct u_realtime urealtime;

/* Window system stuff */
extern NEARDATA winid WIN_MESSAGE;
extern NEARDATA winid WIN_STATUS;
extern NEARDATA winid WIN_MAP, WIN_INVEN;

#ifndef TCAP_H
extern struct tc_gbl_data {   /* also declared in tcap.h */
    char *tc_AS, *tc_AE; /* graphics start and end (tty font swapping) */
    int tc_LI, tc_CO;    /* lines and columns */
} tc_gbl_data;
#define AS gt.tc_gbl_data.tc_AS
#define AE gt.tc_gbl_data.tc_AE
#define LI gt.tc_gbl_data.tc_LI
#define CO gt.tc_gbl_data.tc_CO
#endif

#ifdef PANICTRACE
extern const char *ARGV0;
#endif

struct display_hints {
    boolean botl;            /* partially redo status line */
    boolean botlx;           /* print an entirely new bottom line */
    boolean time_botl;       /* context.botl for 'time' (moves) only */
};
extern struct display_hints disp;

/*
 * 'gX' -- instance_globals holds engine state that does not need to be
 * persisted upon game exit.  The initialization state is well defined
 * and set in decl.c during early early engine initialization.
 *
 * Unlike instance_flags, values in the structure can be of any type.
 *
 * Pulled from other files to be grouped in one place.  Some comments
 * which came with them don't make much sense out of their original context.
 */

struct instance_globals_a {
    /* decl.c */
    int (*afternmv)(void);

    /* detect.c */
    int already_found_flag; /* used to augment first "already found a monster"
                             * message if 'cmdassist' is Off */
    /* do.c */
    boolean at_ladder;

    /* dog.c */
    struct autopickup_exception *apelist;

    /* end.c */
    struct valuable_data amulets[LAST_AMULET + 1 - FIRST_AMULET];

    /* mon.c */
    short *animal_list; /* list of PM values for animal monsters */
    int animal_list_count;

#ifdef CHANGE_COLOR
    /* options.c */
    uint32 altpalette[CLR_MAX];
#endif

    /* pickup.c */
    int A_first_hint; /* menustyle:Full plus 'A' response + !paranoid:A */
    int A_second_hint; /* menustyle:Full plus 'A' response + paranoid:A */
    boolean abort_looting;

    /* shk.c */
    boolean auto_credit;

    /* sounds.c */
    enum soundlib_ids active_soundlib;

    /* trap.c */
    /* context for water_damage(), managed by water_damage_chain();
        when more than one stack of potions of acid explode while processing
        a chain of objects, use alternate phrasing after the first message */
    struct h2o_ctx acid_ctx;

    boolean havestate;
};

struct instance_globals_b {

    /* botl.c */
    struct istat_s blstats[2][MAXBLSTATS];
    boolean blinit;
#ifdef STATUS_HILITES
    long bl_hilite_moves;
#endif

    /* decl.c */
    coord bhitpos; /* place where throw or zap hits or stops */
    struct obj *billobjs; /* objects not yet paid for */

    /* files.c */
    char bones[BONESSIZE];

    /* hack.c */
    unsigned bldrpush_oid; /* id of last boulder pushed */
    long bldrpushtime;     /* turn that a message was given for pushing
                            * a boulder; used in lieu of Norep() */

    /* mkmaze.c */
    lev_region bughack; /* for preserving the insect legs when wallifying
                         * baalz level */

    /* pickup.c */
    boolean bucx_filter;

    /* zap.c */
    struct monst *buzzer; /* zapper/caster/breather who initiates buzz() */

    /* new */
    boolean bot_disabled;

    boolean havestate;
};

struct instance_globals_c {

    struct _cmd_queue *command_queue[NUM_CQS];

    /* botl.c */
    unsigned long cond_hilites[BL_ATTCLR_MAX];
    int condmenu_sortorder;

    /* cmd.c */
    struct cmd Cmd; /* flag.h */
    /* Provide a means to redo the last command.  The flag `in_doagain'
       (decl.c below) is set to true while redoing the command.  This flag
       is tested in commands that require additional input (like `throw'
       which requires a thing and a direction), and the input prompt is
       not shown.  Also, while in_doagain is TRUE, no keystrokes can be
       saved into the saveq. */
    coord clicklook_cc;
    /* decl.c */
    char chosen_windowtype[WINTYPELEN];
    int cmd_key; /* parse() / rhack() */
    cmdcount_nht command_count;
    /* some objects need special handling during destruction or placement */
    struct obj *current_wand;  /* wand currently zapped/applied */
#ifdef DEF_PAGER
    const char *catmore; /* external pager; from getenv() or DEF_PAGER */
#endif

    /* dog.c */
    char catname[PL_PSIZ];

    /* end.c */
    char *crash_email;  // email for crash reports
    char *crash_name;   // human name for crash reports
    int crash_urlmax;   // maximum length for the url of a crash report

    /* symbols.c */
    int currentgraphics;

    /* files.c, cfgfiles.c */
    char *cmdline_rcfile;  /* set in unixmain.c, used in options.c */
    char *config_section_chosen;
    char *config_section_current;
    boolean chosen_symset_start;
    boolean chosen_symset_end;

    /* invent.c */
    /* for perm_invent when operating on a partial inventory display, so that
       persistent one doesn't get shrunk during filtering for item selection
       then regrown to full inventory, possibly being resized in process */
    winid cached_pickinv_win;
    int core_invent_state;

    /* options.c */
    char *cmdline_windowsys; /* set in unixmain.c */
    struct menucoloring *color_colorings; /* alternate set of menu colors */

    /* pickup.c */
    /* current_container is set in use_container(), to be used by the
       callback routines in_container() and out_container() from askchain()
       and use_container().  Also used by menu_loot() and container_gone(). */
    struct obj *current_container;
    boolean class_filter;

    /* questpgr.c */
    char cvt_buf[CVT_BUF_SIZE];

    /* sounds.c */
    enum soundlib_ids chosen_soundlib;

    /* sp_lev.c */
    struct sp_coder *coder;

    /* uhitm.c */
    short corpsenm_digested; /* monster type being digested, set by gulpum */

    /* zap.c */
    /* new */
    boolean converted_savefile_loaded;

    boolean havestate;
};

struct instance_globals_d {

    /* decl.c */
    int doorindex;
    long done_money;
    long domove_attempting;
    long domove_succeeded;
#define DOMOVE_WALK         0x00000001
#define DOMOVE_RUSH         0x00000002
    boolean defer_see_monsters;

    /* dig.c */
    boolean did_dig_msg;

    /* do.c */
    char *dfr_pre_msg;  /* pline() before level change */
    char *dfr_post_msg; /* pline() after level change */
    int did_nothing_flag; /* to augment the no-rest-next-to-monster message */

    /* dog.c */
    char dogname[PL_PSIZ];

    /* end.c */
    long done_seq; /* for counting deaths occurring on same hero_seq */

    /* mon.c */
    boolean disintegested;

    /* objname.c */
    /* distantname used by distant_name() to pass extra information to
       xname_flags(); it would be much cleaner if this were a parameter,
       but that would require all xname() and doname() calls to be modified */
    int distantname;

#ifdef DUMPHTML
    boolean dumping_list;
#endif
    /* pickup.c */
    boolean decor_fumble_override;
    boolean decor_levitate_override;

    /* new */
    boolean deferred_showpaths;
    char *deferred_showpaths_dir;

    boolean havestate;
};

struct instance_globals_e {

    /* cmd.c */
    winid en_win;
    boolean en_via_menu;
    struct ext_func_tab *ext_tlist; /* info for rhack() from doextcmd() */

    /* eat.c */
    char *eatmbuf; /* set by cpostfx() */

    /* mkmaze.c */
    struct bubble *ebubbles;

    /* new stuff */
    int early_raw_messages;   /* if raw_prints occurred early prior
                                 to gb.beyond_savefile_load */

    boolean havestate;
};

struct instance_globals_f {

    /* decl.c */
    struct trap *ftrap;
    char *fqn_prefix[PREFIX_COUNT];
    struct fruit *ffruit;

    /* eat.c */
    boolean force_save_hs;

    /* mhitm.c */
    boolean far_noise;

    /* rumors.c */
    long false_rumor_size;
    unsigned long false_rumor_start;
    long false_rumor_end;

    /* shk.c */
    long int followmsg; /* last time of follow message */

    boolean havestate;
};

struct instance_globals_g {

    /* display.c */
    gbuf_entry gbuf[ROWNO][COLNO];
    coordxy gbuf_start[ROWNO];
    coordxy gbuf_stop[ROWNO];

    /* do_name.c */
    coordxy getposx, getposy; /* cursor position in case of async resize */
    struct selectionvar *gloc_filter_map;
    int gloc_filter_floodfill_match_glyph;

    /* dog.c */
    xint16 gtyp;  /* type of dog's current goal */
    coordxy gx; /* x position of dog's current goal */
    coordxy gy; /* y position of dog's current goal */

    /* dokick.c */
    const char *gate_str;

    /* end.c */
    /* 1st +1: subtracting first from last, 2nd +1: one slot for all glass */
    struct valuable_data gems[LAST_REAL_GEM + 1 - FIRST_REAL_GEM + 1];

    /* invent.c */
    long glyph_reset_timestamp;

    /* nhlua.c */
    boolean gmst_stored;
    long gmst_moves;
    struct obj *gmst_invent;
    genericptr_t *gmst_ubak, *gmst_disco, *gmst_mvitals;
    struct spell gmst_spl_book[MAXSPELL + 1];

    /* pline.c */
    struct gamelog_line *gamelog;

    /* region.c */
    boolean gas_cloud_diss_within;
    int gas_cloud_diss_seen;

    /* new stuff */
    /* per-level glyph mapping flags */
    long glyphmap_perlevel_flags;

    boolean havestate;
};

struct instance_globals_h {

    /* decl.c */
    const char *hname; /* name of the game (argv[0] of main) */
#if defined(MICRO) || defined(WIN32)
    char hackdir[PATHLEN]; /* where rumors, help, record are */
#endif /* MICRO || WIN32 */
    long hero_seq; /* 'moves*8 + n' where n is updated each hero move during
                    * the current turn */

    /* dog.c */
    char horsename[PL_PSIZ];

    /* mhitu.c */
    unsigned hitmsg_mid;
    struct attack *hitmsg_prev;

    boolean havestate;
};

struct instance_globals_i {

    /* decl.c */
    int in_doagain;
    boolean in_mklev;
    boolean in_steed_dismounting;
    struct obj *invent;

    /* do_wear.c */
    /* starting equipment gets auto-worn at beginning of new game,
       and we don't want stealth or displacement feedback then */
    boolean initial_don; /* manipulated in set_wear() */

    /* invent.c */
    char *invbuf;
    unsigned invbufsiz;
    int in_sync_perminvent;

    /* mon.c */
    struct monst **itermonarr; /* temporary array of all N monsters
                                * on the current level */

    /* restore.c */
    struct bucket *id_map;

    /* sp_lev.c */
    boolean in_mk_themerooms;

    /* new */

    boolean havestate;
};

struct instance_globals_j {

    /* apply.c */
    int jumping_is_magic; /* current jump result of magic */

    boolean havestate;
};

struct instance_globals_k {

    coord kickedloc; /* location hero just kicked */

    /* decl.c */
    struct obj *kickedobj;     /* object in flight due to kicking */

    /* read.c */
    boolean known;

    boolean havestate;
};

struct instance_globals_l {

    /* cmd.c */
    cmdcount_nht last_command_count;

    /* decl.c (before being incorporated into instance_globals_*) */
#if defined(UNIX) || defined(VMS)
    int locknum; /* max num of simultaneous users */
#endif
#ifdef MICRO
    char levels[PATHLEN]; /* where levels are */
#endif /* MICRO */

    /* files.c */
    int lockptr;
    char lock[LOCKNAMESIZE];

    /* invent.c */
    int lastinvnr;  /* 0 ... 51 (never saved&restored) */

    /* light.c */
    light_source *light_base;

    /* mklev.c */
    genericptr_t luathemes[MAXDUNGEON];

    /* mon.c */
    unsigned last_hider; /* m_id of hides-under mon seen going into hiding */

    /* nhlan.c */
#ifdef MAX_LAN_USERNAME
    char lusername[MAX_LAN_USERNAME];
    int lusername_size;
#endif

    /* nhlua.c */
    genericptr_t luacore; /* lua_State * */
    char lua_warnbuf[BUFSZ];
    int loglua;
    int lua_sid;

    /* options.c */
    boolean loot_reset_justpicked;

    /* save.c */
    struct obj *looseball;  /* track uball during save and... */
    struct obj *loosechain; /* track uchain since saving might free it */

    /* sp_lev.c */
    char *lev_message;
    lev_region *lregions;

    /* trap.c */
    struct launchplace launchplace;

    /* windows.c */
    struct win_choices *last_winchoice;

    /* new stuff */
    char lua_ver[LUA_VER_BUFSIZ];
    char lua_copyright[LUA_COPYRIGHT_BUFSIZ];

    boolean havestate;
};

struct instance_globals_m {

    /* apply.c */
    int mkot_trap_warn_count;

    /* botl.c */
    int mrank_sz; /* loaded by max_rank_sz */

    /* decl.c */
    cmdcount_nht multi;
    const char *multi_reason;
    char multireasonbuf[QBUFSZ]; /* note: smaller than usual [BUFSZ] */
    /* for xname handling of multiple shot missile volleys:
       number of shots, index of current one, validity check, shoot vs throw */
    struct multishot m_shot;
    boolean mrg_to_wielded; /* weapon picked is merged with wielded one */
    struct menucoloring *menu_colorings;
    struct obj *migrating_objs; /* objects moving to another dungeon level */

    /* dog.c */
    struct monst *mydogs; /* monsters that went down/up together with @ */
    struct monst *migrating_mons; /* monsters moving to another level */

    /* dokick.c */
    struct rm *maploc;

    /* mhitu.c */
    int mhitu_dieroll;

    /* mklev.c */
    boolean made_branch; /* used only during level creation */

    /* mkmap.c */
    coordxy min_rx; /* rectangle bounds for regions */
    coordxy max_rx;
    coordxy min_ry;
    coordxy max_ry;

    /* mkobj.c */
    boolean mkcorpstat_norevive; /* for trolls */

    /* mthrowu.c */
    int mesg_given; /* for m_throw()/thitu() 'miss' message */
    struct monst *mtarget;  /* monster being shot by another monster */
    struct monst *marcher; /* monster that is shooting */

    /* muse.c */
    boolean m_using; /* kludge to use mondied instead of killed */
    struct musable m;

    /* options.c */
    /* Allow the user to map incoming characters to various menu commands. */
    char mapped_menu_cmds[MAX_MENU_MAPPED_CMDS + 1]; /* exported */
    char mapped_menu_op[MAX_MENU_MAPPED_CMDS + 1];

    /* region.c */
    int max_regions;

    /* trap.c */
    boolean mentioned_water; /* set to True by water_damage() if it issues
                              * a message about water; dodip() should make
                              * POT_WATER should become discovered */

    boolean havestate;
};

struct instance_globals_n {

    /* botl.c */
    int now_or_before_idx;   /* 0..1 for array[2][] first index */

    /* decl.c */
    const char *nomovemsg;
    int nsubroom;

    /* dokick.c */
    struct rm nowhere;

    /* files.c */
    int nesting;
    int no_sound_notified; /* run-time option processing: warn once if built
                            * without USER_SOUNDS and config file contains
                            * SOUND=foo or SOUNDDIR=bar */

    /* mhitm.c */
    long noisetime;

    /* mkmap.c */
    char *new_locations;
    int n_loc_filled;

    /* options.c */
    short n_menu_mapped;

    /* potion.c */
    boolean notonhead; /* for long worms */

    /* questpgr.c */
    char nambuf[CVT_BUF_SIZE];

    /* restore.c */
    int n_ids_mapped;

    /* sp_lev.c */
    int num_lregions;

    /* u_init.c */
    short nocreate;
    short nocreate2;
    short nocreate3;
    short nocreate4;

    boolean havestate;
};

struct instance_globals_o {

    struct obj *objs_deleted;

    /* dbridge.c */
    struct entity occupants[ENTITIES];

    /* decl.c */
    int (*occupation)(void);
    int occtime;
    int otg_temp; /* used by object_to_glyph() [otg] */
    struct obj *otg_otmp; /* used by obj_is_piletop() */
    const char *occtxt; /* defined when occupation != NULL */

    /* symbols.c */
    nhsym ov_primary_syms[SYM_MAX];   /* loaded primary symbols          */
    nhsym ov_rogue_syms[SYM_MAX];   /* loaded rogue symbols           */

    /* invent.c */
    /* query objlist callback: return TRUE if obj is at given location */
    coord only;

    /* o_init.c */
    short oclass_prob_totals[MAXOCLASSES];

    /* options.c */

    int opt_phase; /* builtin_opt, syscf_, rc_file_, environ_, play_opt */
    boolean opt_initial;
    boolean opt_from_file;
    boolean opt_need_redraw; /* for doset() */
    boolean opt_need_glyph_reset;
    boolean opt_need_promptstyle;
    boolean opt_reset_customcolors;
    boolean opt_reset_customsymbols;
    boolean opt_update_basic_palette;
    boolean opt_symset_changed;

    /* pickup.c */
    int oldcap; /* last encumbrance */

    /* restore.c */
    struct fruit *oldfruit;

    /* rumors.c */
    int oracle_flg; /* -1=>don't use, 0=>need init, 1=>init done */

    /* uhitm.c */
    boolean override_confirmation; /* Used to flag attacks caused by
                                    * Stormbringer's maliciousness. */
    /* zap.c */
    boolean obj_zapped;

    boolean havestate;
};

struct instance_globals_p {

    /* apply.c */
    int polearm_range_min;
    int polearm_range_max;

    /* decl.c */
    int plnamelen; /* length of plname[] if that came from getlogin() */
    char pl_race; /* character's race */
    struct plinemsg_type *plinemsg_types;

    /* dog.c */
    int petname_used; /* user preferred pet name has been used */
    char preferred_pet; /* '\0', 'c', 'd', 'n' (none) */

    /* symbols.c */
    nhsym primary_syms[SYM_MAX];   /* loaded primary symbols          */

    /* invent.c */
    int perm_invent_toggling_direction;

    /* pickup.c */
    boolean picked_filter;
    int pickup_encumbrance; /* when picking up multiple items in a single
                             * operation, encumbrance after previous item */

    /* pline.c */
    unsigned pline_flags;
    char prevmsg[BUFSZ];

    /* potion.c */
    int potion_nothing;
    int potion_unkn;

    /* pray.c */
    /* values calculated when prayer starts, and used when completed */
    aligntyp p_aligntyp;
    int p_trouble;
    int p_type; /* (-1)-3: (-1)=really naughty, 3=really good */

    /* weapon.c */
    struct obj *propellor;

    /* zap.c */
    int  poly_zapped;

    /* new stuff */
    boolean havestate;
};

struct instance_globals_q {

    boolean havestate;
};

struct instance_globals_r {

    /* symbols.c */
    nhsym rogue_syms[SYM_MAX];   /* loaded rogue symbols           */

    /* extralev.c */
    struct rogueroom r[3][3];

    /* mkmaze.c */
    boolean ransacked;

    /* region.c */
    NhRegion **regions;

    /* rip.c */
    char **rip;

    /* role.c */
    char role_pa[NUM_BP];
    char role_post_attribs;
    struct role_filter rfilter;

    /* shk.c */
    struct repo repo;

    boolean havestate;
};

struct instance_globals_s {

    /* artifact.c */
    int spec_dbon_applies; /* coordinate effects from spec_dbon() with
                              messages in artifact_hit() */

    /* decl.c */
    stairway *stairs;
    int smeq[MAXNROFROOMS + 1];
    boolean stoned; /* done to monsters hit by 'c' */
    struct mkroom *subrooms;

    /* do.c */
    d_level save_dlevel; /* ? [even back in 3.4.3, only used in bones.c] */

    /* symbols.c */
    struct symsetentry symset[NUM_GRAPHICS];
    /* adds UNICODESET */
    struct symset_customization
        sym_customizations[NUM_GRAPHICS + 1][custom_count];
    nhsym showsyms[SYM_MAX]; /* symbols to be displayed */

    /* files.c */
    int symset_count;             /* for pick-list building only */
    int symset_which_set;
    /* SAVESIZE, BONESSIZE, LOCKNAMESIZE are defined in "fnamesiz.h" */
    char SAVEF[SAVESIZE]; /* relative path of save file from playground */
#ifdef MICRO
    char SAVEP[SAVESIZE]; /* holds path of directory for save file */
#endif

    /* invent.c */
    unsigned sortlootmode; /* set by sortloot() for use by sortloot_cmp();
                            * reset by sortloot when done */
    /* mhitm.c */
    boolean skipdrin; /* mind flayer against headless target */

    /* mon.c */
    boolean somebody_can_move;

    /* options.c */
    struct symsetentry *symset_list; /* files.c will populate this with
                                      * list of available sets */
    boolean save_menucolors; /* copy of iflags.use_menu_colors */
    struct menucoloring *save_colorings; /* copy of gm.menu_colorings */
    boolean simple_options_help;

    /* pickup.c */
    boolean sellobj_first; /* True => need sellobj_state(); False => don't */
    boolean shop_filter;

    /* pline.c */
#ifdef DUMPLOG_CORE
    unsigned saved_pline_index;  /* slot in saved_plines[] to use next */
    char *saved_plines[DUMPLOG_MSG_COUNT];
#endif

    /* polyself.c */
    int sex_change_ok; /* controls whether taking on new form or becoming new
                          man can also change sex (ought to be an arg to
                          polymon() and newman() instead) */

    /* shk.c */
    /* auto-response flag for/from "sell foo?" 'a' => 'y', 'q' => 'n' */
    char sell_response;
    int sell_how;

    /* spells.c */
    int spl_sortmode;   /* index into spl_sortchoices[] */
    int *spl_orderindx; /* array of svs.spl_book[] indices */

    /* steal.c */
    unsigned int stealoid; /* object to be stolen */
    unsigned int stealmid; /* monster doing the stealing */

    /* vision.c */
    int seethru; /* 'bubble' debugging: clouds and water don't block light */

    boolean havestate;
};

struct instance_globals_t {

    /* apply.c */
    struct trapinfo trapinfo;

    /* decl.c */
    schar tbx;  /* mthrowu: target x */
    schar tby;  /* mthrowu: target y */
    char toplines[TBUFSZ];
    struct obj *thrownobj;     /* object in flight due to throwing */
    /* Windowing stuff that's really tty oriented, but present for all ports */
    struct tc_gbl_data tc_gbl_data; /* AS,AE, LI,CO */

    /* hack.c */
    anything tmp_anything;
    struct selectionvar *travelmap;

    /* invent.c */
    /* query objlist callback: return TRUE if obj type matches "this_type" */
    int this_type;
    const char *this_title; /* title for inventory list of specific type */

    /* muse.c */
    coordxy trapx;
    coordxy trapy;

    /* rumors.c */
    long true_rumor_size; /* rumor size variables are signed so that value -1
                           * can be used as a flag */
    unsigned long true_rumor_start; /* rumor start offsets are unsigned due
                                     * to use of %lx format */
    long true_rumor_end; /* rumor end offsets are signed because they're
                          * compared with [dlb_]ftell() */

    /* sp_lev.c */
    boolean themeroom_failed;

    /* timeout.c */
    /* ordered timer list */
    struct fe *timer_base; /* "active" */

    /* topten.c */
    winid toptenwin;

    /* uhitm.c */
    int twohits; /* 0: single hit; 1: first of 2; 2: second of 2 */

    boolean havestate;
};

struct instance_globals_u {

    /* botl.c */
    boolean update_all;

    /* decl.c */
    boolean unweapon;

    /* role.c */
    struct Role urole; /* player's role. May be munged in role_init() */
    struct Race urace; /* player's race. May be munged in role_init() */

    /* save.c */
    d_level uz_save;

    /* new stuff */
    boolean havestate;
};

struct instance_globals_v {

    /* botl.c */
    boolean valset[MAXBLSTATS];

    /* end.c */
    struct val_list valuables[3];

    /* mhitm.c */
    boolean vis;

    /* mklev.c */
    coordxy vault_x;
    coordxy vault_y;

    /* mon.c */
    boolean vamp_rise_msg;

    /* pickup.c */
    long val_for_n_or_more;
    /* list of menu classes for query_objlist() and allow_category callback
       (with room for all object classes, 'u'npaid, BUCX, and terminator) */
    char valid_menu_classes[MAXOCLASSES + 1 + 4 + 1];

    /* vision.c */
    seenV **viz_array;   /* used in cansee() and couldsee() macros */
    coordxy *viz_rmin;   /* min could see indices */
    coordxy *viz_rmax;   /* max could see indices */
    boolean vision_full_recalc;

    /* new stuff */
    struct sound_voice voice;

    boolean havestate;
};

struct instance_globals_w {

    /* decl.c */
    int warn_obj_cnt; /* count of monsters meeting criteria */
    long wailmsg;

    /* do_wear.c */
    uint8 wasinwater;

    /* symbols.c */
    nhsym warnsyms[WARNCOUNT]; /* the current warning display symbols */

    /* files.c */
    char wizkit[WIZKIT_MAX];

    /* hack.c */
    int wc; /* current weight_cap(); valid after call to inv_weight() */

    /* mkmaze.c */
    struct trap *wportal;

    /* new */
    struct win_settings wsettings;      /* wintype.h */
    long were_changes;                  /* were.c, allmain.c */

    boolean havestate;
};

struct instance_globals_x {

    /* decl.c */
    int x_maze_max;

    /* lock.c */
    struct xlock_s xlock;

    /* objnam.c */
    char *xnamep; /* obuf[] returned by xname(), for use in doname() for
                   * bounds checking; differs from xname() return value
                   * due to reserving PREFIX bytes at start and possibly
                   * skipping leading "the " after constructing result */

    /* sp_lev.c */
    coordxy xstart, xsize;

    boolean havestate;
};

struct instance_globals_y {

    /* decl.c */
    int y_maze_max;
    struct monst youmonst;

    /* pline.c */
    /* work buffer for You(), &c and verbalize() */
    char *you_buf;
    int you_buf_siz;

    /* sp_lev.c */
    coordxy ystart, ysize;

    boolean havestate;
};

struct instance_globals_z {

    /* mon.c */
    boolean zombify;

    /* muse.c */
    boolean zap_oseen; /* for wands which use mbhitm and are zapped at
                        * players.  We usually want an oseen local to
                        * the function, but this is impossible since the
                        * function mbhitm has to be compatible with the
                        * normal zap routines, and those routines don't
                        * remember who zapped the wand. */

    boolean havestate;
};

struct instance_globals_saved_b {
    /* dungeon.c */
    branch *branches; /* dungeon branch list */
    /* mkmaze.c */
    struct bubble *bbubbles;
    /* o_init.c */
    int bases[MAXOCLASSES + 2]; /* make bases[MAXOCLASSES+1] available */
};

struct instance_globals_saved_c {
    /* decl.c */
    struct context_info context;
};

struct instance_globals_saved_d {
    /* dungeon.c */
    dungeon dungeons[MAXDUNGEON]; /* ini'ed by init_dungeon() */
    struct dgn_topology dungeon_topology;
    /* decl.c */
    dest_area dndest;
    coord *doors; /* array of door locations */
    int doors_alloc; /* doors-array allocated size */
    /* o_init.c */
    short disco[NUM_OBJECTS];
};

struct instance_globals_saved_e {
    /* decl.c */
    struct exclusion_zone *exclusion_zones;
};

struct instance_globals_saved_h {
    /* decl.c */
    int hackpid; /* current process id */
};

struct instance_globals_saved_i {
    /* decl.c */
    coord inv_pos;
};

struct instance_globals_saved_k {
    /* decl.c */
    struct kinfo killer;
};

struct instance_globals_saved_l {
    /* decl.c */
    schar lastseentyp[COLNO][ROWNO]; /* last seen/touched dungeon typ */
    dlevel_t level; /* level map */
    struct linfo level_info[MAXLINFO];
};

struct instance_globals_saved_m {
    /* dungeon.c */
    mapseen *mapseenchn; /*DUNGEON_OVERVIEW*/
    /* decl.c */
    long moves; /* turn counter */
    struct mvitals mvitals[NUMMONS];
};

struct instance_globals_saved_n {
    /* dungeon.c */
    int n_dgns; /* number of dungeons (also used in mklev.c and do.c) */
    /* mkroom.c */
    int nroom;
    /* region.c */
    int n_regions;
};

struct instance_globals_saved_o {
    /* rumors.c */
    unsigned oracle_cnt; /* oracles are handled differently from rumors... */
    unsigned long *oracle_loc;

    /* other */
    long omoves;  /* level timestamp */
};

struct instance_globals_saved_p {
    /* decl.c */
    char plname[PL_NSIZ]; /* player name */
    char pl_character[PL_CSIZ];
    char pl_fruit[PL_FSIZ];
};

struct instance_globals_saved_q {
    /* quest.c */
    struct q_score quest_status;
};

struct instance_globals_saved_r {
    /* mkroom.c */
    struct mkroom rooms[(MAXNROFROOMS + 1) * 2];
};

struct instance_globals_saved_s {
    /* decl.c */
    struct spell spl_book[MAXSPELL + 1];
    s_level *sp_levchn;
};

struct instance_globals_saved_t {
    /* decl.c */
    char tune[6];
    /* timeout.c */
    unsigned long timer_id;
};

struct instance_globals_saved_u {
    /* decl.c */
    dest_area updest;
};

struct instance_globals_saved_x {
    /* mkmaze.c */
    int xmin, xmax; /* level boundaries x */
};

struct instance_globals_saved_y {
    /* mkmaze.c */
    int ymin, ymax; /* level boundaries y */
};

extern struct instance_globals_a ga;
extern struct instance_globals_b gb;
extern struct instance_globals_c gc;
extern struct instance_globals_d gd;
extern struct instance_globals_e ge;
extern struct instance_globals_f gf;
extern struct instance_globals_g gg;
extern struct instance_globals_h gh;
extern struct instance_globals_i gi;
extern struct instance_globals_j gj;
extern struct instance_globals_k gk;
extern struct instance_globals_l gl;
extern struct instance_globals_m gm;
extern struct instance_globals_n gn;
extern struct instance_globals_o go;
extern struct instance_globals_p gp;
extern struct instance_globals_q gq;
extern struct instance_globals_r gr;
extern struct instance_globals_s gs;
extern struct instance_globals_t gt;
extern struct instance_globals_u gu;
extern struct instance_globals_v gv;
extern struct instance_globals_w gw;
extern struct instance_globals_x gx;
extern struct instance_globals_y gy;
extern struct instance_globals_z gz;
extern struct instance_globals_saved_b svb;
extern struct instance_globals_saved_c svc;
extern struct instance_globals_saved_d svd;
extern struct instance_globals_saved_e sve;
extern struct instance_globals_saved_h svh;
extern struct instance_globals_saved_i svi;
extern struct instance_globals_saved_k svk;
extern struct instance_globals_saved_l svl;
extern struct instance_globals_saved_m svm;
extern struct instance_globals_saved_n svn;
extern struct instance_globals_saved_o svo;
extern struct instance_globals_saved_p svp;
extern struct instance_globals_saved_q svq;
extern struct instance_globals_saved_r svr;
extern struct instance_globals_saved_s svs;
extern struct instance_globals_saved_t svt;
extern struct instance_globals_saved_u svu;
extern struct instance_globals_saved_x svx;
extern struct instance_globals_saved_y svy;
extern struct sinfo program_state; /* flags describing game's current state */

struct const_globals {
    const struct obj zeroobj;      /* used to zero out a struct obj */
    const struct monst zeromonst;  /* used to zero out a struct monst */
    const anything zeroany;        /* used to zero out union any */
    const NhRect zeroNhRect;       /* used to zero out NhRect */
};

extern const struct const_globals cg;

extern struct obj hands_obj;

#endif /* DECL_H */
