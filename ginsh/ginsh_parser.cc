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

/* Written by Richard Stallman by simplifying the original so called
   ``semantic'' parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Using locations.  */
#define YYLSP_NEEDED 0



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




/* Copy the first part of user declarations.  */
#line 29 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"

#include "config.h"
#ifdef HAVE_RUSAGE
#include <sys/resource.h>
#else
#include <ctime>
#endif

#if HAVE_UNISTD_H
#include <sys/types.h>
#include <unistd.h>
#endif

#include <stdexcept>

#include "ginsh.h"

#define YYERROR_VERBOSE 1

#ifdef REALLY_HAVE_LIBREADLINE
// Original readline settings
static int orig_completion_append_character;
#if (GINAC_RL_VERSION_MAJOR < 4) || (GINAC_RL_VERSION_MAJOR == 4 && GINAC_RL_VERSION_MINOR < 2)
static char *orig_basic_word_break_characters;
#else
static const char *orig_basic_word_break_characters;
#endif

#if (GINAC_RL_VERSION_MAJOR >= 5)
#define GINAC_RL_COMPLETER_CAST(a) const_cast<char *>((a))
#else
#define GINAC_RL_COMPLETER_CAST(a) (a)
#endif
#endif // REALLY_HAVE_LIBREADLINE

// Expression stack for %, %% and %%%
static void push(const ex &e);
static ex exstack[3];

// Start and end time for the time() function
#ifdef HAVE_RUSAGE
static struct rusage start_time, end_time;
#define START_TIMER getrusage(RUSAGE_SELF, &start_time);
#define STOP_TIMER getrusage(RUSAGE_SELF, &end_time);
#define PRINT_TIME_USED cout << \
   (end_time.ru_utime.tv_sec - start_time.ru_utime.tv_sec) + \
       (end_time.ru_stime.tv_sec - start_time.ru_stime.tv_sec) + \
       double(end_time.ru_utime.tv_usec - start_time.ru_utime.tv_usec) / 1e6 + \
       double(end_time.ru_stime.tv_usec - start_time.ru_stime.tv_usec) / 1e6 \
                       << 's' << endl;
#else
static std::clock_t start_time, end_time;
#define START_TIMER start_time = std::clock();
#define STOP_TIMER end_time = std::clock();
#define PRINT_TIME_USED \
  cout << double(end_time - start_time)/CLOCKS_PER_SEC << 's' << endl;
#endif

// Table of functions (a multimap, because one function may appear with different
// numbers of parameters)
typedef ex (*fcnp)(const exprseq &e);
typedef ex (*fcnp2)(const exprseq &e, int serial);

struct fcn_desc {
	fcn_desc() : p(NULL), num_params(0), is_ginac(false), serial(0) {}
	fcn_desc(fcnp func, int num) : p(func), num_params(num), is_ginac(false), serial(0) {}
	fcn_desc(fcnp2 func, int num, int ser) : p((fcnp)func), num_params(num), is_ginac(true), serial(ser) {}

	fcnp p;		// Pointer to function
	int num_params;	// Number of parameters (0 = arbitrary)
	bool is_ginac;	// Flag: function is GiNaC function
	int serial;	// GiNaC function serial number (if is_ginac == true)
};

typedef multimap<string, fcn_desc> fcn_tab;
static fcn_tab fcns;

static fcn_tab::const_iterator find_function(const ex &sym, int req_params);

// Table to map help topics to help strings
typedef multimap<string, string> help_tab;
static help_tab help;

static void insert_fcn_help(const char *name, const char *str);
static void print_help(const string &topic);
static void print_help_topics(void);


/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
typedef int YYSTYPE;
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif



/* Copy the second part of user declarations.  */


/* Line 214 of yacc.c.  */
#line 229 "ginsh_parser.cc"

#if ! defined (yyoverflow) || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   define YYSTACK_ALLOC alloca
#  endif
# else
#  if defined (alloca) || defined (_ALLOCA_H)
#   define YYSTACK_ALLOC alloca
#  else
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning. */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
# else
#  if defined (__STDC__) || defined (__cplusplus)
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   define YYSIZE_T size_t
#  endif
#  define YYSTACK_ALLOC malloc
#  define YYSTACK_FREE free
# endif
#endif /* ! defined (yyoverflow) || YYERROR_VERBOSE */


#if (! defined (yyoverflow) \
     && (! defined (__cplusplus) \
	 || (defined (YYSTYPE_IS_TRIVIAL) && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  short yyss;
  YYSTYPE yyvs;
  };

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (short) + sizeof (YYSTYPE))				\
      + YYSTACK_GAP_MAXIMUM)

/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined (__GNUC__) && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  register YYSIZE_T yyi;		\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (0)
#  endif
# endif

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack)					\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack, Stack, yysize);				\
	Stack = &yyptr->Stack;						\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (0)

#endif

#if defined (__STDC__) || defined (__cplusplus)
   typedef signed char yysigned_char;
#else
   typedef short yysigned_char;
#endif

/* YYFINAL -- State number of the termination state. */
#define YYFINAL  2
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   300

/* YYNTOKENS -- Number of terminals. */
#define YYNTOKENS  48
/* YYNNTS -- Number of nonterminals. */
#define YYNNTS  10
/* YYNRULES -- Number of rules. */
#define YYNRULES  67
/* YYNRULES -- Number of states. */
#define YYNSTATES  122

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   282

#define YYTRANSLATE(YYX) 						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const unsigned char yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    36,     2,     2,     2,     2,     2,    42,
      39,    40,    32,    30,    47,    31,     2,    33,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,    38,    37,
      28,    27,    29,    41,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,    45,     2,    46,    35,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,    43,     2,    44,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    34
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const unsigned char yyprhs[] =
{
       0,     0,     3,     4,     7,     9,    12,    15,    21,    27,
      33,    39,    42,    45,    48,    51,    54,    57,    60,    62,
      64,    66,    68,    70,    72,    74,    76,    77,    83,    86,
      89,    91,    93,    97,    99,   101,   103,   105,   107,   112,
     116,   120,   124,   128,   132,   136,   140,   144,   148,   152,
     156,   160,   163,   166,   170,   173,   177,   181,   185,   187,
     191,   192,   194,   196,   200,   204,   210,   212
};

/* YYRHS -- A `-1'-separated list of the rules' RHS. */
static const yysigned_char yyrhs[] =
{
      49,     0,    -1,    -1,    49,    50,    -1,    37,    -1,    52,
      37,    -1,    52,    38,    -1,    16,    39,    52,    40,    37,
      -1,    17,    39,    52,    40,    37,    -1,    18,    39,    52,
      40,    37,    -1,    19,    39,    52,    40,    37,    -1,    41,
       4,    -1,    41,    20,    -1,    41,    16,    -1,    41,    17,
      -1,    41,    18,    -1,    41,    19,    -1,    41,    41,    -1,
      14,    -1,    15,    -1,    21,    -1,    22,    -1,    23,    -1,
      24,    -1,    26,    -1,    25,    -1,    -1,    20,    51,    39,
      52,    40,    -1,     1,    37,    -1,     1,    38,    -1,     3,
      -1,     4,    -1,    42,     4,    42,    -1,     5,    -1,     6,
      -1,     7,    -1,     8,    -1,     9,    -1,     4,    39,    53,
      40,    -1,     6,    27,     3,    -1,     4,    27,    52,    -1,
      52,    10,    52,    -1,    52,    11,    52,    -1,    52,    28,
      52,    -1,    52,    12,    52,    -1,    52,    29,    52,    -1,
      52,    13,    52,    -1,    52,    30,    52,    -1,    52,    31,
      52,    -1,    52,    32,    52,    -1,    52,    33,    52,    -1,
      31,    52,    -1,    30,    52,    -1,    52,    35,    52,    -1,
      52,    36,    -1,    39,    52,    40,    -1,    43,    54,    44,
      -1,    45,    56,    46,    -1,    52,    -1,    53,    47,    52,
      -1,    -1,    55,    -1,    52,    -1,    55,    47,    52,    -1,
      45,    57,    46,    -1,    56,    47,    45,    57,    46,    -1,
      52,    -1,    57,    47,    52,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const unsigned short yyrline[] =
{
       0,   143,   143,   144,   147,   148,   157,   165,   173,   187,
     195,   203,   204,   205,   206,   207,   208,   209,   210,   211,
     224,   225,   226,   227,   232,   233,   234,   234,   235,   236,
     239,   240,   241,   242,   243,   244,   245,   246,   247,   255,
     256,   257,   258,   259,   260,   261,   262,   263,   264,   265,
     266,   267,   268,   269,   270,   271,   272,   273,   276,   277,
     280,   281,   284,   285,   288,   289,   292,   293
};
#endif

#if YYDEBUG || YYERROR_VERBOSE
/* YYTNME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals. */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "T_NUMBER", "T_SYMBOL", "T_LITERAL",
  "T_DIGITS", "T_QUOTE", "T_QUOTE2", "T_QUOTE3", "T_EQUAL", "T_NOTEQ",
  "T_LESSEQ", "T_GREATEREQ", "T_QUIT", "T_WARRANTY", "T_PRINT", "T_IPRINT",
  "T_PRINTLATEX", "T_PRINTCSRC", "T_TIME", "T_XYZZY", "T_INVENTORY",
  "T_LOOK", "T_SCORE", "T_COMPLEX_SYMBOLS", "T_REAL_SYMBOLS", "'='", "'<'",
  "'>'", "'+'", "'-'", "'*'", "'/'", "NEG", "'^'", "'!'", "';'", "':'",
  "'('", "')'", "'?'", "'''", "'{'", "'}'", "'['", "']'", "','", "$accept",
  "input", "line", "@1", "exp", "exprseq", "list_or_empty", "list",
  "matrix", "row", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const unsigned short yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,    61,    60,    62,
      43,    45,    42,    47,   282,    94,    33,    59,    58,    40,
      41,    63,    39,   123,   125,    91,    93,    44
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const unsigned char yyr1[] =
{
       0,    48,    49,    49,    50,    50,    50,    50,    50,    50,
      50,    50,    50,    50,    50,    50,    50,    50,    50,    50,
      50,    50,    50,    50,    50,    50,    51,    50,    50,    50,
      52,    52,    52,    52,    52,    52,    52,    52,    52,    52,
      52,    52,    52,    52,    52,    52,    52,    52,    52,    52,
      52,    52,    52,    52,    52,    52,    52,    52,    53,    53,
      54,    54,    55,    55,    56,    56,    57,    57
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const unsigned char yyr2[] =
{
       0,     2,     0,     2,     1,     2,     2,     5,     5,     5,
       5,     2,     2,     2,     2,     2,     2,     2,     1,     1,
       1,     1,     1,     1,     1,     1,     0,     5,     2,     2,
       1,     1,     3,     1,     1,     1,     1,     1,     4,     3,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     2,     2,     3,     2,     3,     3,     3,     1,     3,
       0,     1,     1,     3,     3,     5,     1,     3
};

/* YYDEFACT[STATE-NAME] -- Default rule to reduce with in state
   STATE-NUM when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const unsigned char yydefact[] =
{
       2,     0,     1,     0,    30,    31,    33,    34,    35,    36,
      37,    18,    19,     0,     0,     0,     0,    26,    20,    21,
      22,    23,    25,    24,     0,     0,     4,     0,     0,     0,
      60,     0,     3,     0,    28,    29,     0,     0,     0,     0,
       0,     0,     0,     0,    52,    51,     0,    11,    13,    14,
      15,    16,    12,    17,     0,    62,     0,    61,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    54,     5,     6,    40,    58,     0,    39,     0,     0,
       0,     0,     0,    55,    32,    56,     0,    66,     0,    57,
       0,    41,    42,    44,    46,    43,    45,    47,    48,    49,
      50,    53,    38,     0,     0,     0,     0,     0,     0,    63,
      64,     0,     0,    59,     7,     8,     9,    10,    27,    67,
       0,    65
};

/* YYDEFGOTO[NTERM-NUM]. */
static const yysigned_char yydefgoto[] =
{
      -1,     1,    32,    43,    87,    76,    56,    57,    59,    88
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -36
static const short yypact[] =
{
     -36,   100,   -36,   -35,   -36,   -26,   -36,   -13,   -36,   -36,
     -36,   -36,   -36,   -21,   -18,   -14,    -9,   -36,   -36,   -36,
     -36,   -36,   -36,   -36,     3,     3,   -36,     3,    33,    43,
       3,     9,   -36,    60,   -36,   -36,     3,     3,    52,     3,
       3,     3,     3,    17,   -31,   -31,   122,   -36,   -36,   -36,
     -36,   -36,   -36,   -36,    15,   255,    14,    28,     3,   -19,
       3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,   -36,   -36,   -36,   255,   255,   -25,   -36,   136,   167,
     180,   211,     3,   -36,   -36,   -36,     3,   255,   -15,   -36,
      31,   264,   264,    47,    47,    47,    47,   -16,   -16,   -31,
     -31,   -31,   -36,     3,    49,    50,    57,    62,   224,   255,
     -36,     3,     3,   255,   -36,   -36,   -36,   -36,   -36,   255,
      -3,   -36
};

/* YYPGOTO[NTERM-NUM].  */
static const yysigned_char yypgoto[] =
{
     -36,   -36,   -36,   -36,    -1,   -36,   -36,   -36,   -36,   -28
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If zero, do what YYDEFACT says.
   If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -1
static const unsigned char yytable[] =
{
      33,    36,    34,    35,    70,    71,     4,     5,     6,     7,
       8,     9,    10,    37,    38,   102,    68,    69,    39,    70,
      71,    40,   103,    44,    45,    41,    46,    89,    90,    55,
      42,   110,   111,    24,    25,    74,    75,    47,    78,    79,
      80,    81,    27,   121,   111,    29,    30,    54,    31,    48,
      49,    50,    51,    52,    58,    77,    82,    84,    85,    91,
      92,    93,    94,    95,    96,    97,    98,    99,   100,   101,
      60,    61,    62,    63,    53,    86,   112,    66,    67,    68,
      69,   108,    70,    71,   120,   109,   114,   115,    64,    65,
      66,    67,    68,    69,   116,    70,    71,    72,    73,   117,
       2,     3,   113,     4,     5,     6,     7,     8,     9,    10,
     119,     0,     0,     0,    11,    12,    13,    14,    15,    16,
      17,    18,    19,    20,    21,    22,    23,     0,     0,     0,
      24,    25,    60,    61,    62,    63,     0,    26,     0,    27,
       0,    28,    29,    30,     0,    31,    60,    61,    62,    63,
      64,    65,    66,    67,    68,    69,     0,    70,    71,     0,
       0,     0,    83,     0,    64,    65,    66,    67,    68,    69,
       0,    70,    71,     0,     0,     0,   104,    60,    61,    62,
      63,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      60,    61,    62,    63,     0,    64,    65,    66,    67,    68,
      69,     0,    70,    71,     0,     0,     0,   105,    64,    65,
      66,    67,    68,    69,     0,    70,    71,     0,     0,     0,
     106,    60,    61,    62,    63,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    60,    61,    62,    63,     0,    64,
      65,    66,    67,    68,    69,     0,    70,    71,     0,     0,
       0,   107,    64,    65,    66,    67,    68,    69,     0,    70,
      71,     0,     0,     0,   118,    60,    61,    62,    63,     0,
       0,     0,     0,     0,     0,     0,    62,    63,     0,     0,
       0,     0,     0,    64,    65,    66,    67,    68,    69,     0,
      70,    71,    64,    65,    66,    67,    68,    69,     0,    70,
      71
};

static const yysigned_char yycheck[] =
{
       1,    27,    37,    38,    35,    36,     3,     4,     5,     6,
       7,     8,     9,    39,    27,    40,    32,    33,    39,    35,
      36,    39,    47,    24,    25,    39,    27,    46,    47,    30,
      39,    46,    47,    30,    31,    36,    37,     4,    39,    40,
      41,    42,    39,    46,    47,    42,    43,     4,    45,    16,
      17,    18,    19,    20,    45,     3,    39,    42,    44,    60,
      61,    62,    63,    64,    65,    66,    67,    68,    69,    70,
      10,    11,    12,    13,    41,    47,    45,    30,    31,    32,
      33,    82,    35,    36,   112,    86,    37,    37,    28,    29,
      30,    31,    32,    33,    37,    35,    36,    37,    38,    37,
       0,     1,   103,     3,     4,     5,     6,     7,     8,     9,
     111,    -1,    -1,    -1,    14,    15,    16,    17,    18,    19,
      20,    21,    22,    23,    24,    25,    26,    -1,    -1,    -1,
      30,    31,    10,    11,    12,    13,    -1,    37,    -1,    39,
      -1,    41,    42,    43,    -1,    45,    10,    11,    12,    13,
      28,    29,    30,    31,    32,    33,    -1,    35,    36,    -1,
      -1,    -1,    40,    -1,    28,    29,    30,    31,    32,    33,
      -1,    35,    36,    -1,    -1,    -1,    40,    10,    11,    12,
      13,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      10,    11,    12,    13,    -1,    28,    29,    30,    31,    32,
      33,    -1,    35,    36,    -1,    -1,    -1,    40,    28,    29,
      30,    31,    32,    33,    -1,    35,    36,    -1,    -1,    -1,
      40,    10,    11,    12,    13,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    10,    11,    12,    13,    -1,    28,
      29,    30,    31,    32,    33,    -1,    35,    36,    -1,    -1,
      -1,    40,    28,    29,    30,    31,    32,    33,    -1,    35,
      36,    -1,    -1,    -1,    40,    10,    11,    12,    13,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    12,    13,    -1,    -1,
      -1,    -1,    -1,    28,    29,    30,    31,    32,    33,    -1,
      35,    36,    28,    29,    30,    31,    32,    33,    -1,    35,
      36
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const unsigned char yystos[] =
{
       0,    49,     0,     1,     3,     4,     5,     6,     7,     8,
       9,    14,    15,    16,    17,    18,    19,    20,    21,    22,
      23,    24,    25,    26,    30,    31,    37,    39,    41,    42,
      43,    45,    50,    52,    37,    38,    27,    39,    27,    39,
      39,    39,    39,    51,    52,    52,    52,     4,    16,    17,
      18,    19,    20,    41,     4,    52,    54,    55,    45,    56,
      10,    11,    12,    13,    28,    29,    30,    31,    32,    33,
      35,    36,    37,    38,    52,    52,    53,     3,    52,    52,
      52,    52,    39,    40,    42,    44,    47,    52,    57,    46,
      47,    52,    52,    52,    52,    52,    52,    52,    52,    52,
      52,    52,    40,    47,    40,    40,    40,    40,    52,    52,
      46,    47,    45,    52,    37,    37,    37,    37,    40,    52,
      57,    46
};

#if ! defined (YYSIZE_T) && defined (__SIZE_TYPE__)
# define YYSIZE_T __SIZE_TYPE__
#endif
#if ! defined (YYSIZE_T) && defined (size_t)
# define YYSIZE_T size_t
#endif
#if ! defined (YYSIZE_T)
# if defined (__STDC__) || defined (__cplusplus)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# endif
#endif
#if ! defined (YYSIZE_T)
# define YYSIZE_T unsigned int
#endif

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */

#define YYFAIL		goto yyerrlab

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      yytoken = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { 								\
      yyerror ("syntax error: cannot back up");\
      YYERROR;							\
    }								\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

/* YYLLOC_DEFAULT -- Compute the default location (before the actions
   are run).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)		\
   ((Current).first_line   = (Rhs)[1].first_line,	\
    (Current).first_column = (Rhs)[1].first_column,	\
    (Current).last_line    = (Rhs)[N].last_line,	\
    (Current).last_column  = (Rhs)[N].last_column)
#endif

/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (0)

# define YYDSYMPRINT(Args)			\
do {						\
  if (yydebug)					\
    yysymprint Args;				\
} while (0)

# define YYDSYMPRINTF(Title, Token, Value, Location)		\
do {								\
  if (yydebug)							\
    {								\
      YYFPRINTF (stderr, "%s ", Title);				\
      yysymprint (stderr, 					\
                  Token, Value);	\
      YYFPRINTF (stderr, "\n");					\
    }								\
} while (0)

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_stack_print (short *bottom, short *top)
#else
static void
yy_stack_print (bottom, top)
    short *bottom;
    short *top;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (/* Nothing. */; bottom <= top; ++bottom)
    YYFPRINTF (stderr, " %d", *bottom);
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yy_reduce_print (int yyrule)
#else
static void
yy_reduce_print (yyrule)
    int yyrule;
#endif
{
  int yyi;
  unsigned int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %u), ",
             yyrule - 1, yylno);
  /* Print the symbols being reduced, and their result.  */
  for (yyi = yyprhs[yyrule]; 0 <= yyrhs[yyi]; yyi++)
    YYFPRINTF (stderr, "%s ", yytname [yyrhs[yyi]]);
  YYFPRINTF (stderr, "-> %s\n", yytname [yyr1[yyrule]]);
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (Rule);		\
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YYDSYMPRINT(Args)
# define YYDSYMPRINTF(Title, Token, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   SIZE_MAX < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#if defined (YYMAXDEPTH) && YYMAXDEPTH == 0
# undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif



#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined (__GLIBC__) && defined (_STRING_H)
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
static YYSIZE_T
#   if defined (__STDC__) || defined (__cplusplus)
yystrlen (const char *yystr)
#   else
yystrlen (yystr)
     const char *yystr;
#   endif
{
  register const char *yys = yystr;

  while (*yys++ != '\0')
    continue;

  return yys - yystr - 1;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined (__GLIBC__) && defined (_STRING_H) && defined (_GNU_SOURCE)
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
#   if defined (__STDC__) || defined (__cplusplus)
yystpcpy (char *yydest, const char *yysrc)
#   else
yystpcpy (yydest, yysrc)
     char *yydest;
     const char *yysrc;
#   endif
{
  register char *yyd = yydest;
  register const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

#endif /* !YYERROR_VERBOSE */



#if YYDEBUG
/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yysymprint (FILE *yyoutput, int yytype, YYSTYPE *yyvaluep)
#else
static void
yysymprint (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  if (yytype < YYNTOKENS)
    {
      YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
# ifdef YYPRINT
      YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
    }
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  switch (yytype)
    {
      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}

#endif /* ! YYDEBUG */
/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

#if defined (__STDC__) || defined (__cplusplus)
static void
yydestruct (int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yytype, yyvaluep)
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;

  switch (yytype)
    {

      default:
        break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM);
# else
int yyparse ();
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */



/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;



/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
# if defined (__STDC__) || defined (__cplusplus)
int yyparse (void *YYPARSE_PARAM)
# else
int yyparse (YYPARSE_PARAM)
  void *YYPARSE_PARAM;
# endif
#else /* ! YYPARSE_PARAM */
#if defined (__STDC__) || defined (__cplusplus)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
  
  register int yystate;
  register int yyn;
  int yyresult;
  /* Number of tokens to shift before error messages enabled.  */
  int yyerrstatus;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken = 0;

  /* Three stacks and their tools:
     `yyss': related to states,
     `yyvs': related to semantic values,
     `yyls': related to locations.

     Refer to the stacks thru separate pointers, to allow yyoverflow
     to reallocate them elsewhere.  */

  /* The state stack.  */
  short	yyssa[YYINITDEPTH];
  short *yyss = yyssa;
  register short *yyssp;

  /* The semantic value stack.  */
  YYSTYPE yyvsa[YYINITDEPTH];
  YYSTYPE *yyvs = yyvsa;
  register YYSTYPE *yyvsp;



#define YYPOPSTACK   (yyvsp--, yyssp--)

  YYSIZE_T yystacksize = YYINITDEPTH;

  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;


  /* When reducing, the number of symbols on the RHS of the reduced
     rule.  */
  int yylen;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed. so pushing a state here evens the stacks.
     */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack. Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	short *yyss1 = yyss;


	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow ("parser stack overflow",
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),

		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyoverflowlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyoverflowlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	short *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyoverflowlab;
	YYSTACK_RELOCATE (yyss);
	YYSTACK_RELOCATE (yyvs);

#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;


      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYPACT_NINF)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YYDSYMPRINTF ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yyn == 0 || yyn == YYTABLE_NINF)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */
  YYDPRINTF ((stderr, "Shifting token %s, ", yytname[yytoken]));

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;


  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  yystate = yyn;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 5:
#line 148 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {
		try {
			cout << yyvsp[-1] << endl;
			push(yyvsp[-1]);
		} catch (exception &e) {
			cerr << e.what() << endl;
			YYERROR;
		}
	}
    break;

  case 6:
#line 157 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {
		try {
			push(yyvsp[-1]);
		} catch (exception &e) {
			std::cerr << e.what() << endl;
			YYERROR;
		}
	}
    break;

  case 7:
#line 165 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {
		try {
			yyvsp[-2].print(print_tree(std::cout));
		} catch (exception &e) {
			std::cerr << e.what() << endl;
			YYERROR;
		}
	}
    break;

  case 8:
#line 173 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {
		try {
			ex e = yyvsp[-2];
			if (!e.info(info_flags::integer))
				throw (std::invalid_argument("argument to iprint() must be an integer"));
			long i = ex_to<numeric>(e).to_long();
			cout << i << endl;
			cout << "#o" << oct << i << endl;
			cout << "#x" << hex << i << dec << endl;
		} catch (exception &e) {
			cerr << e.what() << endl;
			YYERROR;
		}
	}
    break;

  case 9:
#line 187 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {
		try {
			yyvsp[-2].print(print_latex(std::cout)); cout << endl;
		} catch (exception &e) {
			std::cerr << e.what() << endl;
			YYERROR;
		}
	}
    break;

  case 10:
#line 195 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {
		try {
			yyvsp[-2].print(print_csrc_double(std::cout)); cout << endl;
		} catch (exception &e) {
			std::cerr << e.what() << endl;
			YYERROR;
		}
	}
    break;

  case 11:
#line 203 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {print_help(ex_to<symbol>(yyvsp[0]).get_name());}
    break;

  case 12:
#line 204 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {print_help("time");}
    break;

  case 13:
#line 205 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {print_help("print");}
    break;

  case 14:
#line 206 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {print_help("iprint");}
    break;

  case 15:
#line 207 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {print_help("print_latex");}
    break;

  case 16:
#line 208 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {print_help("print_csrc");}
    break;

  case 17:
#line 209 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {print_help_topics();}
    break;

  case 18:
#line 210 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {YYACCEPT;}
    break;

  case 19:
#line 211 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {
		cout << "This program is free software; you can redistribute it and/or modify it under\n";
		cout << "the terms of the GNU General Public License as published by the Free Software\n";
		cout << "Foundation; either version 2 of the License, or (at your option) any later\n";
		cout << "version.\n";
		cout << "This program is distributed in the hope that it will be useful, but WITHOUT\n";
		cout << "ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS\n";
		cout << "FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more\n";
		cout << "details.\n";
		cout << "You should have received a copy of the GNU General Public License along with\n";
		cout << "this program. If not, write to the Free Software Foundation, 675 Mass Ave,\n";
		cout << "Cambridge, MA 02139, USA.\n";
	}
    break;

  case 20:
#line 224 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {cout << "Nothing happens.\n";}
    break;

  case 21:
#line 225 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {cout << "You're not carrying anything.\n";}
    break;

  case 22:
#line 226 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {cout << "You're in a twisty little maze of passages, all alike.\n";}
    break;

  case 23:
#line 227 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {
		cout << "If you were to quit now, you would score ";
		cout << (syms.size() > 350 ? 350 : syms.size());
		cout << " out of a possible 350.\n";
	}
    break;

  case 24:
#line 232 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    { symboltype = domain::real; }
    break;

  case 25:
#line 233 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    { symboltype = domain::complex; }
    break;

  case 26:
#line 234 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    { START_TIMER }
    break;

  case 27:
#line 234 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    { STOP_TIMER PRINT_TIME_USED }
    break;

  case 28:
#line 235 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {yyclearin; yyerrok;}
    break;

  case 29:
#line 236 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {yyclearin; yyerrok;}
    break;

  case 30:
#line 239 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {yyval = yyvsp[0];}
    break;

  case 31:
#line 240 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {yyval = yyvsp[0].eval();}
    break;

  case 32:
#line 241 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {yyval = yyvsp[-1];}
    break;

  case 33:
#line 242 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {yyval = yyvsp[0];}
    break;

  case 34:
#line 243 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {yyval = yyvsp[0];}
    break;

  case 35:
#line 244 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {yyval = exstack[0];}
    break;

  case 36:
#line 245 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {yyval = exstack[1];}
    break;

  case 37:
#line 246 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {yyval = exstack[2];}
    break;

  case 38:
#line 247 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {
		fcn_tab::const_iterator i = find_function(yyvsp[-3], yyvsp[-1].nops());
		if (i->second.is_ginac) {
			yyval = ((fcnp2)(i->second.p))(ex_to<exprseq>(yyvsp[-1]), i->second.serial);
		} else {
			yyval = (i->second.p)(ex_to<exprseq>(yyvsp[-1]));
		}
	}
    break;

  case 39:
#line 255 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {yyval = yyvsp[0]; Digits = ex_to<numeric>(yyvsp[0]).to_int();}
    break;

  case 40:
#line 256 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {yyval = yyvsp[0]; const_cast<symbol&>(ex_to<symbol>(yyvsp[-2])).assign(yyvsp[0]);}
    break;

  case 41:
#line 257 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {yyval = yyvsp[-2] == yyvsp[0];}
    break;

  case 42:
#line 258 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {yyval = yyvsp[-2] != yyvsp[0];}
    break;

  case 43:
#line 259 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {yyval = yyvsp[-2] < yyvsp[0];}
    break;

  case 44:
#line 260 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {yyval = yyvsp[-2] <= yyvsp[0];}
    break;

  case 45:
#line 261 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {yyval = yyvsp[-2] > yyvsp[0];}
    break;

  case 46:
#line 262 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {yyval = yyvsp[-2] >= yyvsp[0];}
    break;

  case 47:
#line 263 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {yyval = yyvsp[-2] + yyvsp[0];}
    break;

  case 48:
#line 264 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {yyval = yyvsp[-2] - yyvsp[0];}
    break;

  case 49:
#line 265 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {yyval = yyvsp[-2] * yyvsp[0];}
    break;

  case 50:
#line 266 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {yyval = yyvsp[-2] / yyvsp[0];}
    break;

  case 51:
#line 267 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {yyval = -yyvsp[0];}
    break;

  case 52:
#line 268 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {yyval = yyvsp[0];}
    break;

  case 53:
#line 269 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {yyval = power(yyvsp[-2], yyvsp[0]);}
    break;

  case 54:
#line 270 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {yyval = factorial(yyvsp[-1]);}
    break;

  case 55:
#line 271 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {yyval = yyvsp[-1];}
    break;

  case 56:
#line 272 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {yyval = yyvsp[-1];}
    break;

  case 57:
#line 273 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {yyval = lst_to_matrix(ex_to<lst>(yyvsp[-1]));}
    break;

  case 58:
#line 276 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {yyval = exprseq(yyvsp[0]);}
    break;

  case 59:
#line 277 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {exprseq es(ex_to<exprseq>(yyvsp[-2])); yyval = es.append(yyvsp[0]);}
    break;

  case 60:
#line 280 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {yyval = *new lst;}
    break;

  case 61:
#line 281 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {yyval = yyvsp[0];}
    break;

  case 62:
#line 284 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {yyval = lst(yyvsp[0]);}
    break;

  case 63:
#line 285 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {lst l(ex_to<lst>(yyvsp[-2])); yyval = l.append(yyvsp[0]);}
    break;

  case 64:
#line 288 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {yyval = lst(yyvsp[-1]);}
    break;

  case 65:
#line 289 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {lst l(ex_to<lst>(yyvsp[-4])); yyval = l.append(yyvsp[-1]);}
    break;

  case 66:
#line 292 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {yyval = lst(yyvsp[0]);}
    break;

  case 67:
#line 293 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"
    {lst l(ex_to<lst>(yyvsp[-2])); yyval = l.append(yyvsp[0]);}
    break;


    }

/* Line 993 of yacc.c.  */
#line 1632 "ginsh_parser.cc"

  yyvsp -= yylen;
  yyssp -= yylen;


  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;


  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (YYPACT_NINF < yyn && yyn < YYLAST)
	{
	  YYSIZE_T yysize = 0;
	  int yytype = YYTRANSLATE (yychar);
	  const char* yyprefix;
	  char *yymsg;
	  int yyx;

	  /* Start YYX at -YYN if negative to avoid negative indexes in
	     YYCHECK.  */
	  int yyxbegin = yyn < 0 ? -yyn : 0;

	  /* Stay within bounds of both yycheck and yytname.  */
	  int yychecklim = YYLAST - yyn;
	  int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
	  int yycount = 0;

	  yyprefix = ", expecting ";
	  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
	    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
	      {
		yysize += yystrlen (yyprefix) + yystrlen (yytname [yyx]);
		yycount += 1;
		if (yycount == 5)
		  {
		    yysize = 0;
		    break;
		  }
	      }
	  yysize += (sizeof ("syntax error, unexpected ")
		     + yystrlen (yytname[yytype]));
	  yymsg = (char *) YYSTACK_ALLOC (yysize);
	  if (yymsg != 0)
	    {
	      char *yyp = yystpcpy (yymsg, "syntax error, unexpected ");
	      yyp = yystpcpy (yyp, yytname[yytype]);

	      if (yycount < 5)
		{
		  yyprefix = ", expecting ";
		  for (yyx = yyxbegin; yyx < yyxend; ++yyx)
		    if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR)
		      {
			yyp = yystpcpy (yyp, yyprefix);
			yyp = yystpcpy (yyp, yytname[yyx]);
			yyprefix = " or ";
		      }
		}
	      yyerror (yymsg);
	      YYSTACK_FREE (yymsg);
	    }
	  else
	    yyerror ("syntax error; also virtual memory exhausted");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror ("syntax error");
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* If at end of input, pop the error token,
	     then the rest of the stack, then return failure.  */
	  if (yychar == YYEOF)
	     for (;;)
	       {
		 YYPOPSTACK;
		 if (yyssp == yyss)
		   YYABORT;
		 YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
		 yydestruct (yystos[*yyssp], yyvsp);
	       }
        }
      else
	{
	  YYDSYMPRINTF ("Error: discarding", yytoken, &yylval, &yylloc);
	  yydestruct (yytoken, &yylval);
	  yychar = YYEMPTY;

	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

#ifdef __GNUC__
  /* Pacify GCC when the user code never invokes YYERROR and the label
     yyerrorlab therefore never appears in user code.  */
  if (0)
     goto yyerrorlab;
#endif

  yyvsp -= yylen;
  yyssp -= yylen;
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (yyn != YYPACT_NINF)
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;

      YYDSYMPRINTF ("Error: popping", yystos[*yyssp], yyvsp, yylsp);
      yydestruct (yystos[yystate], yyvsp);
      YYPOPSTACK;
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  if (yyn == YYFINAL)
    YYACCEPT;

  YYDPRINTF ((stderr, "Shifting error token, "));

  *++yyvsp = yylval;


  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#ifndef yyoverflow
/*----------------------------------------------.
| yyoverflowlab -- parser overflow comes here.  |
`----------------------------------------------*/
yyoverflowlab:
  yyerror ("parser stack overflow");
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  return yyresult;
}


#line 301 "/user/jensv/ginac/ginac/ginsh/ginsh_parser.yy"

// Error print routine
int yyerror(char *s)
{
	cerr << s << " at " << yytext << endl;
	return 0;
}

// Push expression "e" onto the expression stack (for ", "" and """)
static void push(const ex &e)
{
	exstack[2] = exstack[1];
	exstack[1] = exstack[0];
	exstack[0] = e;
}


/*
 *  Built-in functions
 */

static ex f_collect(const exprseq &e) {return e[0].collect(e[1]);}
static ex f_collect_distributed(const exprseq &e) {return e[0].collect(e[1], true);}
static ex f_collect_common_factors(const exprseq &e) {return collect_common_factors(e[0]);}
static ex f_convert_H_to_Li(const exprseq &e) {return convert_H_to_Li(e[0], e[1]);}
static ex f_degree(const exprseq &e) {return e[0].degree(e[1]);}
static ex f_denom(const exprseq &e) {return e[0].denom();}
static ex f_eval1(const exprseq &e) {return e[0].eval();}
static ex f_evalf1(const exprseq &e) {return e[0].evalf();}
static ex f_evalm(const exprseq &e) {return e[0].evalm();}
static ex f_eval_integ(const exprseq &e) {return e[0].eval_integ();}
static ex f_expand(const exprseq &e) {return e[0].expand();}
static ex f_gcd(const exprseq &e) {return gcd(e[0], e[1]);}
static ex f_has(const exprseq &e) {return e[0].has(e[1]) ? ex(1) : ex(0);}
static ex f_lcm(const exprseq &e) {return lcm(e[0], e[1]);}
static ex f_lcoeff(const exprseq &e) {return e[0].lcoeff(e[1]);}
static ex f_ldegree(const exprseq &e) {return e[0].ldegree(e[1]);}
static ex f_lsolve(const exprseq &e) {return lsolve(e[0], e[1]);}
static ex f_nops(const exprseq &e) {return e[0].nops();}
static ex f_normal1(const exprseq &e) {return e[0].normal();}
static ex f_numer(const exprseq &e) {return e[0].numer();}
static ex f_numer_denom(const exprseq &e) {return e[0].numer_denom();}
static ex f_pow(const exprseq &e) {return pow(e[0], e[1]);}
static ex f_sqrt(const exprseq &e) {return sqrt(e[0]);}
static ex f_sqrfree1(const exprseq &e) {return sqrfree(e[0]);}
static ex f_subs2(const exprseq &e) {return e[0].subs(e[1]);}
static ex f_tcoeff(const exprseq &e) {return e[0].tcoeff(e[1]);}

#define CHECK_ARG(num, type, fcn) if (!is_a<type>(e[num])) throw(std::invalid_argument("argument " #num " to " #fcn "() must be a " #type))

static ex f_charpoly(const exprseq &e)
{
	CHECK_ARG(0, matrix, charpoly);
	return ex_to<matrix>(e[0]).charpoly(e[1]);
}

static ex f_coeff(const exprseq &e)
{
	CHECK_ARG(2, numeric, coeff);
	return e[0].coeff(e[1], ex_to<numeric>(e[2]).to_int());
}

static ex f_content(const exprseq &e)
{
	return e[0].content(e[1]);
}

static ex f_decomp_rational(const exprseq &e)
{
	return decomp_rational(e[0], e[1]);
}

static ex f_determinant(const exprseq &e)
{
	CHECK_ARG(0, matrix, determinant);
	return ex_to<matrix>(e[0]).determinant();
}

static ex f_diag(const exprseq &e)
{
	size_t dim = e.nops();
	matrix &m = *new matrix(dim, dim);
	for (size_t i=0; i<dim; i++)
		m.set(i, i, e.op(i));
	return m;
}

static ex f_diff2(const exprseq &e)
{
	CHECK_ARG(1, symbol, diff);
	return e[0].diff(ex_to<symbol>(e[1]));
}

static ex f_diff3(const exprseq &e)
{
	CHECK_ARG(1, symbol, diff);
	CHECK_ARG(2, numeric, diff);
	return e[0].diff(ex_to<symbol>(e[1]), ex_to<numeric>(e[2]).to_int());
}

static ex f_divide(const exprseq &e)
{
	ex q;
	if (divide(e[0], e[1], q))
		return q;
	else
		return fail();
}

static ex f_eval2(const exprseq &e)
{
	CHECK_ARG(1, numeric, eval);
	return e[0].eval(ex_to<numeric>(e[1]).to_int());
}

static ex f_evalf2(const exprseq &e)
{
	CHECK_ARG(1, numeric, evalf);
	return e[0].evalf(ex_to<numeric>(e[1]).to_int());
}

static ex f_find(const exprseq &e)
{
	lst found;
	e[0].find(e[1], found);
	return found;
}

static ex f_fsolve(const exprseq &e)
{
	CHECK_ARG(1, symbol, fsolve);
	CHECK_ARG(2, numeric, fsolve);
	CHECK_ARG(3, numeric, fsolve);
	return fsolve(e[0], ex_to<symbol>(e[1]), ex_to<numeric>(e[2]), ex_to<numeric>(e[3]));
}

static ex f_integer_content(const exprseq &e)
{
	return e[0].expand().integer_content();
}

static ex f_integral(const exprseq &e)
{
	CHECK_ARG(0, symbol, integral);
	return integral(e[0], e[1], e[2], e[3]);
}

static ex f_inverse(const exprseq &e)
{
	CHECK_ARG(0, matrix, inverse);
	return ex_to<matrix>(e[0]).inverse();
}

static ex f_is(const exprseq &e)
{
	CHECK_ARG(0, relational, is);
	return (bool)ex_to<relational>(e[0]) ? ex(1) : ex(0);
}

class apply_map_function : public map_function {
	ex apply;
public:
	apply_map_function(const ex & a) : apply(a) {}
	virtual ~apply_map_function() {}
	ex operator()(const ex & e) { return apply.subs(wild() == e, true); }
};

static ex f_map(const exprseq &e)
{
	apply_map_function fcn(e[1]);
	return e[0].map(fcn);
}

static ex f_match(const exprseq &e)
{
	lst repl_lst;
	if (e[0].match(e[1], repl_lst))
		return repl_lst;
	else
		return fail();
}

static ex f_normal2(const exprseq &e)
{
	CHECK_ARG(1, numeric, normal);
	return e[0].normal(ex_to<numeric>(e[1]).to_int());
}

static ex f_op(const exprseq &e)
{
	CHECK_ARG(1, numeric, op);
	int n = ex_to<numeric>(e[1]).to_int();
	if (n < 0 || n >= (int)e[0].nops())
		throw(std::out_of_range("second argument to op() is out of range"));
	return e[0].op(n);
}

static ex f_prem(const exprseq &e)
{
	return prem(e[0], e[1], e[2]);
}

static ex f_primpart(const exprseq &e)
{
	return e[0].primpart(e[1]);
}

static ex f_quo(const exprseq &e)
{
	return quo(e[0], e[1], e[2]);
}

static ex f_rank(const exprseq &e)
{
	CHECK_ARG(0, matrix, rank);
	return ex_to<matrix>(e[0]).rank();
}

static ex f_rem(const exprseq &e)
{
	return rem(e[0], e[1], e[2]);
}

static ex f_resultant(const exprseq &e)
{
	CHECK_ARG(2, symbol, resultant);
	return resultant(e[0], e[1], ex_to<symbol>(e[2]));
}

static ex f_series(const exprseq &e)
{
	CHECK_ARG(2, numeric, series);
	return e[0].series(e[1], ex_to<numeric>(e[2]).to_int());
}

static ex f_sprem(const exprseq &e)
{
	return sprem(e[0], e[1], e[2]);
}

static ex f_sqrfree2(const exprseq &e)
{
	CHECK_ARG(1, lst, sqrfree);
	return sqrfree(e[0], ex_to<lst>(e[1]));
}

static ex f_subs3(const exprseq &e)
{
	CHECK_ARG(1, lst, subs);
	CHECK_ARG(2, lst, subs);
	return e[0].subs(ex_to<lst>(e[1]), ex_to<lst>(e[2]));
}

static ex f_trace(const exprseq &e)
{
	CHECK_ARG(0, matrix, trace);
	return ex_to<matrix>(e[0]).trace();
}

static ex f_transpose(const exprseq &e)
{
	CHECK_ARG(0, matrix, transpose);
	return ex_to<matrix>(e[0]).transpose();
}

static ex f_unassign(const exprseq &e)
{
	CHECK_ARG(0, symbol, unassign);
	const_cast<symbol&>(ex_to<symbol>(e[0])).unassign();
	return e[0];
}

static ex f_unit(const exprseq &e)
{
	return e[0].unit(e[1]);
}

static ex f_dummy(const exprseq &e)
{
	throw(std::logic_error("dummy function called (shouldn't happen)"));
}

// Tables for initializing the "fcns" map and the function help topics
struct fcn_init {
	const char *name;
	fcnp p;
	int num_params;
};

static const fcn_init builtin_fcns[] = {
	{"charpoly", f_charpoly, 2},
	{"coeff", f_coeff, 3},
	{"collect", f_collect, 2},
	{"collect_common_factors", f_collect_common_factors, 1},
	{"collect_distributed", f_collect_distributed, 2},
	{"content", f_content, 2},
	{"convert_H_to_Li", f_convert_H_to_Li, 2},
	{"decomp_rational", f_decomp_rational, 2},
	{"degree", f_degree, 2},
	{"denom", f_denom, 1},
	{"determinant", f_determinant, 1},
	{"diag", f_diag, 0},
	{"diff", f_diff2, 2},
	{"diff", f_diff3, 3},
	{"divide", f_divide, 2},
	{"eval", f_eval1, 1},
	{"eval", f_eval2, 2},
	{"evalf", f_evalf1, 1},
	{"evalf", f_evalf2, 2},
	{"evalm", f_evalm, 1},
	{"eval_integ", f_eval_integ, 1},
	{"expand", f_expand, 1},
	{"find", f_find, 2},
	{"fsolve", f_fsolve, 4},
	{"gcd", f_gcd, 2},
	{"has", f_has, 2},
	{"integer_content", f_integer_content, 1},
	{"integral", f_integral, 4},
	{"inverse", f_inverse, 1},
	{"iprint", f_dummy, 0},      // for Tab-completion
	{"is", f_is, 1},
	{"lcm", f_lcm, 2},
	{"lcoeff", f_lcoeff, 2},
	{"ldegree", f_ldegree, 2},
	{"lsolve", f_lsolve, 2},
	{"map", f_map, 2},
	{"match", f_match, 2},
	{"nops", f_nops, 1},
	{"normal", f_normal1, 1},
	{"normal", f_normal2, 2},
	{"numer", f_numer, 1},
	{"numer_denom", f_numer_denom, 1},
	{"op", f_op, 2},
	{"pow", f_pow, 2},
	{"prem", f_prem, 3},
	{"primpart", f_primpart, 2},
	{"print", f_dummy, 0},       // for Tab-completion
	{"print_csrc", f_dummy, 0},  // for Tab-completion
	{"print_latex", f_dummy, 0}, // for Tab-completion
	{"quo", f_quo, 3},
	{"rank", f_rank, 1},
	{"rem", f_rem, 3},
	{"resultant", f_resultant, 3},
	{"series", f_series, 3},
	{"sprem", f_sprem, 3},
	{"sqrfree", f_sqrfree1, 1},
	{"sqrfree", f_sqrfree2, 2},
	{"sqrt", f_sqrt, 1},
	{"subs", f_subs2, 2},
	{"subs", f_subs3, 3},
	{"tcoeff", f_tcoeff, 2},
	{"time", f_dummy, 0},        // for Tab-completion
	{"trace", f_trace, 1},
	{"transpose", f_transpose, 1},
	{"unassign", f_unassign, 1},
	{"unit", f_unit, 2},
	{NULL, f_dummy, 0}           // End marker
};

struct fcn_help_init {
	const char *name;
	const char *help;
};

static const fcn_help_init builtin_help[] = {
	{"acos", "inverse cosine function"},
	{"acosh", "inverse hyperbolic cosine function"},
	{"asin", "inverse sine function"},
	{"asinh", "inverse hyperbolic sine function"},
	{"atan", "inverse tangent function"},
	{"atan2", "inverse tangent function with two arguments"},
	{"atanh", "inverse hyperbolic tangent function"},
	{"beta", "Beta function"},
	{"binomial", "binomial function"},
	{"cos", "cosine function"},
	{"cosh", "hyperbolic cosine function"},
	{"exp", "exponential function"},
	{"factorial", "factorial function"},
	{"lgamma", "natural logarithm of Gamma function"},
	{"tgamma", "Gamma function"},
	{"log", "natural logarithm"},
	{"psi", "psi function\npsi(x) is the digamma function, psi(n,x) the nth polygamma function"},
	{"sin", "sine function"},
	{"sinh", "hyperbolic sine function"},
	{"tan", "tangent function"},
	{"tanh", "hyperbolic tangent function"},
	{"zeta", "zeta function\nzeta(x) is Riemann's zeta function, zetaderiv(n,x) its nth derivative.\nIf x is a GiNaC::lst, it is a multiple zeta value\nzeta(x,s) is an alternating Euler sum"},
	{"Li2", "dilogarithm"},
	{"Li3", "trilogarithm"},
	{"Li", "(multiple) polylogarithm"},
	{"S", "Nielsen's generalized polylogarithm"},
	{"H", "harmonic polylogarithm"},
	{"Order", "order term function (for truncated power series)"},
	{"Derivative", "inert differential operator"},
	{NULL, NULL}	// End marker
};

#include "ginsh_extensions.h"


/*
 *  Add functions to ginsh
 */

// Functions from fcn_init array
static void insert_fcns(const fcn_init *p)
{
	while (p->name) {
		fcns.insert(make_pair(string(p->name), fcn_desc(p->p, p->num_params)));
		p++;
	}
}

static ex f_ginac_function(const exprseq &es, int serial)
{
	return function(serial, es).eval(1);
}

// All registered GiNaC functions
namespace GiNaC {
void ginsh_get_ginac_functions(void)
{
	vector<function_options>::const_iterator i = function::registered_functions().begin(), end = function::registered_functions().end();
	unsigned serial = 0;
	while (i != end) {
		fcns.insert(make_pair(i->get_name(), fcn_desc(f_ginac_function, i->get_nparams(), serial)));
		++i;
		serial++;
	}
}
}


/*
 *  Find a function given a name and number of parameters. Throw exceptions on error.
 */

static fcn_tab::const_iterator find_function(const ex &sym, int req_params)
{
	const string &name = ex_to<symbol>(sym).get_name();
	typedef fcn_tab::const_iterator I;
	pair<I, I> b = fcns.equal_range(name);
	if (b.first == b.second)
		throw(std::logic_error("unknown function '" + name + "'"));
	else {
		for (I i=b.first; i!=b.second; i++)
			if ((i->second.num_params == 0) || (i->second.num_params == req_params))
				return i;
	}
	throw(std::logic_error("invalid number of arguments to " + name + "()"));
}


/*
 *  Insert help strings
 */

// Normal help string
static void insert_help(const char *topic, const char *str)
{
	help.insert(make_pair(string(topic), string(str)));
}

// Help string for functions, automatically generates synopsis
static void insert_fcn_help(const char *name, const char *str)
{
	typedef fcn_tab::const_iterator I;
	pair<I, I> b = fcns.equal_range(name);
	if (b.first != b.second) {
		string help_str = string(name) + "(";
		for (int i=0; i<b.first->second.num_params; i++) {
			if (i)
				help_str += ", ";
			help_str += "expression";
		}
		help_str += ") - ";
		help_str += str;
		help.insert(make_pair(string(name), help_str));
	}
}

// Help strings for functions from fcn_help_init array
static void insert_help(const fcn_help_init *p)
{
	while (p->name) {
		insert_fcn_help(p->name, p->help);
		p++;
	}
}


/*
 *  Print help to cout
 */

// Help for a given topic
static void print_help(const string &topic)
{
	typedef help_tab::const_iterator I;
	pair<I, I> b = help.equal_range(topic);
	if (b.first == b.second)
		cout << "no help for '" << topic << "'\n";
	else {
		for (I i=b.first; i!=b.second; i++)
			cout << i->second << endl;
	}
}

// List of help topics
static void print_help_topics(void)
{
	cout << "Available help topics:\n";
	help_tab::const_iterator i;
	string last_name = string("*");
	int num = 0;
	for (i=help.begin(); i!=help.end(); i++) {
		// Don't print duplicates
		if (i->first != last_name) {
			if (num)
				cout << ", ";
			num++;
			cout << i->first;
			last_name = i->first;
		}
	}
	cout << "\nTo get help for a certain topic, type ?topic\n";
}


/*
 *  Function name completion functions for readline
 */

static char *fcn_generator(const char *text, int state)
{
	static int len;				// Length of word to complete
	static fcn_tab::const_iterator index;	// Iterator to function being currently considered

	// If this is a new word to complete, initialize now
	if (state == 0) {
		index = fcns.begin();
		len = strlen(text);
	}

	// Return the next function which partially matches
	while (index != fcns.end()) {
		const char *fcn_name = index->first.c_str();
		++index;
		if (strncmp(fcn_name, text, len) == 0)
			return strdup(fcn_name);
	}
	return NULL;
}

#ifdef REALLY_HAVE_LIBREADLINE
static char **fcn_completion(const char *text, int start, int end)
{
	if (rl_line_buffer[0] == '!') {
		// For shell commands, revert back to filename completion
		rl_completion_append_character = orig_completion_append_character;
		rl_basic_word_break_characters = orig_basic_word_break_characters;
		rl_completer_word_break_characters = GINAC_RL_COMPLETER_CAST(rl_basic_word_break_characters);
#if (GINAC_RL_VERSION_MAJOR < 4) || (GINAC_RL_VERSION_MAJOR == 4 && GINAC_RL_VERSION_MINOR < 2)
		return completion_matches(const_cast<char *>(text), (CPFunction *)filename_completion_function);
#else
		return rl_completion_matches(text, rl_filename_completion_function);
#endif
	} else {
		// Otherwise, complete function names
		rl_completion_append_character = '(';
		rl_basic_word_break_characters = " \t\n\"#$%&'()*+,-./:;<=>?@[\\]^`{|}~";
		rl_completer_word_break_characters = GINAC_RL_COMPLETER_CAST(rl_basic_word_break_characters);
#if (GINAC_RL_VERSION_MAJOR < 4) || (GINAC_RL_VERSION_MAJOR == 4 && GINAC_RL_VERSION_MINOR < 2)
		return completion_matches(const_cast<char *>(text), (CPFunction *)fcn_generator);
#else
		return rl_completion_matches(text, fcn_generator);
#endif
	}
}
#endif // REALLY_HAVE_LIBREADLINE

void greeting(void)
{
    cout << "ginsh - GiNaC Interactive Shell (" << PACKAGE << " V" << VERSION << ")" << endl;
    cout << "  __,  _______  Copyright (C) 1999-2008 Johannes Gutenberg University Mainz,\n"
         << " (__) *       | Germany.  This is free software with ABSOLUTELY NO WARRANTY.\n"
         << "  ._) i N a C | You are welcome to redistribute it under certain conditions.\n"
         << "<-------------' For details type `warranty;'.\n" << endl;
    cout << "Type ?? for a list of help topics." << endl;
}

/*
 *  Main program
 */

int main(int argc, char **argv)
{
	// Print banner in interactive mode
	if (isatty(0)) 
		greeting();

	// Init function table
	insert_fcns(builtin_fcns);
	insert_fcns(extended_fcns);
	ginsh_get_ginac_functions();

	// Init help for operators (automatically generated from man page)
	insert_help("operators", "Operators in falling order of precedence:");
#include "ginsh_op_help.h"

	// Init help for built-in functions (automatically generated from man page)
#include "ginsh_fcn_help.h"

	// Help for GiNaC functions is added manually
	insert_help(builtin_help);
	insert_help(extended_help);

	// Help for other keywords
	insert_help("print", "print(expression) - dumps the internal structure of the given expression (for debugging)");
	insert_help("iprint", "iprint(expression) - prints the given integer expression in decimal, octal, and hexadecimal bases");
	insert_help("print_latex", "print_latex(expression) - prints a LaTeX representation of the given expression");
	insert_help("print_csrc", "print_csrc(expression) - prints a C source code representation of the given expression");

#ifdef REALLY_HAVE_LIBREADLINE
	// Init readline completer
	rl_readline_name = argv[0];
#if (GINAC_RL_VERSION_MAJOR < 4) || (GINAC_RL_VERSION_MAJOR == 4 && GINAC_RL_VERSION_MINOR < 2)
	rl_attempted_completion_function = (CPPFunction *)fcn_completion;
#else
	rl_attempted_completion_function = fcn_completion;
#endif
	orig_completion_append_character = rl_completion_append_character;
	orig_basic_word_break_characters = rl_basic_word_break_characters;
#endif

	// Init input file list, open first file
	num_files = argc - 1;
	file_list = argv + 1;
	if (num_files) {
		yyin = fopen(*file_list, "r");
		if (yyin == NULL) {
			cerr << "Can't open " << *file_list << endl;
			exit(1);
		}
		num_files--;
		file_list++;
	}

	// Parse input, catch all remaining exceptions
	int result;
again:	try {
		result = yyparse();
	} catch (exception &e) {
		cerr << e.what() << endl;
		goto again;
	}
	return result;
}

