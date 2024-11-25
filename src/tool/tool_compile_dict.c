#include "tool_internal.h"

/* Validate word.
 */
 
static int dict_validate_word(const char *src,int srcc) {
  if ((srcc<2)||(srcc>7)) {
    fprintf(stderr,"Invalid word length %d, must be in 2..7\n",srcc);
    return -2;
  }
  int i=srcc;
  while (i-->0) {
    if ((src[i]<'A')||(src[i]>'Z')) {
      fprintf(stderr,"Unexpected character '%c'. Must be uppercase letters only.\n",src[i]);
      return -2;
    }
  }
  return 0;
}

/* Compile dict.
 */
 
int tool_compile_dict() {
  
  /* Count the words of each length, and validate each.
   * No need to record the bucket positions. If this block passes, we can infer everything else.
   */
  int count_by_length[8]={0}; // Indexed by length, including 0 and 1.
  int len=2,srcp=0,lineno=1,err;
  while (srcp<tool.srcc) {
    if (srcp>tool.srcc-len) {
      fprintf(stderr,"%s:%d: Unexpected trailing bytes\n",tool.srcpath,lineno);
      return -2;
    }
    if ((srcp+len<tool.srcc)&&(tool.src[srcp+len]!=0x0a)) {
      len++;
      continue;
    }
    const char *word=tool.src+srcp;
    int err=dict_validate_word(word,len);
    if (err<0) {
      fprintf(stderr,"%s:%d: Invalid word\n",tool.srcpath,lineno);
      return -2;
    }
    count_by_length[len]++;
    srcp+=len;
    if ((srcp<tool.srcc)&&(tool.src[srcp]==0x0a)) {
      srcp++;
      lineno++;
    }
  }
  
  /* Confirm we haven't breached 16 bits.
   * We used to assert nonzero count here, but that's really not necessary (and pidgin actually does have zero words, for length seven).
   */
  int i=2; for (;i<=7;i++) {
    if (count_by_length[i]>0xffff) {
      fprintf(stderr,"%s: Too many words of length %d: %d, limit 65535\n",tool.srcpath,i,count_by_length[i]);
      return -2;
    }
  }
  
  /* Emit header: Each count for lengths 2..7, in 2 bytes each.
   */
  if (sr_encode_intbe(&tool.dst,count_by_length[2],2)<0) return -1;
  if (sr_encode_intbe(&tool.dst,count_by_length[3],2)<0) return -1;
  if (sr_encode_intbe(&tool.dst,count_by_length[4],2)<0) return -1;
  if (sr_encode_intbe(&tool.dst,count_by_length[5],2)<0) return -1;
  if (sr_encode_intbe(&tool.dst,count_by_length[6],2)<0) return -1;
  if (sr_encode_intbe(&tool.dst,count_by_length[7],2)<0) return -1;
  
  /* Copy each bucket.
   */
  for (len=2,srcp=0;len<=7;len++) {
    for (i=count_by_length[len];i-->0;srcp+=len+1) {
      if (sr_encode_raw(&tool.dst,tool.src+srcp,len)<0) return -1;
    }
  }
  
  return 0;
}
