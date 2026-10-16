/***********************************************************************
 *
 * Project:  htmlencode
 * Purpose:  Main file.
 *
 ***********************************************************************
 * Copyright 2026 Matthieu Peeters
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 ***********************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "postgres.h"
#include "fmgr.h"
#include "varatt.h"
#include "utils/builtins.h"

/* OS Includes */
/* PostgreSQL Includes */
PG_MODULE_MAGIC;



// It seems it does not really matter for speed whether to copy with mempcpy in the strings.h lib or directly with *a++ = *b++
//
// #define copy_with_library 1
// #define should_be_escaped_in_macro 1


// To quickly check whether a char should be escaped, 
const uint_least32_t match_pattern =
(
 (1 << ('"' - ' ')) +
 (1 << ('&' - ' ')) +
 (1 << ('\''- ' ')) +
 (1 << ('<' - ' ')) +
 (1 << ('>' - ' '))
);


// Because of the limitations of << (no negative numbers, not bigger than the number of bits) there are
// guards on the range of chars that can be escaped. Note that && is evaluated left first. 
#ifdef should_be_escaped_in_macro
#define should_be_escaped(a) (((a) <= '>') && ((a) >= '"') && (match_pattern & (1 << ((a) - ' '))))
#else
int should_be_escaped(char a);
int should_be_escaped(char a) {
  return (((a) <= '>') && ((a) >= '"') && (match_pattern & (1 << ((a) - ' '))));
}
#endif

// &quest; is the error state. 
#define char_to_entity(a) ((a) == '"' ? "&quot;" : (a) == '&' ? "&amp;" : (a) == '\'' ? "&apos;" : (a) == '<' ? "&lt;" : (a) == '>' ? "&gt;" : "&quest;") 


#define max_entity_len (strlen("&quest;"))



// C function to do the copying with the length known. 
static const char*
htmlencode_cstr(const char* str_in, size_t str_in_len)
{

  // ptr_in runs through the source, str_in_end is to guard the end of the source.
  // (empty string: if ptr_in = str_in_end, no code will be executed )
  const char *ptr_in = str_in, *str_in_end = str_in + str_in_len;
  size_t str_out_len;
  char *str_out;
  char* ptr_out;

  // Keep skipping until there is an entity to be translated.
  while(!should_be_escaped(*ptr_in) && ptr_in < str_in_end) {
    ++ptr_in;
  }
  if(ptr_in == str_in_end) { // If there are no entities to be translated, or an empty string, just return the imput. 
    return str_in;
  }

  // Now allocate a new buffer to write the new string to. 
  // make str_out always big enough. 
  str_out_len = str_in_len * max_entity_len + 1; // +1 for the \0. 
  str_out = palloc0(sizeof(char) * str_out_len);

  // Copy the existing non-translated chars to this new buffer and have a ptr_out that points after the copied string. 
  ptr_out = mempcpy(str_out, str_in, sizeof(char) * (ptr_in - str_in));  

  
  while(ptr_in < str_in_end) {
    // printf("%c %b %d %d %b %b\n", *ptr_in, (31 & ((*ptr_in) - 32)), should_be_escaped(*ptr_in), (*ptr_in) - 32, match_pattern, 1 << ((*ptr_in) - 32) );
    // ptr_in always points to a char to be escaped
    ptr_out = stpcpy(ptr_out, char_to_entity(*ptr_in));

    ++ptr_in;

#ifdef copy_with_library    
    const char *in_copy_start = ptr_in;
    while(ptr_in < str_in_end &&  !should_be_escaped(*ptr_in)) {
      ++ptr_in;
    }
    // Now copy the diff from in_copy_start to ptr_in into ptr_out
    if(in_copy_start < ptr_in) { // not sure if mempcpy is efficient enough to determine if nothing should be done..
      ptr_out = mempcpy(ptr_out, in_copy_start, sizeof(char) * (ptr_in - in_copy_start));
    }
#else
    while(ptr_in < str_in_end &&  !should_be_escaped(*ptr_in)) {
      *ptr_out++ = *ptr_in++;
    }
#endif    
  }
  *ptr_out = '\0';
  return str_out;
}


Datum htmlencode(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(htmlencode);

Datum htmlencode(PG_FUNCTION_ARGS) {
  /* Declare SQL function strict, so no test for NULL input */
  text *txt = PG_GETARG_TEXT_P(0);
  const char *encoded = htmlencode_cstr(VARDATA(txt), VARSIZE_ANY_EXHDR(txt));
  if (encoded)
    PG_RETURN_TEXT_P(cstring_to_text(encoded));
  else
    PG_RETURN_NULL();
}


