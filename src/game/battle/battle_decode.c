#include "game/bee.h"
#include "battle.h"

/* Decode resource into new battle.
 */
 
int battle_decode(struct battle *battle,const char *src,int srcc,int rid) {
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
      battler_human_nocontext(&battle->p1);
      battler_human_nocontext(&battle->p2);
      continue;
    }
    if ((linec==8)&&!memcmp(line,"fulldict",8)) {
      battle->p2.dictid=RID_dict_nwl2023;
      continue;
    }
    if ((linec==7)&&!memcmp(line,"preempt",7)) {
      battle->p2.preempt=1;
      continue;
    }
    if ((linec==7)&&!memcmp(line,"lenonly",7)) {
      battle->p1.lenonly=1;
      battle->p2.lenonly=1;
      continue;
    }
    if ((linec==4)&&!memcmp(line,"twin",4)) {
      battle->p2.twin=1;
      continue;
    }
    if ((linec==7)&&!memcmp(line,"bstream",7)) {
      battle->p2.bstream=1;
      continue;
    }
    if ((linec==8)&&!memcmp(line,"novowels",8)) {
      battle->novowels=1;
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
    if ((sepp==8)&&!memcmp(line,"logcolor",8)) {
      if (vc!=6) {
       _invalid_logcolor_:;
        fprintf(stderr,"battle:%d:%d: 'logcolor' value must be six hex digits. Found '%.*s'\n",rid,lineno,vc,v);
        return -2;
      }
      battle->p2.logcolor=0;
      int i=0; for (;i<vc;i++) {
        char ch=v[i];
             if ((ch>='0')&&(ch<='9')) ch=ch-'0';
        else if ((ch>='a')&&(ch<='f')) ch=ch-'a'+10;
        else if ((ch>='A')&&(ch<='F')) ch=ch-'A'+10;
        else goto _invalid_logcolor_;
        battle->p2.logcolor<<=4;
        battle->p2.logcolor|=ch;
      }
      battle->p2.logcolor<<=8;
      battle->p2.logcolor|=0xff;
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
    if ((sepp==4)&&!memcmp(line,"gold",4)) {
      battle->p2.gold=vn;
      continue;
    }
    if ((sepp==2)&&!memcmp(line,"xp",2)) {
      battle->p2.xp=vn;
      continue;
    }
    if ((sepp==4)&&!memcmp(line,"song",4)) {
      battle->songid=vn;
      continue;
    }
    if ((sepp==4)&&!memcmp(line,"book",4)) {
      battle->bookid=vn;
      continue;
    }
    fprintf(stderr,"battle:%d:%d: Unexpected key '%.*s'\n",rid,lineno,sepp,line);
    return -2;
  }
  return 0;
}
