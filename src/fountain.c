/*	SCCS Id: @(#)fountain.c	3.4	2003/03/23	*/
/*	Copyright Scott R. Turner, srt@ucla, 10/27/86 */
/* NetHack may be freely redistributed.  See license for details. */

/* Code for drinking from fountains. */

#include "hack.h"

static void dowatersnakes(void);
static void dowaterdemon(void);
static void dowaternymph(void);
static void gush(int,int,void *);
static void dofindgem(void);

void floating_above(const char *what) {
    You("are floating high above the %s.", what);
}

// Fountain of snakes!
static void dowatersnakes(void) {
    int num = rn1(5, 2);
    struct monst *mtmp;

    if (!(mvitals[PM_WATER_MOCCASIN].mvflags & G_GONE)) {
	if (!Blind)
	    pline("An endless stream of %s pours forth!",
		  Hallucination ? makeplural(rndmonnam()) : "snakes");
	else
	    You_hear("%s hissing!", something);
	while(num-- > 0)
	    if((mtmp = makemon(&mons[PM_WATER_MOCCASIN],
			u.ux, u.uy, NO_MM_FLAGS)) && t_at(mtmp->mx, mtmp->my))
		mintrap(mtmp);
    } else
	pline_The("fountain bubbles furiously for a moment, then calms.");
}

// Water demon
static void dowaterdemon(void) {
    struct monst *mtmp;

    if(!(mvitals[PM_WATER_DEMON].mvflags & G_GONE)) {
	if((mtmp = makemon(&mons[PM_WATER_DEMON],u.ux,u.uy, NO_MM_FLAGS))) {
	    if (!Blind)
		You("unleash %s!", a_monnam(mtmp));
	    else
		You_feel("the presence of evil.");
/* ------------===========STEPHEN WHITE'S NEW CODE============------------ */
	/* Give those on low levels a (slightly) better chance of survival */
	/* 35% at level 1, 30% at level 2, 25% at level 3, etc... */
	if (rnd(100) > (60 + 5*level_difficulty())) {
		pline("Grateful for %s release, %s grants you a wish!",
		      mhis(mtmp), mhe(mtmp));
		makewish();
		mongone(mtmp);
	    } else if (t_at(mtmp->mx, mtmp->my))
		mintrap(mtmp);
	}
    } else
	pline_The("fountain bubbles furiously for a moment, then calms.");
}

// Water Nymph
static void dowaternymph(void) {
	struct monst *mtmp;

	if(!(mvitals[PM_WATER_NYMPH].mvflags & G_GONE) &&
	   (mtmp = makemon(&mons[PM_WATER_NYMPH],u.ux,u.uy, NO_MM_FLAGS))) {
		if (!Blind)
		   You("attract %s!", a_monnam(mtmp));
		else
		   You_hear("a seductive voice.");
		mtmp->msleeping = 0;
		if (t_at(mtmp->mx, mtmp->my))
		    mintrap(mtmp);
	} else
		if (!Blind)
		   pline("A large bubble rises to the surface and pops.");
		else
		   You_hear("a loud pop.");
}

// Gushing forth along LOS from (u.ux, u.uy)
void dogushforth (int drinking) {
	int madepool = 0;

	do_clear_area(u.ux, u.uy, 7, gush, (void *)&madepool);
	if (!madepool) {
	    if (drinking)
		Your("thirst is quenched.");
	    else
		pline("Water sprays all over you.");
	}
}

static void gush(int x, int y, void *poolcnt) {
	struct monst *mtmp;
	struct trap *ttmp;

	if (((x+y)%2) || (x == u.ux && y == u.uy) ||
	    (rn2(1 + distmin(u.ux, u.uy, x, y)))  ||
	    (levl[x][y].typ != ROOM) ||
	    (sobj_at(BOULDER, x, y)) || nexttodoor(x, y))
		return;

	if ((ttmp = t_at(x, y)) != 0 && !delfloortrap(ttmp))
		return;

	if (!((*(int *)poolcnt)++))
	    pline("Water gushes forth from the overflowing fountain!");

	/* Put a pool at x, y */
	levl[x][y].typ = POOL;
	/* No kelp! */
	del_engr_at(x, y);
	water_damage(level.objects[x][y], false, true);

	if ((mtmp = m_at(x, y)) != 0)
		minliquid(mtmp);
	else
		newsym(x,y);
}

// Find a gem in the sparkling waters.
static void dofindgem(void) {
	if (!Blind) You("spot a gem in the sparkling waters!");
	else You_feel("a gem here!");
	mksobj_at(rnd_class(DILITHIUM_CRYSTAL, LUCKSTONE-1),
			 u.ux, u.uy, false, false);
	SET_FOUNTAIN_LOOTED(u.ux,u.uy);
	newsym(u.ux, u.uy);
	exercise(A_WIS, true);			/* a discovery! */
}

void dryup(xchar x, xchar y, boolean isyou) {
	if (IS_FOUNTAIN(levl[x][y].typ) &&
	    (!rn2(3) || FOUNTAIN_IS_WARNED(x,y))) {
		if(isyou && in_town(x, y) && !FOUNTAIN_IS_WARNED(x,y)) {
			struct monst *mtmp;
			SET_FOUNTAIN_WARNED(x,y);
			/* Warn about future fountain use. */
			for(mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
			    if (DEADMONSTER(mtmp)) continue;
			    if ((mtmp->data == &mons[PM_WATCHMAN] ||
				mtmp->data == &mons[PM_WATCH_CAPTAIN]) &&
			       couldsee(mtmp->mx, mtmp->my) &&
			       mtmp->mpeaceful) {
				pline("%s yells:", Amonnam(mtmp));
				verbalize("Hey, stop using that fountain!");
				break;
			    }
			}
			/* You can see or hear this effect */
			if(!mtmp) pline_The("flow reduces to a trickle.");
			return;
		}
#ifdef WIZARD
		if (isyou && wizard) {
			if (yn("Dry up fountain?") == 'n')
				return;
		}
#endif
		/* replace the fountain with ordinary floor */
		levl[x][y].typ = ROOM;
		levl[x][y].looted = 0;
		levl[x][y].blessedftn = 0;
		if (cansee(x,y)) pline_The("fountain dries up!");
		/* The location is seen if the hero/monster is invisible */
		/* or felt if the hero is blind.			 */
		newsym(x, y);
		level.flags.nfountains--;
		if(isyou && in_town(x, y))
		    angry_guards(false);
	}
}

void drinkfountain(void) {
	/* What happens when you drink from a fountain? */
	boolean mgkftn = (levl[u.ux][u.uy].blessedftn == 1);
	int fate = rnd(30);

	if (Levitation) {
		floating_above("fountain");
		return;
	}

	if (mgkftn && u.uluck >= 0 && fate >= 10) {
		int i, ii, littleluck = (u.uluck < 4);

		pline("Wow!  This makes you feel great!");
		/* blessed restore ability */
		for (ii = 0; ii < A_MAX; ii++)
		    if (ABASE(ii) < AMAX(ii)) {
			ABASE(ii) = AMAX(ii);
			flags.botl = 1;
		    }
		/* gain ability, blessed if "natural" luck is high */
		i = rn2(A_MAX);		/* start at a random attribute */
		for (ii = 0; ii < A_MAX; ii++) {
		    if (adjattrib(i, 1, littleluck ? -1 : 0) && littleluck)
			break;
		    if (++i >= A_MAX) i = 0;
		}
		display_nhwindow(WIN_MESSAGE, false);
		pline("A wisp of vapor escapes the fountain...");
		exercise(A_WIS, true);
		levl[u.ux][u.uy].blessedftn = 0;
		return;
	}

	if (fate < 10) {
		pline_The("cool draught refreshes you.");
		u.uhunger += rnd(10); /* don't choke on water */
		newuhs(false);
		if(mgkftn) return;
	} else {
	    switch (fate) {

		case 19: /* Self-knowledge */

			You_feel("self-knowledgeable...");
			display_nhwindow(WIN_MESSAGE, false);
			enlightenment(0);
			exercise(A_WIS, true);
			pline_The("feeling subsides.");
			break;

		case 20: /* Foul water */

			pline_The("water is foul!  You gag and vomit.");
			morehungry(rn1(20, 11));
			vomit();
			break;

		case 21: /* Poisonous */

			pline_The("water is contaminated!");
			if (Poison_resistance) {
			   pline(
			      "Perhaps it is runoff from the nearby %s farm.",
				 fruitname(false));
			   losehp(rnd(4),"unrefrigerated sip of juice",
				KILLED_BY_AN);
			   break;
			}
			losestr(rn1(4,3));
			losehp(rnd(10),"contaminated water", KILLED_BY);
			exercise(A_CON, false);
			break;

		case 22: /* Fountain of snakes! */

			dowatersnakes();
			break;

		case 23: /* Water demon */
			dowaterdemon();
			break;

		case 24: /* Curse an item */ {
			struct obj *obj;

			pline("This water's no good!");
			morehungry(rn1(20, 11));
			exercise(A_CON, false);
			for(obj = invent; obj ; obj = obj->nobj)
				if (!rn2(5))	curse(obj);
			break;
			}

		case 25: /* See invisible */

			if (Blind) {
			    if (Invisible) {
				You("feel transparent.");
			    } else {
			    	You("feel very self-conscious.");
			    	pline("Then it passes.");
			    }
			} else {
			   You("see an image of someone stalking you.");
			   pline("But it disappears.");
			}
			HSee_invisible |= FROMOUTSIDE;
			newsym(u.ux,u.uy);
			exercise(A_WIS, true);
			break;

		case 26: /* See Monsters */

			monster_detect(NULL, 0);
			exercise(A_WIS, true);
			break;

		case 27: /* Find a gem in the sparkling waters. */

			if (!FOUNTAIN_IS_LOOTED(u.ux,u.uy)) {
				dofindgem();
				break;
			}

		case 28: /* Water Nymph */

			dowaternymph();
			break;

		case 29: /* Scare */ {
			struct monst *mtmp;

			pline("This water gives you bad breath!");
			for(mtmp = fmon; mtmp; mtmp = mtmp->nmon)
			    if(!DEADMONSTER(mtmp))
				monflee(mtmp, 0, false, false);
			}
			break;

		case 30: /* Gushing forth in this room */

			dogushforth(true);
			break;

		default:

			pline("This tepid water is tasteless.");
			break;
	    }
	}
	dryup(u.ux, u.uy, true);
}

void dipfountain(struct obj *obj) {
	if (Levitation) {
		floating_above("fountain");
		return;
	}

	/* Don't grant Excalibur when there's more than one object.  */
	/* (quantity could be > 1 if merged daggers got polymorphed) */

	if (obj->otyp == LONG_SWORD && obj->quan == 1L
	    && u.ulevel > 4 && !rn2(8) && !obj->oartifact
	    && !exist_artifact(LONG_SWORD, artiname(ART_EXCALIBUR))) {

		if (u.ualign.type != A_LAWFUL) {
			/* Ha!  Trying to cheat her. */
			pline("A freezing mist rises from the water and envelopes the sword.");
			pline_The("fountain disappears!");
			curse(obj);
			if (obj->spe > -6 && !rn2(3)) obj->spe--;
			obj->oerodeproof = false;
			exercise(A_WIS, false);
		} else {
			/* The lady of the lake acts! - Eric Backus */
			/* Be *REAL* nice */
	  pline("From the murky depths, a hand reaches up to bless the sword.");
			pline("As the hand retreats, the fountain disappears!");
			obj = oname(obj, artiname(ART_EXCALIBUR));
			discover_artifact(ART_EXCALIBUR);
			bless(obj);
			obj->oeroded = obj->oeroded2 = 0;
			obj->oerodeproof = true;
			exercise(A_WIS, true);
		}
		update_inventory();
		levl[u.ux][u.uy].typ = ROOM;
		levl[u.ux][u.uy].looted = 0;
		newsym(u.ux, u.uy);
		level.flags.nfountains--;
		if(in_town(u.ux, u.uy))
		    angry_guards(false);
		return;
	} else if (get_wet(obj, false) && !rn2(2))
		return;

	/* Acid and water don't mix */
	if (obj->otyp == POT_ACID) {
	    useup(obj);
	    return;
	}

	switch (rnd(30)) {
		case 10: /* Curse the item */
			curse(obj);
			break;
		case 11:
		case 12:
		case 13:
		case 14: /* Uncurse the item */
			if(obj->cursed) {
			    if (!Blind)
				pline_The("water glows for a moment.");
			    uncurse(obj);
			} else {
			    pline("A feeling of loss comes over you.");
			}
			break;
		case 15:
		case 16: /* Water Demon */
			dowaterdemon();
			break;
		case 17:
		case 18: /* Water Nymph */
			dowaternymph();
			break;
		case 19:
		case 20: /* an Endless Stream of Snakes */
			dowatersnakes();
			break;
		case 21:
		case 22:
		case 23: /* Find a gem */
			if (!FOUNTAIN_IS_LOOTED(u.ux,u.uy)) {
				dofindgem();
				break;
			}
		case 24:
		case 25: /* Water gushes forth */
			dogushforth(false);
			break;
		case 26: /* Strange feeling */
			pline("A strange tingling runs up your %s.",
							body_part(ARM));
			break;
		case 27: /* Strange feeling */
			You_feel("a sudden chill.");
			break;
		case 28: /* Strange feeling */
			pline("An urge to take a bath overwhelms you.");
#ifndef GOLDOBJ
			if (u.ugold > 10) {
			    u.ugold -= somegold() / 10;
			    You("lost some of your gold in the fountain!");
			    CLEAR_FOUNTAIN_LOOTED(u.ux,u.uy);
			    exercise(A_WIS, false);
			}
#else
			{
			    long money = money_cnt(invent);
			    struct obj *otmp;
                            if (money > 10) {
				/* Amount to loose.  Might get rounded up as fountains don't pay change... */
			        money = somegold(money) / 10;
			        for (otmp = invent; otmp && money > 0; otmp = otmp->nobj) if (otmp->oclass == COIN_CLASS) {
				    int denomination = objects[otmp->otyp].oc_cost;
				    long coin_loss = (money + denomination - 1) / denomination;
                                    coin_loss = min(coin_loss, otmp->quan);
				    otmp->quan -= coin_loss;
				    money -= coin_loss * denomination;
				    if (!otmp->quan) delobj(otmp);
				}
			        You("lost some of your money in the fountain!");
				CLEAR_FOUNTAIN_LOOTED(u.ux,u.uy);
			        exercise(A_WIS, false);
                            }
			}
#endif
			break;
		case 29: /* You see coins */

		/* We make fountains have more coins the closer you are to the
		 * surface.  After all, there will have been more people going
		 * by.	Just like a shopping mall!  Chris Woodbury  */

		    if (FOUNTAIN_IS_LOOTED(u.ux,u.uy)) break;
		    SET_FOUNTAIN_LOOTED(u.ux,u.uy);
		    mkgold((long)
			(rnd((dunlevs_in_dungeon(&u.uz)-dunlev(&u.uz)+1)*2)+5),
			u.ux, u.uy);
		    if (!Blind)
		pline("Far below you, you see coins glistening in the water.");
		    exercise(A_WIS, true);
		    newsym(u.ux,u.uy);
		    break;
	}
	update_inventory();
	dryup(u.ux, u.uy, true);
}

void diptoilet(struct obj *obj) {
	if (Levitation) {
	    floating_above("toilet");
	    return;
	}
	get_wet(obj, false);
	/* KMH -- acid and water don't mix */
	if (obj->otyp == POT_ACID) {
	    useup(obj);
	    return;
	}
	if(is_poisonable(obj)) {
	    if (flags.verbose)  You("cover it in filth.");
	    obj->opoisoned = true;
	}
	if (obj->oclass == FOOD_CLASS) {
	    if (flags.verbose)  pline("My! It certainly looks tastier now...");
	    obj->orotten = true;
	}
	if (flags.verbose)  pline("Yuck!");
}


void breaksink(int x, int y) {
    if(cansee(x,y) || (x == u.ux && y == u.uy))
	pline_The("pipes break!  Water spurts out!");
    level.flags.nsinks--;
    levl[x][y].doormask = 0;
    levl[x][y].typ = FOUNTAIN;
    level.flags.nfountains++;
    newsym(x,y);
}

void breaktoilet(int x, int y) {
    int num = rn1(5, 2);
    struct monst *mtmp;
    pline("The toilet suddenly shatters!");
    level.flags.nsinks--;
    levl[x][y].typ = FOUNTAIN;
    level.flags.nfountains++;
    newsym(x,y);
    if (!rn2(3)) {
      if (!(mvitals[PM_BABY_CROCODILE].mvflags & G_GONE)) {
	if (!Blind) {
	    if (!Hallucination) pline("Oh no! Crocodiles come out from the pipes!");
	    else pline("Oh no! Tons of poopies!");
	} else
	    You("hear something scuttling around you!");
	while(num-- > 0)
	    if((mtmp = makemon(&mons[PM_BABY_CROCODILE],u.ux,u.uy, NO_MM_FLAGS)) &&
	       t_at(mtmp->mx, mtmp->my))
		mintrap(mtmp);
      } else
	pline("The sewers seem strangely quiet.");
    }
}

void drinksink(void) {
	struct obj *otmp;
	struct monst *mtmp;

	if (Levitation) {
		floating_above("sink");
		return;
	}
	switch(rn2(20)) {
		case 0: You("take a sip of very cold water.");
			break;
		case 1: You("take a sip of very warm water.");
			break;
		case 2: You("take a sip of scalding hot water.");
			if (Fire_resistance)
				pline("It seems quite tasty.");
			else losehp(rnd(6), "sipping boiling water", KILLED_BY);
			break;
		case 3: if (mvitals[PM_SEWER_RAT].mvflags & G_GONE)
				pline_The("sink seems quite dirty.");
			else {
				mtmp = makemon(&mons[PM_SEWER_RAT],
						u.ux, u.uy, NO_MM_FLAGS);
				if (mtmp) pline("Eek!  There's %s in the sink!",
					(Blind || !canspotmon(mtmp)) ?
					"something squirmy" :
					a_monnam(mtmp));
			}
			break;
		case 4: do {
				otmp = mkobj(POTION_CLASS,false);
				if (otmp->otyp == POT_WATER) {
					obfree(otmp, NULL);
					otmp = NULL;
				}
			} while(!otmp);
			otmp->cursed = otmp->blessed = 0;
			pline("Some %s liquid flows from the faucet.",
			      Blind ? "odd" :
			      hcolor(OBJ_DESCR(objects[otmp->otyp])));
			otmp->dknown = !(Blind || Hallucination);
			otmp->fromsink = 1; /* kludge for docall() */
			/* dopotion() deallocs dummy potions */
			dopotion(otmp);
			break;
		case 5: if (!(levl[u.ux][u.uy].looted & S_LRING)) {
			    You("find a ring in the sink!");
			    mkobj_at(RING_CLASS, u.ux, u.uy, true);
			    levl[u.ux][u.uy].looted |= S_LRING;
			    exercise(A_WIS, true);
			    newsym(u.ux,u.uy);
			} else pline("Some dirty water backs up in the drain.");
			break;
		case 6: breaksink(u.ux,u.uy);
			break;
		case 7: pline_The("water moves as though of its own will!");
			if ((mvitals[PM_WATER_ELEMENTAL].mvflags & G_GONE)
			    || !makemon(&mons[PM_WATER_ELEMENTAL],
					u.ux, u.uy, NO_MM_FLAGS))
				pline("But it quiets down.");
			break;
		case 8: pline("Yuk, this water tastes awful.");
			more_experienced(1,0);
			newexplevel();
			break;
		case 9: pline("Gaggg... this tastes like sewage!  You vomit.");
			morehungry(rn1(30-ACURR(A_CON), 11));
			vomit();
			break;
		case 10:
			/* KMH, balance patch -- new intrinsic */
			pline("This water contains toxic wastes!");
			if (!Unchanging) {
			if (!Unchanging) {
				You("undergo a freakish metamorphosis!");
				polyself(false);
			}
			}
			break;
		/* more odd messages --JJB */
		case 11: You_hear("clanking from the pipes...");
			break;
		case 12: You_hear("snatches of song from among the sewers...");
			break;
		case 19: if (Hallucination) {
		   pline("From the murky drain, a hand reaches up... --oops--");
				break;
			}
		default: You("take a sip of %s water.",
			rn2(3) ? (rn2(2) ? "cold" : "warm") : "hot");
	}
}

void drinktoilet(void) {
	if (Levitation) {
		floating_above("toilet");
		return;
	}
	if ((youmonst.data->mlet == S_DOG) && (rn2(5))){
		pline("The toilet water is quite refreshing!");
		u.uhunger += 10;
		return;
	}
	switch(rn2(9)) {
/*
		static struct obj *otmp;
 */
		case 0: if (mvitals[PM_SEWER_RAT].mvflags & G_GONE)
				pline("The toilet seems quite dirty.");
			else {
				static struct monst *mtmp;

				mtmp = makemon(&mons[PM_SEWER_RAT], u.ux, u.uy,
					NO_MM_FLAGS);
				pline("Eek!  There's %s in the toilet!",
					Blind ? "something squirmy" :
					a_monnam(mtmp));
			}
			break;
		case 1: breaktoilet(u.ux,u.uy);
			break;
		case 2: pline("Something begins to crawl out of the toilet!");
			if (mvitals[PM_BROWN_PUDDING].mvflags & G_GONE
			    || !makemon(&mons[PM_BROWN_PUDDING], u.ux, u.uy,
					NO_MM_FLAGS))
				pline("But it slithers back out of sight.");
			break;
		case 3:
		case 4: if (mvitals[PM_BABY_CROCODILE].mvflags & G_GONE)
				pline("The toilet smells fishy.");
			else {
				static struct monst *mtmp;

				mtmp = makemon(&mons[PM_BABY_CROCODILE], u.ux,
					 u.uy, NO_MM_FLAGS);
				pline("Egad!  There's %s in the toilet!",
					Blind ? "something squirmy" :
					a_monnam(mtmp));
			}
			break;
		default: pline("Gaggg... this tastes like sewage!  You vomit.");
			morehungry(rn1(30-ACURR(A_CON), 11));
			vomit();
	}
}


void whetstone_fountain_effects(struct obj *obj) {
	if (Levitation) {
		floating_above("fountain");
		return;
	}

	switch (rnd(30)) {
		case 10: /* Curse the item */
			curse(obj);
			break;
		case 11:
		case 12:
		case 13:
		case 14: /* Uncurse the item */
			if(obj->cursed) {
			    if (!Blind)
				pline_The("water glows for a moment.");
			    uncurse(obj);
			} else {
			    pline("A feeling of loss comes over you.");
			}
			break;
		case 15:
		case 16: /* Water Demon */
			dowaterdemon();
			break;
		case 17:
		case 18: /* Water Nymph */
			dowaternymph();
			break;
		case 19:
		case 20: /* an Endless Stream of Snakes */
			dowatersnakes();
			break;
		case 21:
		case 22:
		case 23: /* Find a gem */
			if (!FOUNTAIN_IS_LOOTED(u.ux,u.uy)) {
				dofindgem();
				break;
			}
		case 24:
		case 25: /* Water gushes forth */
			dogushforth(false);
			break;
		case 26: /* Strange feeling */
			pline("A strange tingling runs up your %s.",
							body_part(ARM));
			break;
		case 27: /* Strange feeling */
			You_feel("a sudden chill.");
			break;
		case 28: /* Strange feeling */
			pline("An urge to take a bath overwhelms you.");
#ifndef GOLDOBJ
			if (u.ugold > 10) {
			    u.ugold -= somegold() / 10;
			    You("lost some of your gold in the fountain!");
			    CLEAR_FOUNTAIN_LOOTED(u.ux,u.uy);
			    exercise(A_WIS, false);
			}
#else
			{
			    long money = money_cnt(invent);
			    struct obj *otmp;
                            if (money > 10) {
				/* Amount to loose.  Might get rounded up as fountains don't pay change... */
			        money = somegold(money) / 10;
			        for (otmp = invent; otmp && money > 0; otmp = otmp->nobj) if (otmp->oclass == COIN_CLASS) {
				    int denomination = objects[otmp->otyp].oc_cost;
				    long coin_loss = (money + denomination - 1) / denomination;
                                    coin_loss = min(coin_loss, otmp->quan);
				    otmp->quan -= coin_loss;
				    money -= coin_loss * denomination;
				    if (!otmp->quan) delobj(otmp);
				}
			        You("lost some of your money in the fountain!");
			        levl[u.ux][u.uy].looted &= ~F_LOOTED;
			        exercise(A_WIS, false);
                            }
			}
#endif
			break;
		case 29: /* You see coins */

		/* We make fountains have more coins the closer you are to the
		 * surface.  After all, there will have been more people going
		 * by.	Just like a shopping mall!  Chris Woodbury  */

		    if (levl[u.ux][u.uy].looted) break;
		    levl[u.ux][u.uy].looted |= F_LOOTED;
		    mkgold((long)
			(rnd((dunlevs_in_dungeon(&u.uz)-dunlev(&u.uz)+1)*2)+5),
			u.ux, u.uy);
		    if (!Blind)
		pline("Far below you, you see coins glistening in the water.");
		    exercise(A_WIS, true);
		    newsym(u.ux,u.uy);
		    break;
	}
	update_inventory();
	dryup(u.ux, u.uy, true);
}


void whetstone_toilet_effects(struct obj *obj) {
	if (Levitation) {
	    floating_above("toilet");
	    return;
	}
	if(is_poisonable(obj)) {
	    if (flags.verbose)  You("cover it in filth.");
	    obj->opoisoned = true;
	}
	if (flags.verbose)  pline("Yuck!");
}

void whetstone_sink_effects(struct obj *obj) {
	struct monst *mtmp;

	if (Levitation) {
		floating_above("sink");
		return;
	}
	switch(rn2(20)) {
		case 0: if (mvitals[PM_SEWER_RAT].mvflags & G_GONE)
				pline_The("sink seems quite dirty.");
			else {
				mtmp = makemon(&mons[PM_SEWER_RAT],
						u.ux, u.uy, NO_MM_FLAGS);
				pline("Eek!  There's %s in the sink!",
					Blind ? "something squirmy" :
					a_monnam(mtmp));
			}
			break;
		case 1: if (!(levl[u.ux][u.uy].looted & S_LRING)) {
			    You("find a ring in the sink!");
			    mkobj_at(RING_CLASS, u.ux, u.uy, true);
			    levl[u.ux][u.uy].looted |= S_LRING;
			    exercise(A_WIS, true);
			    newsym(u.ux,u.uy);
			} else pline("Some dirty water backs up in the drain.");
			break;
		case 2: breaksink(u.ux,u.uy);
			break;
		case 3: pline_The("water moves as though of its own will!");
			if ((mvitals[PM_WATER_ELEMENTAL].mvflags & G_GONE)
			    || !makemon(&mons[PM_WATER_ELEMENTAL],
					u.ux, u.uy, NO_MM_FLAGS))
				pline("But it quiets down.");
			break;
		case 4:
			pline("This water contains toxic wastes!");
			obj = poly_obj(obj, STRANGE_OBJECT);
			u.uconduct.polypiles++;
			break;
		case 5: You_hear("clanking from the pipes...");
			break;
		case 6: You_hear("snatches of song from among the sewers...");
			break;
		case 19: if (Hallucination) {
		   pline("From the murky drain, a hand reaches up... --oops--");
				break;
			}
		default:
			break;
	}
}

/*fountain.c*/
