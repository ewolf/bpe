#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "bpelib.h"

void print_utf8_bytes(const char* str) {
    unsigned char *p = (unsigned char*)str;
    while (*p) {
        if (*p < 128) {  // ASCII
            printf("%c (1 byte)\n", *p);
            p++;
        } else if (*p < 224) {  // 2-byte sequence
            printf("%c%c (2 bytes)\n", p[0], p[1]);
            p += 2;
        } else if (*p < 240) {  // 3-byte sequence
            printf("%c%c%c (3 bytes)\n", p[0], p[1], p[2]);
            p += 3;
        } else {  // 4-byte sequence
            printf("%c%c%c%c (4 bytes)\n", p[0], p[1], p[2], p[3]);
            p += 4;
        }
    }
}

void print_text(unsigned int text_size, unsigned int* text, unsigned char** cues) {
  unsigned int i;
  unsigned int max_cue = 0;
  unsigned int cuestore_size = 0;

  for (i=1; i<text_size; i++) {
    int cues_idx = text[i];
    cuestore_size += strlen(cues[cues_idx]);
    if (cues_idx > max_cue) {
      max_cue = cues_idx;
    }
  }

  printf ("text length %d, all occupying %d bytes\n", text_size, text_size + cuestore_size );
  printf ("CUES: ");
  for (i=1; i<=max_cue; i++) {
    printf( "[%s]", cues[i] );
  }
  printf ("\n" );

  printf ("TEXT: ");
  for (i=0; i<text_size; i++) {
    int cues_idx = text[i];
    if (cues_idx > max_cue) {
      max_cue = cues_idx;
    }
    char* cue = cues[cues_idx];
    printf( "[%s]", cue );
  }
  printf ("\n" );


}

void incrementFreq( unsigned int a, unsigned int b, unsigned int** pair_freqs, unsigned int* pair_freqs_b_last_idx, unsigned char** cues ) {
  pair_freqs[a][b]++;
  printf ("INCREMENT '%s'/'%s' --> %d\n", cues[a], cues[b], pair_freqs[a][b] );
  if (b > pair_freqs_b_last_idx[a]) {
    pair_freqs_b_last_idx[a] = b;
  }
}

void decrementFreq( unsigned int a, unsigned int b, unsigned int** pair_freqs, unsigned int* pair_freqs_b_last_idx, unsigned char** cues ) {
  pair_freqs[a][b]--;
  printf ("DECREMENT '%s'/'%s' --> %d\n", cues[a], cues[b], pair_freqs[a][b] );
}

TextCues* bpe_encode(const char* str, 
                     unsigned int min_freq,
                     char delim
                     ) {
  unsigned int text_len = strlen(str);

  unsigned int total_storage = text_len;

  unsigned int delim_idx = 0;

  // text is going to store a list of pointers to string snippets
  // and those snippets are stored in cues
  unsigned int *text = malloc(text_len * sizeof(int));
  memset( text, 0, text_len * sizeof(int));
  
  // cues - a cue is a pair or a letter
  //   the max number of possible cues is approximately
  //   the number of initial possible pairs ( text_len - 1 )
  //   plus the max possible number of iterations. in each iteration, at least
  //   min_freq items is removed
  unsigned int max_cues = 1 + text_len + (text_len / min_freq);
  unsigned char **cues = malloc(max_cues * sizeof(char*));

  memset( cues, 0, max_cues * sizeof(char*) );

  // chars is basically a lookup that checks if a character
  // has been entered into cues array or not
  unsigned char* chars = malloc(UCHAR_MAX * sizeof(char));
  memset( chars, 0, UCHAR_MAX * sizeof(char));

  // bigchars are for storing multibyte utf8 letters
  //   when one of these is encountered, its up to four bytes
  //   are converted into an integer for lookup. the bigchars
  //   array is iterated through. if it contains that integer
  //   it means the letter has been encountered already.
  //
  //   it uses the index it found that integer to look up
  //   its cues index in the bigchars_lookup
  //   
  //   if it is not in bigchars, it ads the string snippet
  //   to the cues array, and records that cues index in 
  //   bigchars_lookup and makes a corresponding entry in 
  //   bigchars
  // 
  unsigned int* bigchars = malloc(text_len * sizeof(int));
  unsigned int* bigchars_lookup = malloc(text_len * sizeof(int));
  unsigned int big_count = 0;

  memset( bigchars, 0, text_len * sizeof(int));
  memset( bigchars_lookup, 0, text_len * sizeof(int));

  // part one: scan all characters in text to add initial 'cues'
  //           which are string snippets. fill the text array
  //           with indexes to the cues array
  unsigned char *p = (unsigned char*)str;
  unsigned int text_idx = 0;
  unsigned int next_cues_idx = 1; // cues[0] is 0 to represent the empty string

  //
  // array of array
  // representing cue index -> cue index -> frequency
  //
  unsigned int* freqs = malloc( max_cues * sizeof(int*) );
  unsigned int** pair_freqs = malloc( max_cues * sizeof(int*) );

  unsigned int* pair_freqs_b_last_idx = malloc( max_cues * sizeof(int) );

  memset( pair_freqs_b_last_idx, 0, max_cues * sizeof(int) );

  unsigned int last_letter = 0;
  unsigned int max_cues_idx = 0;

  printf("max_cues: %d\n", max_cues );
  printf("text length: %d\n", text_len );
  printf("min cue freq: %d\n", min_freq );

  while (*p) {
    unsigned int utf8val;
    unsigned char* snippet;
    unsigned char* start_p = p;

    unsigned int letter = 0, newletter = 0;
    if (*p < 128) {  // ASCII
      utf8val = *p;

      letter = chars[utf8val];
      if (!letter) {
        snippet = malloc( 2*sizeof(char) );
        snippet[0] = p[0];
        snippet[1] = '\0';
        cues[next_cues_idx] = snippet;
        total_storage += 2;
        letter = chars[utf8val] = next_cues_idx;
        newletter = 1;
      }
      p++;
    } 
    else { // a multibyte lette sequence

      if (*p < 224) {  // 2-byte sequence
        utf8val = ((unsigned int) p[0] << 8) | (unsigned int)p[1];
        text_len--;
      } else if (*p < 240) {  // 3-byte sequence
        utf8val = 
          ((unsigned int) p[0] << 16) 
          | ((unsigned int) p[1] << 8) 
          | (unsigned int)p[2];
        text_len -= 2;
      } else {  // 4-byte sequence
        utf8val = 
          ((unsigned int) p[0] << 24) 
          | ((unsigned int) p[1] << 16) 
          | ((unsigned int) p[2] << 8) 
          | (unsigned int)p[3];
        text_len -= 3;
      }

      // find if this lettere has been 
      // encountered before
      int i;
      for (i=0; i<big_count; i++) {
        if (bigchars[i] == utf8val) {
          letter = bigchars_lookup[i];
          break;
        }
      }

      // letter is new, so calucate the snippet
      // and store in the cues array
      if (!letter) {

        if (*p < 224) {  // 2-byte sequence
          snippet = malloc( 3*sizeof(char) );
          snippet[0] = p[0];
          snippet[1] = p[1];
          snippet[2] = '\0';
          total_storage += 3;
        } else if (*p < 240) {  // 3-byte sequence
          snippet = malloc( 4*sizeof(char) );
          snippet[0] = p[0];
          snippet[1] = p[1];
          snippet[2] = p[2];
          snippet[3] = '\0';
          total_storage += 4;
        } else {  // 4-byte sequence
          snippet = malloc( 5*sizeof(char) );
          snippet[0] = p[0];
          snippet[1] = p[1];
          snippet[2] = p[2];
          snippet[3] = p[3];
          snippet[4] = '\0';
          total_storage += 5;
        }

        bigchars[big_count] = utf8val;
        cues[next_cues_idx] = snippet;
        letter = bigchars_lookup[big_count] = next_cues_idx;
        newletter = 1;

        big_count++;
      }
      
      // advance the pointer
      if (*p < 224) {  // 2-byte sequence
        p += 2;
      } else if (*p < 240) {  // 3-byte sequence
        p += 3;
      } else {  // 4-byte sequence
        p += 4;
      }
      
    }

    if (newletter) {
      if (utf8val == delim) 
        delim_idx = letter;
      freqs[letter]++;
      pair_freqs[letter] = malloc( max_cues * sizeof(int) );
      memset( pair_freqs[letter], 0, max_cues * sizeof(int) );
      max_cues_idx = next_cues_idx;
      next_cues_idx++;
    }
    text[text_idx++] = letter;

    // update pair frequency 
    if (last_letter && letter != delim_idx && last_letter != delim_idx ) {
      incrementFreq( last_letter, letter, pair_freqs, pair_freqs_b_last_idx, cues );
    }

    last_letter = letter;

  } // initial scan

  // lets see if these came through all right
  //  print_text( text_len, text, cues );
  unsigned int initial_storage = total_storage;
  printf( "step 2. storage size is %d\n", total_storage );

  // these are not used any longer
  free( chars );
  free( bigchars );
  free( bigchars_lookup );

  unsigned int substring_count = text_len;
  while (1) {
    unsigned int previous_idx = 0;

    //
    // find the top pair by frequency
    //
    unsigned int 
      a_idx,
      max_freq = 0,
      max_a_idx,
      max_b_idx;
    int min_storage = 0;
    for (a_idx=1; a_idx<=max_cues_idx; a_idx++) {
      if (pair_freqs[a_idx]) {
        int* b_cues = pair_freqs[a_idx];
        unsigned int b_idx;
        for (b_idx=1; b_idx<=pair_freqs_b_last_idx[a_idx]; b_idx++ ) 
          {
            unsigned int freq = pair_freqs[a_idx][b_idx];
            unsigned int a_len = strlen(cues[a_idx]);
            unsigned int b_len = strlen(cues[b_idx]);
            // storage is the size it would be in cues minus how many are taken away
            int storage = ( a_len + b_len ) - freq; //v((a_len + b_len) * freq);
            if (freqs[a_idx] <= freq) {
              printf( "A_IDX freq %d\n", freqs[a_idx] );
              storage -= a_len;
            }
            if (freqs[b_idx] <= freq) {
              printf( "B_IDX freq %d\n", freqs[b_idx] );
              storage -= b_len;
            }
            if (storage < 0) {
              printf( "'%s'/'%s' -> %d (storage = (%d + %d) - %d =  %d)\n", cues[a_idx], cues[b_idx], freq, a_len, b_len, freq, storage );
              if (max_freq == 0) {
                max_freq = freq;
                min_storage = storage;
                max_a_idx = a_idx;
                max_b_idx = b_idx;
              }
              else if (storage < min_storage) {
                max_freq = freq;
                min_storage = storage;
                max_a_idx = a_idx;
                max_b_idx = b_idx;
              }
            }
          }
      }
    }
    if (max_freq < min_freq && min_storage >= 0) {
      break;
    }

    char* a_cue = cues[max_a_idx];
    char* b_cue = cues[max_b_idx];
    unsigned int pair_len = (strlen(a_cue)+ strlen(b_cue) + 1 );
    char* cue = malloc( pair_len * sizeof(char));

    strcpy( cue, a_cue );
    strcat( cue, b_cue );

    printf( "NEW PAIR '%s'/'%s' -> '%s'\n", a_cue, b_cue, cue );
    
    cues[next_cues_idx] = cue;
    total_storage += pair_len;

    printf( "FOUND '%s' at cues index %d (freq %d, storage %d)\n", cue, next_cues_idx, max_freq, min_storage );

    pair_freqs[next_cues_idx] = malloc( max_cues * sizeof(int) );

    memset( pair_freqs[next_cues_idx], 0, max_cues * sizeof(int) );
    // remove a to b combination from consideration
    // as it is done and stick a spork in it
    pair_freqs[max_a_idx][max_b_idx] = 0;
    // scan to find the highest pair and replace with that
    // and update the frequencies
    unsigned int i;
    unsigned int penultimate = text_len - 2;
    unsigned int ultimate = text_len - 1;

    printf( "CURRENTLY :" );
    for ( i=0; i<text_len; i++ ) {
      printf( "(%d)'%s' ", i, cues[text[i]] );
    }
    printf( "\n" );
printf( "PREV %d\n", previous_idx );
    printf( "SEEK '%s' and '%s' (up to %d)\n", a_cue, b_cue, ultimate );
    for ( i=0; i<ultimate; i++ ) {
      printf ("LOOP START WITH %d\n", i );
      unsigned int a_idx = text[i], astart;

      while ( (a_idx == 0 || a_idx == delim_idx) && i < ultimate ) {
        i++;
        a_idx = text[i];
      }
      astart = i;
      printf ("ASTART %d\n", astart );
      if (a_idx == max_a_idx && i < ultimate ) {
        i++;
        unsigned int b_idx = text[i], bstart;
        while ((b_idx == 0 || b_idx == delim_idx) && i < text_len ) {
          i++;
          b_idx = text[i];
        }
        bstart = i;
        printf ("BSTART %d\n", bstart );

        printf( "%d) '%s'/'%s'\n", astart, cues[a_idx], cues[b_idx] );
        if (b_idx == max_b_idx && i < text_len) {
          text[astart] = next_cues_idx;
          text[bstart] = 0;
          total_storage--;
          freqs[next_cues_idx]++;
          freqs[a_idx]--;
          freqs[b_idx]--;

          if (previous_idx != 0 && previous_idx != delim_idx) {
            printf ("PREVIOUS '%s' '%s'\n", cues[previous_idx], cues[a_idx]);
            if (previous_idx == next_cues_idx ) {
              printf("REVERSE "); decrementFreq( previous_idx, a_idx, pair_freqs, pair_freqs_b_last_idx, cues );
            }
            printf( "PRE" ); incrementFreq( previous_idx, next_cues_idx, pair_freqs, pair_freqs_b_last_idx, cues );
            if (previous_idx != a_idx && a_idx != b_idx )
              printf( "PRE" ); decrementFreq( previous_idx, a_idx, pair_freqs, pair_freqs_b_last_idx, cues );
          }

          substring_count--;

          i++;
          unsigned int c_idx = text[i], cstart;
          while ((c_idx == 0 || c_idx == delim_idx) && i < text_len ) {
            i++;
            c_idx = text[i];
          }
          cstart = i;
          if (c_idx != 0 && c_idx != delim_idx && cstart < text_len ) {
            printf( "POST" ); incrementFreq( next_cues_idx, c_idx, pair_freqs, pair_freqs_b_last_idx, cues );
            if (b_idx != a_idx && c_idx != b_idx )
              printf( "POST" ); decrementFreq( b_idx, c_idx, pair_freqs, pair_freqs_b_last_idx, cues );
          }


          // reset i to just before c
          i = cstart - 1;

          previous_idx = next_cues_idx;
        } else {
          previous_idx = a_idx;
          i = bstart - 1;
        }
      } else {
        previous_idx = a_idx;
      }
      printf ( "LOOP %d (prev '%s')\n", i, cues[previous_idx] );
    }
    next_cues_idx++;
    max_cues_idx = next_cues_idx;
    printf( "\n" );
  }

  unsigned int* compressed = malloc((substring_count+1) * sizeof(int));
  
  unsigned int write_idx = 0;

  unsigned int i;
  for ( i=0; i<text_len; i++ ) {
    int cue_idx = text[i];
    if (cue_idx) {
      compressed[write_idx++] = cue_idx;
    }
  }
  
//  print_text( write_idx, compressed, cues );

//  printf( "\ndone\n" );

  free( text );
  free( pair_freqs_b_last_idx );

  for (i=1; i<next_cues_idx; i++) {
    free( pair_freqs[i] );
  }
  printf( "done, final size %d from %d\n", total_storage, initial_storage );
  free(pair_freqs);

  TextCues* textCues = malloc( sizeof( TextCues ) );

  textCues->cues     = cues;
  textCues->text      = compressed;
  textCues->text_size = write_idx;
  textCues->cues_size = next_cues_idx;

  return textCues;
} //bpe_encode

int main() {
//    const char *str = " Hello, World! Привет, Мир! こんにちは、世界！ ";
//    print_utf8_bytes(str);


//    TextCues* tc = bpe_encode("updating tests. writing a test suite in the www/recipes/tests directory. want a program that will load them all and run the tests for each of them, using SP.init, maybe make an SP.clear that will empty out the body. update tester so that there is something like doneTesting but not for the page, maybe just returns the test results as an object.",2,'|');


    TextCues* tc = bpe_encode("abcde abcde abcde abcde",2,'|');

    print_text( tc->text_size, tc->text, tc->cues );

    // frees everything, w00t
    int i;
    for (i=0; i<tc->cues_size; i++) {
      free( tc->cues[i] );
    }
    free( tc->text );
    free( tc->cues );
    free( tc );

    return 0;
}
