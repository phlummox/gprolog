/*-------------------------------------------------------------------------*
 * GNU Prolog                                                              *
 *                                                                         *
 * Part  : Prolog Compiler                                                 *
 * File  : top_comp.c                                                      *
 * Descr.: compiler main (shell) program                                   *
 * Author: Daniel Diaz                                                     *
 *                                                                         *
 * Copyright (C) 1999,2000 Daniel Diaz                                     *
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

#include "../EnginePl/gp_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>

#ifdef M_ix86_win32
#include <windows.h>
#include <process.h>
#include <fcntl.h>
#include <io.h>
#define StrStr(s1,s2) Str_Case_Str(s1,s2)
char *Str_Case_Str(char *s1, char *s2);
#else
#include <dirent.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/wait.h>
#define StrStr(s1,s2) strstr(s1,s2)
#endif

#ifdef M_ix86_cygwin
#include <process.h>
#endif

#include "../EnginePl/arch_dep.h"
#include "../EnginePl/wam_regs.h"

#include "decode_hexa.c"
#include "copying.c"

#include "../EnginePl/machine1.c"

#if 0
#define DEBUG
#endif




/*---------------------------------*
 * Constants                       *
 *---------------------------------*/

#define MAX_SUB_DIRS               128

#define MAX_FILES                  1024

#define CMD_LINE_MAX_OPT           4096
#define CMD_LINE_LENGTH            (MAXPATHLEN+CMD_LINE_MAX_OPT+1)

#define TEMP_FILE_PREFIX           GPLC

#define OBJ_FILE_OBJ_BEGIN         "obj_begin"
#define OBJ_FILE_OBJ_END           "obj_end"
#define OBJ_FILE_ALL_PL_BIPS       "all_pl_bips"
#define OBJ_FILE_ALL_FD_BIPS       "all_fd_bips"
#define OBJ_FILE_TOP_LEVEL         "top_level"
#define OBJ_FILE_DEBUGGER          "debugger"

#define EXE_FILE_PL2WAM            "pl2wam"
#define EXE_FILE_WAM2MA            "wam2ma"
#define EXE_FILE_MA2ASM            "ma2asm"
#define EXE_FILE_ASM               AS
#define EXE_FILE_FD2C              "fd2c"
#define EXE_FILE_CC                CC
#define EXE_FILE_LINK              CC
#define EXE_FILE_STRIP             STRIP




#define FILE_PL                    0
#define FILE_WAM                   1
#define FILE_MA                    2
#define FILE_ASM                   3
#define FILE_OBJ                   4

#define FILE_FD                    5
#define FILE_C                     6
#define FILE_LINK                  7




#define PL_SUFFIX                  ".pl"
#define PL_SUFFIX_ALTERNATE        ".pro"
#define WAM_SUFFIX                 ".wam"
#define WBC_SUFFIX                 ".wbc"
#define MA_SUFFIX                  ".ma"
#define FD_SUFFIX                  ".fd"
#define C_SUFFIX                   ".c"
#define C_SUFFIX_ALTERNATE         "|.C|.CC|.cc|.cxx|.c++|.cpp|"

#define CC_COMPILE_OPT             "-c "
#define CC_INCLUDE_OPT             "-I"



/*---------------------------------*
 * Type Definitions                *
 *---------------------------------*/

typedef struct
{
  char *name;
  char *suffix;
  int type;
  char *work_name1;
  char *work_name2;
}
FileInf;


typedef struct
{
  char *exe_name;
  char opt[CMD_LINE_MAX_OPT];
  char *out_opt;
}
CmdInf;




/*---------------------------------*
 * Global Variables                *
 *---------------------------------*/

char *start_path;

int devel_mode = 0;
char *devel_dir[MAX_SUB_DIRS];

FileInf file[MAX_FILES];
int nb_file = 0;

int stop_after = FILE_LINK;
int verbose = 0;
char *file_name_out = NULL;

int def_local_size = -1;
int def_global_size = -1;
int def_trail_size = -1;
int def_cstr_size = -1;
int fixed_sizes = 0;
int needs_stack_file = 0;

int bc_mode = 0;
int gui_console = 0;
int no_top_level = 0;
int min_pl_bips = 0;
int min_fd_bips = 0;
int no_debugger = 0;
int no_fd_lib = 0;
int strip = 0;

int no_decode_hex = 0;

char warn[1024] = "";

char *temp_dir = NULL;
int no_del_temp_files = 0;

CmdInf cmd_pl2wam = { EXE_FILE_PL2WAM, " ", "-o " };
CmdInf cmd_wam2ma = { EXE_FILE_WAM2MA, " ", "-o " };
CmdInf cmd_ma2asm = { EXE_FILE_MA2ASM, " ", "-o " };
CmdInf cmd_asm = { EXE_FILE_ASM, " ", "-o " };
CmdInf cmd_fd2c = { EXE_FILE_FD2C, " ", "-o " };
CmdInf cmd_cc = { EXE_FILE_CC, " ", CC_OBJ_NAME_OPT };
CmdInf cmd_link = { EXE_FILE_LINK, " ", CC_EXE_NAME_OPT };

char *cc_fd2c_flags = CFLAGS " ";




char *suffixes[] =
  { PL_SUFFIX, WAM_SUFFIX, MA_SUFFIX, ASM_SUFFIX, OBJ_SUFFIX,
  FD_SUFFIX, C_SUFFIX, NULL
};




/*---------------------------------*
 * Function Prototypes             *
 *---------------------------------*/

char *Search_Path(char *file);

void Init_Develop_Dir(void);

void Init_Develop_Dir_Rec(char *path, char ***p);

void Determine_Pathnames(void);

void Compile_Files(void);

void New_Work_File(FileInf *f, int stage, int stop_after);

void Free_Work_File2(FileInf *f);

void Compile_Cmd(CmdInf *c, FileInf *f);

void Link_Cmd(void);

void Exec_One_Cmd(char *str, int no_decode_hex);

int Spawn_Decode_Hex(char *arg[]);

void Delete_Temp_File(char *name);

void Find_File(char *file, char *suff, char *file_path);

void Fatal_Error(char *format, ...);

void Parse_Arguments(int argc, char *argv[]);

void Display_Help(void);



#define Record_Link_Warn_Option(i) sprintf(warn+strlen(warn),"%s ",argv[i])



#define Before_Cmd(cmd)                                                     \
     if (verbose)                                                           \
         fprintf(stderr,"%s\n",cmd)

#define After_Cmd(error)                                                    \
     if (error)                                                             \
         Fatal_Error("compilation failed")



char *last_opt;

#define Check_Arg(i,str)           (last_opt=str,strncmp(argv[i],str,strlen(argv[i]))==0)

#define Add_Last_Option(opt)       sprintf(opt+strlen(opt),"%s ",last_opt)

#define Add_Option(i,opt)          sprintf(opt+strlen(opt),"%s ",argv[i])




/*-------------------------------------------------------------------------*
 * MAIN                                                                    *
 *                                                                         *
 *-------------------------------------------------------------------------*/
int
main(int argc, char *argv[])
{
  static char resolved[MAXPATHLEN + 1];	/* realpath() fills up to MAXPATHLEN (NULL) */
  static char buff[MAXPATHLEN];
  char *p;

#ifdef M_ix86_win32
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);
#endif

  Parse_Arguments(argc, argv);
  if (verbose)
    fprintf(stderr, "\n");

  if ((start_path = getenv(ENV_VARIABLE)) != NULL)
    goto path_found;

  strcpy(buff, argv[0]);

#ifdef DEBUG
  fprintf(stderr, "value of argv[0]: <%s>\n", buff);
#endif

  if (strcasecmp(buff + strlen(buff) - strlen(EXE_SUFFIX), EXE_SUFFIX) != 0)
    strcat(buff, EXE_SUFFIX);

#ifdef DEBUG
  fprintf(stderr, "value of argv[0] with suffix: <%s>\n", buff);
#endif

#ifndef M_ix86_win32
  if (access(buff, X_OK) == 0)
    start_path = buff;
  else
#endif
  if ((start_path = Search_Path(buff)) == NULL)
    goto path_not_found;

#if defined(__unix__) || defined(__CYGWIN__)
  if (realpath(start_path, resolved + 1) == NULL)	/* +1 for eventual @ */
    goto path_not_found;
#else
  strcpy(resolved + 1, start_path);	/* +1 for eventual @ */
#endif

#ifdef DEBUG
  fprintf(stderr, "path of the executable: %s\n", start_path);
#endif

  sprintf(buff, DIR_SEP_S "bin" DIR_SEP_S "%s", GPLC);

  if ((p = StrStr(resolved + 1, buff)) != NULL)
    {
      *p = '\0';
      start_path = resolved + 1;
      goto path_found;
    }

  sprintf(buff, DIR_SEP_S "TopComp" DIR_SEP_S "%s", GPLC);

#ifdef DEBUG
  fprintf(stderr, "path of the executable: %s\n", start_path);
  fprintf(stderr, "sub-path to search:     %s\n", buff);
#endif
  if ((p = StrStr(resolved + 1, buff)) != NULL)
    {
      *p = '\0';
      start_path = resolved;
      *start_path = '@';	/* to enforce development mode */
      goto path_found;
    }

path_not_found:
  Fatal_Error
    ("cannot find the path for %s, set the environment variable %s",
     PROLOG_NAME, ENV_VARIABLE);

path_found:
  if (*start_path == '@')	/* development mode */
    {
      start_path++;
      devel_mode = 1;
    }

  strcat(cmd_cc.opt, CFLAGS_MACHINE " " CFLAGS_REGS CC_COMPILE_OPT);

  if (devel_mode)
    Init_Develop_Dir();
  else
    sprintf(cmd_cc.opt + strlen(cmd_cc.opt), "%s%s" DIR_SEP_S "include ",
	    CC_INCLUDE_OPT, start_path);

  strcat(cmd_link.opt, LDFLAGS " ");

  if (verbose)
    fprintf(stderr, "Path used: %s %s\n", start_path,
	    (devel_mode) ? "(development mode)" : "");

  Compile_Files();

  return 0;
}




/*-------------------------------------------------------------------------*
 * SEARCH_PATH                                                             *
 *                                                                         *
 *-------------------------------------------------------------------------*/
char *
Search_Path(char *file)
{
#ifndef M_ix86_win32

  char *path = getenv("PATH");
  char *p;
  int l;
  static char buff[MAXPATHLEN];

  if (path == NULL)
    return NULL;

  p = path;
  for (;;)
    {
      if ((p = strchr(path, ':')) != NULL)
	{
	  l = p - path;
	  strncpy(buff, path, l);
	}
      else
	{
	  strcpy(buff, path);
	  l = strlen(buff);
	}

      buff[l++] = DIR_SEP_C;

      strcpy(buff + l, file);

      if (access(buff, X_OK) == 0)
	return buff;

      if (p == NULL)
	break;

      path = p + 1;
    }

  return NULL;

#else

  static char buff[MAXPATHLEN];
  char *file_part;

  SearchPath(NULL, file, ".exe", MAXPATHLEN, buff, &file_part);

  return buff;

#endif
}




/*-------------------------------------------------------------------------*
 * INIT_DEVELOP_DIR                                                        *
 *                                                                         *
 *-------------------------------------------------------------------------*/
void
Init_Develop_Dir(void)
{
  char **p;

  p = devel_dir;

  Init_Develop_Dir_Rec(start_path, &p);

  *p = NULL;
}




/*-------------------------------------------------------------------------*
 * INIT_DEVELOP_DIR                                                        *
 *                                                                         *
 *-------------------------------------------------------------------------*/
void
Init_Develop_Dir_Rec(char *path, char ***p)
{
#ifdef M_ix86_win32
  HANDLE h;
  WIN32_FIND_DATA d;
#else
  DIR *dir;
  struct dirent *cur_entry;
  struct stat info;
#endif
  char *name;
  char buff[MAXPATHLEN];


#if 0
  fprintf(stderr, "Searching sub-dir in: %s\n", path);
#endif

#ifdef M_ix86_win32
  sprintf(buff, "%s\\*.*", path);
  h = FindFirstFile(buff, &d);
  if (h == INVALID_HANDLE_VALUE)
#else
  dir = opendir(path);
  if (dir == NULL)
#endif
    Fatal_Error("Cannot access to %s", path);


#ifdef M_ix86_win32
  do
    {
      name = d.cFileName;
#else
  while ((cur_entry = readdir(dir)) != NULL)
    {
      name = cur_entry->d_name;
#endif
      sprintf(buff, "%s" DIR_SEP_S "%s", path, name);
      if (*name != '.' &&
#ifdef M_ix86_win32
	  (d.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
#else
	  stat(buff, &info) == 0 && S_ISDIR(info.st_mode))
#endif
      {
#ifdef DEBUG
	fprintf(stderr, "sub-directory for include: %s (full: %s)\n", name,
		buff);
#endif
	**p = strdup(buff);
	(*p)++;
	sprintf(cmd_cc.opt + strlen(cmd_cc.opt), "%s%s ",
		CC_INCLUDE_OPT, buff);
	Init_Develop_Dir_Rec(buff, p);
      }
#ifdef M_ix86_win32
    }
  while (FindNextFile(h, &d) != 0);
  FindClose(h);
#else
    }
  closedir(dir);
#endif
}




/*-------------------------------------------------------------------------*
 * COMPILE_FILES                                                           *
 *                                                                         *
 *-------------------------------------------------------------------------*/
void
Compile_Files(void)
{
  FileInf *f;
  int stage;
  int stage_end;
  int l;
  FILE *fd;


  if (stop_after < FILE_LINK)
    {
      if (*warn)
	fprintf(stderr, "link not done - ignored option(s): %s\n", warn);

      stage_end = stop_after;
      needs_stack_file = 0;

      if (bc_mode)
	{
	  suffixes[FILE_WAM] = WBC_SUFFIX;
	  strcat(cmd_pl2wam.opt, "--wam-for-byte-code ");
	}
    }
  else
    stage_end = FILE_ASM;

  if (needs_stack_file)
    {
      f = file + nb_file;

      f->work_name2 = NULL;
      New_Work_File(f, FILE_WAM, 10000);	/* to create work_name2 */
      f->name = f->work_name2;
      f->suffix = f->name + strlen(f->name) - strlen(suffixes[FILE_MA]);
      f->type = FILE_MA;
      f->work_name1 = f->name, f->work_name2 = NULL;

      if (verbose)
	fprintf(stderr, "creating stack size file: %s\n", f->name);

      if ((fd = fopen(f->name, "wt")) == NULL)
	Fatal_Error("cannot open stack size file (%s)", f->name);

      if (def_local_size >= 0)
	fprintf(fd, "long global def_local_size = %d\n", def_local_size);
      if (def_global_size >= 0)
	fprintf(fd, "long global def_global_size = %d\n", def_global_size);
      if (def_trail_size >= 0)
	fprintf(fd, "long global def_trail_size = %d\n", def_trail_size);
      if (def_cstr_size >= 0)
	fprintf(fd, "long global def_cstr_size = %d\n", def_cstr_size);
      if (fixed_sizes)
	fprintf(fd, "long global fixed_sizes = 1\n");

      fclose(fd);
    }

  if (verbose)
    fprintf(stderr, "\n*** Compiling\n");


  for (f = file; f->name; f++)
    {
      if (verbose &&
	  (f->type == FILE_FD || f->type == FILE_C || f->type <= stage_end))
	fprintf(stderr, "\n--- file: %s\n", f->name);

      if (f->type == FILE_FD && stop_after >= FILE_ASM)
	{
	  stage = FILE_FD;	/* to generate the correct C suffix */
	  New_Work_File(f, stage,
			(stop_after == FILE_FD) ? stop_after : 10000);
	  Compile_Cmd(&cmd_fd2c, f);
	  if (stop_after != FILE_FD)
	    {
	      stage = FILE_ASM;	/* to generate the correct obj suffix */
	      New_Work_File(f, stage, stop_after);
	      l = strlen(cmd_cc.opt);	/* add fd2c C options */
	      strcpy(cmd_cc.opt + l, cc_fd2c_flags);
	      Compile_Cmd(&cmd_cc, f);
	      cmd_cc.opt[l] = '\0';	/* remove them */
	    }
	  goto free_work_file;
	}

      if (f->type == FILE_C && stop_after >= FILE_ASM
	  && stop_after != FILE_FD)
	{
	  stage = FILE_ASM;	/* to generate the correct obj suffix */
	  New_Work_File(f, stage, stop_after);
	  Compile_Cmd(&cmd_cc, f);
	  goto free_work_file;
	}

      if (f->type == FILE_FD || f->type == FILE_C ||
	  stop_after == FILE_FD || f->type > stop_after)
	{
	  fprintf(stderr, "unused input file: %s\n", f->name);
	  continue;
	}

      for (stage = f->type; stage <= stage_end; stage++)
	{
	  New_Work_File(f, stage, stop_after);
	  switch (stage)
	    {
	    case FILE_PL:
	      Compile_Cmd(&cmd_pl2wam, f);
	      break;

	    case FILE_WAM:
	      Compile_Cmd(&cmd_wam2ma, f);
	      break;

	    case FILE_MA:
	      Compile_Cmd(&cmd_ma2asm, f);
	      if (needs_stack_file && f == file + nb_file &&
		  !no_del_temp_files)
		{
		  if (verbose)
		    fprintf(stderr, "deleting stack size file\n");
		  Delete_Temp_File(f->name);
		}

	      break;

	    case FILE_ASM:
	      Compile_Cmd(&cmd_asm, f);
	      break;
	    }
	}

    free_work_file:
      Free_Work_File2(f);	/* to suppress last useless temp file */
    }

  if (stop_after < FILE_LINK)
    return;

  if (verbose)
    fprintf(stderr, "\n*** Linking\n\n");

  Link_Cmd();

  /* removing temp files after link */
  for (f = file; f->name; f++)
    if (f->work_name1 != f->name)
      Delete_Temp_File(f->work_name1);
}




/*-------------------------------------------------------------------------*
 * NEW_WORK_FILE                                                           *
 *                                                                         *
 *-------------------------------------------------------------------------*/
void
New_Work_File(FileInf *f, int stage, int stop_after)
{
  static char buff[MAXPATHLEN];
  char *p;

  if (stage < stop_after)	/* intermediate stage */
    {
      p = tempnam(temp_dir, TEMP_FILE_PREFIX);
      sprintf(buff, "%s%s", p, suffixes[stage + 1]);
      free(p);
    }
  else /* final stage */ if (file_name_out)	/* specified output filename */
    strcpy(buff, file_name_out);
  else
    {
      strcpy(buff, f->name);
      strcpy(buff + (f->suffix - f->name), suffixes[stage + 1]);
    }

  Free_Work_File2(f);
  f->work_name2 = strdup(buff);
}




/*-------------------------------------------------------------------------*
 * FREE_WORK_FILE2                                                         *
 *                                                                         *
 *-------------------------------------------------------------------------*/
void
Free_Work_File2(FileInf *f)
{
  if (f->work_name2 != NULL)
    {
      if (f->work_name1 != f->name)
	Delete_Temp_File(f->work_name1);

      f->work_name1 = f->work_name2;
    }
}




/*-------------------------------------------------------------------------*
 * COMPILE_CMD                                                             *
 *                                                                         *
 *-------------------------------------------------------------------------*/
void
Compile_Cmd(CmdInf *c, FileInf *f)
{
  static char buff[CMD_LINE_LENGTH];

  sprintf(buff, "%s%s%s%s %s", c->exe_name, c->opt, c->out_opt,
	  f->work_name2, f->work_name1);

  Exec_One_Cmd(buff, 1);
}




/*-------------------------------------------------------------------------*
 * LINK_CMD                                                                *
 *                                                                         *
 *-------------------------------------------------------------------------*/
void
Link_Cmd(void)
{
  static char file_out[MAXPATHLEN];
  static char buff[CMD_LINE_LENGTH];
  FileInf *f;


  if (no_fd_lib)
    min_fd_bips = 1;

  if (file_name_out == NULL)
    {
      f = file;
      strcpy(file_out, f->name);
      file_out[f->suffix - f->name] = '\0';
      file_name_out = file_out;
    }

  sprintf(buff, "%s%s%s%s ", cmd_link.exe_name, cmd_link.opt,
	  cmd_link.out_opt, file_name_out);

  Find_File(OBJ_FILE_OBJ_BEGIN, OBJ_SUFFIX, buff + strlen(buff));
  strcat(buff, " ");

  for (f = file; f->name; f++)
    sprintf(buff + strlen(buff), "%s ", f->work_name1);

  if (!min_pl_bips)
    {
      Find_File(OBJ_FILE_ALL_PL_BIPS, OBJ_SUFFIX, buff + strlen(buff));
      strcat(buff, " ");
    }

#ifndef NO_USE_FD_SOLVER
  if (!min_fd_bips)
    {
      Find_File(OBJ_FILE_ALL_FD_BIPS, OBJ_SUFFIX, buff + strlen(buff));
      strcat(buff, " ");
    }
#endif

  if (!no_top_level)
    {
      Find_File(OBJ_FILE_TOP_LEVEL, OBJ_SUFFIX, buff + strlen(buff));
      strcat(buff, " ");
    }

  if (!no_debugger)
    {
      Find_File(OBJ_FILE_DEBUGGER, OBJ_SUFFIX, buff + strlen(buff));
      strcat(buff, " ");
    }

#ifndef NO_USE_FD_SOLVER
  if (!no_fd_lib)
    {
      Find_File(LIB_BIPS_FD, "", buff + strlen(buff));
      strcat(buff, " ");

      Find_File(LIB_ENGINE_FD, "", buff + strlen(buff));
      strcat(buff, " ");
    }
#endif

  Find_File(LIB_BIPS_PL, "", buff + strlen(buff));
  strcat(buff, " ");

  Find_File(OBJ_FILE_OBJ_END, OBJ_SUFFIX, buff + strlen(buff));
  strcat(buff, " ");

  Find_File(LIB_ENGINE_PL, "", buff + strlen(buff));
  strcat(buff, " ");

#ifndef NO_USE_LINEDIT
  Find_File(LIB_LINEDIT, "", buff + strlen(buff));
  strcat(buff, " ");
#endif

  strcat(buff, LDLIBS " ");

  if (gui_console)
    {				/* modify Linedit/Makefile.in to follow this list of linker objects */
      Find_File(LIB_W32GUICONS, "", buff + strlen(buff));
      strcat(buff, " ");
      Find_File("w32gc_interf.obj", "", buff + strlen(buff));
      strcat(buff, " ");
      Find_File("GUICons.res", "", buff + strlen(buff));
      strcat(buff, " /link /subsystem:windows ");
    }
#ifdef M_ix86_win32
  else
    strcat(buff, "/link /subsystem:console ");
#endif

  Exec_One_Cmd(buff, no_decode_hex);

  if (strip && *EXE_FILE_STRIP != ':' && *EXE_FILE_STRIP != '\0')
    {
      sprintf(buff, "%s %s%s", EXE_FILE_STRIP, file_name_out, EXE_SUFFIX);
      Exec_One_Cmd(buff, 1);
    }
}




/*-------------------------------------------------------------------------*
 * EXEC_ONE_CMD                                                            *
 *                                                                         *
 *-------------------------------------------------------------------------*/
void
Exec_One_Cmd(char *cmd, int no_decode_hex)
{
#if 1
  int status;
  static char *arg[2] = { NULL, (char *) 1 };

  arg[0] = cmd;

  Before_Cmd(cmd);

  if (no_decode_hex == 1)
    status = M_Spawn(arg);
  else
    status = Spawn_Decode_Hex(arg);

  if (status == -1)
    {
      fprintf(stderr, "error trying to execute ");
      perror(arg[0]);
    }

  if (status == -2)
    fprintf(stderr, "error trying to execute %s: unknown error", arg[0]);

  After_Cmd(status);

#else

  int status;

  Before_Cmd(cmd);
#ifdef DEBUG
  fprintf(stderr, "executing system() for: %s\n", cmd);
#endif
  status = system(cmd);
  status >>= 8;
  if (status == -1 || status == 127)
    Fatal_Error("error trying to execute %s", cmd);

  After_Cmd(status);
#endif
}




/*-------------------------------------------------------------------------*
 * SPAWN_DECODE_HEX                                                        *
 *                                                                         *
 *-------------------------------------------------------------------------*/
int
Spawn_Decode_Hex(char *arg[])
{
  int pid, status;
  FILE *f_out;
  static char buff[CMD_LINE_LENGTH];

  pid = M_Spawn_Redirect(arg, 0, NULL, &f_out, &f_out);
  if (pid == -1 || pid == -2)
    return pid;

  for (;;)
    {
      fgets(buff, sizeof(buff), f_out);
      if (feof(f_out))
	break;

#ifndef DEBUG
      fputs(Decode_Hexa(buff, "predicate(%s)", 1, 1), stderr);
#else
      fprintf(stderr, "piped line:%s",
	      Decode_Hexa(buff, "predicate(%s)", 1, 1));
#endif
    }

  if (fclose(f_out))
    return -1;

  status = M_Get_Status(pid);

#ifdef DEBUG
  fprintf(stderr, "error status: %d\n", status);
#endif

  return status;
}




/*-------------------------------------------------------------------------*
 * DELETE_TEMP_FILE                                                        *
 *                                                                         *
 *-------------------------------------------------------------------------*/
void
Delete_Temp_File(char *name)
{
  if (no_del_temp_files)
    return;

#if 1
  if (verbose)
    fprintf(stderr, "delete %s\n", name);
#endif

  unlink(name);
}




/*-------------------------------------------------------------------------*
 * FIND_FILE                                                               *
 *                                                                         *
 *-------------------------------------------------------------------------*/
void
Find_File(char *file, char *suff, char *file_path)
{
  char name[MAXPATHLEN];
  char **p;

  sprintf(name, "%s%s", file, suff);
  if (!devel_mode)
    {
      sprintf(file_path, "%s" DIR_SEP_S "lib" DIR_SEP_S "%s", start_path,
	      name);
      if (access(file_path, F_OK) == 0)
	return;
    }
  else
    for (p = devel_dir; *p; p++)
      {
	sprintf(file_path, "%s" DIR_SEP_S "%s", *p, name);
	if (access(file_path, F_OK) == 0)
	  return;
      }

  Fatal_Error("cannot locate file %s", name);
}




#ifdef M_ix86_win32

/*-------------------------------------------------------------------------*
 * STR_CASE_STR                                                            *
 *                                                                         *
 *-------------------------------------------------------------------------*/
char *
Str_Case_Str(char *s1, char *s2)
{
  int l1 = strlen(s1);
  int l2 = strlen(s2);
  char *end = s1 + l1 - l2;

  while (s1 <= end)
    {
      if (strncasecmp(s1, s2, l2) == 0)
	return s1;
      s1++;
    }

  return NULL;
}

#endif




/*-------------------------------------------------------------------------*
 * FATAL_ERROR                                                             *
 *                                                                         *
 *-------------------------------------------------------------------------*/
void
Fatal_Error(char *format, ...)
{
  FileInf *f;
  va_list arg_ptr;

  va_start(arg_ptr, format);
  vfprintf(stderr, format, arg_ptr);
  va_end(arg_ptr);

  fprintf(stderr, "\n");

  if (no_del_temp_files)
    exit(1);

  if (verbose)
    fprintf(stderr, "deleting temporary files before exit\n");

  for (f = file; f->name; f++)
    {
      if (f->work_name1 && f->work_name1 != f->name &&
	  (file_name_out == NULL
	   || strcasecmp(f->work_name1, file_name_out) != 0))
	Delete_Temp_File(f->work_name1);

      if (f->work_name2 && f->work_name2 != f->name &&
	  (file_name_out == NULL
	   || strcasecmp(f->work_name2, file_name_out) != 0))
	Delete_Temp_File(f->work_name2);
    }

  exit(1);
}




/*-------------------------------------------------------------------------*
 * PARSE_ARGUMENTS                                                         *
 *                                                                         *
 *-------------------------------------------------------------------------*/
void
Parse_Arguments(int argc, char *argv[])
{
  int i, file_name_out_i;
  char **p, *q;
  FileInf *f = file;


  for (i = 1; i < argc; i++)
    {
      if (*argv[i] == '-' && argv[i][1] != '\0')
	{
	  if (Check_Arg(i, "-o") || Check_Arg(i, "--output"))
	    {
	      file_name_out_i = i;
	      if (++i >= argc)
		Fatal_Error("FILE missing after %s option", last_opt);

	      file_name_out = argv[i];
	      continue;
	    }

	  if (Check_Arg(i, "-W") || Check_Arg(i, "--wam-for-native"))
	    {
	      stop_after = FILE_PL;
	      bc_mode = 0;
	      continue;
	    }

	  if (Check_Arg(i, "-w") || Check_Arg(i, "--wam-for-byte-code"))
	    {
	      stop_after = FILE_PL;
	      bc_mode = 1;
	      continue;
	    }

	  if (Check_Arg(i, "-M") || Check_Arg(i, "--mini-assembly"))
	    {
	      stop_after = FILE_WAM;
	      bc_mode = 0;
	      continue;
	    }

	  if (Check_Arg(i, "-S") || Check_Arg(i, "--assembly"))
	    {
	      stop_after = FILE_MA;
	      bc_mode = 0;
	      continue;
	    }

	  if (Check_Arg(i, "-c") || Check_Arg(i, "--object"))
	    {
	      stop_after = FILE_ASM;
	      bc_mode = 0;
	      continue;
	    }

	  if (Check_Arg(i, "-F") || Check_Arg(i, "--fd-to-c"))
	    {
	      stop_after = FILE_FD;
	      bc_mode = 0;
	      continue;
	    }

	  if (Check_Arg(i, "--comment"))
	    {
	      Add_Last_Option(cmd_wam2ma.opt);
	      Add_Last_Option(cmd_ma2asm.opt);
	      continue;
	    }

	  if (Check_Arg(i, "--temp-dir"))
	    {
	      if (++i >= argc)
		Fatal_Error("PATH missing after %s option", last_opt);

	      temp_dir = argv[i];
	      continue;
	    }

	  if (Check_Arg(i, "--no-del-temp-files"))
	    {
	      no_del_temp_files = 1;
	      continue;
	    }

	  if (Check_Arg(i, "--no-decode-hexa"))
	    {
	      no_decode_hex = 1;
	      continue;
	    }

	  if (Check_Arg(i, "--version") || Check_Arg(i, "-v") ||
	      Check_Arg(i, "--verbose"))
	    {
	      Display_Copying("Prolog compiler");
	      if (Check_Arg(i, "--version"))
		exit(0);

	      verbose = 1;
	      continue;
	    }

	  if (Check_Arg(i, "-h") || Check_Arg(i, "--help"))
	    {
	      Display_Help();
	      exit(0);
	    }

	  if (Check_Arg(i, "--pl-state"))
	    {
	      if (++i >= argc)
		Fatal_Error("FILE missing after %s option", last_opt);

	      if (access(argv[i], R_OK) != 0)
		{
		  perror(argv[i]);
		  exit(1);
		}

	      Add_Last_Option(cmd_pl2wam.opt);
	      last_opt = argv[i];
	      Add_Last_Option(cmd_pl2wam.opt);
	      continue;
	    }

	  if (Check_Arg(i, "--no-inline") ||
	      Check_Arg(i, "--no-reorder") ||
	      Check_Arg(i, "--no-reg-opt") ||
	      Check_Arg(i, "--min-reg-opt") ||
	      Check_Arg(i, "--no-opt-last-subterm") ||
	      Check_Arg(i, "--fast-math") ||
	      Check_Arg(i, "--keep-void-inst") ||
	      Check_Arg(i, "--no-susp-warn") ||
	      Check_Arg(i, "--no-singl-warn") ||
	      Check_Arg(i, "--no-redef-error") ||
	      Check_Arg(i, "--no-call-c") ||
	      Check_Arg(i, "--compile-msg") || Check_Arg(i, "--statistics"))
	    {
	      Add_Last_Option(cmd_pl2wam.opt);
	      continue;
	    }

	  if (Check_Arg(i, "--c-compiler"))
	    {
	      if (++i >= argc)
		Fatal_Error("FILE missing after %s option", last_opt);

	      cmd_cc.exe_name = argv[i];
	      continue;
	    }

	  if (Check_Arg(i, "-C"))
	    {
	      if (++i >= argc)
		Fatal_Error("OPTION missing after %s option", last_opt);

	      Add_Option(i, cmd_cc.opt);
/* if C options specified do not take into account fd2c default C options */
	      cc_fd2c_flags = "";
	      continue;
	    }

	  if (Check_Arg(i, "-A"))
	    {
	      if (++i >= argc)
		Fatal_Error("OPTION missing after %s option", last_opt);

	      Add_Option(i, cmd_asm.opt);
	      continue;
	    }

	  if (Check_Arg(i, "--local-size"))
	    {
	      Record_Link_Warn_Option(i);
	      if (++i >= argc)
		Fatal_Error("SIZE missing after %s option", last_opt);
	      def_local_size = strtol(argv[i], &q, 10);
	      if (*q || def_local_size < 0)
		Fatal_Error("invalid stack size (%s)", argv[i]);
	      Record_Link_Warn_Option(i);
	      needs_stack_file = 1;
	      continue;
	    }

	  if (Check_Arg(i, "--global-size"))
	    {
	      Record_Link_Warn_Option(i);
	      if (++i >= argc)
		Fatal_Error("SIZE missing after %s option", last_opt);
	      def_global_size = strtol(argv[i], &q, 10);
	      if (*q || def_global_size < 0)
		Fatal_Error("invalid stack size (%s)", argv[i]);
	      Record_Link_Warn_Option(i);
	      needs_stack_file = 1;
	      continue;
	    }

	  if (Check_Arg(i, "--trail-size"))
	    {
	      Record_Link_Warn_Option(i);
	      if (++i >= argc)
		Fatal_Error("SIZE missing after %s option", last_opt);
	      def_trail_size = strtol(argv[i], &q, 10);
	      if (*q || def_trail_size < 0)
		Fatal_Error("invalid stack size (%s)", argv[i]);
	      Record_Link_Warn_Option(i);
	      needs_stack_file = 1;
	      continue;
	    }

	  if (Check_Arg(i, "--cstr-size"))
	    {
	      Record_Link_Warn_Option(i);
	      if (++i >= argc)
		Fatal_Error("SIZE missing after %s option", last_opt);
	      def_cstr_size = strtol(argv[i], &q, 10);
	      if (*q || def_cstr_size < 0)
		Fatal_Error("invalid stack size (%s)", argv[i]);
	      Record_Link_Warn_Option(i);
	      needs_stack_file = 1;
	      continue;
	    }

	  if (Check_Arg(i, "--fixed-sizes"))
	    {
	      Record_Link_Warn_Option(i);
	      fixed_sizes = 1;
	      needs_stack_file = 1;
	      continue;
	    }

	  if (Check_Arg(i, "--no-top-level"))
	    {
	      Record_Link_Warn_Option(i);
	      no_top_level = 1;
	      no_debugger = 1;
	      continue;
	    }

	  if (Check_Arg(i, "--gui-console"))
	    {
#ifdef W32_GUI_CONSOLE
	      Record_Link_Warn_Option(i);
	      gui_console = 1;
#else
	      fprintf(stderr, "Warning: Win32 GUI Console not available\n");
#endif
	      continue;
	    }

	  if (Check_Arg(i, "--no-debugger"))
	    {
	      Record_Link_Warn_Option(i);
	      no_debugger = 1;
	      continue;
	    }

	  if (Check_Arg(i, "--min-pl-bips"))
	    {
	      Record_Link_Warn_Option(i);
	      min_pl_bips = 1;
	      continue;
	    }

	  if (Check_Arg(i, "--min-fd-bips"))
	    {
	      Record_Link_Warn_Option(i);
	      min_fd_bips = 1;
	      continue;
	    }

	  if (Check_Arg(i, "--min-bips") || Check_Arg(i, "--min-size"))
	    {
	      Record_Link_Warn_Option(i);
	      no_top_level = no_debugger = min_pl_bips = min_fd_bips = 1;
	      if (Check_Arg(i, "--min-size"))
		strip = 1;
	      continue;
	    }

	  if (Check_Arg(i, "--no-fd-lib"))
	    {
	      Record_Link_Warn_Option(i);
	      no_fd_lib = 1;
	      continue;
	    }

	  if (Check_Arg(i, "-s") || Check_Arg(i, "--strip"))
	    {
	      Record_Link_Warn_Option(i);
	      strip = 1;
	      continue;
	    }

	  if (Check_Arg(i, "-L"))
	    {
	      Record_Link_Warn_Option(i);
	      if (++i >= argc)
		Fatal_Error("OPTION missing after %s option", last_opt);

	      Add_Option(i, cmd_link.opt);
	      Record_Link_Warn_Option(i);
	      continue;
	    }

	  Fatal_Error("unknown option %s - try %s --help", argv[i], GPLC);
	}

      if (nb_file == MAX_FILES - 1)	/* reserve 1 for stack sizes file */
	Fatal_Error("too many files (max=%d)", MAX_FILES);

      nb_file++;

      f->name = argv[i];

      if ((f->suffix = strrchr(argv[i], '.')) == NULL)
	f->suffix = argv[i] + strlen(argv[i]);

      if (strcasecmp(PL_SUFFIX_ALTERNATE, f->suffix) == 0)
	f->type = FILE_PL;
      else
	if ((q = StrStr(C_SUFFIX_ALTERNATE, f->suffix)) &&
	    q[-1] == '|' && q[strlen(f->suffix)] == '|')
	f->type = FILE_C;
      else
	{
	  f->type = FILE_LINK;
	  for (p = suffixes; *p; p++)
	    if (strcasecmp(*p, f->suffix) == 0)
	      {
		f->type = p - suffixes;
		break;
	      }
	}

      f->work_name1 = f->name;
      f->work_name2 = NULL;

      if (f->type != FILE_LINK && access(f->name, R_OK) != 0)
	{
	  perror(f->name);
	  exit(1);
	}

      f++;
    }


  if (f == file)
    {
      if (verbose)
	exit(0);		/* --verbose with no files same as --version */
      else
	Fatal_Error("no input file specified");
    }

  f->name = NULL;

  if (nb_file > 1 && stop_after < FILE_LINK && file_name_out)
    {
      Record_Link_Warn_Option(file_name_out_i);
      Record_Link_Warn_Option(file_name_out_i + 1);
      file_name_out = NULL;
    }
}




/*-------------------------------------------------------------------------*
 * DISPLAY_HELP                                                            *
 *                                                                         *
 *-------------------------------------------------------------------------*/
void
Display_Help(void)
#define L(msg)  fprintf(stderr,"%s\n",msg)
{
  fprintf(stderr, "Usage: %s [OPTION]... FILE...\n", GPLC);
  L(" ");
  L("General options:");
  L("  -o FILE, --output FILE      set output file name");
  L("  -W, --wam-for-native        stop after producing WAM file(s)");
  L
    ("  -w, --wam-for-byte-code     stop after producing WAM for byte-code file(s) (force --no-call-c)");
  L
    ("  -M, --mini-assembly         stop after producing mini-assembly file(s)");
  L("  -S, --assembly              stop after producing assembly file(s)");
  L
    ("  -F, --fd-to-c               stop after producing C file(s) from FD file(s)");
  L("  -c, --object                stop after producing object file(s)");
  L
    ("  --temp-dir PATH             use PATH as directory for temporary files");
  L("  --no-del-temp               do not delete temporary files");
  L
    ("  --no-decode-hexa            do not decode hexadecimal predicate names");
  L("  -v, --verbose               print executed commands");
  L("  -h, --help                  print this help and exit");
  L("  --version                   print version number and exit");
  L(" ");
  L("Prolog to WAM compiler options:");
  L
    ("  --pl-state FILE             read FILE to set the initial Prolog state");
  L("  --no-inline                 do not inline predicates");
  L("  --no-reorder                do not reorder predicate arguments");
  L("  --no-reg-opt                do not optimize registers");
  L("  --min-reg-opt               minimally optimize registers");
  L
    ("  --no-opt-last-subterm       do not optimize last subterm compilation");
  L
    ("  --fast-math                 fast mathematical mode (assume integer arithmetic)");
  L
    ("  --keep-void-inst            keep void instructions in the output file");
  L
    ("  --no-susp-warn              do not show warnings for suspicious predicates");
  L
    ("  --no-singl-warn             do not show warnings for named singleton variables");
  L
    ("  --no-redef-error            do not show errors for built-in redefinitions");
  L
    ("  --no-call-c                 do not allow the use of fd_tell, '$call_c',...");
  L("  --compile-msg               print a compile message");
  L("  --statistics                print statistics information");
  L(" ");
  L("WAM to mini-assembly translator options:");
  L("  --comment                   include comments in the output file");
  L(" ");
  L("Mini-assembly to assembly translator options:");
  L("  --comment                   include comments in the output file");
  L(" ");
  L("C Compiler options:");
  L("  --c-compiler FILE           use FILE as C compiler");
  L("  -C OPTION                   pass OPTION to the C compiler");
  L(" ");
  L("Assembler options:");
  L("  -A OPTION                   pass OPTION to the assembler");
  L(" ");
  L("Linker options:");
  L("  --local-size N              set default local  stack size to N Kb");
  L("  --global-size N             set default global stack size to N Kb");
  L("  --trail-size N              set default trail  stack size to N Kb");
  L("  --cstr-size N               set default cstr   stack size to N Kb");
  L
    ("  --fixed-sizes               do not consult environment variables at run-time");
  L("  --gui-console               link the Win32 GUI console");
  L
    ("  --no-top-level              do not link the top-level (force --no-debugger)");
  L("  --no-debugger               do not link the Prolog/WAM debugger");
  L
    ("  --min-pl-bips               link only used Prolog built-in predicates");
  L
    ("  --min-fd-bips               link only used FD solver built-in predicates");
  L
    ("  --min-bips                  same as: --no-top-level --min-pl-bips --min-fd-bips");
  L("  --min-size                  same as: --min-bips --strip");
  L
    ("  --no-fd-lib                 do not look for the FD library (maintenance only)");
  L("  -s, --strip                 strip the executable");
  L("  -L OPTION                   pass OPTION to the linker");
  L("");
  L("Report bugs to bug-prolog@gnu.org.");
}

#undef L