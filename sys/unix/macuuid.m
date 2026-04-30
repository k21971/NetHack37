/* macuuid.m */
/* Copyright Michael Allison, 2023 */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

void free_macos_uuid(void);
void get_macos_uuid(char *);

#ifdef DEBUG
#undef DEBUG
#endif

/* #import <Foundation/Foundation.h> */
#import <AppKit/AppKit.h>

static const char *macos_uuid = 0;

void
free_macos_uuid(void)
{
    if (macos_uuid) {
        free((genericptr_t) macos_uuid);
        macos_uuid = 0;
    } 
}

void
get_macos_uuid(char *target)
{
    NSString *uuidString = [[NSUUID UUID] UUIDString];
    const char *str_uuid, *src;
    char *dst;
    int i;
    
    if (macos_uuid)
        free_macos_uuid();

    /* str_uuid = [uuidString cStringUsingEncoding:NSUTF8StringEncoding]; */
    str_uuid = [uuidString cStringUsingEncoding:NSASCIIStringEncoding];
    macos_uuid = dupstr(str_uuid);
    
    if (macos_uuid) {
        src = macos_uuid;
        dst = target;
        for (i = 0; i < 36; ++i) {
            *dst = *src;
            ++src;
            ++dst;
        }
        dst[36] = '\0';
    }
    free_macos_uuid();
    macos_uuid = 0;
    return;
}


/* end of macuuid.m */
