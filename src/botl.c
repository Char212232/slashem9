/*	SCCS Id: @(#)botl.c	3.4	1996/07/15	*/
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

#include "nhstr.h"

const char *hu_stat[] = {
	"Satiated",
	"",
	"Hungry",
	"Weak",
	"Fainting",
	"Fainted",
	"Starved"};
const char *hu_abbrev_stat[] = {
	"Sat",
	"",
	"Hun",
	"Wea",
	"Ftg",
	"Ftd",
	"Sta"};

const char *enc_stat[] = {
	"",
	"Burdened",
	"Stressed",
	"Strained",
	"Overtaxed",
	"Overloaded"};
const char *enc_abbrev_stat[] = {
	"",
	"Brd",
	"Ssd",
	"Snd",
	"Otd",
	"Old"};

extern const struct percent_color_option *hp_colors;
extern const struct percent_color_option *pw_colors;
extern const struct text_color_option *text_colors;

struct color_option text_color_of(const char *text, const struct text_color_option *color_options) {
	if (color_options == NULL) {
		struct color_option result = {NO_COLOR, 0};
		return result;
	}
	if (strstri(color_options->text, text) || strstri(text, color_options->text))
		return color_options->color_option;
	return text_color_of(text, color_options->next);
}

struct color_option percentage_color_of(int value, int max, const struct percent_color_option *color_options) {
	if (color_options == NULL) {
		struct color_option result = {NO_COLOR, 0};
		return result;
	}
	if (100 * value <= color_options->percentage * max)
		return color_options->color_option;
	return percentage_color_of(value, max, color_options->next);
}

void add_colored_text_match(const char *text, const char *match, nhstr *newbot) {
	if ((*text == '\0') || (*match == '\0')) return;

	if (!iflags.use_status_colors) {
		nhscatf(newbot, " %S", text);
	} else {
		nhscatfc(newbot, text_color_of(match, text_colors).color, " %S", text);
	}
}

void add_colored_text(const char *text, nhstr *newbot) {
	add_colored_text_match(text, text, newbot);
}

static int mrank_sz = 0; /* loaded by max_rank_sz (from u_init) */

static const char *rank(void);

/* convert experience level (1..30) to rank index (0..8) */
int xlev_to_rank(int xlev) {
	return (xlev <= 2) ? 0 : (xlev <= 30) ? ((xlev + 2) / 4) : 8;
}

#if 0 /* not currently needed */
/* convert rank index (0..8) to experience level (1..30) */
int rank_to_xlev(int rank) {
	return (rank <= 0) ? 1 : (rank <= 8) ? ((rank * 4) - 2) : 30;
}
#endif

const char *rank_of(int lev, short monnum, boolean female) {
	struct Role *role;
	int i;

	/* Find the role */
	for (role = (struct Role *)roles; role->name.m; role++)
		if (monnum == role->malenum || monnum == role->femalenum)
			break;
	if (!role->name.m)
		role = &urole;

	/* Find the rank */
	for (i = xlev_to_rank((int)lev); i >= 0; i--) {
		if (female && role->rank[i].f) return role->rank[i].f;
		if (role->rank[i].m) return role->rank[i].m;
	}

	/* Try the role name, instead */
	if (female && role->name.f)
		return role->name.f;
	else if (role->name.m)
		return role->name.m;
	return "Player";
}

static const char *rank(void) {
	return rank_of(u.ulevel, Role_switch, flags.female);
}

int title_to_mon(const char *str, int *rank_indx, int *title_length) {
	int i, j;

	/* Loop through each of the roles */
	for (i = 0; roles[i].name.m; i++)
		for (j = 0; j < 9; j++) {
			if (roles[i].rank[j].m && !strncmpi(str,
							    roles[i].rank[j].m, strlen(roles[i].rank[j].m))) {
				if (rank_indx) *rank_indx = j;
				if (title_length) *title_length = strlen(roles[i].rank[j].m);
				return roles[i].malenum;
			}
			if (roles[i].rank[j].f && !strncmpi(str,
							    roles[i].rank[j].f, strlen(roles[i].rank[j].f))) {
				if (rank_indx) *rank_indx = j;
				if (title_length) *title_length = strlen(roles[i].rank[j].f);
				return ((roles[i].femalenum != NON_PM) ?
						roles[i].femalenum :
						roles[i].malenum);
			}
		}
	return NON_PM;
}

void max_rank_sz(void) {
	int i, r, maxr = 0;

	for (i = 0; i < 9; i++) {
		if (urole.rank[i].m && (r = strlen(urole.rank[i].m)) > maxr) maxr = r;
		if (urole.rank[i].f && (r = strlen(urole.rank[i].f)) > maxr) maxr = r;
	}
	mrank_sz = maxr;
	return;
}

long botl_score(void) {
	int deepest = deepest_lev_reached(false);
	long umoney = money_cnt(invent) + hidden_gold();

	if ((umoney -= u.umoney0) < 0L) umoney = 0L;
	return umoney + u.urexp + (long)(50 * (deepest - 1)) + (long)(deepest > 30 ? 10000 : deepest > 20 ? 1000 * (deepest - 20) : 0);
}

static char *botl_player(void) {
	static char player[MAXCO];
	char *nb;
	char mbot[MAXCO - 15];
	int k = 0;

	strcpy(player, plname);
	if ('a' <= player[0] && player[0] <= 'z') player[0] += 'A' - 'a';
	player[10] = 0;
	sprintf(nb = eos(player), " the ");

	if (Upolyd) {
		strncpy(mbot, mons[u.umonnum].mname, SIZE(mbot) - 1);
		mbot[SIZE(mbot) - 1] = 0;
		while (mbot[k] != 0) {
			if ((k == 0 || (k > 0 && mbot[k - 1] == ' ')) &&
			    'a' <= mbot[k] && mbot[k] <= 'z')
				mbot[k] += 'A' - 'a';
			k++;
		}
		sprintf(eos(nb), "%s", mbot);
	} else
		sprintf(eos(nb), "%s", rank());
	return player;
}

static nhstr botl_strength(void) {
	if (ACURR(A_STR) > 18) {
		if (ACURR(A_STR) > STR18(100)) {
			return nhsfmt("%2i", ACURR(A_STR) - 100);
		} else if (ACURR(A_STR) < STR18(100)) {
			//TODO: extend nhsfmt
			static char buf[BUFSZ];
			sprintf(buf, "18/%02i", ACURR(A_STR) - 18);
			return nhsdupz(buf);
		} else {
			return nhsdupz("18/**");
		}
	} else {
		static char buf[BUFSZ];
		sprintf(buf, "%i", ACURR(A_STR));
		//return nhsdupz(buf);
		return nhsfmt("%i", (int)ACURR(A_STR));
	}
}

nhstr bot1str() {
	nhstr dret = {0};
	nhstr *ret = &dret;

	if (iflags.hitpointbar) {
		//TODO: inverse
		nhscatz(ret, "[");
	}

	if (iflags.hitpointbar) {
		char *pltitle = botl_player();
		usize bar_length = strlen(pltitle);
		usize filledbar;
		if (uhp() < 0) {
			filledbar = bar_length;	 //highlight the whole thing
		} else {
			filledbar = uhp() * bar_length / uhpmax();
		}

		// TODO: use_inverse (once attrs are a thing)
		nhscatznc(ret, pltitle, filledbar, percentage_color_of(uhp(), uhpmax(), hp_colors).color);
		nhscatz(ret, pltitle + filledbar);

		nhscatz(ret, "]");
	}

	nhscatz(ret, "  ");

	nhscatf(ret, "St:%/s ", botl_strength());
	nhscatf(ret, "Dx:%i Co:%i In:%i Wi:%i Ch:%i",
		ACURR(A_DEX), ACURR(A_CON), ACURR(A_INT), ACURR(A_WIS), ACURR(A_CHA));
	nhscatz(ret, (u.ualign.type == A_CHAOTIC) ? "  Chaotic" : (u.ualign.type == A_NEUTRAL) ? "  Neutral" : "  Lawful");

	if (flags.showscore)
		nhscatf(ret, " S:%l", botl_score());

	return dret;
}

void bot1() {
	curs(WIN_STATUS, 1, 0);

	nhstr newbot1 = bot1str();
	putnstr(WIN_STATUS, 0, newbot1);
	del_nhs(&newbot1);
}

/* provide the name of the current level for display by various ports */
int describe_level(char *buf, int verbose) {
	int ret = 1;

	/* TODO:	Add in dungeon name */
	if (Is_knox(&u.uz))
		sprintf(buf, "%s ", dungeons[u.uz.dnum].dname);
	else if (In_quest(&u.uz))
		sprintf(buf, "Home %d ", dunlev(&u.uz));
	else if (In_endgame(&u.uz))
		sprintf(buf,
			Is_astralevel(&u.uz) ? "Astral Plane " : "End Game ");
	else {
		if (verbose)
			sprintf(buf, "%s, level %d ", dungeons[u.uz.dnum].dname, depth(&u.uz));
		else
			sprintf(buf, "Dlvl:%d", depth(&u.uz));
		ret = 0;
	}
	return ret;
}

/* [ALI] Line 2 abbreviation levels:
 *	0 - No abbreviation
 *	1 - Omit gold
 *	2 - Abbreviated status tags
 *	3 - Disable show options
 *	4 - Omit dungeon level
 *
 * We omit gold first since the '$' command is always available.
 *
 * While the abbreviated status tags are very difficult to interpret, we use
 * these before disabling the show options on the basis that the user always
 * has the choice of turning the show options off if that would be preferable.
 *
 * Last to go is the dungeon level on the basis that there is no way of
 * finding this information other than via the status line.
 */

static int bot2_abbrev = 0; /* Line 2 abbreviation level (max 4) */

nhstr bot2str(void) {
	nhstr dret = {0};
	nhstr *ret = &dret;

	int hp, hpmax;
	int cap = near_capacity();

	hp = Upolyd ? u.mh : u.uhp;
	hpmax = Upolyd ? u.mhmax : u.uhpmax;

	if (hp < 0) hp = 0;
	if (bot2_abbrev < 4) {
		char buf[BUFSZ];
		describe_level(buf, false);
		nhscatz(ret, buf);
	}

	if (bot2_abbrev < 1)
		nhscatf(ret, " %c:%l", oc_syms[COIN_CLASS], money_cnt(invent));

	nhscatfc(ret, percentage_color_of(hp, hpmax, hp_colors).color, " HP:%i(%i)", hp, hpmax);
	nhscatfc(ret, percentage_color_of(u.uen, u.uenmax, pw_colors).color, " Pw:%i(%i)", u.uen, u.uenmax);
	nhscatf(ret, " AC:%i", u.uac);

	if (Upolyd) {
		nhscatf(ret, " HD:%i", ((u.ulycn == u.umonnum) ? u.ulevel : mons[u.umonnum].mlevel));
	} else if (flags.showexp && bot2_abbrev < 3) {
		nhscatf(ret, " Xp:%i/%l", u.ulevel, u.uexp);
	} else {
		nhscatf(ret, " Exp:%i", u.ulevel);
	}

	if (flags.showweight && bot2_abbrev < 3)
		nhscatf(ret, " Wt:%l/%l", (long)(inv_weight() + weight_cap()), (long)weight_cap());

	if (flags.time && bot2_abbrev < 3)
		nhscatf(ret, " T:%l", moves);

	if (iflags.showrealtime) {
		time_t currenttime = get_realtime();
		nhscatf(ret, " RT:%l:%2.2l", (long)(currenttime/3600), (long)((currenttime % 3600) / 60));
	}

	// just give a bit of breathing room
	nhscatz(ret, " ");

	if (hu_stat[u.uhs][0]) {
		add_colored_text_match((bot2_abbrev >= 2) ? hu_abbrev_stat[u.uhs] : hu_stat[u.uhs], hu_stat[u.uhs], ret);
	}

	/* WAC further Up
		if (flags.showscore)
	                sprintf(nb,"%c:%-2ld  Score:%ld", oc_syms[COIN_CLASS],
	                   u.ugold, botl_score());
	*/
	/* KMH -- changed to Lev */

	if (Levitation) add_colored_text("Lev", ret);

	if (Confusion) add_colored_text_match(bot2_abbrev >= 2 ? "Cnf" : "Conf", "Conf", ret);

	if (Sick) {
		if (u.usick_type & SICK_VOMITABLE)
			add_colored_text_match(bot2_abbrev >= 2 ? "FPs" : "FoodPois", "FoodPois", ret);
		if (u.usick_type & SICK_NONVOMITABLE)
			add_colored_text("Ill", ret);
	}

	if (Blind)
		add_colored_text_match(bot2_abbrev >= 2 ? "Bnd" : "Blind", "Blind", ret);
	if (Deaf)
		add_colored_text("Deaf", ret);
	if (Stunned)
		add_colored_text_match(bot2_abbrev >= 2 ? "Stn" : "Stun", "Stun", ret);
	if (Hallucination)
		add_colored_text_match(bot2_abbrev >= 2 ? "Hal" : "Hallu", "Hallu", ret);
	if (Slimed)
		add_colored_text_match(bot2_abbrev >= 2 ? "Slm" : "Slime", "Slime", ret);
	if (u.ustuck && !u.uswallow && !sticks(youmonst.data))
		add_colored_text("Held", ret);
	if (cap > UNENCUMBERED)
		add_colored_text_match(bot2_abbrev >= 2 ? enc_abbrev_stat[cap] : enc_stat[cap], enc_stat[cap], ret);

	return dret;
}

static void bot2(void) {
	curs(WIN_STATUS, 1, 1);

	nhstr newbot2 = bot2str();
	putnstr(WIN_STATUS, 0, newbot2);
	del_nhs(&newbot2);
}

/* WAC -- Shorten bot1 to fit in len spaces.
 * Not currently used
 * Longest string past Str: is
 * ". Str:18/99 Dx:11 Co:13 In:12 Wi:14 Ch:14 Neutral" or 49 Chars long.
 */
#if 0
const char *shorten_bot1(const char *str, int len) {
	static char cbuf[BUFSZ];

	const char *bp0 = str;
	char *bp1 = cbuf;
	int k = 0;

	do {
		*bp1++ = *bp0;
		k++;
	} while(*bp0++ && k < (len - 49));

	cbuf[k] = '.';
	bp1++;

	bp0 = index(str, ':') - 3;
	do {
		*bp1++ = *bp0;
	} while(*bp0++);
	return cbuf;
}
#endif /* 0 */

/* ALI -- Shorten bot2 to fit in len spaces.
 * Currently only used by tty port
 * After the fourth attempt the longest practical bot2 becomes:
 *      HP:700(700) Pw:111(111) AC:-127 Exp:30
 *      Sat Lev Cnf FPs Ill Bnd Stn Hal Slm Old
 * -- or just under 80 characters
 */
#ifdef TTY_GRAPHICS
#if 0
void shorten_bot2(nhstr **str, usize len) {
	nhstr *s;
	for (bot2_abbrev = 1; bot2_abbrev <= 4; bot2_abbrev++) {
		s = bot2str();
		if (s->len <= len) {
			break;
		}
		del_nhs(s);
	}

	bot2_abbrev = 0;

	char *ret = nhs2cstr_tmp(s);
	del_nhs(s);
	return ret;
}
#endif
#endif /* TTY_GRAPHICS */

static void (*raw_handler)();

static void bot_raw(boolean reconfig) {
	const char *botl_raw_values[24], **rv = botl_raw_values;
	char strength[8], dex[5], con[5], itl[5], wis[5], cha[5], score[21];
	int uhp;
	char dlevel[BUFSZ];
	char hp[21], hpmax[21], pw[21], pwmax[21], gold[21], ac[21], elevel[21];
	char expr[21], iweight[21], capacity[21], flgs[21], tim[21];
	*rv++ = reconfig ? "player" : botl_player();
	nhstr nstr = botl_strength();
	*rv++ = reconfig ? "strength" : (sprintf(strength, "%.7s", nhs2cstr_tmp_destroy(&nstr)), strength);
	*rv++ = reconfig ? "dexterity" : (sprintf(dex, "%d", ACURR(A_DEX)), dex);
	*rv++ = reconfig ? "constitution" : (sprintf(con, "%d", ACURR(A_CON)), con);
	*rv++ = reconfig ? "intelligence" : (sprintf(itl, "%d", ACURR(A_INT)), itl);
	*rv++ = reconfig ? "wisdom" : (sprintf(wis, "%d", ACURR(A_WIS)), wis);
	*rv++ = reconfig ? "charisma" : (sprintf(cha, "%d", ACURR(A_CHA)), cha);
	*rv++ = reconfig ? "alignment" : u.ualign.type == A_CHAOTIC ? "Chaotic" : u.ualign.type == A_NEUTRAL ? "Neutral" : "Lawful";
	if (flags.showscore)
		*rv++ = reconfig ? "score" :
			(sprintf(score, "%ld", botl_score()), score);
	uhp = Upolyd ? u.mh : u.uhp;
	if (uhp < 0) uhp = 0;
	describe_level(dlevel, true);
	eos(dlevel)[-1] = 0;
	*rv++ = reconfig ? "dlevel" : dlevel;
	*rv++ = reconfig ? "gold" : (sprintf(gold, "%ld", money_cnt(invent)), gold);
	*rv++ = reconfig ? "hp" : (sprintf(hp, "%d", uhp), hp);
	*rv++ = reconfig ? "hpmax" :
			   (sprintf(hpmax, "%d", Upolyd ? u.mhmax : u.uhpmax), hpmax);
	*rv++ = reconfig ? "pw" : (sprintf(pw, "%d", u.uen), pw);
	*rv++ = reconfig ? "pwmax" : (sprintf(pwmax, "%d", u.uenmax), pwmax);
	*rv++ = reconfig ? "ac" : (sprintf(ac, "%d", u.uac), ac);
	sprintf(elevel, "%u",
		Upolyd && u.ulycn != u.umonnum ? mons[u.umonnum].mlevel : u.ulevel);
	*rv++ = reconfig ? (Upolyd ? "hitdice" : "elevel") : elevel;

	if (flags.showexp)
		*rv++ = reconfig ? "experience" : (sprintf(expr, "%ld", u.uexp), expr);

	if (flags.showweight) {
		*rv++ = reconfig ? "weight" : (sprintf(iweight, "%ld", (long)(inv_weight() + weight_cap())), iweight);
		*rv++ = reconfig ? "capacity" : (sprintf(capacity, "%ld", (long)weight_cap()), capacity);
	}

	if (flags.time)
		*rv++ = reconfig ? "time" : (sprintf(tim, "%ld", moves), tim);
	*rv++ = reconfig ? "hunger" : strcmp(hu_stat[u.uhs], "        ") ? hu_stat[u.uhs] : "";
	*rv++ = reconfig ? "encumberance" : enc_stat[near_capacity()];
	*rv++ = reconfig ? "flags" : (sprintf(flgs, "%d", (Levitation ? RAW_STAT_LEVITATION : 0) | (Confusion ? RAW_STAT_CONFUSION : 0) | (Sick && (u.usick_type & SICK_VOMITABLE) ? RAW_STAT_FOODPOIS : 0) | (Sick && (u.usick_type & SICK_NONVOMITABLE) ? RAW_STAT_ILL : 0) | (Blind ? RAW_STAT_BLIND : 0) | (Stunned ? RAW_STAT_STUNNED : 0) | (Hallucination ? RAW_STAT_HALLUCINATION : 0) | (Slimed ? RAW_STAT_SLIMED : 0)), flgs);
	(*raw_handler)(reconfig, rv - botl_raw_values, botl_raw_values);
}

void bot_reconfig(void) {
	if (raw_handler)
		bot_raw(true);
	context.botl = 1;
}

void bot_set_handler(void (*handler)(void)) {
	raw_handler = handler;
	bot_reconfig();
}

void bot(void) {
	/*
	 * ALI: Cope with the fact that u_init may not have been
	 * called yet. This happens if the player selection menus
	 * are long enough to overwite the status line. In this
	 * case we will be called when the menu is removed while
	 * youmonst.data is still NULL.
	 */
	if (!youmonst.data)
		return;
	if (raw_handler)
		bot_raw(false);
	else {
		bot1();
		bot2();
	}
	context.botl = context.botlx = 0;
}

/*botl.c*/
