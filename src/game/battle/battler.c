#include "game/bee.h"
#include "battle.h"
#include "battler.h"
#include "dict.h"

#define BATTLER_AVATAR_W 48
#define BATTLER_AVATAR_H 64

/* Init human.
 */
 
void battler_init_human(struct battler *battler) {
  battler->human=1;
  battler->logcolor=0x6030ffff;
  battler->maxword=7;
  battler->dictid=RID_dict_nwl2023;
  if (battler->id==1) {
    memcpy(battler->name,"Dot",3);
    battler->namec=3;
    battler->avatar.y=0;
    battler->hp=g.hp;
    memcpy(battler->inventory,g.inventory,sizeof(battler->inventory));
  } else {
    memcpy(battler->name,"Moon",4);
    battler->namec=4;
    battler->avatar.y=BATTLER_AVATAR_H;
    battler->hp=100;
  }
  battler->disphp=battler->hp;
  battler->avatar.imageid=RID_image_avatars;
  battler->avatar.x=0;
  battler->avatar.w=BATTLER_AVATAR_W;
  battler->avatar.h=BATTLER_AVATAR_H;
  battler->avatar.face=BATTLER_FACE_NEUTRAL;
}

/* Init generic CPU.
 */
 
void battler_init_cpu(struct battler *battler) {
  battler->human=0;
  battler->logcolor=0xc04040ff;
  battler->dictid=RID_dict_nwl2023;//TODO "pidgin", once i finish composing it
  battler->wakeup= 1.0;
  battler->charge=10.0;
  battler->maxword=7;
  battler->reqlen=0;
  battler->finisher=0;
  battler->forbidden[0]=0;
  battler->super_effective[0]=0;
  memcpy(battler->name,"Foe",3);
  battler->namec=3;
  battler->avatar.imageid=RID_image_avatars;
  battler->avatar.x=0;
  battler->avatar.y=BATTLER_AVATAR_H*2;
  battler->avatar.w=BATTLER_AVATAR_W;
  battler->avatar.h=BATTLER_AVATAR_H;
  battler->avatar.face=BATTLER_FACE_NEUTRAL;
}

/* Add letter to this "starts" list if warranted.
 * Return new length (either c or c+1).
 */
 
static int battler_insert_start(char *v,int c,char ch) {
  if (ch<'A') return c;
  if (ch>'Z') return c;
  if (c>=7) return 7;
  int i=0; for (;i<c;i++) {
    if (v[i]==ch) return c;
    if (v[i]>ch) {
      memmove(v+i+1,v+i,c-i);
      v[i]=ch;
      return c+1;
    }
  }
  v[c]=ch;
  return c+1;
}

/* Begin round.
 */
 
void battler_begin_round(struct battler *battler,struct battle *battle) {
  memset(battler->stage,0,sizeof(battler->stage));
  battler->modifier=0;
  battler->avatar.face=BATTLER_FACE_NEUTRAL;
  battler->ready=0;
  battler->confirm_fold=0;
  battler->erasing=0;
  battler->sely=2;
  battler->selx=0;
  battler->wcmodal=0;
  battler->damage_done=0;
  
  /* Reset the search, if we're CPU-controlled.
   */
  if (!battler->human) {
    if (!battler->bucketc) {
      battler->bucketc=dict_get_all(battler->bucketv,sizeof(battler->bucketv)/sizeof(battler->bucketv[0]),battler->dictid);
      while (battler->bucketc&&(battler->bucketv[battler->bucketc-1].len>battler->maxword)) battler->bucketc--;
      // If we're lenonly, empty buckets 2, 3, and 4. Spares us the need to consider lenonly during selection.
      if (battler->lenonly) {
        struct dict_bucket *bucket=battler->bucketv;
        int i=battler->bucketc;
        for (;i-->0;bucket++) {
          if (bucket->len<5) bucket->c=0;
        }
      }
    }
    battler->searchbucketp=0;
    battler->searchwordp=0;
    battler->candidatec=0;
    battler->gatherclock=0.0;
    battler->searchclock=0.0;
    battler->startc=0;
    int i=7; while (i-->0) {
      battler->startc=battler_insert_start(battler->startv,battler->startc,battler->hand[i]);
    }
    battler->startp=0;
  }
}

/* Add a candidate to the list.
 */
 
static void battler_add_candidate(struct battler *battler,const char *v,int c,int score) {
  if (score<0) return;
  
  // If the list is full, either abort (if we're worse than the current worst), or drop the current worst.
  if (battler->candidatec>=BATTLER_SEARCH_LIMIT) {
    if (score<=battler->candidatev[0].score) {
      return;
    } else {
      battler->candidatec=BATTLER_SEARCH_LIMIT-1;
      memmove(battler->candidatev,battler->candidatev+1,sizeof(struct candidate)*battler->candidatec);
    }
  }
  
  int lo=0,hi=battler->candidatec;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct candidate *q=battler->candidatev+ck;
         if (score<q->score) hi=ck;
    else if (score>q->score) lo=ck+1;
    else { lo=ck; break; }
  }
  
  struct candidate *dst=battler->candidatev+lo;
  memmove(dst+1,dst,sizeof(struct candidate)*(battler->candidatec-lo));
  battler->candidatec++;
  memset(dst->word,0,7);
  memcpy(dst->word,v,c);
  dst->score=score;
}

/* Nonzero if the given word is playable from the given hand.
 * Hand may contain wildcards.
 */
 
static int battler_can_play_word(const char *hand,const char *v,int c) {
  if (c>7) return 0;
  char remaining[7];
  memcpy(remaining,hand,7);
  int remainingc=7;
  for (;c-->0;v++) {
    int i=remainingc,ok=0,wcp=-1;
    while (i-->0) {
      if (*v==remaining[i]) {
        ok=1;
        remainingc--;
        memmove(remaining+i,remaining+i+1,remainingc-i);
        break;
      }
      if (remaining[i]=='@') wcp=i;
    }
    if (ok) continue;
    if (wcp<0) return 0;
    remainingc--;
    memmove(remaining+wcp,remaining+wcp+1,remainingc-wcp);
  }
  return 1;
}

/* Advance search.
 * Examine no more than one word, then return.
 */
 
static void battler_advance_search(struct battler *battler) {
  if (battler->searchbucketp>=battler->bucketc) return;
  const struct dict_bucket *bucket=battler->bucketv+battler->searchbucketp;
  if ((battler->searchwordp>=bucket->c)||(battler->startp>=battler->startc)) {
    battler->searchbucketp++;
    battler->searchwordp=0;
    battler->startp=0;
    return;
  }
  const char *word=bucket->v+battler->searchwordp*(bucket->len+1);
  if (word[0]<battler->startv[battler->startp]) {
    battler->searchwordp++;
    return;
  }
  if (word[0]>battler->startv[battler->startp]) {
    battler->startp++;
    return;
  }
  if (battler_can_play_word(battler->hand,word,bucket->len)) {
    int score=dict_rate_word(0,battler->dictid,word,bucket->len);
    battler_add_candidate(battler,word,bucket->len,score);
  }
  battler->searchwordp++;
}

/* Update, GATHER stage.
 */
 
void battler_update(struct battler *battler,double elapsed,struct battle *battle) {
  if (battler->human) {
    // Probably nothing to do.
  } else {
    battler->gatherclock+=elapsed;
    
    /* If we haven't finished searching yet, pay that out a little.
     */
    if (battler->searchbucketp<battler->bucketc) {
      battler->searchclock+=elapsed;
      while ((battler->searchclock-=BATTLER_SEC_PER_SEARCH)>0.0) {
        battler_advance_search(battler);
      }
    }
    
    /* With (preempt), if we are fully charged, auto-fold the other player.
     */
    if (battler->preempt&&(battler->gatherclock>battler->charge)) {
      struct battler *other=(battler==&battle->p1)?&battle->p2:&battle->p1;
      other->confirm_fold=1;
      other->ready=1;
    }
  }
}

/* For CPU player, stage the given word.
 * Input is 7 bytes, zero-padded, uppercase only.
 * If somehow this word can't be made from our hand, we'll stage something feasible and let nature take its course.
 * But of course (src) should have come from the dictionary search, and would only accept ones we can make.
 */
 
static void battler_stage_candidate(struct battler *battler,const char *src) {
  memset(battler->stage,0,sizeof(battler->stage));
  int i=7,stagec=0;
  for (;i-->0;src++) {
    if (!*src) break;
    int handp=0,wcp=-1,ok=0;
    for (;handp<7;handp++) {
      if (battler->hand[handp]==*src) {
        ok=1;
        battler->stage[stagec++]=*src;
        battler->hand[handp]=0;
        break;
      }
      if (battler->hand[handp]=='@') wcp=handp;
    }
    if (ok) continue;
    if (wcp<0) {
      fprintf(stderr,"%s:%d:PANIC: %.*s expected to play a '%c' but doesn't have one!\n",__FILE__,__LINE__,battler->namec,battler->name,*src);
      continue;
    }
    battler->hand[wcp]=0;
    battler->stage[stagec++]=(*src)+0x20;
  }
}

/* Commit gather.
 */
 
void battler_commit(struct battler *battler,struct battle *battle) {
  battler->erasing=0;
  battler->wcmodal=0;

  /* If we're a robot, copy a word from our candidates into stage, or fold.
   */
  if (!battler->human) {
    if (battler->candidatec<1) {
      battler->confirm_fold=1;
    } else if (battler->gatherclock<battler->wakeup) {
      battler->confirm_fold=1;
    } else {
      double trange=battler->charge-battler->wakeup;
      if (trange<=0.0) trange=1.0;
      double norm=(battler->gatherclock-battler->wakeup)/trange;
      int p;
      if (norm<=0.0) p=0;
      else if (norm>=1.0) p=battler->candidatec-1;
      else p=(int)(norm*battler->candidatec);
      if (p<0) p=0; else if (p>=battler->candidatec) p=battler->candidatec-1;
      const struct candidate *can=battler->candidatev+p;
      battler_stage_candidate(battler,can->word);
    }
  }
  
  if (battler->confirm_fold) {
    memset(battler->stage,0,sizeof(battler->stage));
    memset(battler->hand,0,sizeof(battler->hand));
    if (battler->modifier) {
      battler->inventory[battler->modifier]++;
      battler->modifier=0;
    }
    battler->attackc=0;
    battler->force=0;
    return;
  }
  
  int len=0;
  while ((len<7)&&battler->stage[len]) len++;
  memcpy(battler->attack,battler->stage,len);
  battler->attackc=len;
  memset(&battler->detail,0,sizeof(battler->detail));
  battler->detail.modifier=battler->modifier;
  battler->detail.forbidden=battler->forbidden;
  battler->detail.super_effective=battler->super_effective;
  battler->detail.lenonly=battler->lenonly;
  battler->force=dict_rate_word(&battler->detail,battler->dictid,battler->stage,len);
  
  // dict doesn't enforce (reqlen) but we can, easily. Just make sure that negatives remain untouched.
  if (battler->reqlen&&(len!=battler->reqlen)&&(battler->force>0)) {
    battler->force=0;
  }
  
  // Likewise, dict doesn't enforce (finisher).
  if (battler->finisher&&(battler->force>0)) {
    struct battler *foe=(battler==&battle->p1)?&battle->p2:&battle->p1;
    if (battler->force>=foe->hp) {
      const char *q=battler->attack;
      int i=battler->attackc,ok=0;
      for (;i-->0;q++) {
        if ((*q==battler->finisher)||(*q==battler->finisher+0x20)) {
          ok=1;
          break;
        }
      }
      if (!ok) {
        battler->force=foe->hp-1;
      }
    }
  }
  
  memset(battler->stage,0,sizeof(battler->stage));
}

/* Submit by human player.
 */
 
static void battler_submit(struct battler *battler) {
  if (battler->erasing) {
    battler->erasing=0;
    battler->inventory[ITEM_ERASER]++;
  }
  if (!battler->stage[0]) {
    //TODO rejection sound effect
    return;
  }
  //TODO sound effect
  battler->ready=1;
}

/* Fold by human player.
 */
 
static void battler_fold(struct battler *battler) {
  if (battler->erasing) {
    battler->erasing=0;
    battler->inventory[ITEM_ERASER]++;
  }
  if (battler->confirm_fold) {
    //TODO sound effect
    battler->ready=1;
  } else {
    //TODO sound effect
    battler->confirm_fold=1;
  }
}

/* Pick item.
 */
 
static void battler_pick_item(struct battler *battler,int itemid) {
  if (itemid==ITEM_ERASER) {
    // Perfectly ok to enter erase mode while a modifier is selected. That's why ERASER is special here.
    if (battler->erasing) {
      //TODO sound effect
      battler->erasing=0;
      battler->inventory[ITEM_ERASER]++;
    } else if (battler->inventory[ITEM_ERASER]) {
      //TODO sound effect
      battler->erasing=1;
      battler->inventory[ITEM_ERASER]--;
    } else {
      //TODO rejection sound effect
    }
  // All non-eraser items are modifiers:
  } else {
    if (battler->erasing) {
      battler->erasing=0;
      battler->inventory[ITEM_ERASER]++;
    }
    if (itemid==battler->modifier) {
      //TODO sound effect for unselect modifier
      battler->modifier=0;
      battler->inventory[itemid]++;
    } else if (battler->inventory[itemid]>0) {
      //TODO sound effect for select modifier
      if (battler->modifier) battler->inventory[battler->modifier]++;
      battler->modifier=itemid;
      battler->inventory[itemid]--;
    } else {
      //TODO rejection sound effect
    }
  }
}

/* Unstage the last letter played (press B).
 */
 
static void battler_unstage_recent(struct battler *battler) {
  int stagec=0;
  while ((stagec<7)&&battler->stage[stagec]) stagec++;
  if (!stagec) {
    //TODO rejection sound effect
    return;
  }
  int i=0; for (;i<7;i++) {
    if (battler->hand[i]) continue;
    stagec--;
    char tileid=battler->stage[stagec];
    battler->stage[stagec]=0;
    if ((tileid>='a')&&(tileid<='z')) tileid='@'; // regeneralize wildcard
    battler->hand[i]=tileid;
    //TODO sound effect
    return;
  }
}

/* Unstage focussed letter.
 */
 
static void battler_unstage_letter(struct battler *battler) {
  if ((battler->selx<0)||(battler->selx>=7)||!battler->stage[battler->selx]) {
    //TODO rejection sound effect
    return;
  }
  if (battler->erasing) { // Unceremoniously drop erasing and proceed. We can't erase staged letters, would be complicated.
    battler->erasing=0;
    battler->inventory[ITEM_ERASER]++;
  }
  char tileid=battler->stage[battler->selx];
  if ((tileid>='a')&&(tileid<='z')) tileid='@';
  memmove(battler->stage+battler->selx,battler->stage+battler->selx+1,7-battler->selx-1);
  battler->stage[6]=0;
  int i=0; for (;i<7;i++) {
    if (battler->hand[i]) continue;
    battler->hand[i]=tileid;
    //TODO sound effect
    return;
  }
}

/* Stage focussed letter.
 */
 
static void battler_stage_letter(struct battler *battler,char wildcard_choice) {
  if ((battler->selx<0)||(battler->selx>=7)||!battler->hand[battler->selx]) {
    //TODO rejection sound effect
    return;
  }
  char tileid=battler->hand[battler->selx];
  if (battler->erasing) {
    if (tileid=='@') {
      //TODO rejection sound effect
      return;
    }
    //TODO sound effect
    battler->hand[battler->selx]='@';
    battler->erasing=0;
    return;
  }
  if (wildcard_choice) tileid=wildcard_choice;
  if (tileid=='@') {
    battler->wcmodal=1;
    battler->wcx=WCMODAL_COLC>>1;
    battler->wcy=WCMODAL_ROWC>>1;
    return;
  }
  battler->hand[battler->selx]=0;
  int i=0; for (;i<7;i++) {
    if (battler->stage[i]) continue;
    battler->stage[i]=tileid;
    //TODO sound effect
    return;
  }
}

/* User input.
 */
 
void battler_move(struct battler *battler,int dx,int dy) {
  if (battler->ready) return;
  //TODO sound effect
  
  if (battler->wcmodal) {
    battler->wcx+=dx;
    if (battler->wcx<0) battler->wcx=WCMODAL_COLC-1;
    else if (battler->wcx>=WCMODAL_COLC) battler->wcx=0;
    battler->wcy+=dy;
    if (battler->wcy<0) battler->wcy=WCMODAL_ROWC-1;
    else if (battler->wcy>=WCMODAL_ROWC) battler->wcy=0;
    return;
  }
  
  battler->confirm_fold=0;
  if (dy) {
    battler->sely+=dy;
    if (battler->sely<0) battler->sely=2;
    else if (battler->sely>2) battler->sely=0;
    if (battler->sely==0) battler->selx=0; // Moving to the top row puts you on "submit", always.
  } else if (dx) {
    battler->selx+=dx;
    if (battler->selx<0) battler->selx=6;
    else if (battler->selx>6) battler->selx=0;
  }
}

void battler_activate(struct battler *battler) {
  if (battler->ready) return;
  
  if (battler->wcmodal) {
    battler->wcmodal=0;
    char letter='a'+battler->wcy*WCMODAL_COLC+battler->wcx;
    if ((letter<'a')||(letter>'z')) {
      //TODO sound effect (dismiss modal)
      return;
    } else {
      battler_stage_letter(battler,letter);
      return;
    }
  }
  
  switch (battler->sely) {
    case 0: switch (battler->selx) {
        case 0: battler_submit(battler); break;
        case 1: battler_fold(battler); break;
        case 2: battler_pick_item(battler,ITEM_ERASER); break;
        case 3: battler_pick_item(battler,ITEM_2XLETTER); break;
        case 4: battler_pick_item(battler,ITEM_3XLETTER); break;
        case 5: battler_pick_item(battler,ITEM_2XWORD); break;
        case 6: battler_pick_item(battler,ITEM_3XWORD); break;
      } break;
    case 1: battler_unstage_letter(battler); break;
    case 2: battler_stage_letter(battler,0); break;
  }
}

void battler_cancel(struct battler *battler) {
  if (battler->wcmodal) {
    //TODO sound effect
    battler->wcmodal=0;
    return;
  }
  if (battler->ready) {
    //TODO sound effect
    battler->ready=0;
    battler->confirm_fold=0;
    return;
  }
  if (battler->erasing) {
    //TODO sound effect
    battler->erasing=0;
    battler->inventory[ITEM_ERASER]++;
    return;
  }
  if (battler->confirm_fold) {
    //TODO sound effect
    battler->confirm_fold=0;
    return;
  }
  battler_unstage_recent(battler);
}
