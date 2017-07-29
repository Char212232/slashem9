/*	SCCS Id: @(#)rip.c	3.4	2003/01/08	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

static void center(int, char *);

extern const char * const killed_by_prefix[];	/* from topten.c */

#if defined(TTY_GRAPHICS) || defined(CURSES_GRAPHICS) || defined(MSWIN_GRAPHICS) || defined(GTK_GRAPHICS)
# define TEXT_TOMBSTONE
#endif
#if defined(mac) || defined(WIN32_GRAPHICS)
# ifndef TEXT_TOMBSTONE
#  define TEXT_TOMBSTONE
# endif
#endif

#ifdef TEXT_TOMBSTONE

#ifndef NH320_DEDICATION
/* A normal tombstone for end of game display. */
static const char *rip_txt[] = {
"                       ----------",
"                      /          \\",
"                     /    REST    \\",
"                    /      IN      \\",
"                   /     PEACE      \\",
"                  /                  \\",
"                  |                  |", /* Name of player */
"                  |                  |", /* Amount of $ */
"                  |                  |", /* Type of death */
"                  |                  |", /* . */
"                  |                  |", /* . */
"                  |                  |", /* . */
"                  |       1001       |", /* Real year of death */
"                 *|     *  *  *      | *",
"        _________)/\\\\_//(\\/(/\\)/\\//\\/|_)_______",
0
};
#define STONE_LINE_CENT 28	/* char[] element of center of stone face */
#else	/* NH320_DEDICATION */
/* NetHack 3.2.x displayed a dual tombstone as a tribute to Izchak. */
static const char *rip_txt[] = {
"              ----------                      ----------",
"             /          \\                    /          \\",
"            /    REST    \\                  /    This    \\",
"           /      IN      \\                /  release of  \\",
"          /     PEACE      \\              /   NetHack is   \\",
"         /                  \\            /   dedicated to   \\",
"         |                  |            |  the memory of   |",
"         |                  |            |                  |",
"         |                  |            |  Izchak Miller   |",
"         |                  |            |   1935 - 1994    |",
"         |                  |            |                  |",
"         |                  |            |     Ascended     |",
"         |       1001       |            |                  |",
"      *  |     *  *  *      | *        * |      *  *  *     | *",
" _____)/\\|\\__//(\\/(/\\)/\\//\\/|_)________)/|\\\\_/_/(\\/(/\\)/\\/\\/|_)____",
0
};
#define STONE_LINE_CENT 19	/* char[] element of center of stone face */
#endif	/* NH320_DEDICATION */
#define STONE_LINE_LEN 16	/* # chars that fit on one line
				 * (note 1 ' ' border)
				 */
#define NAME_LINE 6		/* *char[] line # for player name */
#define GOLD_LINE 7		/* *char[] line # for amount of gold */
#define DEATH_LINE 8		/* *char[] line # for death description */
#define YEAR_LINE 12		/* *char[] line # for year */

static char **rip;

static void
center(line, text)
int line;
char *text;
{
	char *ip,*op;
	ip = text;
	op = &rip[line][STONE_LINE_CENT - ((strlen(text)+1)>>1)];
	while(*ip) *op++ = *ip++;
}


void
genl_outrip(tmpwin, how)
winid tmpwin;
int how;
{
	char **dp;
	char *dpx;
	char buf[BUFSZ];
	int x;
	int line;

	rip = dp = (char **) alloc(sizeof(rip_txt));
	for (x = 0; rip_txt[x]; x++) {
		dp[x] = alloc((unsigned int)(strlen(rip_txt[x]) + 1));
		strcpy(dp[x], rip_txt[x]);
	}
	dp[x] = NULL;

	/* Put name on stone */
	sprintf(buf, "%s", plname);
	buf[STONE_LINE_LEN] = 0;
	center(NAME_LINE, buf);

	/* Put $ on stone */
#ifndef GOLDOBJ
	sprintf(buf, "%ld Au", u.ugold);
#else
	sprintf(buf, "%ld Au", done_money);
#endif
	buf[STONE_LINE_LEN] = 0; /* It could be a *lot* of gold :-) */
	center(GOLD_LINE, buf);

	/* Put together death description */
	switch (killer_format) {
		default: impossible("bad killer format?");
		case KILLED_BY_AN:
                      if (Instant_Death) {
                        strcpy(buf, "instantly ");
                        strcat(buf, killed_by_prefix[how]);
                      }
                      else if (Quick_Death) {
                        strcpy(buf, "quickly ");
                        strcat(buf, killed_by_prefix[how]);
                      } else strcpy(buf, killed_by_prefix[how]);
			strcat(buf, an(killer));
			break;
		case KILLED_BY:
                      if (Instant_Death) {
                        strcpy(buf, "instantly ");
                        strcat(buf, killed_by_prefix[how]);
                      }
                      else if (Quick_Death) {
                        strcpy(buf, "quickly ");
                        strcat(buf, killed_by_prefix[how]);
                      } else strcpy(buf, killed_by_prefix[how]);
			strcat(buf, killer);
			break;
		case NO_KILLER_PREFIX:
			strcpy(buf, killer);
			break;
	}

	/* Put death type on stone */
	for (line=DEATH_LINE, dpx = buf; line<YEAR_LINE; line++) {
		int i,i0;
		char tmpchar;

		if ( (i0=strlen(dpx)) > STONE_LINE_LEN) {
				for(i = STONE_LINE_LEN;
				    ((i0 > STONE_LINE_LEN) && i); i--)
					if(dpx[i] == ' ') i0 = i;
				if(!i) i0 = STONE_LINE_LEN;
		}
		tmpchar = dpx[i0];
		dpx[i0] = 0;
		center(line, dpx);
		if (tmpchar != ' ') {
			dpx[i0] = tmpchar;
			dpx= &dpx[i0];
		} else  dpx= &dpx[i0+1];
	}

	/* Put year on stone */
	sprintf(buf, "%4d", getyear());
	center(YEAR_LINE, buf);

	putstr(tmpwin, 0, "");
	for(; *dp; dp++)
		putstr(tmpwin, 0, *dp);

	putstr(tmpwin, 0, "");
	putstr(tmpwin, 0, "");

	for (x = 0; rip_txt[x]; x++) {
		free((void *)rip[x]);
	}
	free((void *)rip);
	rip = 0;
}

#endif	/* TEXT_TOMBSTONE */

/*rip.c*/
