# limregex
A C programming exercise,
DFA-based regular expression implementation

Supports only * | ( ) ? . \d \D \w \W \s \S.
With backslash and "\xHH" escapes and UTF-8. 
  
**NO support for + ,**  
     + and \+ both match character '+'.  
**NO support for character classes,**  
     '[', ']' will be characters to match.  
**NO support for counted repetition,**  
     '{', '}' will be characters to match.  
**NO support for backreference.**  

## Demo:
        unsigned int match_length, code[50];
        /*  compile */
        limregexcl(code, 50, "hs|(s|hh)");
        /*  match   */
        match_length = limregexec("sssssh", code);

TODO: fix bugs, refactor all codes 
