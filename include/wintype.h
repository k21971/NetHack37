/* NetHack 3.7  wintype.h       $NHDT-Date: 1717880364 2024/06/08 20:59:24 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.52 $ */
/* Copyright (c) David Cohrs, 1991                                */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef WINTYPE_H
#define WINTYPE_H

typedef int winid; /* a window identifier */

/* generic parameter - must not be any larger than a pointer */
typedef union any {
    genericptr_t a_void;
    struct obj *a_obj;
    struct monst *a_monst;
    int a_int;
    int a_xint16;
    int a_xint8;
    char a_char;
    schar a_schar;
    uchar a_uchar;
    unsigned int a_uint;
    long a_long;
    unsigned long a_ulong;
    coordxy a_coordxy;
    int *a_iptr;
    xint16 *a_xint16ptr;
    xint8 *a_xint8ptr;
    long *a_lptr;
    coordxy *a_coordxyptr;
    unsigned long *a_ulptr;
    unsigned *a_uptr;
    const char *a_string;
    int (*a_nfunc)(void);
    unsigned long a_mask32; /* used by status highlighting */
    int64 a_int64;
    uint64 a_uint64;
    /* add types as needed */
} anything;
#define ANY_P union any /* avoid typedef in prototypes
                         * (buggy old Ultrix compiler) */

/* symbolic names for the data types housed in anything */
enum any_types {
    ANY_VOID = 1,
    ANY_OBJ,         /* struct obj */
    ANY_MONST,       /* struct monst (not used) */
    ANY_INT,         /* int */
    ANY_CHAR,        /* char */
    ANY_UCHAR,       /* unsigned char */
    ANY_SCHAR,       /* signed char */
    ANY_UINT,        /* unsigned int */
    ANY_LONG,        /* long */
    ANY_ULONG,       /* unsigned long */
    ANY_IPTR,        /* pointer to int */
    ANY_UPTR,        /* pointer to unsigned int */
    ANY_LPTR,        /* pointer to long */
    ANY_ULPTR,       /* pointer to unsigned long */
    ANY_STR,         /* pointer to null-terminated char string */
    ANY_NFUNC,       /* pointer to function taking no args, returning int */
    ANY_MASK32,      /* 32-bit mask (stored as unsigned long) */

    ANY_INVALID      /* leave this last */
};

/* menu return list */
typedef struct mi {
    anything item;     /* identifier */
    long count;        /* count */
    unsigned itemflags; /* item flags */
} menu_item;
#define MENU_ITEM_P struct mi

/* These would be in sym.h and display.h if they weren't needed to
   define the windowproc interface for X11 which doesn't include
   most of the main NetHack header files */

struct classic_representation {
    int color;
    int symidx;
};

struct unicode_representation {
    uint32 utf32ch;
    uint8 *utf8str;
};

typedef struct glyph_map_entry {
    unsigned glyphflags;
    struct classic_representation sym;
    uint32 customcolor;
    uint16 color256idx;
    short int tileidx;
#ifdef ENHANCED_SYMBOLS
    struct unicode_representation *u;
#endif
} glyph_map;

/* glyph plus additional info
   if you add fields or change the ordering, fix up the following:
        g_info initialization in display.c
        nul_glyphinfo initialization in display.c
 */
typedef struct glyphinfo {
    int glyph;            /* the display entity */
    int ttychar;
    uint32 framecolor;
    glyph_map gm;
} glyph_info;
/*#define GLYPH_INFO_P struct glyphinfo //not used*/

/* select_menu() "how" argument types */
/* [MINV_PICKMASK in monst.h assumes these have values of 0, 1, 2] */
#define PICK_NONE 0 /* user picks nothing (display only) */
#define PICK_ONE 1  /* only pick one */
#define PICK_ANY 2  /* can pick any amount */

/* window types */
/* any additional port specific types should be defined in win*.h */
#define NHW_MESSAGE 1
#define NHW_STATUS 2
#define NHW_MAP 3
#define NHW_MENU 4
#define NHW_TEXT 5
#if defined(DUMPLOG) || defined(DUMPHTML)
/* There's no windump.h, so these go here for now */
#define NHW_DUMPTXT 6
#define NHW_DUMPHTML 7
#endif
#define NHW_PERMINVENT 8
#define NHW_LAST_TYPE NHW_PERMINVENT

/* attribute types for putstr; the same as the ANSI value, for convenience */
#define ATR_NONE       0
#define ATR_BOLD       1
#define ATR_DIM        2
#define ATR_ITALIC     3
#define ATR_ULINE      4
#define ATR_BLINK      5
#define ATR_INVERSE    7
/* not a display attribute but passed to putstr() as an attribute;
   can be masked with one regular display attribute */
#define ATR_URGENT    16
#define ATR_NOHISTORY 32

/* Style attributes for HTML dumplogs. Currently masked if not
   dumping because other window ports don't define how they are
   handled.  This masking could be generalised with a WC#_ flag
   to allow other windowports to support them. */
#define ATR_HEADING   (iflags.in_dumplog ?  64 : 0) /* rendered as <h2> in HTML dump */
#define ATR_SUBHEAD   (iflags.in_dumplog ? 128 : 0) /* rendered as <h3> in HTML dump */
#define ATR_PREFORM   (iflags.in_dumplog ? 256 : 0) /* preformatted - for preserving indentation */

/* nh_poskey() modifier types */
#define CLICK_1 1
#define CLICK_2 2
#define NUM_MOUSE_BUTTONS 2

/* invalid winid */
#define WIN_ERR ((winid) -1)

/* menu window keyboard commands (may be mapped); menu_shift_right and
   menu_shift_left are for interacting with persistent inventory window */
/* clang-format off */
#define MENU_FIRST_PAGE         '^'
#define MENU_LAST_PAGE          '|'
#define MENU_NEXT_PAGE          '>'
#define MENU_PREVIOUS_PAGE      '<'
#define MENU_SHIFT_RIGHT        '}'
#define MENU_SHIFT_LEFT         '{'
#define MENU_SELECT_ALL         '.'
#define MENU_UNSELECT_ALL       '-'
#define MENU_INVERT_ALL         '@'
#define MENU_SELECT_PAGE        ','
#define MENU_UNSELECT_PAGE      '\\'
#define MENU_INVERT_PAGE        '~'
#define MENU_SEARCH             ':'

#define MENU_ITEMFLAGS_NONE           0x0000000U
#define MENU_ITEMFLAGS_SELECTED       0x0000001U
#define MENU_ITEMFLAGS_SKIPINVERT     0x0000002U
#define MENU_ITEMFLAGS_SKIPMENUCOLORS 0x0000004U

/* 3.7+ enhanced menu flags that not all window ports are likely to
 * support initially.
 *
 * As behavior and appearance modification flags are added, the various
 * individual window ports will likely have to be updated to respond
 * to the flags in an appropriate way.
 */

#define MENU_BEHAVE_STANDARD      0x0000000U
#define MENU_BEHAVE_PERMINV       0x0000001U

enum perm_invent_toggles {
    toggling_off = -1,
    toggling_not =  0,
    toggling_on  =  1
};

/* perm_invent modes */
enum inv_mode_bits {
    InvNormal   = 1,
    InvShowGold = 2,
    InvSparse   = 4, /* must be ORed with Normal or ShowGold to be valid */
    InvInUse    = 8
};
enum inv_modes { /* 'perminv_mode' option settings */
    InvOptNone       = 0,           /* no perm_invent */
    InvOptOn         = InvNormal,   /* 1 */
    InvOptFull       = InvShowGold, /* 2 */
#if 1 /*#ifdef TTY_PERM_INVENT*/
    /* confusingly-named "sparse mode" shows all inventory letters, even when
       their slots are empty; only meaningful for tty's perm_invent */
    InvOptOn_grid    = InvNormal | InvSparse,   /* 5 */
    InvOptFull_grid  = InvShowGold | InvSparse, /* 6 */
#endif
    InvOptInUse      = InvInUse,    /* 8 */
};

enum to_core_flags {
    active           = 0x001,
    too_small        = 0x002,
    prohibited       = 0x004,
    no_init_done     = 0x008,
    too_early        = 0x010,
};

enum from_core_requests {
    invalid_core_request = 0,
    set_mode             = 1,
    request_settings     = 2,
    set_menu_promptstyle = 3,
};

struct to_core {
    long tocore_flags;
    boolean active;
    boolean use_update_inventory;    /* disable the newer slot interface */
    int maxslot;
    int needrows, needcols;
    int haverows, havecols;
};

struct from_core {
    enum from_core_requests core_request;
    enum inv_modes invmode;
    color_attr menu_promptstyle;
};

struct win_request_info_t {
    struct to_core tocore;
    struct from_core fromcore;
};

typedef struct win_request_info_t win_request_info;
extern win_request_info zerowri;    /* windows.c */

/* #define CORE_INVENT */

/* In a binary with multiple window interfaces linked in, this is
 * a structure to track certain interface capabilities that cannot be
 * statically done at compile time. Some of them can be toggled and
 * the core needs to know if they are active or not at the time.
 */

enum win_display_modes {
    wdmode_traditional = 0,
    wdmode_tiled
};

struct win_settings {
    enum win_display_modes wdmode;
    uint32 map_frame_color;
};

/* clang-format on */

#endif /* WINTYPE_H */
