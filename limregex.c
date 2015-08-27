/* 
 * limregex
 * --- A DFA-based regular expression
 *      prototype implementation,
 *      WITHOUT submatch extraction.
 *
 * Supports only * | ( ) ? . \d \D \w \W \s \S.
 * Support backslash"\" and "\xHH" escapes.
 * Support UTF-8 and could be configured
 *  to support other encodings.
 * NO support for + ,
 *      + and \+ both will match character '+'.
 * NO support for character classes,
 *      '[', ']' will be regarded as normal character.
 * NO support for counted repetition,
 *      '{', '}' will be regarded as normal character.
 * NO support for backreference.
 *
 * Copyright (c) 2015 X. ZHANG <201560039@uibe.edu.cn>
 * Released under the MIT licence, see bottom of file.
 */

#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <limits.h>
#include <ctype.h>
#include "limregex.h"

/*  Fixed final(accept) state for NFA.
 *  initial_state 1                 */
#define FINAL_STATE 0

/*  Make sure operators in postfix RegExp
 *  are all greater than characters.    */
#define OP_MIN 0x8000

/*  Mark input characters in a FA transition.
 *  METACHAR:       transition for character range
 *  EPSILON:        Epsilon-move
 *  EXTRACT_FLAG:   not a transition but descript
 *                  submatch-extraction range
 */ 
enum deltaInputFlag {
    METACHAR = ESCAPE_CHAR<<8,
    EXTRACT_FLAG = (ESCAPE_CHAR+1)<<8,
    EPSILON = (ESCAPE_CHAR+2)<<8
};

/*  Operators near the bottom have higher precedence.    */
enum regexpOperator {
    LPAREN = OP_MIN + 1,
    RPAREN,
    UNION,
    CONCAT,
    CLOSURE,
    QUESTION,
    EXTRACT
};

enum subsetState{ACTIVE = 0, COMPLETE = 1, FINAL = 2};

/*  Transition Function */
struct FAdelta{
    int before;
    int input;
    int after;
    int nparen;
};

enum regexpVMcode{
    JMP = 1,
    JDEG,
    JNDEG,
    JWRD,
    JNWRD,
    JSPC,
    JNSPC,
    JANY,
    JEQ,
    JNEQ,
    FRWRD,
    FAIL,
    ACCEPT,
    ACCEPTM1
};

char ischartype[127] = { 0, ['w'] = 'w', ['W'] = 'W', ['s'] = 's', ['S'] = 'S', ['d'] = 'd', ['D'] = 'D' };

/*  \xHH    escape sequences    */
unsigned short escx1[127] = { 0x100, ['0'] = 0x00, ['1'] = 0x10, ['2'] = 0x20, ['3'] = 0x30, ['4'] = 0x40, ['5'] = 0x50, ['6'] = 0x60, ['7'] = 0x70, ['8'] = 0x80, ['9'] = 0x90, ['A'] = 0xa0, ['B'] = 0xb0, ['C'] = 0xc0, ['D'] = 0xd0, ['E'] = 0xe0, ['F'] = 0xf0, ['a'] = 0xa0, ['b'] = 0xb0, ['c'] = 0xc0, ['d'] = 0xd0, ['e'] = 0xe0, ['f'] = 0xf0 };

unsigned short escx0[127] = { 0x100, ['0'] = 0x0, ['1'] = 0x1, ['2'] = 0x2, ['3'] = 0x3, ['4'] = 0x4, ['5'] = 0x5, ['6'] = 0x6, ['7'] = 0x7, ['8'] = 0x8, ['9'] = 0x9, ['A'] = 0xa, ['B'] = 0xb, ['C'] = 0xc, ['D'] = 0xd, ['E'] = 0xe, ['F'] = 0xf, ['a'] = 0xa, ['b'] = 0xb, ['c'] = 0xc, ['d'] = 0xd, ['e'] = 0xe, ['f'] = 0xf };

/*  Virtual Machine
 *  @param  str     Input String
 *  @param  regexvm limregexcl() compiled VM instructions
 *  @return int     Match Length
 *                  >0  accepted
 *                  0   rejected
 */
int limregexec(const char str[], unsigned int regexvm[]){
    unsigned int *pc = regexvm;
    const char *c = str;
    while(1){
        switch(*pc){
            case JMP:
                pc = regexvm + pc[1];
                break;
            case JEQ:
                pc++;
                if(*pc++ == *c)
                    pc = regexvm + pc[0];
                else pc++;
                break;
            case FRWRD:
                c++;
                pc++;
                break;
            case JANY:/*    .   */
                c += (mblen(c, MB_CUR_MAX)>0)?
                    (mblen(c, MB_CUR_MAX)-1):1;
                pc = regexvm + pc[1];
                break;
            case JDEG:
                pc++;
                if(isdigit(*c))
                    pc = regexvm + pc[0];
                else pc++;
                break;
            case JNDEG:
                pc++;
                if(isdigit(*c))pc++;
                else
                    pc = regexvm + pc[0];
                break;
            case JWRD:
                pc++;
                if(isalnum(*c) || *c=='_')
                    pc = regexvm + pc[0];
                else pc++;
                break;
            case JNWRD:
                pc++;
                if(isalnum(*c) || *c=='_')pc++;
                else
                    pc = regexvm + pc[0];
                break;
            case JSPC:
                pc++;
                if(isspace(*c))
                    pc = regexvm + pc[0];
                else pc++;
                break;
            case JNSPC:
                pc++;
                if(isspace(*c))pc++;
                else
                    pc = regexvm + pc[0];
                break;
            case JNEQ:
                pc++;
                if(*pc++ == *c)pc++;
                else pc = regexvm + pc[0];
                break;
            case FAIL:
                return 0;
            case ACCEPT:
                return ( c - str + 1 );
            case ACCEPTM1:
                if(c[1] == '\0')
                    return ( c - str + 1);
            default: pc++;
        }
    }
    return 0;
}

/*  qsort() NFA moves compare function for sortNfa()
 */
static int nfaCmp(const void *ap, const void *bp){
    const struct FAdelta *a = *(struct FAdelta **)ap;
    const struct FAdelta *b = *(struct FAdelta **)bp;
    if(a->input == EXTRACT_FLAG && b->input == EXTRACT_FLAG)return (0);
    else if(b->input == EXTRACT_FLAG)return (1);
    else if(a->input == EXTRACT_FLAG)return (-1);
    else return (a->before - b->before)?(a->before - b->before):(b->input - a->input);
}

/*  qsort() DFA moves compare function for sortDfa()    */
static int dfaCmp(const void *ap, const void *bp){
    const struct FAdelta *a = *(struct FAdelta **)ap;
    const struct FAdelta *b = *(struct FAdelta **)bp;
    if(a->input == EXTRACT_FLAG && b->input == EXTRACT_FLAG)return (0);
    else if(b->input == EXTRACT_FLAG)return (1);
    else if(a->input == EXTRACT_FLAG)return (-1);
    else return (a->before - b->before)?(a->before - b->before):(a->input - b->input);
}

/*  Sorting an array of pointers instead sorting
 *  the NFA moves array.
 *  1.  All moves with mark EXTRACT_FLAG are at the top.
 *  2.  Moves with metachar or epsilon-move mark
 *      are at the top, then is other moves with the
 *      same prev state.
 */
static void sortNfa(struct FAdelta *nfaDeltaRef[], int nfaDeltaLen){
    qsort(nfaDeltaRef, nfaDeltaLen, sizeof(struct FAdelta *), nfaCmp);
}

/*  Sorting an array of pointers instead sorting
 *  the DFA moves array.
 *  1.  All moves with mark EXTRACT_FLAG are at the top.
 */
static void sortDfa(struct FAdelta *dfaDeltaRef[], int dfaDeltaLen){
    qsort(dfaDeltaRef, dfaDeltaLen, sizeof(struct FAdelta *), dfaCmp);
}

/*  Convert infix RegExp to postfix exp,
 *  and add concat operator.
 *  
 *  @param  post        Array to store postfix expression
 *  @param  postSize    Allocated size of post[]
 *  @param  regexp      Infix RegExp string
 *  @param  regexpSize  Input RegExp string length
 *  @return int     Length of the expression in post[]
 */
static unsigned int regexpPost(unsigned int post[], unsigned int postSize, const char *regexp, unsigned int regexpSize){
    /*  parenthese  */
    unsigned int pn = 0;

    unsigned int stack[regexpSize];
    unsigned int top = 0;
    
    unsigned int rn = 0;
    unsigned int v = 0;
    unsigned int concat = 0;
    int charWidth = 1;
    while(regexp[rn] && pn<postSize){
        switch(regexp[rn]){
            case '(':
                if(concat)stack[top++] = CONCAT;
                stack[top++] = LPAREN;
                rn++;
                concat = 0;
                continue;
            case ')':
                while(top && stack[top-1]>LPAREN)
                    post[pn++] = stack[--top];
                post[pn++] = EXTRACT;
                top--;
                rn++;
                break;
            case '|':
                concat = 0;
                if(regexp[rn-1] == '('
                        && regexp[rn+1] == ')'){
                    /*  (|) */
                    rn++;
                    post[pn++] = EPSILON;
                    continue;
                }
                while(top && stack[top-1]>UNION)
                    post[pn++] = stack[--top];
                stack[top++] = UNION;
                if(regexp[rn+1] == '|'
                        || regexp[rn+1] == ')'
                        || regexp[rn-1] == '(')
                    /*  (|xxx), (xxx|)  */
                    post[pn++] = EPSILON;
                rn++;
                continue;
            case '*':
                while(top && stack[top-1]>CLOSURE)
                    post[pn++] = stack[--top];
                stack[top++] = CLOSURE;
                rn++;
                break;
            case '?':
                /*  (xxx)?  =>  ((xxx)|)    */
                while(top && stack[top-1]>CLOSURE)
                    post[pn++] = stack[--top];
                post[pn++] = EPSILON;
                post[pn++] = UNION;
                rn++;
                break;
            case '.':
                if(concat){
                    while(top && stack[top-1]>CONCAT)
                        post[pn++] = stack[--top];
                    stack[top++] = CONCAT;
                }
                post[pn++] = regexp[rn++] | METACHAR;
                break;
            case ESCAPE_CHAR:
                if(regexp[rn+1]=='x'
                        && (v=escx1[regexp[rn+2]&0x7f]
                            + escx0[regexp[rn+3]&0x7f])
                        <0x100){
                    /*  \xHH    v = 0xHH    */
                    post[pn++] = v;
                    rn+=4;
                }else if(ischartype[regexp[rn+1]&0x7f]){
                    /*  \d, \w ...  */
                    post[pn++] = regexp[++rn] | METACHAR;
                    ++rn;
                }else{
                    /*  \\, \* ...  */
                    post[pn++] = regexp[++rn];
                    ++rn;
                }
                if(concat){
                    while(top && stack[top-1]>CONCAT)
                        post[pn++] = stack[--top];
                    stack[top++] = CONCAT;
                }
                break;
            default:
                if(concat){
                    while(top && stack[top-1]>CONCAT)
                        post[pn++] = stack[--top];
                    stack[top++] = CONCAT;
                }
                charWidth = mblen(regexp+rn, CHARW_MAX);
                /*  \x20\xE7\xBE\x9F*\x20   ('---' = concat)
                 *  =>  \x20---(\xE7---\xBE---\x9f)*---\x20
                 */ 
                if((v=charWidth)>1){
                    while(v--) post[pn++] = regexp[rn++];
                    while(--charWidth) post[pn++] = CONCAT;
                }else post[pn++] = regexp[rn++];
        }
        concat = 1;
    }
    while(top)post[pn++] = stack[--top];
    return pn;
}


/*  Convert postfix expression to NFA transition function.
 *
 *  @param  nfaDelta        Array to store NFA moves
 *  @param  nfaDeltaSize    Allocated size of nfaDelta[]
 *  @param  post            Array of postfix expression
 *  @param  postSize        Length of post[]
 *  @return int         Number of moves in nfaDelta[]
 */
static int regexpPostNfa(struct FAdelta nfaDelta[], unsigned int nfaDeltaSize, unsigned int *post, unsigned int postSize){
    /*  stack of before state   */
    unsigned int stBf[nfaDeltaSize];
    /*  stack of after state    */
    unsigned int stAf[nfaDeltaSize];
    /*  top ptr for stBf, stAf  */
    unsigned int top = 0;
    /*  new NFA delta to create */
    unsigned int newDelta = 0;
    /*  available NFA node label    */
    unsigned int label = 0;
    /*  submatch parenthese label   */
    unsigned int npn = 0;

    stAf[top] = label++;    stBf[top] = label++;    top++;

    for(unsigned int m = postSize; m--;){
        switch(post[m]){
            case UNION:
                /*  [x]->[y]    =>
                 *  [x]->[y]
                 *  [x]->[y]
                 */
                stBf[top] = stBf[top-1];
                stAf[top] = stAf[top-1];
                top++;
                break;
            case CONCAT:
                /*  [x]->[y]    =>
                 *  [x]->[z]->[y]
                 */
                stBf[top] = label++;
                stAf[top] = stAf[top-1];
                stAf[top-1] = stBf[top];
                top++;
                break;
            case CLOSURE:
                /*  [x]->[y]    =>
                 *  [x]->[z]->[y]
                 *  [z]->[z]
                 */
                nfaDelta[newDelta].before
                    = stBf[top-1];
                nfaDelta[newDelta].after
                    = label;
                nfaDelta[newDelta].input
                    = EPSILON;
                nfaDelta[newDelta].nparen
                    = 0;
                newDelta++;

                nfaDelta[newDelta].before
                    = label;
                nfaDelta[newDelta].after
                    = stAf[top-1];
                nfaDelta[newDelta].input
                    = EPSILON;
                nfaDelta[newDelta].nparen
                    = 0;
                newDelta++;

                stBf[top-1] = label;
                stAf[top-1] = label;
                label++;
                break;
            case EXTRACT:
                nfaDelta[newDelta].before
                    = stBf[top-1];
                nfaDelta[newDelta].after
                    = stAf[top-1];
                nfaDelta[newDelta].input
                    = EXTRACT_FLAG;
                nfaDelta[newDelta].nparen
                    = ++npn;
                newDelta++;
                break;
            default:
                /*  [x]->[y]    =>
                 *  [x]--(input)-->[y]
                 */
                nfaDelta[newDelta].before
                    = stBf[top-1];
                nfaDelta[newDelta].after
                    = stAf[top-1];
                nfaDelta[newDelta].input
                    = post[m];
                nfaDelta[newDelta].nparen
                    = 0;
                newDelta++;
                top--;
        }
    }
    return newDelta;
}


/*  Index moves of different prev state
 *  in sorted array of pointers that point to NFA moves.
 */
void indexNfaDeltas(struct FAdelta **nfaDeltaRef, int nfaDeltaLen, struct FAdelta **index[], int indexSize){
    int i = 0;
    index[i] = nfaDeltaRef + i;
    while(i<nfaDeltaLen && nfaDeltaRef[i]->input == EXTRACT_FLAG)i++;
    index[nfaDeltaRef[i]->before] = nfaDeltaRef + i;
    i++;
    for(; i<nfaDeltaLen; i++){
        if(nfaDeltaRef[i]->before != nfaDeltaRef[i-1]->before)
            index[nfaDeltaRef[i]->before] = nfaDeltaRef + i;
    }
    index[indexSize] 
        = nfaDeltaRef + i;
}

void indexDfaDeltas(struct FAdelta **dfaDeltaRef, int dfaDeltaLen, struct FAdelta **index[], int indexSize){
    int i = 0;
    while(dfaDeltaRef[i]->input == EXTRACT_FLAG)i++;
    index[dfaDeltaRef[i]->before] = dfaDeltaRef + i;
    while(++i<dfaDeltaLen){
        if(dfaDeltaRef[i]->before != dfaDeltaRef[i-1]->before)
            index[dfaDeltaRef[i]->before] = dfaDeltaRef + i;
    }
    index[indexSize] 
        = dfaDeltaRef + dfaDeltaLen;
}

int isCharType(int metachar, char c){
    if(metachar & METACHAR){
        switch(metachar & 0xff){
            case '.': return 1;
            case 'd': return isdigit(c);
            case 'D': return !isdigit(c);
            case 'w': return isalnum(c)||c=='_';
            case 'W': return !(isalnum(c)||c=='_');
            case 's': return isspace(c);
            case 'S': return !isspace(c);
        }
    }
    return 0;
}

/*  Find subset name by set elements.
 *  @return int Subset index
 */
int sub_findSubset(int *dfaLabel[], int dfaLabelSize, int newSubset[], int newSubsetSize){
    for(int label = 0; label<dfaLabelSize; label++){
        if((dfaLabel[label+1] - dfaLabel[label]) != newSubsetSize)
            continue;
        if(memcmp(dfaLabel[label], newSubset, newSubsetSize)==0)
            return label;
    }
    return dfaLabelSize;
}

/*  Find DFA state label which has the
 *  nfa state label as an subset element.
 *  Search start from dfaN to dfaLabelLen.
 */
int nfaLabelDfaLabel(int nfaLabel, int dfaN, int *dfaLabel[], int dfaLabelLen){
    for(int ld = dfaN; ld < dfaLabelLen; ld++){
        for(int ln = 0; dfaLabel[ld] + ln < dfaLabel[ld+1]; ln++)
            if(dfaLabel[ld][ln] == nfaLabel)return ld;
    }
    return dfaLabelLen;
}

/*  Insert subset of nfa states which are
 *  the next state of a certain input
 *  and a certain prev state.
 *
 *  @param  index   Cursor of NFA moves, and index of sorted
 *                  pointers array of pointers of NFA moves.
 *  @return int Size of inserted subsets.
 */
int sub_afterSubset(int *index, struct FAdelta *nfaDelta[], int nfaDeltaSize, int *dfaLabel[], int *labelSize){
    /*  newSubsetSize:            |<-       ->|
     *  subset:   |1|2|3|4|5|6|7|8| | | | | | | | | | | | | |
     *  dfaLabel   0 1   2 3 ..... ^labelSize  ^newSize
     *  
     *  */
    int newSize = *labelSize + 1;
    int newSubsetSize = 0;
    int subset;
    int first = *index;
    dfaLabel[newSize] = dfaLabel[*labelSize];
    struct FAdelta *currDelta;
    if(!(nfaDelta[first]->input & METACHAR)){
        for(int n = 0; nfaDelta[n]->input & METACHAR; n++){
            if(isCharType(nfaDelta[n]->input, nfaDelta[first]->input & 0xff)){
                dfaLabel[newSize][newSubsetSize++]
                = nfaDelta[n]->after;
            }
        }
    }
    for(currDelta = nfaDelta[first];
            *index < nfaDeltaSize
            && currDelta->input == nfaDelta[first]->input;
            currDelta = nfaDelta[++(*index)]){ 
        if(currDelta->input == EXTRACT_FLAG)continue;
        dfaLabel[newSize][newSubsetSize++] = currDelta->after;
    }
    dfaLabel[newSize] += newSubsetSize;
    if((subset = sub_findSubset(dfaLabel, *labelSize, dfaLabel[*labelSize], newSubsetSize)) < *labelSize)
        /*  subset already exist    */
        return subset;
    else {
        *labelSize = newSize;
        return(newSize-1);
    }
}

/*  qsort() NFA moves compare function for sub_insDfaDelta()
 *  Order by input desc, next state asc.
 */
int nfaCmpInput(const void *ap, const void *bp){
    const struct FAdelta *a = *(struct FAdelta **)ap;
    const struct FAdelta *b = *(struct FAdelta **)bp;
    return(b->input - a->input)?(b->input - a->input):(a->after-b->after);
}

/*  Add next state for active(incomplete) DFA state(subset).
 */
void sub_insDfaDelta(int label, int *dfaLabel[], int *labelSize, struct FAdelta **nfaDeltaIndex[], int nfaDeltaIndexLen, struct FAdelta **newDfaDelta, int labelStates[]){
    /*  prepare */
    int nfaSubsetSize = 0;
    int subsetEl[nfaDeltaIndexLen];
    int subsetElLen = 0;
    /*  copy subset element */
    for(int subset = 0; dfaLabel[label] + subset < dfaLabel[label+1]; subset++){
        if(dfaLabel[label][subset] == FINAL_STATE){
            labelStates[label] |= FINAL;
            continue;
        }
        subsetEl[subsetElLen++] = dfaLabel[label][subset];
    }
    /*  Epsilon-closure, and,
     *  calculate NFA moves related to this DFA state.
     */
    for(int ElIter = 0; ElIter < subsetElLen; ElIter++){
        if(nfaDeltaIndex[subsetEl[ElIter]] == NULL)
            continue;
        nfaSubsetSize
            += nfaDeltaIndex[subsetEl[ElIter]+1]
            - nfaDeltaIndex[subsetEl[ElIter]];
            for(int n=0; nfaDeltaIndex[subsetEl[ElIter]] + n
                    < nfaDeltaIndex[subsetEl[ElIter]+1]
                    && nfaDeltaIndex[subsetEl[ElIter]][n]->input
                    == EPSILON; n++){
                subsetEl[subsetElLen++] = nfaDeltaIndex[subsetEl[ElIter]][n]->after;
                if(nfaDeltaIndex[subsetEl[ElIter]][n]->after == FINAL_STATE)labelStates[label] |= FINAL;
            }
    }
    if(nfaSubsetSize == 0)return;
    
    struct FAdelta *nfaSubset[nfaSubsetSize];
    int currSubsetSize = 0;
    int copySize = 0;
    for(int ElIter = subsetElLen; ElIter--;){
        if(subsetEl[ElIter] == 0)continue;
        copySize = nfaDeltaIndex[subsetEl[ElIter]+1]
        - nfaDeltaIndex[subsetEl[ElIter]];
        memcpy(nfaSubset+currSubsetSize
                , nfaDeltaIndex[subsetEl[ElIter]]
                , copySize * sizeof(struct FAdelta *));
        currSubsetSize += copySize;
    }
    /*  group(sort) by input    */
    qsort(nfaSubset, nfaSubsetSize, sizeof(struct FAdelta *), nfaCmpInput);
    struct FAdelta *currDfaDelta;
    int nfaDeltaIter = 0;
    /*  for each input  */
    while(nfaDeltaIter < nfaSubsetSize){
        while(nfaSubset[nfaDeltaIter]->input == EPSILON)
            nfaDeltaIter++;
        currDfaDelta = (*newDfaDelta)++;
        currDfaDelta->before = label;
        currDfaDelta->input = nfaSubset[nfaDeltaIter]->input;
        if(currDfaDelta->input == EXTRACT_FLAG){
            currDfaDelta->after = 
                currDfaDelta->nparen = nfaSubset[nfaDeltaIter]->nparen;
        }
        currDfaDelta->after
            = sub_afterSubset(&nfaDeltaIter, nfaSubset, nfaSubsetSize, dfaLabel, labelSize);
    }
}

/*  TODO:   Implement submatch extraction.
 */
void regexpExtructIndex(int indexb[], int indexa[], int *dfaLabel[], int dfaLabelLen, struct FAdelta **nfaDeltaIndex[]){
    for(int dlabel = 0; dlabel<dfaLabelLen; dlabel++){
        indexa[dlabel] = 0;
        indexb[dlabel] = 0;
        for(int nlabeli = 0; nlabeli + dfaLabel[dlabel] < dfaLabel[dlabel+1]; nlabeli++){
            for(int i = 0; nfaDeltaIndex[0]+i < nfaDeltaIndex[1]; i++){
                if(dfaLabel[dlabel][nlabeli] == nfaDeltaIndex[0][i]->before)
                    indexb[dlabel] = nfaDeltaIndex[0][i]->nparen;
                if(dfaLabel[dlabel][nlabeli] == nfaDeltaIndex[0][i]->after)
                    indexa[dlabel] = nfaDeltaIndex[0][i]->nparen;
            }
        }
    }
}

/*  Convert NFA to DFA.
 *  @return int Number of moves in dfaDelta[]
 */
int regexpNfaDfa(struct FAdelta **nfaDeltaIndex[], int nfaDeltaIndexLen, struct FAdelta dfaDelta[], int dfaDeltaLen, int subsetLabelStates[], int extructIndexa[], int extructIndexb[]){
    /*  XXX: appropriate subsetSize */
    int subsetSize = nfaDeltaIndex[nfaDeltaIndexLen]-nfaDeltaIndex[1];
    int subsetElements[subsetSize];
    int *subsetLabels[subsetSize];

    subsetElements[0] = 1;
    subsetLabels[0] = subsetElements;
    subsetLabels[1] = subsetElements + 1;
    subsetLabelStates[0] = ACTIVE;
    int subsetLen = 1;
    struct FAdelta *currDfaDelta = dfaDelta;
    for(int i = 0;
            i<subsetLen
            && currDfaDelta-dfaDelta
            < dfaDeltaLen; i++){
        if(subsetLabelStates[i] & COMPLETE) continue;
        sub_insDfaDelta(i, subsetLabels, &subsetLen, nfaDeltaIndex, nfaDeltaIndexLen, &currDfaDelta, subsetLabelStates);
        subsetLabelStates[i] |= COMPLETE;
    }

    regexpExtructIndex(extructIndexa, extructIndexb, subsetLabels, subsetLen, nfaDeltaIndex);
    return (currDfaDelta - dfaDelta);
}

/*  Compile DFA to VM instructions.
 */
int regexpDfaCl(struct FAdelta **dfaDeltaIndex[], int dfaDeltaIndexLen, int dfaLabelState[], unsigned int instr[], int instrLen){
    int dfaDeltaLen = dfaDeltaIndex[dfaDeltaIndexLen] - dfaDeltaIndex[0];
    struct FAdelta **deltaRef = dfaDeltaIndex[0];
    int labelAddr[dfaDeltaIndexLen+1];
    for(int n = 0; n<dfaDeltaIndexLen+1; n++)labelAddr[n] = 0;
    int n = 0;
    int initJmpPos = 0;
    int currLabel = -1;
    initJmpPos = n;
    instr[n++] = JMP;
    instr[n] = n + 4;
    n++;
    instr[n++] = ACCEPT;

    for(int i = 0; i < dfaDeltaLen; i++){
        if(n >= instrLen)return(-1);

        if(currLabel != deltaRef[i]->before){
            currLabel = deltaRef[i]->before;
            instr[n++] = FAIL;
            labelAddr[currLabel] = n;
            if(dfaLabelState[deltaRef[i]->before] & FINAL)
                instr[n++] = ACCEPTM1;
            instr[n++] = FRWRD;
        }

        if(deltaRef[i]->input & METACHAR){
            switch(deltaRef[i]->input & 0xff){
                case '.':
                    instr[n++] = JANY;
                    break;
                case 's':
                    instr[n++] = JSPC;
                    break;
                case 'S':
                    instr[n++] = JNSPC;
                    break;
                case 'd':
                    instr[n++] = JDEG;
                    break;
                case 'D':
                    instr[n++] = JNDEG;
                    break;
                case 'w':
                    instr[n++] = JWRD;
                    break;
                case 'W':
                    instr[n++] = JNWRD;
                    break;
            }
        }else{
            instr[n++] = JEQ;
            instr[n++] = deltaRef[i]->input & 0xff;
        }
        deltaRef[i]->input = n++;
    }
    /*  fill in the jump-to address */
    for(int i = 0; i < dfaDeltaLen; i++){
        instr[deltaRef[i]->input] = labelAddr[deltaRef[i]->after];
        if(!instr[deltaRef[i]->input])
            instr[deltaRef[i]->input] = initJmpPos + 2;
    }
    instr[n++] = FAIL;
    return n;
}

/*  Compile a Regular Expression.
 *  @param  regexVM     Array to store VM instructions
 *  @param  VMSize      Allocated size of regexVM[]
 *  @param  regexStr    RegExp string
 *  @return int     Number of instructions.
 *                  <0  for no enough space in regexVM[].
 *                  0   for regexpStr = "\0".
 */
int limregexcl(unsigned int regexVM[], int VMSize, const char regexStr[]){
    setlocale(LC_CTYPE, UTF_8);
    int regexpStrLen = strlen(regexStr);
    if(regexpStrLen == 0)return 0;
    /*  XXX: appropriate size for postexp[]     */
    unsigned int postexp[regexpStrLen*2+1];
    unsigned int postLen = regexpPost(postexp, regexpStrLen*2+1, regexStr, regexpStrLen);

    /*  XXX: appropriate size of nfaDeltas[]    */
    struct FAdelta nfaDeltas[postLen];
    unsigned int nfaDeltaLen = regexpPostNfa(nfaDeltas, postLen, postexp, postLen);

    /*  sort NFA moves  */
    struct FAdelta *nfaDeltasRef[nfaDeltaLen];
    /*  sort array of pointers instead array of struct  */
    for(int n=0; n<nfaDeltaLen; n++)
        nfaDeltasRef[n] = nfaDeltas + n;

    sortNfa(nfaDeltasRef, nfaDeltaLen);
    
    /*  index NFA prev moves    */
    int nfaDeltasIndexLen = nfaDeltasRef[nfaDeltaLen-1]->before+1;
    struct FAdelta **nfaDeltasIndex[nfaDeltasIndexLen+1];
    indexNfaDeltas(nfaDeltasRef, nfaDeltaLen, nfaDeltasIndex, nfaDeltasIndexLen);

    /*  nfa->dfa    */
    /*  XXX: appropriate size for dfaDeltas[]   */
    struct FAdelta dfaDeltas[postLen];
    int dfaLabelStates[postLen];
    int extructIndexa[postLen];
    int extructIndexb[postLen];
    /*  set all DFA state as ACTIVE */
    memset(dfaLabelStates, 0, sizeof(int)*postLen);
    memset(extructIndexa, 0, sizeof(int)*postLen);
    memset(extructIndexb, 0, sizeof(int)*postLen);
    
    int dfaDeltaLen = regexpNfaDfa(nfaDeltasIndex, nfaDeltasIndexLen, dfaDeltas, postLen, dfaLabelStates, extructIndexa, extructIndexb);

    struct FAdelta *dfaDeltasRef[dfaDeltaLen];
    /*  sort array of pointers instead array of struct  */
    for(int n=0; n<dfaDeltaLen; n++)
        dfaDeltasRef[n] = dfaDeltas + n;

    sortDfa(dfaDeltasRef, dfaDeltaLen);
    int dfaDeltasIndexLen = dfaDeltasRef[dfaDeltaLen-1]->before+1;
    struct FAdelta **dfaDeltasIndex[dfaDeltasIndexLen+1];
    indexDfaDeltas(dfaDeltasRef, dfaDeltaLen, dfaDeltasIndex, dfaDeltasIndexLen);

    int codeLen = regexpDfaCl(dfaDeltasIndex, dfaDeltasIndexLen, dfaLabelStates, regexVM, VMSize);
    return codeLen;
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
