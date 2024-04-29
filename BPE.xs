#define PERL_NO_GET_CONTEXT
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "ppport.h"
#include "string.h"

#include "bpelib/bpelib.h"

#include "const-c.inc"

MODULE = BPE		PACKAGE = BPE		

INCLUDE: const-xs.inc

HV*
encode(str,min_freq,ignore)
       const char * str
       int          min_freq
       const char * ignore
  CODE:
    TextCues * tc = bpe_encode(str,min_freq,ignore[0]);
    HV * rv = newHV();
    hv_store(rv, "text_size", 9, newSVuv(tc->text_size), 0); // 0 to have it compute the hashed value
    hv_store(rv, "cues_size", 9, newSVuv(tc->cues_size), 0); // 0 to have it compute the hashed value
    AV * cues = newAV();    
    AV * text = newAV();
    int i;
    for (i=0; i<tc->text_size; i++) {
      av_push( text, newSVuv( tc->text[i] ) );
    }
    hv_store(rv, "text", 4, newRV_inc((SV*)text), 0);

    // first cue index is for the empty string
    av_push( cues, newSVpv( "", 0 ) );

    for (i=1; i<tc->cues_size; i++) {
      char* cue = tc->cues[i];
      av_push( cues, newSVpv( cue, strlen(cue) ));
    }
    hv_store(rv, "cues", 4, newRV_inc((SV*)cues), 0);

    RETVAL = rv;
  OUTPUT:
    RETVAL
