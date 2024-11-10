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

/* Decode resource into new battle.
 */
 
static int battle_decode(struct battle *battle,const char *src,int srcc,int rid) {
  int srcp=0,lineno=1;
  for (;srcp<srcc;lineno++) {
    const char *line=src+srcp;
    int linec=0,sepp=-1;
    while ((srcp<srcc)&&(src[srcp++]!=0x0a)) {
      if ((sepp<0)&&(line[linec]==0x20)) sepp=linec;
      linec++;
    }
    // Fields with no value:
    if ((linec==7)&&!memcmp(line,"player2",7)) {
      battle->p2.human=1;
      battle->p2.dictid=RID_dict_nwl2023;
      continue;
    }
    if ((linec==8)&&!memcmp(line,"fulldict",8)) {
      battle->p2.dictid=RID_dict_nwl2023;
      continue;
    }
    // Everything else requires a value:
    if (sepp<0) {
      fprintf(stderr,"battle:%d:%d: Unexpected valueless line '%.*s'\n",rid,lineno,linec,line);
      return -2;
    }
    const char *v=line+sepp+1;
    int vc=linec-sepp-1;
    // Text values:
    if ((sepp==4)&&!memcmp(line,"name",4)) {
      if (vc>=sizeof(battle->p2.name)) vc=sizeof(battle->p2.name)-1;
      memcpy(battle->p2.name,v,vc);
      battle->p2.namec=vc;
      battle->p2.name[vc]=0;
      continue;
    }
    if ((sepp==9)&&!memcmp(line,"forbidden",9)) {
      if (vc>=(int)sizeof(battle->p1.forbidden)) {
        fprintf(stderr,"battle:%d:%d: 'forbidden' len %d, limit %d\n",rid,lineno,vc,(int)sizeof(battle->p1.forbidden)-1);
        return -2;
      }
      memcpy(battle->p1.forbidden,v,vc);
      battle->p1.forbidden[vc]=0;
      continue;
    }
    if ((sepp==15)&&!memcmp(line,"super_effective",15)) {
      if (vc>=(int)sizeof(battle->p1.super_effective)) {
        fprintf(stderr,"battle:%d:%d: 'super_effective' len %d, limit %d\n",rid,lineno,vc,(int)sizeof(battle->p1.super_effective)-1);
        return -2;
      }
      memcpy(battle->p1.super_effective,v,vc);
      battle->p1.super_effective[vc]=0;
      continue;
    }
    if ((sepp==8)&&!memcmp(line,"finisher",8)) {
      if (vc!=1) {
        fprintf(stderr,"battle:%d:%d: 'finisher' value must be exactly one char. Found '%.*s'\n",rid,lineno,vc,v);
        return -2;
      }
      battle->p1.finisher=v[0];
      continue;
    }
    // Integer values:
    int vn=0,vp=0;
    for (;vp<vc;vp++) {
      if ((v[vp]<'0')||(v[vp]>'9')) {
        fprintf(stderr,"battle:%d:%d: Expected positive decimal integer, found '%.*s'\n",rid,lineno,vc,v);
        return -2;
      }
      vn*=10;
      vn+=v[vp]-'0';
    }
    if ((sepp==2)&&!memcmp(line,"hp",2)) {
      battle->p2.hp=vn;
      battle->p2.disphp=vn;
      continue;
    }
    if ((sepp==7)&&!memcmp(line,"maxword",7)) {
      battle->p2.maxword=vn;
      continue;
    }
    if ((sepp==6)&&!memcmp(line,"reqlen",6)) {
      battle->p1.reqlen=vn;
      continue;
    }
    if ((sepp==7)&&!memcmp(line,"imageid",7)) {
      battle->p2.avatar.imageid=vn;
      continue;
    }
    if ((sepp==8)&&!memcmp(line,"imagerow",8)) {
      battle->p2.avatar.y=battle->p2.avatar.h*vn;
      continue;
    }
    if ((sepp==6)&&!memcmp(line,"wakeup",6)) {
      battle->p2.wakeup=(double)vn/1000.0;
      continue;
    }
    if ((sepp==6)&&!memcmp(line,"charge",6)) {
      battle->p2.charge=(double)vn/1000.0;
      continue;
    }
    fprintf(stderr,"battle:%d:%d: Unexpected key '%.*s'\n",rid,lineno,sepp,line);
    return -2;
  }
  return 0;
}

/* Load resource.
 */

int battle_load(struct battle *battle,const char *src,int srcc,int rid) {
  fprintf(stderr,"%s battle:%d srcc=%d\n",__func__,rid,srcc);//TODO
  int err;
  battler_init_human(&battle->p1);
  battler_init_cpu(&battle->p2);
  if ((err=battle_decode(battle,src,srcc,rid))<0) return err;
  
  /* Shuffle letterbag.
   * Also, tho it's not strictly required, draw both battler's hands.
   * Doing it here lets the human players peek at their tiles before the clock starts ticking.
   */
  letterbag_reset(&battle->letterbag);
  letterbag_draw(battle->p1.hand,&battle->letterbag);
  letterbag_draw(battle->p2.hand,&battle->letterbag);
  
  battle_begin_WELCOME(battle);
  if (!(battle->rid=rid)) battle->rid=-1;
  return 0;
}

/* Enter GATHER stage.
 */
 
static void battle_begin_GATHER(struct battle *battle) {
  battle->stage=BATTLE_STAGE_GATHER;
  
  // If both battlers are robots, run for a fixed interval.
  // At least one human, we wait for manual submit.
  if (!battle->p1.human&&!battle->p2.human) {
    battle->stageclock=5.0;//TODO
  } else {
    battle->stageclock=0.0;
  }
  
  battle->w_msg=0;
  battle->first=0;
  battle->second=0;
  battle->p1.avatar.face=0;
  battle->p2.avatar.face=0;
  
  int p1ok=letterbag_draw_partial(battle->p1.hand,&battle->letterbag);
  int p2ok=letterbag_draw_partial(battle->p2.hand,&battle->letterbag);
  if (!p1ok&&!p2ok) {
    //TODO Notify the user somehow?
    fprintf(stderr,"%s:%d: Reshuffle letterbag.\n",__FILE__,__LINE__);
    battle_log(battle,"Reshuffling letters.",-1,0xffffffff);
    letterbag_reset(&battle->letterbag);
    letterbag_draw(battle->p1.hand,&battle->letterbag);
    letterbag_draw(battle->p2.hand,&battle->letterbag);
  }
  battler_begin_round(&battle->p1,battle);
  battler_begin_round(&battle->p2,battle);
}

/* Enter P1WIN or P2WIN, whichever is appropriate.
 */
 
static void battle_begin_WIN(struct battle *battle) {
  if (battle->p1.hp<=0) {
    fprintf(stderr,"%.*s wins!\n",battle->p2.namec,battle->p2.name);
    battle_logf(battle,battle->p2.human?0x00ff00ff:0xff0000ff,"%s wins!",battle->p2.name);
    battle->stage=BATTLE_STAGE_P2WIN;
    battle->p1.avatar.face=4;
    battle->p2.avatar.face=5;
  } else {
    fprintf(stderr,"%.*s wins!\n",battle->p1.namec,battle->p1.name);
    battle_logf(battle,battle->p1.human?0x00ff00ff:0xff0000ff,"%s wins!",battle->p1.name);
    //TODO Check prizes we're going to award and log them.
    battle->stage=BATTLE_STAGE_P1WIN;
    battle->p1.avatar.face=5;
    battle->p2.avatar.face=4;
  }
  battle->stageclock=0.0;
}

/* Commit attacks.
 * Return nonzero if both battlers are still alive.
 */
 
static int battle_commit_attack(struct battle *battle,struct battler *attacker,struct battler *victim) {
  if (attacker->force<0) {
    //attacker->hp+=attacker->force;
    fprintf(stderr,"%.*s plays '%.*s' and it backfires for %d points!\n",attacker->namec,attacker->name,attacker->attackc,attacker->attack,attacker->force);
    if (attacker->hp<=0) return 0;
  } else {
    //victim->hp-=attacker->force;
    fprintf(stderr,"%.*s plays '%.*s' and deals %d points of damage!\n",attacker->namec,attacker->name,attacker->attackc,attacker->attack,attacker->force);
    if (victim->hp<=0) return 0;
  }
  return 1;
}

/* Enter next stage.
 */
 
static void battle_advance(struct battle *battle) {
  switch (battle->stage) {
    case BATTLE_STAGE_WELCOME: {
        battle_begin_GATHER(battle);
      } break;
      
    case BATTLE_STAGE_GATHER: {
        battle->stage=BATTLE_STAGE_ATTACK1;
        battle->stageclock=BATTLE_ATTACK_TIME;
        battler_commit(&battle->p1,battle);
        battler_commit(&battle->p2,battle);
        letterbag_draw_partial(battle->p1.hand,&battle->letterbag);
        letterbag_draw_partial(battle->p2.hand,&battle->letterbag);
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
    
    case BATTLE_STAGE_GATHER: {
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
  if (battle->stage!=BATTLE_STAGE_GATHER) return;
  struct battler *battler=0;
  switch (playerid) {
    case 1: battler=&battle->p1; break;
    case 2: battler=&battle->p2; break;
  }
  if (!battler||!battler->human) return;
  battler_move(battler,dx,dy);
}

void battle_activate(struct battle *battle,int playerid) {
  // Used to be able to click thru any non-interactive stage, but we shouldn't do that anymore.
  // Attacks take effect during play; if we allow skipping, they might not happen.
  //if (battle->stage!=BATTLE_STAGE_GATHER) {
  if ((battle->stage==BATTLE_STAGE_P1WIN)||(battle->stage==BATTLE_STAGE_P2WIN)) {
    battle_advance(battle);
    return;
  }
  struct battler *battler=0;
  switch (playerid) {
    case 1: battler=&battle->p1; break;
    case 2: battler=&battle->p2; break;
  }
  if (!battler||!battler->human) return;
  battler_activate(battler);
}

void battle_cancel(struct battle *battle,int playerid) {
  if (battle->stage!=BATTLE_STAGE_GATHER) return;
  struct battler *battler=0;
  switch (playerid) {
    case 1: battler=&battle->p1; break;
    case 2: battler=&battle->p2; break;
  }
  if (!battler||!battler->human) return;
  battler_cancel(battler);
}

/* Begin showing damage, if we're not already.
 */
 
void battle_begin_damage(struct battle *battle,struct battler *victim,int force) {
  if (victim->damageclock>0.0) return;
  if (victim->damage_done) return;
  victim->damageclock=BATTLE_DAMAGE_TIME;
  victim->damage=force;
  victim->avatar.face=3;
  victim->hp-=force;
  victim->damage_done=1;
  
  // Apply and report the 7-letter bonus if applicable.
  struct battler *winner=0;
  if (victim==&battle->p1) winner=&battle->p2;
  else winner=&battle->p1;
  if (winner->human&&winner->detail.valid&&(winner->attackc==7)&&(winner->inventory[ITEM_3XWORD]<99)) {
    fprintf(stderr,"%.*s played a 7-letter word, granting 3xword bonus item.\n",winner->namec,winner->name);
    winner->inventory[ITEM_3XWORD]++;
    battle->bonus3x=1.5;
  }
}

/* Commit to globals.
 */
 
void battle_commit_to_globals(struct battle *battle) {
  if (battle->p1.human) {
    if ((g.hp=battle->p1.hp)<=0) {
      g.hp=0;
    } else {
      g.xp++;//TODO Should XP per battle be variable? And what about gold?
    }
    memcpy(g.inventory,battle->p1.inventory,sizeof(g.inventory));
    g.world.status_bar_dirty=1;
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
