/* NetHack 3.7	wintty.h	$NHDT-Date: 1656014599 2022/06/23 20:03:19 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.55 $ */
/* Copyright (c) David Cohrs, 1991,1992                           */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef WINTTY_H
#define WINTTY_H

#ifndef WINDOW_STRUCTS
#define WINDOW_STRUCTS

#ifdef TTY_PERM_INVENT

enum { tty_perminv_minrow = 28, tty_perminv_mincol = 79 };
/* for static init of zerottycell, put pointer first */
union ttycellcontent {
    glyph_info *gi;
    char ttychar;
};
struct tty_perminvent_cell {
    Bitfield(refresh, 1);
    Bitfield(text, 1);
    Bitfield(glyph, 1);
    union ttycellcontent content;
    uint32 color;       /* adjusted color 0 = ignore
                         * 1-16             = NetHack color + 1
                         * 17..16,777,233   = 24-bit color  + 17
                         */
};
#endif

/* menu structure */
typedef struct tty_mi {
    struct tty_mi *next;
    anything identifier; /* user identifier */
    long count;          /* user count */
    char *str;           /* description string (including accelerator) */
    glyph_info glyphinfo;    /* glyph */
    int attr;            /* string attribute */
    int color;           /* string color */
    boolean selected;    /* TRUE if selected by user */
    unsigned itemflags;  /* item flags */
    char selector;       /* keyboard accelerator */
    char gselector;      /* group accelerator */
} tty_menu_item;

/* descriptor for tty-based windows */
struct WinDesc {
    int flags;             /* window flags */
    xint16 type;           /* type of window */
    boolean active;        /* true if window is active */
    boolean blanked;       /* for erase_tty_screen(); not used [yet?] */
    short offx, offy;      /* offset from topleft of display */
    long rows, cols;       /* dimensions */
    long curx, cury;       /* current cursor position */
    long maxrow, maxcol;   /* the maximum size used -- for MENU wins;
                            * maxcol is also used by WIN_MESSAGE for
                            * tracking the ^P command */
    unsigned long mbehavior; /* menu behavior flags (MENU) */
    short *datlen;         /* allocation size for *data */
    char **data;           /* window data [row][column] */
    char *morestr;         /* string to display instead of default */
    tty_menu_item *mlist;  /* menu information (MENU) */
    tty_menu_item **plist; /* menu page pointers (MENU) */
    long plist_size;       /* size of allocated plist (MENU) */
    long npages;           /* number of pages in menu (MENU) */
    long nitems;           /* total number of items (MENU) */
    short how;             /* menu mode - pick 1 or N (MENU) */
    char menu_ch;          /* menu char (MENU) */
#ifdef TTY_PERM_INVENT
    struct tty_perminvent_cell **cells;
#endif
};

/* window flags */
#define WIN_CANCELLED 1
#define WIN_STOP 1        /* for NHW_MESSAGE; stops output; sticks until
                           * next input request or reversed by WIN_NOSTOP */
#define WIN_LOCKHISTORY 2 /* for NHW_MESSAGE; suppress history updates */
#define WIN_NOSTOP 4      /* current message has been designated as urgent;
                           * prevents WIN_STOP from becoming set if current
                           * message triggers --More-- and user types ESC
                           * (current message won't have been seen yet) */

/* topline states */
#define TOPLINE_EMPTY          0 /* empty */
#define TOPLINE_NEED_MORE      1 /* non-empty, need --More-- */
#define TOPLINE_NON_EMPTY      2 /* non-empty, no --More-- required */
#define TOPLINE_SPECIAL_PROMPT 3 /* special prompt state */

/* descriptor for tty-based displays -- all the per-display data */
struct DisplayDesc {
    short rows, cols;   /* width and height of tty display */
    short curx, cury;   /* current cursor position on the screen */
    uint32 color;        /* current color */
    uint32 colorflags;   /* NH_BASIC_COLOR or 24-bit color */
    uint32 framecolor;   /* current background color */
    int attrs;          /* attributes in effect */
    int toplin;         /* flag for topl stuff */
    int rawprint;       /* number of raw_printed lines since synch */
    int inmore;         /* non-zero if more() is active */
    int inread;         /* non-zero if reading a character */
    int intr;           /* non-zero if inread was interrupted */
    winid lastwin;      /* last window used for I/O */
    char dismiss_more;  /* extra character accepted at --More-- */
    int topl_utf8;      /* non-zero if utf8 in str */
    int mixed;          /* we are processing mixed output */
};

#endif /* WINDOW_STRUCTS */

#ifdef STATUS_HILITES
struct tty_status_fields {
    int idx;
    int color;
    int attr;
    int x, y;
    size_t lth;
    boolean valid;
    boolean dirty;
    boolean redraw;
    boolean sanitycheck; /* was 'last_in_row' */
};
#endif

#define MAXWIN 20 /* maximum number of windows, cop-out */

/* tty dependent window types */
#ifdef NHW_BASE
#undef NHW_BASE
#endif
#define NHW_BASE (NHW_LAST_TYPE + 1)

/* external declarations */

extern struct window_procs tty_procs;

/* port specific variable declarations */
extern winid BASE_WINDOW;

extern struct WinDesc *wins[MAXWIN];

extern struct DisplayDesc *ttyDisplay; /* the tty display descriptor */

extern char morc;         /* last character typed to xwaitforspace */
extern char defmorestr[]; /* default --more-- prompt */

/* port specific external function references */

/* ### getline.c ### */

extern void xwaitforspace(const char *);

/* ### termcap.c, video.c ### */

/*
 * TERM or NO_TERMS
 *
 * The tty windowport interface relies on lower-level support routines
 * to actually manipulate the terminal/display. Those are the right place
 * for doing strange and arcane things such as outputting escape sequences
 * to select a color or whatever.  wintty.c should concern itself with WHERE
 * to put stuff in a window.

 * The TERM routines are found in:
 *
 * !NO_TERMS:          termcap.c
 *
 * NO_TERMS:
 *            WINCON   sys/windows/consoletty.c
 *            MSDOS    sys/msdos/video.c
 *
 */
extern void backsp(void);
extern void cl_end(void);
extern void cl_eos(void);
extern void graph_on(void);
extern void graph_off(void);
extern void home(void);
extern void standoutbeg(void);
extern void standoutend(void);
extern int term_attr_fixup(int);
extern void term_clear_screen(void);
extern void term_curs_set(int);
extern void term_end_attr(int attr);
extern void term_end_color(void);
extern void term_end_extracolor(void);
extern void term_end_raw_bold(void);
extern void term_end_screen(void);
extern void term_start_attr(int attr);
extern void term_start_bgcolor(int color);
extern void term_start_color(int color);
extern void term_start_extracolor(uint32, uint16);
extern void term_start_raw_bold(void);
extern void term_start_screen(void);
extern void term_startup(int *, int *);
extern void term_shutdown(void);

extern int xputc(int);
extern void xputs(const char *);
#if 0
extern void revbeg(void);
extern void boldbeg(void);
extern void blinkbeg(void);
extern void dimbeg(void);
extern void m_end(void);
#endif
#if defined(SCREEN_VGA) || defined(SCREEN_8514) || defined(SCREEN_VESA)
extern void xputg(const glyph_info *, const glyph_info *);
#endif

/* ### topl.c ### */

extern void show_topl(const char *);
extern void remember_topl(void);
extern void addtopl(const char *);
extern void more(void);
extern void update_topl(const char *);
extern void putsyms(const char *);

/* ### wintty.c ### */

#ifdef CLIPPING
extern void setclipped(void);
#endif
extern void docorner(int, int, int);
extern void end_glyphout(void);
extern void g_putch(int);
#ifdef ENHANCED_SYMBOLS
#if defined(WIN32) || defined(UNIX) || defined(MSDOS)
extern void g_pututf8(uint8 *);
#endif
#endif /* ENHANCED_SYMBOLS */
extern void erase_tty_screen(void);
extern void win_tty_init(int);

/* tty interface */
extern void tty_init_nhwindows(int *, char **);
extern void tty_preference_update(const char *);
extern void tty_player_selection(void);
extern void tty_askname(void);
extern void tty_get_nh_event(void);
extern void tty_exit_nhwindows(const char *);
extern void tty_suspend_nhwindows(const char *);
extern void tty_resume_nhwindows(void);
extern winid tty_create_nhwindow(int);
extern void tty_clear_nhwindow(winid);
extern void tty_display_nhwindow(winid, boolean);
extern void tty_dismiss_nhwindow(winid);
extern void tty_destroy_nhwindow(winid);
extern void tty_curs(winid, int, int);
extern void tty_putstr(winid, int, const char *);
extern void tty_putmixed(winid window, int attr, const char *str);
extern void tty_display_file(const char *, boolean);
extern void tty_start_menu(winid, unsigned long);
extern void tty_add_menu(winid, const glyph_info *, const ANY_P *, char,
                         char, int, int, const char *, unsigned int);
extern void tty_end_menu(winid, const char *);
extern int tty_select_menu(winid, int, MENU_ITEM_P **);
extern char tty_message_menu(char, int, const char *);
extern void tty_mark_synch(void);
extern void tty_wait_synch(void);
#ifdef CLIPPING
extern void tty_cliparound(int, int);
#endif
#ifdef POSITIONBAR
extern void tty_update_positionbar(char *);
#endif
extern void tty_print_glyph(winid, coordxy, coordxy, const glyph_info *,
                            const glyph_info *);
extern void tty_raw_print(const char *);
extern void tty_raw_print_bold(const char *);
extern int tty_nhgetch(void);
extern int tty_nh_poskey(coordxy *, coordxy *, int *);
extern void tty_nhbell(void);
extern int tty_doprev_message(void);
extern char tty_yn_function(const char *, const char *, char);
extern void tty_getlin(const char *, char *);
extern int tty_get_ext_cmd(void);
extern void tty_number_pad(int);
extern void tty_delay_output(void);
#ifdef CHANGE_COLOR
extern void tty_change_color(int color, long rgb, int reverse);
#ifdef MAC
extern void tty_change_background(int white_or_black);
extern short set_tty_font_name(winid, char *);
#endif
extern char *tty_get_color_string(void);
#endif
extern void tty_status_enablefield(int, const char *, const char *, boolean);
extern void tty_status_init(void);
extern void tty_status_update(int, genericptr_t, int, int,
                              int, unsigned long *);

extern void genl_outrip(winid, int, time_t);

extern char *tty_getmsghistory(boolean);
extern void tty_putmsghistory(const char *, boolean);
extern void tty_update_inventory(int);
extern win_request_info *tty_ctrl_nhwindow(winid, int, win_request_info *);

#ifdef TTY_PERM_INVENT
extern void tty_refresh_inventory(int start, int stop, int y);
#endif

/* termcap is implied if NO_TERMS is not defined */
#ifndef NO_TERMS
#ifndef NO_TERMCAP_HEADERS
#include <curses.h>
#ifdef clear_screen /* avoid a conflict */
#undef clear_screen
#endif
#include <term.h>
#ifdef bell
#undef bell
#endif
#ifdef color_names
#undef color_names
#endif
#ifdef tone
#undef tone
#endif
#ifdef hangup
#undef hangup
#endif
#else
extern int tgetent(char *, const char *);
extern void tputs(const char *, int, int (*)(int));
extern int tgetnum(const char *);
extern int tgetflag(const char *);
extern char *tgetstr(const char *, char **);
extern char *tgoto(const char *, int, int);
#endif /* NO_TERMCAP_HEADERS */
#else  /* ?NO_TERMS */
#ifdef MAC
#ifdef putchar
#undef putchar
#undef putc
#endif
#define putchar term_putc
#define fflush term_flush
#define puts term_puts
extern int term_putc(int c);
extern int term_flush(void *desc);
extern int term_puts(const char *str);
#endif /* MAC */
#if defined(MSDOS) || defined(WIN32)
#if defined(SCREEN_BIOS) || defined(SCREEN_DJGPPFAST) || defined(WIN32)
#undef putchar
#undef putc
#undef puts
#define putchar(x) xputc(x) /* these are in video.c, nttty.c */
#define putc(x) xputc(x)
#define puts(x) xputs(x)
#endif /*SCREEN_BIOS || SCREEN_DJGPPFAST || WIN32 */
#ifdef POSITIONBAR
extern void video_update_positionbar(char *);
#endif
#endif /*MSDOS*/
#endif /* NO_TERMS */

#endif /* WINTTY_H */
