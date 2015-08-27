/*
 * Copyright (c) 2015 Xiaochen Z. <201560039.uibe.edu.cn>
 * Released under the MIT licence, see bottom of file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "limregex.h"

char re0[]  =   "hs|(s|hh)s*h";
char str0[] =   "sssssh";

int main(int argc, char *argv[]){
    if(argc>2){
        int match_len;
        char res[strlen(argv[2])];
        int code_len = strlen(argv[1]) * 10;
        unsigned int *regexp_code
            = malloc((size_t)(code_len * sizeof(unsigned int)));
        if(regexp_code){
            /*  compile */
            while(limregexcl(regexp_code, code_len, argv[1])<0){
                code_len *= 2;
                unsigned int *new_re_code
                    = realloc(regexp_code,
                            code_len * sizeof(unsigned int));
                if(new_re_code != regexp_code){
                    free(regexp_code);
                    regexp_code = new_re_code;
                }
            }
        }
        /*  match   */
        if((match_len = limregexec(argv[2], regexp_code)) > 0){
            snprintf(res, match_len+1, "%s", argv[2]);
            printf("Matched \"%s\".\n", res);
        }else{
            printf("String doesn't match.\n");
        }
        free(regexp_code);
    }else{
        printf("Usage: limregex-demo regexp string\n"
                "Regular expression: %s\n"
                "String: %s\n", re0, str0);
        
        unsigned int match, code[50];
        /*  compile */
        limregexcl(code, 50, re0);
        /*  match   */
        match = limregexec(str0, code);
        
        printf("Match Length: %d\n", match);
    }
    return EXIT_SUCCESS;
}

/*
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated
 * documentation files (the “Software”), to deal in the
 * Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall
 * be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY
 * KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
 * OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
