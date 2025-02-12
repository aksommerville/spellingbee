#include "game/bee.h"
#include "game/battle/battle.h"
#include <stdarg.h>

/* Cleanup.
 */

void battle_cleanup(struct battle *battle) {
  egg_texture_del(battle->texid_msg);
  egg_texture_del(battle->log_texid);
  if (battle->log) free(battle->log);
}

/* Init.
 */
 
void battle_init(struct battle *battle) {
  battle->rid=0;
  battle->p1.id=1;
  battle->p2.id=2;
  
  if ((battle->logw=g.fbw-TILESIZE*17-2)<1) battle->logw=1;
  if ((battle->logh=(g.fbh>>1)-10)<1) battle->logh=1;
  battle->log=calloc(battle->logw<<2,battle->logh);
}

/* Enter WELCOME stage, part of load.
 */
 
static void battle_begin_WELCOME(struct battle *battle) {
  battle->stage=BATTLE_STAGE_WELCOME;
  battle->stageclock=BATTLE_WELCOME_TIME;
  
  /* Set a meaningful message saying who's fighting.
   */
  char msg[64];
  int msgc=0;
  if (battle->p1.human==battle->p2.human) {
    // Both human or both CPU, use a symmetric message.
    memcpy(msg+msgc,battle->p1.name,battle->p1.namec);
    msgc+=battle->p1.namec;
    memcpy(msg+msgc," vs ",4);
    msgc+=4;
    memcpy(msg+msgc,battle->p2.name,battle->p2.namec);
    msgc+=battle->p2.namec;
    msg[msgc++]='!';
  } else {
    // One player, much more likely, don't call out the player's name.
    memcpy(msg+msgc,battle->p2.name,battle->p2.namec);
    msgc+=battle->p2.namec;
    memcpy(msg+msgc," draws near!",12);
    msgc+=12;
  }
  battle->texid_msg=font_tex_oneline(g.font,msg,msgc,g.fbw,0xffffffff);
  egg_texture_get_status(&battle->w_msg,&battle->h_msg,battle->texid_msg);
  
  battle_log(battle,"Fight!!!",8,0xff0000ff);
}

/* Enter CONFIG2 stage.
 */
 
static void battle_begin_CONFIG2(struct battle *battle) {
  battle->stage=BATTLE_STAGE_CONFIG2;
  battle->stageclock=0.0;
}

/* Load resource.
 */

int battle_load(struct battle *battle,const char *src,int srcc,int rid) {
  TRACE("battle:%d",rid)
  int err;
  battler_init_human(&battle->p1);
  battler_init_cpu(&battle->p2);
  if ((err=battle_decode(battle,src,srcc,rid))<0) return err;
  
  /* Shuffle letterbag.
   * Also, tho it's not strictly required, draw both battler's hands.
   * Doing it here lets the human players peek at their tiles before the clock starts ticking.
   */
  letterbag_reset(&battle->letterbag);
  if (battle->novowels) letterbag_remove_vowels(&battle->letterbag);
  letterbag_draw(battle->p1.hand,&battle->letterbag);
  if (battle->p2.twin) memcpy(battle->p2.hand,battle->p1.hand,sizeof(battle->p1.hand));
  else letterbag_draw(battle->p2.hand,&battle->letterbag);
  
  egg_play_song(battle->songid,0,1);
  
  battle_begin_WELCOME(battle);
  if (!(battle->rid=rid)) battle->rid=-1;
  battle->bgrow=g.world.battlebg;
  return 0;
}

/* Initialize 2-player battle.
 */
 
int battle_load_twoplayer(struct battle *battle) {
  TRACE("")

  battler_human_nocontext(&battle->p1);
  battler_human_nocontext(&battle->p2);
  
  letterbag_reset(&battle->letterbag);
  letterbag_draw(battle->p1.hand,&battle->letterbag);
  letterbag_draw(battle->p2.hand,&battle->letterbag);
  
  egg_play_song(RID_song_fourteen_circles,0,1);
  
  battle_begin_CONFIG2(battle);
  battle->rid=-1;
  battle->bgrow=1;
  battle->bgname="Laboratory";
  return 0;
}

/* Enter GATHER stage.
 */
 
static void battle_begin_GATHER(struct battle *battle) {
  battle->stage=BATTLE_STAGE_GATHER;
  
  // If both battlers are robots, run for a fixed interval.
  // NB: Robot-vs-robot never actually happens.
  // At least one human, we wait for manual submit.
  if (!battle->p1.human&&!battle->p2.human) {
    battle->stageclock=5.0;
  } else {
    battle->stageclock=0.0;
  }
  
  battle->hurryclock=0.0;
  battle->hurrysmall=0.0;
  battle->w_msg=0;
  battle->first=0;
  battle->second=0;
  battle->p1.avatar.face=0;
  battle->p2.avatar.face=0;
  
  int p1ok=letterbag_draw_partial(battle->p1.hand,&battle->letterbag);
  int p2ok=p1ok;
  if (battle->p2.twin) memcpy(battle->p2.hand,battle->p1.hand,sizeof(battle->p1.hand));
  else p2ok=letterbag_draw_partial(battle->p2.hand,&battle->letterbag);
  if (!p1ok&&!p2ok) {
    battle_log(battle,"Reshuffling letters.",-1,0xffffffff);
    letterbag_reset(&battle->letterbag);
    if (battle->novowels) letterbag_remove_vowels(&battle->letterbag);
    letterbag_draw(battle->p1.hand,&battle->letterbag);
    if (battle->p2.twin) memcpy(battle->p2.hand,battle->p1.hand,sizeof(battle->p1.hand));
    else letterbag_draw(battle->p2.hand,&battle->letterbag);
  }
  battler_begin_round(&battle->p1,battle);
  battler_begin_round(&battle->p2,battle);
}

/* Enter P1WIN or P2WIN, whichever is appropriate.
 */
 
static void battle_begin_WIN(struct battle *battle) {
  if (battle->p1.hp<=0) {
    TRACE("p2 wins")
    battle_logf(battle,battle->p2.human?0x00ff00ff:0xff0000ff,"%s wins!",battle->p2.name);
    battle->stage=BATTLE_STAGE_P2WIN;
    battle->p1.avatar.face=4;
    battle->p2.avatar.face=5;
    if (battle->p2.human) egg_play_song(RID_song_win_battle,0,0);
    else egg_play_song(RID_song_fatal,0,0);
  } else {
    TRACE("p1 wins")
    battle_logf(battle,battle->p1.human?0x00ff00ff:0xff0000ff,"%s wins!",battle->p1.name);
    if (battle->p1.human) {
      if (battle->p2.gold) battle_logf(battle,0xffff00ff,"Gained %d gold.",battle->p2.gold);
      if (battle->p2.xp) battle_logf(battle,0xffffffff,"Gained %d XP.",battle->p2.xp);
    }
    battle->stage=BATTLE_STAGE_P1WIN;
    battle->p1.avatar.face=5;
    battle->p2.avatar.face=4;
    egg_play_song(RID_song_win_battle,0,0);
  }
  battle->stageclock=0.0;
}

/* Commit attacks.
 * Return nonzero if both battlers are still alive.
 */
 
static int battle_commit_attack(struct battle *battle,struct battler *attacker,struct battler *victim) {
  TRACE("%d '%.*s' valid=%d score=%d",(attacker==&battle->p1)?1:2,attacker->attackc,attacker->attack,attacker->detail.valid,attacker->force);
  if (attacker->attackc&&!attacker->detail.valid) {
    if (attacker->hp<=0) return 0;
  } else {
    if (victim->hp<=0) return 0;
  }
  return 1;
}

/* Enter next stage.
 */
 
static void battle_advance(struct battle *battle) {
  switch (battle->stage) {
    case BATTLE_STAGE_CONFIG2: {
        battle_begin_WELCOME(battle);
      } break;
      
    case BATTLE_STAGE_WELCOME: {
        battle_begin_GATHER(battle);
      } break;
      
    case BATTLE_STAGE_GATHER: {
        battle->last_arrived=0;
        battle->stage=BATTLE_STAGE_ATTACK1;
        battle->stageclock=BATTLE_ATTACK_TIME;
        battler_commit(&battle->p1,battle);
        battler_commit(&battle->p2,battle);
        letterbag_draw_partial(battle->p1.hand,&battle->letterbag);
        if (battle->p2.twin) memcpy(battle->p2.hand,battle->p1.hand,sizeof(battle->p1.hand));
        else letterbag_draw_partial(battle->p2.hand,&battle->letterbag);
        if (!battle->first) battle->first=&battle->p1;
        if (battle->first==&battle->p1) battle->second=&battle->p2;
        else battle->second=&battle->p1;
        battle->first->avatar.face=1;
        battle->second->avatar.face=0;
        if (battle->first->confirm_fold) {
          battle->first->avatar.face=2;
          battle->stageclock=BATTLE_FOLD_TIME;
        }
        char zword[8]={0};
        memcpy(zword,battle->first->attack,battle->first->attackc);
        if (zword[0]) battle_logf(battle,battle->first->logcolor,"%s = %d",zword,battle->first->force);
        else battle_logf(battle,battle->first->logcolor,"(fold)");
      } break;
      
    case BATTLE_STAGE_ATTACK1: {
        battle->last_arrived=0;
        if (battle_commit_attack(battle,battle->first,battle->second)) {
          battle->stage=BATTLE_STAGE_ATTACK2;
          battle->stageclock=BATTLE_ATTACK_TIME;
          battle->first->avatar.face=0;
          battle->second->avatar.face=1;
          battle->p1.damage_done=0;
          battle->p2.damage_done=0;
          if (battle->second->confirm_fold) {
            battle->second->avatar.face=2;
            battle->stageclock=BATTLE_FOLD_TIME;
          }
          char zword[8]={0};
          memcpy(zword,battle->second->attack,battle->second->attackc);
          if (zword[0]) battle_logf(battle,battle->second->logcolor,"%s = %d",zword,battle->second->force);
          else battle_logf(battle,battle->second->logcolor,"(fold)");
        } else {
          battle_begin_WIN(battle);
        }
      } break;
    case BATTLE_STAGE_ATTACK2: {
        if (battle_commit_attack(battle,battle->second,battle->first)) {
          battle_begin_GATHER(battle);
        } else {
          battle_begin_WIN(battle);
        }
      } break;
      
    case BATTLE_STAGE_P1WIN:
    case BATTLE_STAGE_P2WIN:
    case BATTLE_STAGE_TERM:
    default: {
        battle->stage=BATTLE_STAGE_TERM;
        battle->stageclock=0.0;
      } break;
  }
}

/* Update the hurry clock, and damage the laggard if warranted.
 */
 
static void battle_update_hurry(struct battle *battle,double elapsed) {
  if (!battle->p1.human||!battle->p2.human) return; // Only for human-vs-human battles.
  if (battle->p1.ready||battle->p2.ready) {
    battle->hurryclock+=elapsed;
  } else {
    // Both not ready. Zero the clock and do nothing.
    battle->hurryclock=0.0;
    return;
  }
  if (battle->hurryclock<7.0) return; // You get a few seconds for free.
  if ((battle->hurrysmall-=elapsed)>0.0) return;
  battle->hurrysmall+=1.0;
  // Deal 1 HP of damage to whichver isn't ready. Do not take their final HP.
  if (battle->p1.ready) {
    if (battle->p2.hp<2) return;
    battle->p2.hp--;
  } else if (battle->p2.ready) {
    if (battle->p1.hp<2) return;
    battle->p1.hp--;
  }
  egg_play_sound(RID_sound_letterslap);
}

/* Update.
 */

int battle_update(struct battle *battle,double elapsed) {

  // Animations.
  if ((battle->cursorclock-=elapsed)<=0.0) {
    battle->cursorclock+=0.150;
    if (++(battle->cursorframe)>=4) battle->cursorframe=0;
  }
  battle->p1.damageclock-=elapsed;
  battle->p2.damageclock-=elapsed;
  battle->p1.hand_recent_clock+=elapsed;
  battle->p2.hand_recent_clock+=elapsed;
  if ((battle->p1.disphp!=battle->p1.hp)||(battle->p2.disphp!=battle->p2.hp)) {
    battle->hpclock+=elapsed;
    while (battle->hpclock>0.0) {
      battle->hpclock-=BATTLE_HP_TICK_TIME;
      if (battle->p1.disphp<battle->p1.hp) battle->p1.disphp++;
      else if (battle->p1.disphp>battle->p1.hp) battle->p1.disphp--;
      if (battle->p2.disphp<battle->p2.hp) battle->p2.disphp++;
      else if (battle->p2.disphp>battle->p2.hp) battle->p2.disphp--;
    }
    // (hp) can go negative but (disphp) can't. When that happens, we churn needlessly here, but I don't think it matters.
    if (battle->p1.disphp<0) battle->p1.disphp=0;
    if (battle->p2.disphp<0) battle->p2.disphp=0;
  }
  battle->bonus3x-=elapsed;
  
  // Advance stage if the clock says so.
  if (battle->stageclock>0.0) {
    if ((battle->stageclock-=elapsed)<=0.0) {
      battle_advance(battle);
    }
  }
  
  // Update per stage.
  switch (battle->stage) {
    case BATTLE_STAGE_WELCOME: break;
    case BATTLE_STAGE_CONFIG2: break;
    
    case BATTLE_STAGE_GATHER: {
        battle_update_hurry(battle,elapsed);
        battler_update(&battle->p1,elapsed,battle);
        battler_update(&battle->p2,elapsed,battle);
        // Set (first) if we don't have it yet.
        if (!battle->first) {
          if (battle->p1.ready) battle->first=&battle->p1;
          else if (battle->p2.ready) battle->first=&battle->p2;
        // But if (first) unreadies, forget them.
        } else if (!battle->first->ready) {
          battle->first=0;
        }
        // If every human has submitted, advance.
        if (
          (!battle->p1.human||battle->p1.ready)&&
          (!battle->p2.human||battle->p2.ready)
        ) {
          battle_advance(battle);
        }
      } break;
    
    case BATTLE_STAGE_ATTACK1: break;
    case BATTLE_STAGE_ATTACK2: break;
    
    case BATTLE_STAGE_P1WIN: {
        battle->p1.avatar.face=5;
        battle->p2.avatar.face=4;
      } break;
    case BATTLE_STAGE_P2WIN: {
        battle->p1.avatar.face=4;
        battle->p2.avatar.face=5;
      } break;
    
    case BATTLE_STAGE_TERM: default: return 0;
  }
  return 1;
}

/* User input.
 */

void battle_move(struct battle *battle,int playerid,int dx,int dy) {
  switch (battle->stage) {
  
    case BATTLE_STAGE_CONFIG2: {
        if (dy) {
          if ((playerid==1)&&battle->p1.ready) return;
          if ((playerid==2)&&battle->p2.ready) return;
          battle->bgrow+=dy;
          if (battle->bgrow<0) battle->bgrow=6;
          else if (battle->bgrow>6) battle->bgrow=0;
          switch (battle->bgrow) {
            case 0: battle->bgname="Gym"; break;
            case 1: battle->bgname="Laboratory"; break;
            case 2: battle->bgname="Garden"; break;
            case 3: battle->bgname="Cemetery"; break;
            case 4: battle->bgname="Cellar"; break;
            case 5: battle->bgname="Queen"; break;
            case 6: battle->bgname="Underground"; break;
          }
          egg_play_sound(RID_sound_ui_motion);
        } else {
          struct battler *battler=0;
          switch (playerid) {
            case 1: battler=&battle->p1; break;
            case 2: battler=&battle->p2; break;
          }
          if (battler&&!battler->ready) {
            battler_adjust_image(battler,dx);
            egg_play_sound(RID_sound_ui_motion);
          }
        }
      } break;
      
    case BATTLE_STAGE_GATHER: {
        struct battler *battler=0;
        switch (playerid) {
          case 1: battler=&battle->p1; break;
          case 2: battler=&battle->p2; break;
        }
        if (!battler||!battler->human) return;
        battler_move(battler,dx,dy);
      } break;
  }
}

void battle_activate(struct battle *battle,int playerid) {
  switch (battle->stage) {
    
    case BATTLE_STAGE_CONFIG2: {
        switch (playerid) {
          case 1: battle->p1.ready=1; break;
          case 2: battle->p2.ready=1; break;
        }
        egg_play_sound(RID_sound_ui_activate);
        if (battle->p1.ready&&battle->p2.ready) {
          battle_begin_WELCOME(battle);
        }
      } break;
      
    case BATTLE_STAGE_P1WIN:
    case BATTLE_STAGE_P2WIN: {
        battle_advance(battle);
      } break;
      
    case BATTLE_STAGE_GATHER: {
        struct battler *battler=0;
        switch (playerid) {
          case 1: battler=&battle->p1; break;
          case 2: battler=&battle->p2; break;
        }
        if (!battler||!battler->human) return;
        battler_activate(battler);
      } break;
  }
}

void battle_cancel(struct battle *battle,int playerid) {
  switch (battle->stage) {
    
    case BATTLE_STAGE_CONFIG2: {
        egg_play_sound(RID_sound_ui_dismiss);
        if ((playerid==1)&&battle->p1.ready) {
          battle->p1.ready=0;
        } else if ((playerid==2)&&battle->p2.ready) {
          battle->p2.ready=0;
        } else {
          battle->stage=BATTLE_STAGE_TERM;
        }
      } break;
      
    case BATTLE_STAGE_GATHER: {
        struct battler *battler=0;
        switch (playerid) {
          case 1: battler=&battle->p1; break;
          case 2: battler=&battle->p2; break;
        }
        if (!battler||!battler->human) return;
        battler_cancel(battler);
      } break;
  }
}

/* Begin showing damage, if we're not already.
 */
 
void battle_begin_damage(struct battle *battle,struct battler *victim,int force) {
  if (victim->damageclock>0.0) return;
  if (victim->damage_done) return;
  victim->damageclock=BATTLE_DAMAGE_TIME;
  victim->damage=force;
  if (force>0) {
    victim->avatar.face=3;
    victim->hp-=force;
  }
  victim->damage_done=1;
  
  // Apply and report the 7-letter bonus if applicable.
  struct battler *winner=0;
  if (victim==&battle->p1) winner=&battle->p2;
  else winner=&battle->p1;
  if (winner->human&&winner->detail.valid&&(winner->attackc==7)&&(winner->inventory[ITEM_3XWORD]<99)) {
    winner->inventory[ITEM_3XWORD]++;
    battle->bonus3x=1.5;
  }
}

/* Commit to globals.
 * We're not responsible for returned books, but we do set the flag if there is one.
 */
 
void battle_commit_to_globals(struct battle *battle) {
  if (battle->p1.human&&!battle->p2.human) {
    if ((g.stats.hp=battle->p1.hp)<=0) {
      g.stats.hp=0;
    } else {
      if ((g.stats.xp+=battle->p2.xp)>0x7fff) g.stats.xp=0x7fff;
      if ((g.stats.gold+=battle->p2.gold)>=0x7fff) g.stats.gold=0x7fff;
      if ((battle->flagid>0)&&(battle->flagid<256)) {
        flag_set(battle->flagid,1);
      }
    }
    if (battle->restore_hp) g.stats.hp=battle->restore_hp;
    memcpy(g.stats.inventory,battle->p1.inventory,sizeof(g.stats.inventory));
    g.world.status_bar_dirty=1;
    save_game();
  }
  if (battle->on_finish) {
    void (*cb)(struct battle*,void*)=battle->on_finish;
    void *userdata=battle->userdata;
    battle->on_finish=0;
    battle->userdata=0;
    cb(battle,userdata);
  }
}

/* Add a line to the log.
 */
 
void battle_log(struct battle *battle,const char *src,int srcc,uint32_t rgba) {
  int rowh=10;
  memmove(battle->log,battle->log+battle->logw*rowh,battle->logw*(battle->logh-rowh)*4);
  memset(battle->log+battle->logw*(battle->logh-rowh),0,battle->logw*rowh*4);
  font_render_string(
    battle->log,battle->logw,battle->logh,battle->logw<<2,
    0,battle->logh-rowh,
    g.font,src,srcc,rgba
  );
  battle->logdirty=1;
}

/* Add formatted string to log.
 */
 
void battle_logf(struct battle *battle,uint32_t rgba,const char *fmt,...) {
  va_list vargs;
  va_start(vargs,fmt);
  char tmp[256];
  int tmpc=0;
  while (*fmt) {
    if (*fmt=='%') {
      fmt++;
      switch (*fmt) {
        case 's': {
            fmt++;
            const char *src=va_arg(vargs,const char*);
            if (src) {
              for (;*src;src++) {
                if (tmpc<sizeof(tmp)) tmp[tmpc++]=*src;
                else break;
              }
            }
          } break;
        case 'd': {
            fmt++;
            int n=va_arg(vargs,int);
            if (n<0) {
              int digitc=2,limit=-10;
              while (n<=limit) { digitc++; if (limit<=INT_MIN/10) break; limit*=10; }
              if (tmpc>(int)sizeof(tmp)-digitc) break;
              int i=digitc;
              for (;i-->1;n/=10) tmp[tmpc+i]='0'-n%10;
              tmp[tmpc]='-';
              tmpc+=digitc;
            } else {
              int digitc=1,limit=10;
              while (n>=limit) { digitc++; if (limit>=INT_MAX/10) break; limit*=10; }
              if (tmpc>(int)sizeof(tmp)-digitc) break;
              int i=digitc;
              for (;i-->0;n/=10) tmp[tmpc+i]='0'+n%10;
              tmpc+=digitc;
            }
          } break;
        default: {
            if (tmpc<sizeof(tmp)) tmp[tmpc++]='%';
          }
      }
    } else {
      if (tmpc<sizeof(tmp)) tmp[tmpc++]=*fmt;
      fmt++;
    }
  }
  battle_log(battle,tmp,tmpc,rgba);
}

/* Set dark.
 */
 
void battle_set_dark(struct battle *battle) {
  battle->dark=1;
  
  // Replace the welcome message, so we're not saying the monster's name.
  egg_texture_del(battle->texid_msg);
  battle->texid_msg=font_tex_oneline(g.font,"Mystery Monster draws near!",27,g.fbw,0xffffffff);
  egg_texture_get_status(&battle->w_msg,&battle->h_msg,battle->texid_msg);
}

/* Set caption with an ordinal.
 */
 
void battle_set_caption(struct battle *battle,const char *desc,int seq,int count) {
  egg_texture_del(battle->texid_msg);
  int descc=0;
  while (desc[descc]) descc++;
  if (descc>40) return;
  char text[256];
  memcpy(text,desc,descc);
  int textc=descc;
  text[textc++]=' ';
  if (seq>=100) text[textc++]='0'+(seq/100)%10;
  if (seq>=10) text[textc++]='0'+(seq/10)%10;
  text[textc++]='0'+seq%10;
  memcpy(text+textc," of ",4);
  textc+=4;
  if (count>=100) text[textc++]='0'+(count/100)%10;
  if (count>=10) text[textc++]='0'+(count/10)%10;
  text[textc++]='0'+count%10;
  battle->texid_msg=font_tex_oneline(g.font,text,textc,g.fbw,0xffffffff);
  egg_texture_get_status(&battle->w_msg,&battle->h_msg,battle->texid_msg);
}

/* Set finish callback.
 */
 
void battle_on_finish(struct battle *battle,void (*cb)(struct battle *battle,void *userdata),void *userdata) {
  battle->on_finish=cb;
  battle->userdata=userdata;
}
