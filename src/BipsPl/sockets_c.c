/*-------------------------------------------------------------------------*
 * GNU Prolog                                                              *
 *                                                                         *
 * Part  : Prolog buit-in predicates                                       *
 * File  : sockets_c.c                                                     *
 * Descr.: sockets management - C part                                     *
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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

       /* old versions of CYGWIN do not support AF_UNIX - modify next line */
#if defined(__unix__) || defined(__CYGWIN__)
#define SUPPORT_AF_UNIX
#endif

#ifdef SUPPORT_AF_UNIX
#include <sys/un.h>
#endif
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define OBJ_INIT Socket_Initializer

#include "engine_pl.h"
#include "bips_pl.h"




/*---------------------------------*
 * Constants                       *
 *---------------------------------*/

/*---------------------------------*
 * Type Definitions                *
 *---------------------------------*/

/*---------------------------------*
 * Global Variables                *
 *---------------------------------*/

#ifdef SUPPORT_AF_UNIX
static int atom_AF_UNIX;
#endif
static int atom_AF_INET;




/*---------------------------------*
 * Function Prototypes             *
 *---------------------------------*/

static
  Bool Create_Socket_Streams(int sock, char *stream_name,
			     int *stm_in, int *stm_out);




/*-------------------------------------------------------------------------*
 * SOCKET_INITIALIZER                                                      *
 *                                                                         *
 *-------------------------------------------------------------------------*/
static void
Socket_Initializer(void)
{
#ifdef SUPPORT_AF_UNIX
  atom_AF_UNIX = Create_Atom("AF_UNIX");
#endif
  atom_AF_INET = Create_Atom("AF_INET");
}




/*-------------------------------------------------------------------------*
 * SOCKET_2                                                                *
 *                                                                         *
 *-------------------------------------------------------------------------*/
Bool
Socket_2(WamWord domain_word, WamWord socket_word)
{
  int domain;
  int sock;

  domain = Rd_Atom_Check(domain_word);
  if (
#ifdef SUPPORT_AF_UNIX
       domain != atom_AF_UNIX &&
#endif
       domain != atom_AF_INET)
    Pl_Err_Domain(domain_socket_domain, domain_word);

  Check_For_Un_Variable(socket_word);


#ifdef SUPPORT_AF_UNIX
  if (domain == atom_AF_UNIX)
    sock = socket(AF_UNIX, SOCK_STREAM, 0);
  else
#endif
    sock = socket(AF_INET, SOCK_STREAM, 0);

  Os_Test_Error(sock == -1);

  return Get_Integer(sock, socket_word);
}




/*-------------------------------------------------------------------------*
 * SOCKET_CLOSE_1                                                          *
 *                                                                         *
 *-------------------------------------------------------------------------*/
Bool
Socket_Close_1(WamWord socket_word)
{
  int sock;

  sock = Rd_Integer_Check(socket_word);
  if (sock < 2)
    {
      errno = EBADF;
      Os_Test_Error(1);
    }
  else
    Os_Test_Error(close(sock));

  return TRUE;
}




/*-------------------------------------------------------------------------*
 * SOCKET_BIND_2                                                           *
 *                                                                         *
 *-------------------------------------------------------------------------*/
Bool
Socket_Bind_2(WamWord socket_word, WamWord address_word)
{
  WamWord word, tag, *adr;
  WamWord *stc_adr;
  int dom;
  int sock;
  int port;
  int l;

#ifdef SUPPORT_AF_UNIX
  char *path_name;
  struct sockaddr_un adr_un;
#endif
  struct sockaddr_in adr_in;
  static int atom_host_name = -1;	/* not created in an init since implies to */

  /* establish a connection */

  sock = Rd_Integer_Check(socket_word);


  Deref(address_word, word, tag, adr);

  if (tag == REF)
    Pl_Err_Instantiation();

  if (tag != STC)
  err_domain:
    Pl_Err_Domain(domain_socket_address, word);

  stc_adr = UnTag_STC(word);

#ifdef SUPPORT_AF_UNIX
  if (Functor_Arity(atom_AF_UNIX, 1) == Functor_And_Arity(stc_adr))
    dom = AF_UNIX;
  else
#endif
  if (Functor_Arity(atom_AF_INET, 2) == Functor_And_Arity(stc_adr))
    dom = AF_INET;
  else
    goto err_domain;

#ifdef SUPPORT_AF_UNIX
  if (dom == AF_UNIX)
    {
      path_name = Rd_String_Check(Arg(stc_adr, 0));
      if ((path_name = M_Absolute_Path_Name(path_name)) == NULL)
	Pl_Err_Domain(domain_os_path, Arg(stc_adr, 0));

      adr_un.sun_family = AF_UNIX;
      strcpy(adr_un.sun_path, path_name);
      unlink(path_name);
      Os_Test_Error(bind(sock, (struct sockaddr *) &adr_un, sizeof(adr_un))
		    == -1);
      return TRUE;
    }
#endif
  /* case AF_INET */

  Deref(Arg(stc_adr, 0), word, tag, adr);
  if (tag == REF)
    {
      if (atom_host_name < 0)
	atom_host_name = Create_Allocate_Atom(M_Host_Name_From_Name(NULL));

      Get_Atom(atom_host_name, word);
    }
  else
    Rd_Atom_Check(word);	/* only to test the type */

  port = 0;
  Deref(Arg(stc_adr, 1), word, tag, adr);
  if (tag != REF)
    port = Rd_Integer_Check(word);

  adr_in.sin_port = htons(port);
  adr_in.sin_family = AF_INET;
  adr_in.sin_addr.s_addr = INADDR_ANY;

  Os_Test_Error(bind(sock, (struct sockaddr *) &adr_in, sizeof(adr_in)));
  if (tag == INT)
    return TRUE;

  l = sizeof(adr_in);
  Os_Test_Error(getsockname(sock, (struct sockaddr *) &adr_in, &l));

  port = ntohs(adr_in.sin_port);

  return Get_Integer(port, word);
}




/*-------------------------------------------------------------------------*
 * SOCKET_CONNECT_4                                                        *
 *                                                                         *
 *-------------------------------------------------------------------------*/
Bool
Socket_Connect_4(WamWord socket_word, WamWord address_word,
		 WamWord stm_in_word, WamWord stm_out_word)
{
  WamWord word, tag, *adr;
  WamWord *stc_adr;
  int dom;
  int sock;
  int port;
  char *host_name;

#ifdef SUPPORT_AF_UNIX
  char *path_name;
  struct sockaddr_un adr_un;
#endif
  struct sockaddr_in adr_in;
  struct hostent *host_entry;
  int stm_in, stm_out;
  char stream_name[256];

  sock = Rd_Integer_Check(socket_word);

  Deref(address_word, word, tag, adr);

  if (tag == REF)
    Pl_Err_Instantiation();

  if (tag != STC)
  err_domain:
    Pl_Err_Domain(domain_socket_address, word);

  stc_adr = UnTag_STC(word);

#ifdef SUPPORT_AF_UNIX
  if (Functor_Arity(atom_AF_UNIX, 1) == Functor_And_Arity(stc_adr))
    dom = AF_UNIX;
  else
#endif
  if (Functor_Arity(atom_AF_INET, 2) == Functor_And_Arity(stc_adr))
    dom = AF_INET;
  else
    goto err_domain;

#ifdef SUPPORT_AF_UNIX
  if (dom == AF_UNIX)
    {
      path_name = Rd_String_Check(Arg(stc_adr, 0));
      if ((path_name = M_Absolute_Path_Name(path_name)) == NULL)
	Pl_Err_Domain(domain_os_path, Arg(stc_adr, 0));

      adr_un.sun_family = AF_UNIX;
      strcpy(adr_un.sun_path, path_name);
      Os_Test_Error(connect
		    (sock, (struct sockaddr *) &adr_un, sizeof(adr_un)));
      sprintf(stream_name, "socket_stream(connect('AF_UNIX'('%s')),%d)",
	      path_name, sock);
      goto create_streams;
    }
#endif
  /* case AF_INET */
  host_name = Rd_String_Check(Arg(stc_adr, 0));
  port = Rd_Integer_Check(Arg(stc_adr, 1));

  host_entry = gethostbyname(host_name);
  if (host_entry == NULL)
    return FALSE;

  adr_in.sin_family = AF_INET;
  adr_in.sin_port = htons(port);
  memcpy(&adr_in.sin_addr, host_entry->h_addr_list[0],
	 host_entry->h_length);

  Os_Test_Error(connect(sock, (struct sockaddr *) &adr_in, sizeof(adr_in)));
  sprintf(stream_name, "socket_stream(connect('AF_INET'('%s',%d)),%d)",
	  host_name, port, sock);

#ifdef SUPPORT_AF_UNIX
create_streams:
#endif
  if (!Create_Socket_Streams(sock, stream_name, &stm_in, &stm_out))
    return FALSE;

  Get_Integer(stm_in, stm_in_word);
  Get_Integer(stm_out, stm_out_word);
  return TRUE;
}




/*-------------------------------------------------------------------------*
 * SOCKET_LISTEN_2                                                         *
 *                                                                         *
 *-------------------------------------------------------------------------*/
Bool
Socket_Listen_2(WamWord socket_word, WamWord length_word)
{
  int sock;
  int length;

  sock = Rd_Integer_Check(socket_word);
  length = Rd_Integer_Check(length_word);

  Os_Test_Error(listen(sock, length));
  return TRUE;
}




/*-------------------------------------------------------------------------*
 * SOCKET_ACCEPT_4                                                         *
 *                                                                         *
 *-------------------------------------------------------------------------*/
Bool
Socket_Accept_4(WamWord socket_word, WamWord client_word,
		WamWord stm_in_word, WamWord stm_out_word)
{
  int sock, cli_sock;
  int l;
  struct sockaddr_in adr_in;
  int stm_in, stm_out;
  char *cli_ip_adr = "AF_UNIX";
  char stream_name[256];


  l = sizeof(adr_in);
  sock = Rd_Integer_Check(socket_word);

  cli_sock = accept(sock, (struct sockaddr *) &adr_in, &l);

  Os_Test_Error(cli_sock < 0);

  if (adr_in.sin_family == AF_INET)
    {
      cli_ip_adr = inet_ntoa(adr_in.sin_addr);
      if (cli_ip_adr == NULL)
	return FALSE;
      Get_Atom(Create_Allocate_Atom(cli_ip_adr), client_word);
    }

  sprintf(stream_name, "socket_stream(accept('%s'),%d)", cli_ip_adr,
	  cli_sock);

  if (!Create_Socket_Streams(cli_sock, stream_name, &stm_in, &stm_out))
    return FALSE;

  Get_Integer(stm_in, stm_in_word);
  Get_Integer(stm_out, stm_out_word);
  return TRUE;
}




/*-------------------------------------------------------------------------*
 * ASSOC_SOCKET_STREAMS_3                                                  *
 *                                                                         *
 *-------------------------------------------------------------------------*/
Bool
Assoc_Socket_Streams_3(WamWord socket_word,
		       WamWord stm_in_word, WamWord stm_out_word)
{
  int stm_in, stm_out;
  char stream_name[256];

  int sock = Rd_Integer_Check(socket_word);

  sprintf(stream_name, "socket_stream(assoc(%d))", sock);
  if (!Create_Socket_Streams(sock, stream_name, &stm_in, &stm_out))
    return FALSE;

  Get_Integer(stm_in, stm_in_word);
  Get_Integer(stm_out, stm_out_word);
  return TRUE;
}




/*-------------------------------------------------------------------------*
 * CREATE_SOCKET_STREAMS                                                   *
 *                                                                         *
 *-------------------------------------------------------------------------*/
static Bool
Create_Socket_Streams(int sock, char *stream_name,
		      int *stm_in, int *stm_out)
{
  StmProp prop;
  int fd;
  FILE *f_in, *f_out;
  int file_name;

  Os_Test_Error((fd = dup(sock)) < 0);
  Os_Test_Error((f_in = fdopen(sock, "rt")) == NULL);
  Os_Test_Error((f_out = fdopen(fd, "wt")) == NULL);

  file_name = Create_Allocate_Atom(stream_name);

  prop.mode = STREAM_MODE_READ;
  prop.input = TRUE;
  prop.output = FALSE;
  prop.text = TRUE;
  prop.reposition = FALSE;
  prop.eof_action = STREAM_EOF_ACTION_RESET;
  prop.buffering = STREAM_BUFFERING_BLOCK;
  prop.tty = FALSE;
  prop.special_close = FALSE;
  prop.other = 4;

  *stm_in = Add_Stream(file_name, (long) f_in, prop,
		       NULL, NULL, NULL, NULL, NULL, NULL, NULL);

  prop.mode = STREAM_MODE_WRITE;
  prop.input = FALSE;
  prop.output = TRUE;

  *stm_out = Add_Stream(file_name, (long) f_out, prop,
			NULL, NULL, NULL, NULL, NULL, NULL, NULL);

  return TRUE;
}




/*-------------------------------------------------------------------------*
 * HOSTNAME_ADDRESS_2                                                      *
 *                                                                         *
 *-------------------------------------------------------------------------*/
Bool
Hostname_Address_2(WamWord host_name_word, WamWord host_address_word)
{
  WamWord word, tag, *adr;
  char *host_name;
  char *host_address;
  struct hostent *host_entry;
  struct in_addr iadr;

  Deref(host_name_word, word, tag, adr);
  if (tag == REF)
    {
      host_address = Rd_String_Check(host_address_word);
      host_name = M_Host_Name_From_Adr(host_address);
      return host_name && Un_String_Check(host_name, host_name_word);
    }

  host_name = Rd_String_Check(word);

  Check_For_Un_Atom(host_address_word);

  host_entry = gethostbyname(host_name);
  if (host_entry == NULL)
    return FALSE;

  memcpy(&iadr.s_addr, host_entry->h_addr_list[0], host_entry->h_length);
  host_address = inet_ntoa(iadr);

  return Un_String_Check(host_address, host_address_word);
}