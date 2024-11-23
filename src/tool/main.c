#include "tool_internal.h"

struct tool tool={0};

/* Return one of our compilable type ids, if it's named in the path.
 * Zero for no match.
 */
 
static int compilable_tid_from_path(const char *path) {
  if (!path) return 0;
  while (*path) {
    if (*path=='/') { path++; continue; }
    const char *name=path;
    int namec=0;
    while (*path&&(*path!='/')) { namec++; path++; }
    
    int i=namec; while (i-->0) {
      if (name[i]=='-') {
        name+=i+1;
        namec-=i+1;
        break;
      }
    }
    
    if ((namec==3)&&!memcmp(name,"map",3)) return EGG_TID_map;
    if ((namec==6)&&!memcmp(name,"sprite",6)) return EGG_TID_sprite;
    if ((namec==9)&&!memcmp(name,"tilesheet",9)) return EGG_TID_tilesheet;
    if ((namec==4)&&!memcmp(name,"dict",4)) return EGG_TID_dict;
  }
  return 0;
}

/* Main, memory to memory.
 * Read (srcpath) componentwise to determine what kind of thing we're doing, then dispatch to that.
 */
 
static int tool_main_inner() {
  int tid=compilable_tid_from_path(tool.srcpath);
  if (!tid) tid=compilable_tid_from_path(tool.dstpath);
  switch (tid) {
    case EGG_TID_map: return tool_compile_map();
    case EGG_TID_sprite: return tool_compile_sprite();
    case EGG_TID_tilesheet: return tool_compile_tilesheet();
    case EGG_TID_dict: return tool_compile_dict();
  }
  fprintf(stderr,"%s: Unable to determine resource type from path. Must contain '/map/', '/sprite/', or '/tilesheet/'\n",tool.srcpath);
  return -2;
}

/* Main.
 */
 
int main(int argc,char **argv) {
  tool.exename="tool";
  if ((argc>=1)&&argv&&argv[0]&&argv[0][0]) tool.exename=argv[0];
  int argi=1,valid=1; while (argi<argc) {
    const char *arg=argv[argi++];
    if (!arg||!arg[0]) continue;
    if (!memcmp(arg,"-o",2)) {
      if (tool.dstpath) { valid=0; break; }
      tool.dstpath=arg+2;
    } else if (!memcmp(arg,"--toc=",6)) {
      if (tool.tocpath) { valid=0; break; }
      tool.tocpath=arg+6;
    } else if (arg[0]=='-') {
      valid=0;
      break;
    } else if (tool.srcpath) {
      valid=0;
      break;
    } else {
      tool.srcpath=arg;
    }
  }
  if (!valid||!tool.dstpath||!tool.srcpath) {
    fprintf(stderr,"Usage: %s -oOUTPUT INPUT [--toc=HEADER]\n",tool.exename);
    return 1;
  }
  
  if ((tool.srcc=file_read(&tool.src,tool.srcpath))<0) {
    fprintf(stderr,"%s: Failed to read file.\n",tool.srcpath);
    return 1;
  }
  
  int err=tool_main_inner();
  if (err<0) {
    if (err!=-2) fprintf(stderr,"%s: Unspecified error compiling file.\n",tool.srcpath);
    return 1;
  }
  
  if (file_write(tool.dstpath,tool.dst.v,tool.dst.c)<0) {
    fprintf(stderr,"%s: Failed to write file, %d bytes.\n",tool.dstpath,tool.dst.c);
    return 1;
  }
  return 0;
}
