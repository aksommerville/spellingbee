#include "bee.h"
#include "hack.h"

// Even hackier: Since we're the first hack, we declare the shared global too.
#ifdef HACK_SELECTED
  const struct hack *ghack=&HACK_SELECTED;
#else
  const struct hack *ghack=0;
#endif

#define AVATAR_IMAGE_ID RID_image_avatars
#define AVATAR_ROW 4
#define AVATAR_DIR -1 /* Which direction should he face? -1=left 1=right */
#define AVATAR_INCLUDE_BONEZONE 1 /* Seven backgrounds but we can only show six. You get one of the bonezone or the gauntlet. */

static double aa_clock=0.0;
static int aa_frame=0; // 0..9

static int _aa_init() {
  return 0;
}

static void _aa_update(double elapsed,int input) {
  if ((aa_clock-=elapsed)<=0.0) {
    aa_clock+=1.000;
    if (++aa_frame>=10) aa_frame=0;
  }
}

static void _aa_render() {
  int subw=g.fbw/3;
  int subh=g.fbh>>1;
  int col,row;

  int texidbg=texcache_get_image(&g.texcache,RID_image_battlebg);
  int srcx=(AVATAR_DIR<0)?(g.fbw-subw):0;
  int srcy=AVATAR_INCLUDE_BONEZONE*subh;
  for (row=0;row<2;row++) {
    for (col=0;col<3;col++,srcy+=subh) {
      graf_draw_decal(&g.graf,texidbg,col*subw,row*subh,srcx,srcy,subw,subh,0);
    }
  }
  
  // Placement might not be exactly where they go in real life, but should be very close.
  int texidfg=texcache_get_image(&g.texcache,AVATAR_IMAGE_ID);
  int face=0;
  switch (aa_frame) {
    case 1: face=1; break;
    case 3: face=2; break;
    case 5: face=3; break;
    case 7: face=4; break;
    case 9: face=5; break;
  }
  srcx=face*TILESIZE*3;
  srcy=AVATAR_ROW*TILESIZE*4;
  int xform=(AVATAR_DIR<0)?EGG_XFORM_XREV:0;
  for (row=0;row<2;row++) {
    int dsty=(subh>>1)-((TILESIZE*4)>>1)+row*subh;
    for (col=0;col<3;col++) {
      int dstx=(subw>>1)-((TILESIZE*3)>>1)+col*subw;
      graf_draw_decal(&g.graf,texidfg,dstx,dsty,srcx,srcy,TILESIZE*3,TILESIZE*4,xform);
    }
  }
}

const struct hack hack_avatar_animation={
  .init=_aa_init,
  .update=_aa_update,
  .render=_aa_render,
};
