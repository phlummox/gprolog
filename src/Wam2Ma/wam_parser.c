/*-------------------------------------------------------------------------*
 * GNU Prolog                                                              *
 *                                                                         *
 * Part  : WAM to mini-assembler translator                                *
 * File  : wam_parser.c                                                    *
 * Descr.: parser                                                          *
 * Author: Daniel Diaz                                                     *
 *                                                                         *
 * Copyright (C) 1999-2001 Daniel Diaz                                     *
 *                                                                         *
 * GNU Prolog is free software; you can redistribute it and/or modify it   *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2, or any later version.       *
 *                                                                         *
 * GNU Prolog is distributed in the hope that it will be useful, but       *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU        *
 * General Public License for more details.                                *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc.  *
 * 59 Temple Place - Suite 330, Boston, MA 02111, USA.                     *
 *-------------------------------------------------------------------------*/

/* $Id$ */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <locale.h>


#include "wam_parser.h"
#include "wam_protos.h"




/*---------------------------------*
 * Constants                       *
 *---------------------------------*/

#define MAX_FCT_ARITY              10
#define MAX_LINE_LEN               32767
#define MAX_STR_LEN                4096
#define MAX_ARGS                   4096




/*---------------------------------*
 * Type Definitions                *
 *---------------------------------*/

typedef enum
{
  ATOM = 256,			/* order of ATOM INTEGER F_N important */
  INTEGER,
  F_N,

  FLOAT,
  X_Y,
  LABEL,

  LST_INTEGER,			/* order important */
  LST_F_N,

  LST_ATOM_INTEGER,		/* order important */
  LST_INTEGER_INTEGER,
  LST_F_N_INTEGER,
}
ArgTyp;




typedef struct
{
  char *keyword;
  void (*fct) ();
  int nb_args;
  ArgTyp arg_type[MAX_FCT_ARITY];
}
InstInf;




/*---------------------------------*
 * Global Variables                *
 *---------------------------------*/

InstInf inst[] = {
  {"get_variable", F_get_variable, 2, {X_Y, INTEGER}},
  {"get_value", F_get_value, 2, {X_Y, INTEGER}},
  {"get_atom", F_get_atom, 2, {ATOM, INTEGER}},
  {"get_integer", F_get_integer, 2, {INTEGER, INTEGER}},
  {"get_float", F_get_float, 2, {FLOAT, INTEGER}},
  {"get_nil", F_get_nil, 1, {INTEGER}},
  {"get_list", F_get_list, 1, {INTEGER}},
  {"get_structure", F_get_structure, 2, {F_N, INTEGER,}},

  {"put_variable", F_put_variable, 2, {X_Y, INTEGER}},
  {"put_void", F_put_void, 1, {INTEGER}},
  {"put_value", F_put_value, 2, {X_Y, INTEGER}},
  {"put_unsafe_value", F_put_unsafe_value, 2, {X_Y, INTEGER}},
  {"put_atom", F_put_atom, 2, {ATOM, INTEGER}},
  {"put_integer", F_put_integer, 2, {INTEGER, INTEGER}},
  {"put_float", F_put_float, 2, {FLOAT, INTEGER}},
  {"put_nil", F_put_nil, 1, {INTEGER}},
  {"put_list", F_put_list, 1, {INTEGER}},
  {"put_structure", F_put_structure, 2, {F_N, INTEGER,}},
  {"math_load_value", F_math_load_value, 2, {X_Y, INTEGER}},
  {"math_fast_load_value", F_math_fast_load_value, 2, {X_Y, INTEGER}},

  {"unify_variable", F_unify_variable, 1, {X_Y}},
  {"unify_void", F_unify_void, 1, {INTEGER}},
  {"unify_value", F_unify_value, 1, {X_Y}},
  {"unify_local_value", F_unify_local_value, 1, {X_Y}},
  {"unify_atom", F_unify_atom, 1, {ATOM}},
  {"unify_integer", F_unify_integer, 1, {INTEGER}},
  {"unify_nil", F_unify_nil, 0, {0}},
  {"unify_list", F_unify_list, 0, {0}},
  {"unify_structure", F_unify_structure, 1, {F_N}},

  {"allocate", F_allocate, 1, {INTEGER}},
  {"deallocate", F_deallocate, 0, {0}},

  {"call", F_call, 1, {F_N,}},
  {"execute", F_execute, 1, {F_N,}},
  {"proceed", F_proceed, 0, {0}},
  {"fail", F_fail, 0, {0}},

  {"label", F_label, 1, {INTEGER}},

  {"switch_on_term", F_switch_on_term, 5, {LABEL, LABEL, LABEL, LABEL, LABEL}},
  {"switch_on_atom", F_switch_on_atom, 1, {LST_ATOM_INTEGER}},
  {"switch_on_integer", F_switch_on_integer, 1, {LST_INTEGER_INTEGER}},
  {"switch_on_structure", F_switch_on_structure, 1, {LST_F_N_INTEGER,}},

  {"try_me_else", F_try_me_else, 1, {INTEGER}},
  {"retry_me_else", F_retry_me_else, 1, {INTEGER}},
  {"trust_me_else_fail", F_trust_me_else_fail, 0, {0}},

  {"try", F_try, 1, {INTEGER}},
  {"retry", F_retry, 1, {INTEGER}},
  {"trust", F_trust, 1, {INTEGER}},

  {"load_cut_level", F_load_cut_level, 1, {INTEGER}},
  {"cut", F_cut, 1, {X_Y}},

  {"function", F_function, 3, {ATOM, INTEGER, LST_INTEGER}},

  {"call_c", F_call_c, 2, {ATOM, LST_INTEGER}},
  {"call_c_test", F_call_c_test, 2, {ATOM, LST_INTEGER}},
  {"call_c_jump", F_call_c_jump, 2, {ATOM, LST_INTEGER}},


  {"foreign_call_c", F_foreign_call_c, 7,
   {ATOM, ATOM, ATOM, INTEGER, INTEGER, LST_ATOM_INTEGER, LST_INTEGER}},

  {NULL, NULL, 0, {0}}
};


ArgVal arg[MAX_ARGS];


jmp_buf jumper;




	  /* scanner variables */

int keep_source_lines;

FILE *file_in;

int cur_line_no;
char cur_line_str[MAX_LINE_LEN];
char *cur_line_p;
char *beg_last_token;

char str_val[MAX_STR_LEN];
long int_val;
double dbl_val;




/*---------------------------------*
 * Function Prototypes             *
 *---------------------------------*/

static void Parser(void);

static void Read_Instruction(void);

static void Read_Argument(int arg_type, ArgVal **top);

static void Read_Token(int what);

static int Scanner(int complex_atom);




/*-------------------------------------------------------------------------*
 * PARSE_MAM_FILE                                                          *
 *                                                                         *
 *-------------------------------------------------------------------------*/
int
Parse_Wam_File(char *file_name_in, int comment)
{
  int ret_val;

  keep_source_lines = comment;

  if (file_name_in == NULL)
    file_in = stdin;
  else if ((file_in = fopen(file_name_in, "rt")) == NULL)
    {
      fprintf(stderr, "cannot open input file %s\n", file_name_in);
      return 0;
    }

  cur_line_p = cur_line_str;
  cur_line_str[0] = '\0';
  cur_line_no = 0;

  if ((ret_val = setjmp(jumper)) == 0)
    Parser();

  if (file_in != stdin)
    fclose(file_in);

  return ret_val == 0;
}




/*-------------------------------------------------------------------------*
 * PARSER                                                                  *
 *                                                                         *
 *-------------------------------------------------------------------------*/
static void
Parser(void)
{
  int k;
  char *name;
  int arity;
  int inside_directive;
  int dynamic, public, built_in, built_in_fd;
  int system;
  ArgVal *top;

  while ((k = Scanner(0)) != 0)	/* end of file */
    {
      if (k != ATOM)
	Syntax_Error
	  ("file_name, predicate, directive or ensure_linked expected");

      if (strcmp(str_val, "file_name") == 0)
	{
	  Read_Token('(');
	  Read_Token(ATOM);
	  Read_Token(')');
	  Read_Token('.');
	  Prolog_File_Name(strdup(str_val));
	  continue;
	}

      if (strcmp(str_val, "ensure_linked") == 0)
	{
	  top = arg;
	  Read_Token('(');
	  Read_Argument(LST_F_N, &top);
	  Read_Token(')');
	  Read_Token('.');
	  Ensure_Linked(arg);
	  continue;
	}

      inside_directive = system = dynamic = public = built_in =
	built_in_fd = 0;

      if (strcmp(str_val, "directive") == 0)
	inside_directive = 1;
      else if (strcmp(str_val, "predicate") != 0)
	Syntax_Error("file_name, predicate or directive expected");

      Read_Token('(');
      if (!inside_directive)
	{
	  Read_Token(ATOM);
	  Read_Token('/'), Read_Token(INTEGER);
	  name = strdup(str_val);
	  arity = int_val;
	  Read_Token(',');

	  Read_Token(INTEGER);	/* pl_line */
	  Read_Token(',');

	  Read_Token(ATOM);
	  if (strcmp(str_val, "dynamic") == 0)
	    dynamic = 1;
	  else if (strcmp(str_val, "static") != 0)
	    Syntax_Error("static or dynamic expected");

	  Read_Token(',');

	  Read_Token(ATOM);
	  if (strcmp(str_val, "public") == 0)
	    public = 1;
	  else if (strcmp(str_val, "private") != 0)
	    Syntax_Error("public or private expected");

	  Read_Token(',');

	  Read_Token(ATOM);
	  if (strcmp(str_val, "built_in") == 0)
	    built_in = 1;
	  else if (strcmp(str_val, "built_in_fd") == 0)
	    built_in_fd = 1;
	  else if (strcmp(str_val, "user") != 0)
	    Syntax_Error("user, built_in or built_in_fd expected");

	  Read_Token(',');

	  New_Predicate(name, arity, int_val, dynamic, public,
			built_in, built_in_fd);
	}
      else
	{
	  Read_Token(INTEGER);	/* pl_line */
	  Read_Token(',');

	  Read_Token(ATOM);
	  if (strcmp(str_val, "system") == 0)
	    system = 1;
	  else if (strcmp(str_val, "user") != 0)
	    Syntax_Error("user or system expected");

	  Read_Token(',');
	  New_Directive(int_val, system);
	}

      Read_Token('[');
      for (;;)
	{
	  Read_Instruction();
	  k = Scanner(0);
	  if (k == ']')
	    break;
	  if (k != ',')
	    Syntax_Error("] or , expected");
	}
      Read_Token(')');
      Read_Token('.');
    }
}




/*-------------------------------------------------------------------------*
 * READ_INSTRUCTION                                                        *
 *                                                                         *
 *-------------------------------------------------------------------------*/
static void
Read_Instruction(void)
{
  InstInf *in;
  ArgVal *top = arg;
  int i;

  if (Scanner(0) != ATOM)
    Syntax_Error("wam instruction expected");

  for (in = inst; in->keyword; in++)
    if (strcmp(str_val, in->keyword) == 0)
      break;

  if (in->keyword == NULL)
    Syntax_Error("unknown wam instruction");

  if (in->nb_args)
    {
      Read_Token('(');
      for (i = 0; i < in->nb_args; i++)
	{
	  if (i > 0)
	    Read_Token(',');

	  Read_Argument(in->arg_type[i], &top);
	}
      Read_Token(')');
    }

  (*in->fct) (arg);
}




/*-------------------------------------------------------------------------*
 * READ_ARGUMENT                                                           *
 *                                                                         *
 * arguments are loaded in the array 'arg' as follows:                     *
 *                                                                         *
 * ATOM         : the (char *) pointing to a copy of the associated string *
 * INTEGER      : the associated (int)                                     *
 * FLOAT        : the associated (double)                                  *
 * X_Y          : the (int) associated to the var no (Y vars from 5000)    *
 * F_N          : the loading of ATOM F and the loading of INTEGER N       *
 * LABEL        : the associated (int) or -1 for 'fail'                    *
 * LST_INTEGER  : an (int) n associated to the number of element and,      *
 *                for each integer the loading of INTEGER                  *
 * LST_F_N      : an (int) n associated to the number of element and,      *
 *                for each f/n the loading of F_N                          *
 * LST_A_INTEGER: (A=ATOM, INTEGER or F_N)                                 *
 *                an (int) n associated to the number of cases and,        *
 *                for each pair (A,INTEGER),                               *
 *                the loading of A and the loading of INTEGER              *
 *-------------------------------------------------------------------------*/
static void
Read_Argument(int arg_type, ArgVal **top)
{
  int k, n;
  ArgVal *top1;

  switch (arg_type)
    {
    case ATOM:
      Read_Token(ATOM);
      Add_Arg(*top, char *, strdup(str_val));

      break;

    case INTEGER:
      Read_Token(INTEGER);
      Add_Arg(*top, long, int_val);

      break;

    case FLOAT:
      Read_Token(FLOAT);
      Add_Arg(*top, double, dbl_val);

      break;

    case X_Y:
      if (Scanner(0) != ATOM ||
	  (*str_val != 'x' && *str_val != 'y') || str_val[1] != '\0')
	Syntax_Error("x(...) or y(...) expected");
      Read_Token('(');
      Read_Token(INTEGER);
      Read_Token(')');
      if (*str_val == 'x')
	Add_Arg(*top, long, int_val);

      else
	Add_Arg(*top, long, 5000 + int_val);

      break;

    case F_N:
      Read_Argument(ATOM, top);
      Read_Token('/');
      Read_Argument(INTEGER, top);
      break;

    case LABEL:
      k = Scanner(0);
      if (k != INTEGER)
	{
	  if (k != ATOM || strcmp(str_val, "fail") != 0)
	    Syntax_Error("label or fail expected");
	  else
	    int_val = -1;
	}
      Add_Arg(*top, long, int_val);

      break;

    case LST_INTEGER:
    case LST_F_N:
      arg_type = arg_type - LST_INTEGER + INTEGER;
      top1 = *top;
      Add_Arg(*top, long, 0);	/* reserve space for case counter */

      n = 0;
      k = Scanner(1);
      if (k == ATOM && strcmp(str_val, "[]") == 0)	/* empty list */
	break;
      if (k != '[')
	Syntax_Error("[] or [ expected");

      for (;;)
	{
	  n++;
	  Read_Argument(arg_type, top);
	  k = Scanner(0);
	  if (k == ']')
	    break;
	  if (k != ',')
	    Syntax_Error("] or , expected");
	}
      Add_Arg(top1, long, n);

      break;

    default:			/*  LST_x_INTEGER with x in (ATOM,INTEGER,F_N) */
      arg_type = arg_type - LST_ATOM_INTEGER + ATOM;
      top1 = *top;
      Add_Arg(*top, long, 0);	/* reserve space for case counter */

      n = 0;
      k = Scanner(1);
      if (k == ATOM && strcmp(str_val, "[]") == 0)	/* empty list */
	break;
      if (k != '[')
	Syntax_Error("[] or [ expected");

      for (;;)
	{
	  n++;
	  Read_Token('(');
	  Read_Argument(arg_type, top);
	  Read_Token(',');
	  Read_Argument(INTEGER, top);
	  Read_Token(')');
	  k = Scanner(0);
	  if (k == ']')
	    break;
	  if (k != ',')
	    Syntax_Error("] or , expected");
	}
      Add_Arg(top1, long, n);

      break;
    }
}




/*-------------------------------------------------------------------------*
 * READ_TOKEN                                                              *
 *                                                                         *
 *-------------------------------------------------------------------------*/
static void
Read_Token(int what)
{
  char str[80];
  int k;

  k = Scanner(what == ATOM);

  if (k == what)
    return;

  if (what >= 256 && k == '(')
    {
      Read_Token(what);		/* maybe ( what ) (useful for operators) */
      Read_Token(')');
      return;
    }

  switch (what)
    {
    case ATOM:
      Syntax_Error("atom expected");
      break;

    case INTEGER:
      Syntax_Error("integer expected");
      break;

    case FLOAT:
      Syntax_Error("float expected");
      break;

    default:
      sprintf(str, "%c expected", what);
      Syntax_Error(str);
      break;
    }
}




/*-------------------------------------------------------------------------*
 * SCANNER                                                                 *
 *                                                                         *
 *-------------------------------------------------------------------------*/
static int
Scanner(int complex_atom)
{
  char *p, *p1;
  long i;
  double d;
  double strtod();


  for (;;)
    {
      while (isspace(*cur_line_p))
	cur_line_p++;

      if (*cur_line_p != '\0' && *cur_line_p != '%')
	break;

      fgets(cur_line_str, sizeof(cur_line_str), file_in);
      if (feof(file_in))
	return 0;

      cur_line_no++;
      cur_line_p = cur_line_str;

      if (keep_source_lines)
	{
	  while (isspace(*cur_line_p))
	    cur_line_p++;

	  if (*cur_line_p)
	    {
	      p = cur_line_p + strlen(cur_line_p) - 1;
	      if (*p == '\n')
		*p = '\0';
	      Source_Line(cur_line_no, cur_line_p);
	    }
	}
    }

  beg_last_token = cur_line_p;

  if (*cur_line_p == '\'')	/* quoted atom */
    {
      p = str_val;
      cur_line_p++;
      while (*cur_line_p != '\'' || cur_line_p[1] == '\'')
	{
	  if (*cur_line_p == '\'')
	    {
	      *p++ = '\'';
	      cur_line_p += 2;
	      continue;
	    }

	  if (*cur_line_p == '\"')
	    {
	      *p++ = '\\';
	      *p++ = '"';
	      cur_line_p++;
	      continue;
	    }

	  if ((*p++ = *cur_line_p++) == '\\')
	    {
	      if (*cur_line_p == '\\' ||	/* \\ */
		  strchr("abfnrtv", *cur_line_p))	/* \a \b \f \n \r \t \v */
		*p++ = *cur_line_p++;
	      else
		{
		  if (*cur_line_p == 'x')
		    {
		      cur_line_p++;
		      i = 16;
		    }
		  else
		    i = 8;
		  i = strtol(cur_line_p, &p1, i);	/* stop on the closing \ */
		  cur_line_p = p1 + 1;
		  sprintf(p, "%03lo", i);
		  p += 3;
		}
	    }
	}

      cur_line_p++;
      *p = '\0';
      return ATOM;
    }

  if (isalpha(*cur_line_p) || *cur_line_p == '_')	/* atom */
    {
      p = str_val;
      while (isalnum(*cur_line_p) || *cur_line_p == '_')
	*p++ = *cur_line_p++;

      *p = '\0';
      return ATOM;
    }

  if (complex_atom)
    {
      if ((cur_line_p[0] == '[' && cur_line_p[1] == ']') ||	/* [] and {} */
	  (cur_line_p[0] == '{' && cur_line_p[1] == '}'))
	{
	  str_val[0] = *cur_line_p++;
	  str_val[1] = *cur_line_p++;
	  str_val[2] = '\0';
	  return ATOM;
	}

      if (strchr("#$&*+-./:<=>?@\\^~", *cur_line_p))	/* symbol char */
	{
	  p = str_val;
	  do
	    {
	      if (*cur_line_p == '"' || *cur_line_p == '\\')
		*p++ = '\\';
	      *p++ = *cur_line_p++;
	    }
	  while (strchr("#$&*+-./:<=>?@\\^~", *cur_line_p));
	  *p = '\0';
	  return ATOM;
	}

      if (strchr("!;,", *cur_line_p))	/* solo char */
	{
	  str_val[0] = *cur_line_p++;
	  str_val[1] = '\0';
	  return ATOM;
	}
    }


  i = strtol(cur_line_p, &p, 0);
  if (p == cur_line_p)		/* not an integer return that character */
    return *cur_line_p++;
  d = strtod(cur_line_p, &p1);

  if (p1 == p)			/* integer */
    {
      int_val = i;
      cur_line_p = p;
      return INTEGER;
    }
  /* float */
  dbl_val = d;
  cur_line_p = p1;
  return FLOAT;
}




/*-------------------------------------------------------------------------*
 * SYNTAX_ERROR                                                            *
 *                                                                         *
 *-------------------------------------------------------------------------*/
void
Syntax_Error(char *s)
{
  char *p = cur_line_str + strlen(cur_line_str) - 1;

  if (*p == '\n')
    *p = '\0';

  fprintf(stderr, "line %d: %s\n", cur_line_no, s);
  fprintf(stderr, "%s\n", cur_line_str);

  for (p = cur_line_str; p < beg_last_token; p++)
    if (!isspace(*p))
      *p = ' ';

  *p = '\0';
  fprintf(stderr, "%s^ here\n", cur_line_str);

  longjmp(jumper, 1);
}
