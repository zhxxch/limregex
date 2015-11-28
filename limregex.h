/*
 * Copyright (C) 2015 ZHANG X. <201560039.uibe.edu.cn>
 *
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

#ifndef LIMREGEX_H
#define LIMREGEX_H

/*  '\\' (backslash) as escape character    */
#define ESCAPE_CHAR 0x5c

/*  Max number of bytes of an multibyte character   */
#define CHARW_MAX MB_CUR_MAX

/*  Encoding    */
#define ENCODING UTF_8
#define UTF_8 "en_US.UTF-8"

/*  Compile a Regular Expression.
 *  Input:  Array of uint to store instructions,
 *          Above array size
 *          RegExp string
 *  Output: Number of RegExp VM instructions
 */
int limregexcl( unsigned int[], int,    const char[]    );

/*  Execute a compiled RegExp.
 *  Input:  String,
 *          Array of instructions
 *  Output: Match Length
 */
int limregexec( const char[],   unsigned int[]  );

#endif
