/* i18n.h
 *
 * Copyright (c) 2000, 2001 Conectiva S/A
 *
 * Author: Alfredo K. Kojima <kojima@conectiva.com.br>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#ifndef _I18N_H_
#define _I18N_H_

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef ENABLE_NLS
#include <locale.h>
# include <libintl.h>
# define _(String) gettext (String)
# ifdef gettext_noop
#  define N_(String) gettext_noop (String)
# else
#  define N_(String) (String)
# endif
#else
/* Stubs that do something close enough.  */
# undef  textdomain
# define textdomain(String) (String)
# undef  gettext
# define gettext(String) (String)
# undef  dgettext
# define dgettext(Domain,Message) (Message)
# undef  dcgettext
# define dcgettext(Domain,Message,Type) (Message)
# undef  bindtextdomain
# define bindtextdomain(Domain,Directory) (Domain)
# undef  _
# define _(String) (String)
# undef  N_
# define N_(String) (String)
#endif


#endif
