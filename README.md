# limregex
A DFA-based regular expression prototype implementation  
    -----**WITHOUT submatch extraction**.  
  
Supports only * | ( ) ? . \d \D \w \W \s \S.  
Support backslash"\" and "\xHH" escapes.  
Support UTF-8 and could be configured to support other encodings.  
  
**NO support for + ,**  
     + and \+ both will match character '+'.  
**NO support for character classes,**  
     '[', ']' will be regarded as normal character.  
**NO support for counted repetition,**  
     '{', '}' will be regarded as normal character.  
**NO support for backreference.**  

## Demo:
        unsigned int match_length, code[50];
        /*  compile */
        limregexcl(code, 50, "hs|(s|hh)");
        /*  match   */
        match_length = limregexec("sssssh", code);

TODO: submatch extraction  
