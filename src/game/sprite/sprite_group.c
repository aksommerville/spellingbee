#include "game/bee.h"

struct sprite_group sprite_groupv[32]={0};

/* Delete.
 */
 
void sprite_group_del(struct sprite_group *group) {
  if (!group) return;
  if (!group->refc) return; // immortal
  if (group->refc-->1) return;
  if (group->spritev) free(group->spritev);
  free(group);
}

/* Retain.
 */
 
int sprite_group_ref(struct sprite_group *group) {
  if (!group) return -1;
  if (!group->refc) return 0;
  if (group->refc==INT_MAX) return -1;
  group->refc++;
  return 0;
}

/* New.
 */
 
struct sprite_group *sprite_group_new() {
  struct sprite_group *group=calloc(1,sizeof(struct sprite_group));
  if (!group) return 0;
  group->refc=1;
  return group;
}

/* Search, private.
 */
 
static int sprite_groupv_search(const struct sprite *sprite,const struct sprite_group *group) {
  int lo=0,hi=sprite->groupc;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct sprite_group *q=sprite->groupv[ck];
         if (group<q) hi=ck;
    else if (group>q) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

static int group_spritev_search(const struct sprite_group *group,const struct sprite *sprite) {
  //TODO This assumes all groups can sort by address. Probably at least VISIBLE will not.
  int lo=0,hi=group->spritec;
  while (lo<hi) {
    int ck=(lo+hi)>>1;
    const struct sprite *q=group->spritev[ck];
         if (sprite<q) hi=ck;
    else if (sprite>q) lo=ck+1;
    else return ck;
  }
  return -lo-1;
}

/* Insert, private.
 */
 
static int sprite_groupv_insert(struct sprite *sprite,int p,struct sprite_group *group) {
  if (sprite->groupc>=sprite->groupa) {
    int na=sprite->groupa+4;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(sprite->groupv,sizeof(void*)*na);
    if (!nv) return -1;
    sprite->groupv=nv;
    sprite->groupa=na;
  }
  if (sprite_group_ref(group)<0) return -1;
  memmove(sprite->groupv+p+1,sprite->groupv+p,sizeof(void*)*(sprite->groupc-p));
  sprite->groupc++;
  sprite->groupv[p]=group;
  return 0;
}

static int group_spritev_insert(struct sprite_group *group,int p,struct sprite *sprite) {
  if (group->spritec>=group->spritea) {
    int na=group->spritea+16;
    if (na>INT_MAX/sizeof(void*)) return -1;
    void *nv=realloc(group->spritev,sizeof(void*)*na);
    if (!nv) return -1;
    group->spritev=nv;
    group->spritea=na;
  }
  if (sprite_ref(sprite)<0) return -1;
  memmove(group->spritev+p+1,group->spritev+p,sizeof(void*)*(group->spritec-p));
  group->spritec++;
  group->spritev[p]=sprite;
  return 0;
}

/* Remove, private.
 */
 
static void sprite_groupv_remove(struct sprite *sprite,int p) {
  if ((p<0)||(p>=sprite->groupc)) return;
  struct sprite_group *group=sprite->groupv[p];
  sprite->groupc--;
  memmove(sprite->groupv+p,sprite->groupv+p+1,sizeof(void*)*(sprite->groupc-p));
  sprite_group_del(group);
}

static void group_spritev_remove(struct sprite_group *group,int p) {
  if ((p<0)||(p>=group->spritec)) return;
  struct sprite *sprite=group->spritev[p];
  group->spritec--;
  memmove(group->spritev+p,group->spritev+p+1,sizeof(void*)*(group->spritec-p));
  sprite_del(sprite);
}

/* Test membership, public.
 */

int sprite_group_has(const struct sprite_group *group,const struct sprite *sprite) {
  if (!group||!sprite) return 0;
  return (sprite_groupv_search(sprite,group)>=0);
}

/* Add, public.
 */
 
int sprite_group_add(struct sprite_group *group,struct sprite *sprite) {
  if (!group||!sprite) return -1;
  int grpp=sprite_groupv_search(sprite,group);
  if (grpp>=0) return 0;
  grpp=-grpp-1;
  int sprp=group_spritev_search(group,sprite);
  if (sprp>=0) return -1;
  sprp=-sprp-1;
  if (sprite_groupv_insert(sprite,grpp,group)<0) return -1;
  if (group_spritev_insert(group,sprp,sprite)<0) {
    sprite_groupv_remove(sprite,grpp);
    return -1;
  }
  return 1;
}

/* Remove, public.
 */
    
int sprite_group_remove(struct sprite_group *group,struct sprite *sprite) {
  if (!group||!sprite) return -1;
  int grpp=sprite_groupv_search(sprite,group);
  if (grpp<0) return 0;
  int sprp=group_spritev_search(group,sprite);
  if (sprp<0) return -1;
  sprite_groupv_remove(sprite,grpp);
  group_spritev_remove(group,sprp);
  return 1;
}

/* Clear group.
 */

void sprite_group_clear(struct sprite_group *group) {
  if (!group) return;
  while (group->spritec>0) {
    group->spritec--;
    struct sprite *sprite=group->spritev[group->spritec];
    int grpp=sprite_groupv_search(sprite,group);
    if (grpp>=0) sprite_groupv_remove(sprite,grpp);
    sprite_del(sprite);
  }
}

/* Kill sprite.
 */
 
void sprite_kill_now(struct sprite *sprite) {
  if (sprite_ref(sprite)<0) return;
  while (sprite->groupc>0) {
    sprite->groupc--;
    struct sprite_group *group=sprite->groupv[sprite->groupc];
    int sprp=group_spritev_search(group,sprite);
    if (sprp>=0) group_spritev_remove(group,sprp);
    sprite_group_del(group);
  }
  sprite_del(sprite);
}

/* Kill all sprites in group.
 */
 
void sprite_group_kill(struct sprite_group *group) {
  while (group->spritec>0) {
    group->spritec--;
    struct sprite *sprite=group->spritev[group->spritec];
    int grpp=sprite_groupv_search(sprite,group);
    if (grpp>=0) sprite_groupv_remove(sprite,grpp);
    sprite_kill_now(sprite);
    sprite_del(sprite);
  }
}

/* Update all sprites in group.
 */

void sprite_group_update(struct sprite_group *group,double elapsed) {
  int i=group->spritec;
  while (i-->0) {
    struct sprite *sprite=group->spritev[i];
    if (sprite->type->update) sprite->type->update(sprite,elapsed);
  }
}

/* Render a group.
 */
  
void sprite_group_render(struct sprite_group *group,int16_t addx,int16_t addy) {
  //TODO sort?
  int imageid=0,texid=0;
  int i=0; for (;i<group->spritec;i++) {
    struct sprite *sprite=group->spritev[i];
    if (sprite->type->render) {
      sprite->type->render(sprite,addx,addy);
    } else {
      if (sprite->imageid!=imageid) {
        graf_flush(&g.graf);
        imageid=sprite->imageid;
        texid=texcache_get_image(&g.texcache,imageid);
      }
      int16_t dstx=(int16_t)(sprite->x*TILESIZE)+addx;
      int16_t dsty=(int16_t)(sprite->y*TILESIZE)+addy;
      graf_draw_tile(&g.graf,texid,dstx,dsty,sprite->tileid,sprite->xform);
    }
  }
}
