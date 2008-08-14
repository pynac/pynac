/* A Bison parser, made by GNU Bison 1.875c.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     T_NUMBER = 258,
     T_SYMBOL = 259,
     T_LITERAL = 260,
     T_DIGITS = 261,
     T_QUOTE = 262,
     T_QUOTE2 = 263,
     T_QUOTE3 = 264,
     T_EQUAL = 265,
     T_NOTEQ = 266,
     T_LESSEQ = 267,
     T_GREATEREQ = 268,
     T_QUIT = 269,
     T_WARRANTY = 270,
     T_PRINT = 271,
     T_IPRINT = 272,
     T_PRINTLATEX = 273,
     T_PRINTCSRC = 274,
     T_TIME = 275,
     T_XYZZY = 276,
     T_INVENTORY = 277,
     T_LOOK = 278,
     T_SCORE = 279,
     T_COMPLEX_SYMBOLS = 280,
     T_REAL_SYMBOLS = 281,
     NEG = 282
   };
#endif
#define T_NUMBER 258
#define T_SYMBOL 259
#define T_LITERAL 260
#define T_DIGITS 261
#define T_QUOTE 262
#define T_QUOTE2 263
#define T_QUOTE3 264
#define T_EQUAL 265
#define T_NOTEQ 266
#define T_LESSEQ 267
#define T_GREATEREQ 268
#define T_QUIT 269
#define T_WARRANTY 270
#define T_PRINT 271
#define T_IPRINT 272
#define T_PRINTLATEX 273
#define T_PRINTCSRC 274
#define T_TIME 275
#define T_XYZZY 276
#define T_INVENTORY 277
#define T_LOOK 278
#define T_SCORE 279
#define T_COMPLEX_SYMBOLS 280
#define T_REAL_SYMBOLS 281
#define NEG 282




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
typedef int YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



