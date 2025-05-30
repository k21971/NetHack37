/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* NetHack 3.7 curswins.c */
/* Copyright (c) Karl Garrison, 2010. */
/* NetHack may be freely redistributed.  See license for details. */

#if defined(CURSES_UNICODE) && !defined(_XOPEN_SOURCE_EXTENDED)
#define _XOPEN_SOURCE_EXTENDED 1
#endif
#include "curses.h"
#include "hack.h"
#include "wincurs.h"
#include "cursinit.h"
#include "cursmisc.h"
#include "curswins.h"
#include "cursstat.h"

/* Window handling for curses interface */

/* Private declarations */

typedef struct nhw {
    winid nhwin;                /* NetHack window id */
    WINDOW *curwin;             /* Curses window pointer */
    int width;                  /* Usable width not counting border */
    int height;                 /* Usable height not counting border */
    int x;                      /* start of window on terminal (left) */
    int y;                      /* start of window on terminal (top) */
    int orientation;            /* Placement of window relative to map */
    boolean clr_inited;         /* fg/bg/colorpair inited? */
    int fg, bg;                 /* foreground, background color index */
    int colorpair;              /* color pair of fg, bg */
    boolean border;             /* Whether window has a visible border */
} nethack_window;

typedef struct nhwd {
    winid nhwid;                /* NetHack window id */
    struct nhwd *prev_wid;      /* Pointer to previous entry */
    struct nhwd *next_wid;      /* Pointer to next entry */
} nethack_wid;

typedef struct nhchar {
    int ch;                     /* character */
    int color;                  /* color info for character */
    int framecolor;                /* background color info for character */
    int attr;                   /* attributes of character */
    struct unicode_representation *unicode_representation;
} nethack_char;

static boolean map_clipped;     /* Map window smaller than 80x21 */
static nethack_window nhwins[NHWIN_MAX];        /* NetHack window array */
static nethack_char map[ROWNO][COLNO];  /* Map window contents */
static nethack_wid *nhwids = NULL;      /* NetHack wid array */

static boolean is_main_window(winid wid);
static void write_char(WINDOW * win, int x, int y, nethack_char ch);
static void clear_map(void);

/* Create a window with the specified size and orientation */

WINDOW *
curses_create_window(int wid, int width, int height, orient orientation)
{
    int mapx = 0, mapy = 0, maph = 0, mapw = 0;
    int startx = 0;
    int starty = 0;
    WINDOW *win;
    boolean map_border = FALSE;
    int mapb_offset = 0;

    if ((orientation == UP) || (orientation == DOWN) ||
        (orientation == LEFT) || (orientation == RIGHT)) {
        if (svm.moves > 0) {
            map_border = curses_window_has_border(MAP_WIN);
            curses_get_window_xy(MAP_WIN, &mapx, &mapy);
            curses_get_window_size(MAP_WIN, &maph, &mapw);
        } else {
            map_border = TRUE;
            mapx = 0;
            mapy = 0;
            maph = term_rows;
            mapw = term_cols;
        }
    }

    if (map_border) {
        mapb_offset = 1;
    }

    width += 2;                 /* leave room for bounding box */
    height += 2;

    if ((width > term_cols) || (height > term_rows)) {
        impossible(
                "curses_create_window: Terminal too small for dialog window");
        width = term_cols;
        height = term_rows;
    }
    switch (orientation) {
    default:
        impossible("curses_create_window: Bad orientation");
        FALLTHROUGH;
        /*FALLTHRU*/
    case CENTER:
        startx = (term_cols / 2) - (width / 2);
        starty = (term_rows / 2) - (height / 2);
        break;
    case UP:
        if (svm.moves > 0) {
            startx = (mapw / 2) - (width / 2) + mapx + mapb_offset;
        } else {
            startx = 0;
        }

        starty = mapy + mapb_offset;
        break;
    case DOWN:
        if (svm.moves > 0) {
            startx = (mapw / 2) - (width / 2) + mapx + mapb_offset;
        } else {
            startx = 0;
        }

        starty = height - mapy - 1 - mapb_offset;
        break;
    case LEFT:
        if (map_border && (width < term_cols))
            startx = 1;
        else
            startx = 0;
        starty = term_rows - height;
        break;
    case RIGHT:
        if (svm.moves > 0) {
            startx = (mapw + mapx + (mapb_offset * 2)) - width;
        } else {
            startx = term_cols - width;
        }

        starty = 0;
        break;
    }

    if (startx < 0) {
        startx = 0;
    }

    if (starty < 0) {
        starty = 0;
    }

    win = newwin(height, width, starty, startx);

    if (curses_is_text(wid))
        wid = TEXT_WIN;
    else if (curses_is_menu(wid))
        wid = MENU_WIN;

    if (nhwins[wid].clr_inited < 1)
        curses_toggle_color_attr(win, DIALOG_BORDER_COLOR, NONE, ON);
    box(win, 0, 0);
    if (nhwins[wid].clr_inited < 1)
        curses_toggle_color_attr(win, DIALOG_BORDER_COLOR, NONE, OFF);
    return win;
}

int
curses_win_clr_inited(int wid)
{
    if (curses_is_text(wid)) {
        wid = TEXT_WIN;
    } else if (curses_is_menu(wid)) {
        wid = MENU_WIN;
    }
    return nhwins[wid].clr_inited;
}

void
curses_set_wid_colors(int wid, WINDOW *win)
{
    if (wid == TEXT_WIN || curses_is_text(wid)) {
        wid = TEXT_WIN;
        if (!nhwins[wid].clr_inited)
            curses_parse_wid_colors(wid, iflags.wcolors[wcolor_text].fg,
                                    iflags.wcolors[wcolor_text].bg);
    } else if (wid == MENU_WIN || curses_is_menu(wid)) {
        wid = MENU_WIN;
        if (!nhwins[wid].clr_inited)
            curses_parse_wid_colors(wid, iflags.wcolors[wcolor_menu].fg,
                                    iflags.wcolors[wcolor_menu].bg);
    }
    /* FIXME: colors and nhwins[] entry for perm invent window */
    if (nhwins[wid].clr_inited > 0) {
        wbkgd(win ? win : nhwins[wid].curwin,
              COLOR_PAIR(nhwins[wid].colorpair));
    }
}

/* Erase and delete curses window, and refresh standard windows */

void
curses_destroy_win(WINDOW *win)
{
    int mapwidth = 0, winwidth, dummyht;

    /*
     * In case map is narrower than the space alloted for it, if we
     * are destroying a popup window and it is wider than the map,
     * erase the popup first.  It probably has overwritten some of
     * the next-to-map empty space.  If we don't clear that now, the
     * base window will remember it and redisplay it during refreshes.
     *
     * Note: since we almost never destroy non-popups, we don't really
     * need to determine whether 'win' is one.  Overhead for unnecessary
     * erasure is negligible.
     */
    getmaxyx(win, dummyht, winwidth); /* macro, assigns to its args */
    if (mapwin)
        getmaxyx(mapwin, dummyht, mapwidth);
    if (winwidth > mapwidth) {
        werase(win);
        wnoutrefresh(win);
    }

    delwin(win);
    if (win == activemenu)
        activemenu = NULL;
    curses_refresh_nethack_windows();
    nhUse(dummyht);
}


/* Refresh nethack windows if they exist, or base window if not */

void
curses_refresh_nethack_windows(void)
{
    WINDOW *status_window, *message_window, *map_window, *inv_window;

    status_window = curses_get_nhwin(STATUS_WIN);
    message_window = curses_get_nhwin(MESSAGE_WIN);
    map_window = curses_get_nhwin(MAP_WIN);
    inv_window = curses_get_nhwin(INV_WIN);

    if (!iflags.window_inited) {
        return;
    }

    if (svm.moves == 0) {
        /* Main windows not yet displayed; refresh base window instead */
        touchwin(stdscr);
        refresh();
    } else {
        if (status_window != NULL) {
            curses_set_wid_colors(STATUS_WIN, NULL);
            touchwin(status_window);
            wnoutrefresh(status_window);
        }
        if (map_window != NULL) {
            touchwin(map_window);
            wnoutrefresh(map_window);
        }
        if (message_window != NULL) {
            curses_set_wid_colors(MESSAGE_WIN, NULL);
            touchwin(message_window);
            wnoutrefresh(message_window);
        }
        if (inv_window) {
            touchwin(inv_window);
            wnoutrefresh(inv_window);
        }
        doupdate();
    }
}


/* Return curses window pointer for given NetHack winid */

WINDOW *
curses_get_nhwin(winid wid)
{
    if (!is_main_window(wid)) {
        impossible("curses_get_nhwin: wid %d out of range. Not a main window.",
                   wid);
        return NULL;
    }

    return nhwins[wid].curwin;
}

boolean
parse_hexstr(char *colorbuf, int *red, int *green, int *blue)
{
    int len = colorbuf ? strlen(colorbuf) : 0;

    if (len == 7 && colorbuf[0] == '#') {
        char tmpbuf[16];

        Sprintf(tmpbuf, "0x%c%c", colorbuf[1], colorbuf[2]);
        *red = strtol(tmpbuf, NULL, 0);
        Sprintf(tmpbuf, "0x%c%c", colorbuf[3], colorbuf[4]);
        *green = strtol(tmpbuf, NULL, 0);
        Sprintf(tmpbuf, "0x%c%c", colorbuf[5], colorbuf[6]);
        *blue = strtol(tmpbuf, NULL, 0);
        return TRUE;
    }
    return FALSE;
}

void
curses_parse_wid_colors(int wid, char *fg, char *bg)
{

    if (curses_is_text(wid)) {
        wid = TEXT_WIN;
    } else if (curses_is_menu(wid)) {
        wid = MENU_WIN;
    }

    if (nhwins[wid].clr_inited)
        return;

    int nh_fg = fg ? match_str2clr(fg, TRUE) : CLR_MAX;
    int nh_bg = bg ? match_str2clr(bg, TRUE) : CLR_MAX;
    int r, g, b;

    if (nh_fg == CLR_MAX) {
        if (fg && parse_hexstr(fg, &r, &g, &b)) {
            nh_fg = curses_init_rgb(r, g, b);
        } else {
            nh_fg = -1;
        }
    }
    if (nh_bg == CLR_MAX) {
        if (bg && parse_hexstr(bg, &r, &g, &b)) {
            nh_bg = curses_init_rgb(r, g, b);
        } else {
            nh_bg = -1;
        }
    }

    nhwins[wid].fg = nh_fg;
    nhwins[wid].bg = nh_bg;
    if (nh_fg == -1 || nh_bg == -1) {
        nhwins[wid].clr_inited = -1;
    } else {
        nhwins[wid].colorpair = curses_init_pair(nh_fg, nh_bg);
        nhwins[wid].clr_inited = 1;
    }
}


/* Add curses window pointer and window info to list for given NetHack winid */

void
curses_add_nhwin(
    winid wid, int height, int width, int y, int x,
    orient orientation, boolean border)
{
    WINDOW *win;
    int real_width = width;
    int real_height = height;

    if (!is_main_window(wid)) {
        impossible("curses_add_nhwin: wid %d out of range. Not a main window.",
                   wid);
        return;
    }

    nhwins[wid].nhwin = wid;
    nhwins[wid].border = border;
    nhwins[wid].width = width;
    nhwins[wid].height = height;
    nhwins[wid].x = x;
    nhwins[wid].y = y;
    nhwins[wid].orientation = orientation;
    nhwins[wid].fg = nhwins[wid].bg = 0;
    nhwins[wid].colorpair = -1;
    nhwins[wid].clr_inited = 0;

    if (border) {
        real_width += 2;        /* leave room for bounding box */
        real_height += 2;
    }

    win = newwin(real_height, real_width, y, x);
    nhwins[wid].curwin = win;

    switch (wid) {
    case MESSAGE_WIN:
        messagewin = win;
        curses_parse_wid_colors(wid, iflags.wcolors[wcolor_message].fg,
                                iflags.wcolors[wcolor_message].bg);
        curses_set_wid_colors(wid, NULL);
        break;
    case STATUS_WIN:
        statuswin = win;
        curses_parse_wid_colors(wid, iflags.wcolors[wcolor_status].fg,
                                iflags.wcolors[wcolor_status].bg);
        curses_set_wid_colors(wid, NULL);
        break;
    case MAP_WIN:
        mapwin = win;
        map_clipped = (width < COLNO || height < ROWNO);
        break;
    }

    if (border) {
        box(win, 0, 0);
    }

}


/* Add wid to list of known window IDs */

void
curses_add_wid(winid wid)
{
    nethack_wid *new_wid;
    nethack_wid *widptr = nhwids;

    new_wid = (nethack_wid *) alloc((unsigned) sizeof (nethack_wid));
    new_wid->nhwid = wid;

    new_wid->next_wid = NULL;

    if (widptr == NULL) {
        new_wid->prev_wid = NULL;
        nhwids = new_wid;
    } else {
        while (widptr->next_wid != NULL) {
            widptr = widptr->next_wid;
        }
        new_wid->prev_wid = widptr;
        widptr->next_wid = new_wid;
    }
}


/* refresh a curses window via given nethack winid */

void
curses_refresh_nhwin(winid wid)
{
    curses_set_wid_colors(wid, NULL);
    wnoutrefresh(curses_get_nhwin(wid));
    doupdate();
}


/* Delete curses window via given NetHack winid and remove entry from list */

void
curses_del_nhwin(winid wid)
{
    if (curses_is_menu(wid) || curses_is_text(wid)) {
        curses_del_menu(wid, TRUE);
        return;
    } else if (wid == INV_WIN) {
        curses_del_menu(wid, TRUE);
        /* don't return yet */
    }

    if (!is_main_window(wid)) {
        impossible("curses_del_nhwin: wid %d out of range. Not a main window.",
                   wid);
        return;
    }
    if (nhwins[wid].curwin != NULL) {
        delwin(nhwins[wid].curwin);
        nhwins[wid].curwin = NULL;
    }
    nhwins[wid].nhwin = -1;
}


/* Delete wid from list of known window IDs */

void
curses_del_wid(winid wid)
{
    nethack_wid *tmpwid;
    nethack_wid *widptr;

    if (curses_is_menu(wid) || curses_is_text(wid)) {
        curses_del_menu(wid, FALSE);
    }

    for (widptr = nhwids; widptr; widptr = widptr->next_wid) {
        if (widptr->nhwid == wid) {
            if ((tmpwid = widptr->prev_wid) != NULL) {
                tmpwid->next_wid = widptr->next_wid;
            } else {
                nhwids = widptr->next_wid;      /* New head mode, or NULL */
            }
            if ((tmpwid = widptr->next_wid) != NULL) {
                tmpwid->prev_wid = widptr->prev_wid;
            }
            free(widptr);
            break;
        }
    }
}

/* called by destroy_nhwindows() prior to exit */
void
curs_destroy_all_wins(void)
{
    curses_count_window((char *) 0); /* clean up orphan */

#if 0   /* this works but confuses the static analyzer (from llvm-19,
         * which reports "warning: Use of memory after it is freed") */
    while (nhwids)
        curses_del_wid(nhwids->nhwid);
#else
    while (nhwids) {
        nethack_wid *tmpptr = nhwids;

        curses_del_wid(tmpptr->nhwid);
    }
#endif
}

/* Print a single character in the given window at the given coordinates */

void
curses_putch(winid wid, int x, int y, int ch,
#ifdef ENHANCED_SYMBOLS
             struct unicode_representation *unicode_representation,
#endif
             int color, int framecolor, int attr)
{
    static boolean map_initted = FALSE;
    int sx, sy, ex, ey;
    boolean border = curses_window_has_border(wid);
    nethack_char nch;
/*
    if (wid == STATUS_WIN) {
        curses_update_stats();
    }
*/
    if (wid != MAP_WIN) {
        return;
    }

    if (!map_initted) {
        clear_map();
        map_initted = TRUE;
    }

    --x; /* map column [0] is not used; draw column [1] in first screen col */
    map[y][x].ch = ch;
    map[y][x].color = color;
    map[y][x].framecolor = framecolor;
    map[y][x].attr = attr;
#ifdef ENHANCED_SYMBOLS
    map[y][x].unicode_representation = unicode_representation;
#endif
    nch = map[y][x];

    (void) curses_map_borders(&sx, &sy, &ex, &ey, -1, -1);

    if ((x >= sx) && (x <= ex) && (y >= sy) && (y <= ey)) {
        if (border) {
            x++;
            y++;
        }

        write_char(mapwin, x - sx, y - sy, nch);
    }
    /* refresh after every character?
     * Fair go, mate! Some of us are playing from Australia! */
    /* wrefresh(mapwin); */
}


/* Get x, y coordinates of curses window on the physical terminal window */

void
curses_get_window_xy(winid wid, int *x, int *y)
{
    if (!is_main_window(wid)) {
        impossible(
              "curses_get_window_xy: wid %d out of range. Not a main window.",
                   wid);
        *x = 0;
        *y = 0;
        return;
    }

    *x = nhwins[wid].x;
    *y = nhwins[wid].y;
}


/* Get usable width and height curses window on the physical terminal window */

void
curses_get_window_size(winid wid, int *height, int *width)
{
    *height = nhwins[wid].height;
    *width = nhwins[wid].width;
}


/* Determine if given window has a visible border */

boolean
curses_window_has_border(winid wid)
{
    if (curses_is_menu(wid))
        wid = MENU_WIN;
    else if (curses_is_text(wid))
        wid = TEXT_WIN;
    return nhwins[wid].border;
}


/* Determine if window for given winid exists */

boolean
curses_window_exists(winid wid)
{
    nethack_wid *widptr;

    for (widptr = nhwids; widptr; widptr = widptr->next_wid)
        if (widptr->nhwid == wid)
            return TRUE;

    return FALSE;
}


/* Return the orientation of the specified window */

int
curses_get_window_orientation(winid wid)
{
    if (!is_main_window(wid)) {
        impossible(
     "curses_get_window_orientation: wid %d out of range. Not a main window.",
                   wid);
        return CENTER;
    }

    return nhwins[wid].orientation;
}


/* Output a line of text to specified NetHack window with given coordinates
   and text attributes */

void
curses_puts(winid wid, int attr, const char *text)
{
    anything Id;
    WINDOW *win = NULL;

    if (is_main_window(wid)) {
        win = curses_get_nhwin(wid);
    }

    if (wid == MESSAGE_WIN) {
        /* if a no-history message is being shown, remove it */
        if (counting)
            curses_count_window((char *) 0);

        curses_message_win_puts(text, FALSE);
        return;
    }

#if 0
    if (wid == STATUS_WIN) {
        curses_update_stats();     /* We will do the write ourselves */
        return;
    }
#endif

    if (curses_is_menu(wid) || curses_is_text(wid)) {
        if (!curses_menu_exists(wid)) {
            impossible(
                     "curses_puts: Attempted write to nonexistent window %d!",
                       wid);
            return;
        }
        Id = cg.zeroany;
        curses_add_nhmenu_item(wid, &nul_glyphinfo, &Id, 0, 0,
                               attr, NO_COLOR, text, MENU_ITEMFLAGS_NONE);
    } else {
        curses_set_wid_colors(wid, NULL);
        waddstr(win, text);
        wnoutrefresh(win);
    }
}


/* Clear the contents of a window via the given NetHack winid */

void
curses_clear_nhwin(winid wid)
{
    WINDOW *win = curses_get_nhwin(wid);
    boolean border = curses_window_has_border(wid);

    if (wid == MAP_WIN) {
        clearok(win, TRUE);     /* Redraw entire screen when refreshed */
        clear_map();
    }

    curses_set_wid_colors(wid, NULL);
    werase(win);

    if (border) {
        box(win, 0, 0);
    }
}

/* Change colour of window border to alert player to something */
void
curses_alert_win_border(winid wid, boolean onoff)
{
    WINDOW *win = curses_get_nhwin(wid);

    if (!win || !curses_window_has_border(wid))
        return;
    curses_set_wid_colors(wid, NULL);
    if (onoff)
        curses_toggle_color_attr(win, ALERT_BORDER_COLOR, NONE, ON);
    box(win, 0, 0);
    if (onoff)
        curses_toggle_color_attr(win, ALERT_BORDER_COLOR, NONE, OFF);
    wnoutrefresh(win);
}


void
curses_alert_main_borders(boolean onoff)
{
    curses_alert_win_border(MAP_WIN, onoff);
    curses_alert_win_border(MESSAGE_WIN, onoff);
    curses_alert_win_border(STATUS_WIN, onoff);
    curses_alert_win_border(INV_WIN, onoff);
}

/* Return true if given wid is a main NetHack window */

static boolean
is_main_window(winid wid)
{
    if (wid == MESSAGE_WIN || wid == MAP_WIN
        || wid == STATUS_WIN || wid == INV_WIN)
        return TRUE;

    return FALSE;
}


/* Unconditionally write a single character to a window at the given
coordinates without a refresh.  Currently only used for the map. */

/* convert nhcolor (fg) and framecolor (bg) to curses colorpair */
int
get_framecolor(int nhcolor, int framecolor)
{
    /* curses_toggle_color_attr() adds the +1 and takes care of COLORS < 16 */
    return (16 * (framecolor % 8)) + (nhcolor % 16);
}

static void
write_char(WINDOW * win, int x, int y, nethack_char nch)
{
    int curscolor = nch.color, cursattr = nch.attr;

    if (nch.framecolor != NO_COLOR) {
        curscolor = get_framecolor(nch.color, nch.framecolor);
        if (nch.attr == A_REVERSE)
            cursattr = A_NORMAL; /* hilited pet looks odd otherwise */
    }
    curses_toggle_color_attr(win, curscolor, cursattr, ON);
#if defined(CURSES_UNICODE) && defined(ENHANCED_SYMBOLS)
    if ((nch.unicode_representation && nch.unicode_representation->utf8str)
        || SYMHANDLING(H_IBM)) {
        /* CP437 to Unicode mapping according to the Unicode Consortium */
        static const uint16 cp437[256] = {
            0x0020, 0x263A, 0x263B, 0x2665, 0x2666, 0x2663, 0x2660, 0x2022,
            0x25D8, 0x25CB, 0x25D9, 0x2642, 0x2640, 0x266A, 0x266B, 0x263C,
            0x25BA, 0x25C4, 0x2195, 0x203C, 0x00B6, 0x00A7, 0x25AC, 0x21A8,
            0x2191, 0x2193, 0x2192, 0x2190, 0x221F, 0x2194, 0x25B2, 0x25BC,
            0x0020, 0x0021, 0x0022, 0x0023, 0x0024, 0x0025, 0x0026, 0x0027,
            0x0028, 0x0029, 0x002a, 0x002b, 0x002c, 0x002d, 0x002e, 0x002f,
            0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
            0x0038, 0x0039, 0x003a, 0x003b, 0x003c, 0x003d, 0x003e, 0x003f,
            0x0040, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
            0x0048, 0x0049, 0x004a, 0x004b, 0x004c, 0x004d, 0x004e, 0x004f,
            0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
            0x0058, 0x0059, 0x005a, 0x005b, 0x005c, 0x005d, 0x005e, 0x005f,
            0x0060, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
            0x0068, 0x0069, 0x006a, 0x006b, 0x006c, 0x006d, 0x006e, 0x006f,
            0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
            0x0078, 0x0079, 0x007a, 0x007b, 0x007c, 0x007d, 0x007e, 0x2302,
            0x00c7, 0x00fc, 0x00e9, 0x00e2, 0x00e4, 0x00e0, 0x00e5, 0x00e7,
            0x00ea, 0x00eb, 0x00e8, 0x00ef, 0x00ee, 0x00ec, 0x00c4, 0x00c5,
            0x00c9, 0x00e6, 0x00c6, 0x00f4, 0x00f6, 0x00f2, 0x00fb, 0x00f9,
            0x00ff, 0x00d6, 0x00dc, 0x00a2, 0x00a3, 0x00a5, 0x20a7, 0x0192,
            0x00e1, 0x00ed, 0x00f3, 0x00fa, 0x00f1, 0x00d1, 0x00aa, 0x00ba,
            0x00bf, 0x2310, 0x00ac, 0x00bd, 0x00bc, 0x00a1, 0x00ab, 0x00bb,
            0x2591, 0x2592, 0x2593, 0x2502, 0x2524, 0x2561, 0x2562, 0x2556,
            0x2555, 0x2563, 0x2551, 0x2557, 0x255d, 0x255c, 0x255b, 0x2510,
            0x2514, 0x2534, 0x252c, 0x251c, 0x2500, 0x253c, 0x255e, 0x255f,
            0x255a, 0x2554, 0x2569, 0x2566, 0x2560, 0x2550, 0x256c, 0x2567,
            0x2568, 0x2564, 0x2565, 0x2559, 0x2558, 0x2552, 0x2553, 0x256b,
            0x256a, 0x2518, 0x250c, 0x2588, 0x2584, 0x258c, 0x2590, 0x2580,
            0x03b1, 0x00df, 0x0393, 0x03c0, 0x03a3, 0x03c3, 0x00b5, 0x03c4,
            0x03a6, 0x0398, 0x03a9, 0x03b4, 0x221e, 0x03c6, 0x03b5, 0x2229,
            0x2261, 0x00b1, 0x2265, 0x2264, 0x2320, 0x2321, 0x00f7, 0x2248,
            0x00b0, 0x2219, 0x00b7, 0x221a, 0x207f, 0x00b2, 0x25a0, 0x00a0
        };
        attr_t attr;
        short pair;
        wchar_t wch[3];
        uint32 utf32ch;
        cchar_t cch;

        if (SYMHANDLING(H_UTF8)) {
            utf32ch = nch.unicode_representation->utf32ch;
        } else if (SYMHANDLING(H_IBM)) {
            utf32ch = cp437[(uint8)nch.ch];
        } else {
            utf32ch = (uint8)nch.ch;
        }
        if (sizeof(wchar_t) == 2 && utf32ch >= 0x10000) {
            /* UTF-16 surrogate pair */
            wch[0] = (wchar_t)((utf32ch >> 10) + 0xD7C0);
            wch[1] = (wchar_t)((utf32ch & 0x3FF) + 0xDC00);
            wch[2] = L'\0';
        } else {
            wch[0] = (wchar_t)utf32ch;
            wch[1] = L'\0';
        }
        wmove(win, y, x);
        wattr_get(win, &attr, &pair, NULL);
        setcchar(&cch, wch, attr, pair, NULL);
        mvwadd_wch(win, y, x, &cch);
    } else
#endif
#ifdef PDCURSES
        mvwaddrawch(win, y, x, nch.ch);
#else
        mvwaddch(win, y, x, nch.ch);
#endif
    curses_toggle_color_attr(win, curscolor, cursattr, OFF);
}


/* Draw the entire visible map onto the screen given the visible map
boundaries */

void
curses_draw_map(int sx, int sy, int ex, int ey)
{
    int curx, cury;
    int bspace = 0;

#ifdef MAP_SCROLLBARS
    int sbsx, sbsy, sbex, sbey, count;
    nethack_char hsb_back, hsb_bar, vsb_back, vsb_bar;
#endif

    if (curses_window_has_border(MAP_WIN)) {
        bspace++;
    }
#ifdef MAP_SCROLLBARS
    hsb_back.ch = '-';
    hsb_back.color = SCROLLBAR_BACK_COLOR;
    hsb_back.framecolor = NO_COLOR;
    hsb_back.attr = A_NORMAL;
    hsb_back.unicode_representation = NULL;
    hsb_bar.ch = '*';
    hsb_bar.color = SCROLLBAR_COLOR;
    hsb_bar.framecolor = NO_COLOR;
    hsb_bar.attr = A_NORMAL;
    hsb_bar.unicode_representation = NULL;
    vsb_back.ch = '|';
    vsb_back.color = SCROLLBAR_BACK_COLOR;
    vsb_back.framecolor = NO_COLOR;
    vsb_back.attr = A_NORMAL;
    vsb_back.unicode_representation = NULL;
    vsb_bar.ch = '*';
    vsb_bar.color = SCROLLBAR_COLOR;
    vsb_bar.framecolor = NO_COLOR;
    vsb_bar.attr = A_NORMAL;
    vsb_bar.unicode_representation = NULL;

    /* Horizontal scrollbar */
    if (sx > 0 || ex < (COLNO - 1)) {
         sbsx = (int) (((long) sx * (long) (ex - sx + 1)) / (long) COLNO);
         sbex = (int) (((long) ex * (long) (ex - sx + 1)) / (long) COLNO);

        if (sx > 0 && sbsx == 0)
            ++sbsx;
        if (ex < COLNO - 1 && sbex == COLNO - 1)
            --sbex;

        for (count = 0; count < sbsx; count++) {
            write_char(mapwin, count + bspace, ey - sy + 1 + bspace, hsb_back);
        }
        for (count = sbsx; count <= sbex; count++) {
            write_char(mapwin, count + bspace, ey - sy + 1 + bspace, hsb_bar);
        }
        for (count = sbex + 1; count <= (ex - sx); count++) {
            write_char(mapwin, count + bspace, ey - sy + 1 + bspace, hsb_back);
        }
    }

    /* Vertical scrollbar */
    if (sy > 0 || ey < (ROWNO - 1)) {
        sbsy = (int) (((long) sy * (long) (ey - sy + 1)) / (long) ROWNO);
        sbey = (int) (((long) ey * (long) (ey - sy + 1)) / (long) ROWNO);

        if (sy > 0 && sbsy == 0)
            ++sbsy;
        if (ey < ROWNO - 1 && sbey == ROWNO - 1)
            --sbey;

        for (count = 0; count < sbsy; count++) {
            write_char(mapwin, ex - sx + 1 + bspace, count + bspace, vsb_back);
        }
        for (count = sbsy; count <= sbey; count++) {
            write_char(mapwin, ex - sx + 1 + bspace, count + bspace, vsb_bar);
        }
        for (count = sbey + 1; count <= (ey - sy); count++) {
            write_char(mapwin, ex - sx + 1 + bspace, count + bspace, vsb_back);
        }
    }
#endif /* MAP_SCROLLBARS */

    for (curx = sx; curx <= ex; curx++) {
        for (cury = sy; cury <= ey; cury++) {
            write_char(mapwin, curx - sx + bspace, cury - sy + bspace,
                       map[cury][curx]);
        }
    }
}


/* Init map array to blanks */

static void
clear_map(void)
{
    int x, y;

    for (x = 0; x < COLNO; x++) {
        for (y = 0; y < ROWNO; y++) {
            map[y][x].ch = ' ';
            map[y][x].color = NO_COLOR;
            map[y][x].attr = A_NORMAL;
            map[y][x].unicode_representation = NULL;
        }
    }
}


/* Determine visible boundaries of map, and determine if it needs to be
based on the location of the player. */

boolean
curses_map_borders(int *sx, int *sy, int *ex, int *ey, int ux, int uy)
{
    static int width = 0;
    static int height = 0;
    static int osx = 0;
    static int osy = 0;
    static int oex = 0;
    static int oey = 0;
    static int oux = -1;
    static int ouy = -1;

    if ((oux == -1) || (ouy == -1)) {
        oux = u.ux;
        ouy = u.uy;
    }

    if (ux == -1) {
        ux = oux;
    } else {
        oux = ux;
    }

    if (uy == -1) {
        uy = ouy;
    } else {
        ouy = uy;
    }

    curses_get_window_size(MAP_WIN, &height, &width);

#ifdef MAP_SCROLLBARS
    if (width < COLNO) {
        height--;               /* room for horizontal scrollbar */
    }

    if (height < ROWNO) {
        width--;                /* room for vertical scrollbar */

        if (width == COLNO) {
            height--;
        }
    }
#endif /* MAP_SCROLLBARS */

    if (width >= COLNO) {
        *sx = 0;
        *ex = COLNO - 1;
    } else {
        *ex = (width / 2) + ux;
        *sx = *ex - (width - 1);

        if (*ex >= COLNO) {
            *sx = COLNO - width;
            *ex = COLNO - 1;
        } else if (*sx < 0) {
            *sx = 0;
            *ex = width - 1;
        }
    }

    if (height >= ROWNO) {
        *sy = 0;
        *ey = ROWNO - 1;
    } else {
        *ey = (height / 2) + uy;
        *sy = *ey - (height - 1);

        if (*ey >= ROWNO) {
            *sy = ROWNO - height;
            *ey = ROWNO - 1;
        } else if (*sy < 0) {
            *sy = 0;
            *ey = height - 1;
        }
    }

    if ((*sx != osx) || (*sy != osy) || (*ex != oex) || (*ey != oey) ||
        map_clipped) {
        osx = *sx;
        osy = *sy;
        oex = *ex;
        oey = *ey;
        return TRUE;
    }

    return FALSE;
}
