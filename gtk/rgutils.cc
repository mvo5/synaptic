/* rgmisc.cc 
 * 
 * Copyright (c) 2003 Michael Vogt
 * 
 * Author: Michael Vogt <mvo@debian.org>
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

#include <apt-pkg/fileutl.h>

#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <string>
#include <stdio.h>
#include <cstdlib>
#include <cstring>
#include <pwd.h>
#include <assert.h>
#include <iostream>

#include "i18n.h"
#include "rgutils.h"


// helper
GdkPixbuf *
get_gdk_pixbuf(const gchar *name, int size)
{
   GtkIconTheme *theme;
   GdkPixbuf *pixbuf;
   GError *error = NULL;

   theme = gtk_icon_theme_get_default();
   pixbuf = gtk_icon_theme_load_icon(theme, name, size, 
				     (GtkIconLookupFlags)0, &error);
   if (pixbuf == NULL) 
      std::cerr << "Warning, failed to load: " << name 
		<< error->message << std::endl;

   return pixbuf;
}

GtkWidget *get_gtk_image(const gchar *name, int size)
{
   GdkPixbuf *buf;
   buf = get_gdk_pixbuf(name, size);
   if(!buf)
      return NULL;
   return gtk_image_new_from_pixbuf(buf);
}

void RGFlushInterface()
{
   XSync(GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), False);

   while (gtk_events_pending()) {
      gtk_main_iteration();
   }
}

/*
 * SizeToStr: Converts a size long into a human-readable SI string
 * ----------------------------------------------------
 * A maximum of four digits are shown before conversion to the next highest
 * unit. The maximum length of the string will be five characters unless the
 * size is more than ten yottabytes.
 *
 * mvo: we use out own SizeToStr function as the SI spec says we need a 
 *      space between the number and the unit (this isn't the case in stock apt
 */
std::string SizeToStr(double Size)
{
   char S[300];
   double ASize;
   if (Size >= 0) {
      ASize = Size;
   } else {
      ASize = -1 * Size;
   }

   /* Bytes, kilobytes, megabytes, gigabytes, terabytes, petabytes, exabytes,
    * zettabytes, yottabytes.
    */
   char Ext[] = { ' ', 'k', 'M', 'G', 'T', 'P', 'E', 'Z', 'Y' };
   int I = 0;
   while (I <= 8) {
      if (ASize < 100 && I != 0) {
         snprintf(S, 300, "%.1f %cB", ASize, Ext[I]);
         break;
      }

      if (ASize < 10000) {
         snprintf(S, 300, "%.0f %cB", ASize, Ext[I]);
         break;
      }
      ASize /= 1000.0;
      I++;
   }
   return S;
}

std::vector<const gchar*> GetBrowserCommand(const gchar *link)
{
   std::vector<const gchar*> cmd;
   if (FileExists("/usr/bin/xdg-open")) {
      cmd.push_back("/usr/bin/xdg-open");
      cmd.push_back(link);
   } else if (FileExists("/usr/bin/firefox")) {
      cmd.push_back("/usr/bin/firefox");
      cmd.push_back(link);
   } else if (FileExists("/usr/bin/iceweasel")) {
      cmd.push_back("/usr/bin/iceweasel");
      cmd.push_back(link);
   } else if (FileExists("/usr/bin/konqueror")) {
      cmd.push_back("/usr/bin/konqueror");
      cmd.push_back(link);
   }
   return cmd;
}

bool RunAsSudoUserCommand(std::vector<const gchar*> cmd)
{
   std::vector<const gchar*> prefix;
    gchar *sudo_user;
    passwd* pwd;
    if (cmd.empty()) {
       std::cerr << "Empty command for RunAsSudoUserCommand" << std::endl;
       return true;
    }
    bool getuidbyname = false;
    // try pkexec first, then sudo
    sudo_user = getenv("PKEXEC_UID");
    
    if (sudo_user == NULL) {
       sudo_user = getenv("SUDO_UID");
    }
    if (sudo_user == NULL) {
       sudo_user = getenv("USER");
       getuidbyname = true;
    }
    if (sudo_user == NULL) {
       return false;
    }
    if(strncmp("root", sudo_user, strlen("root")) == 0){
        return false;
    }
    if(!getuidbyname){
        pwd = getpwuid(atoi(sudo_user));
    }
    else{
         pwd = getpwnam(sudo_user);
    }
    sudo_user = pwd->pw_name;
#if 0 // does not work for some reason
    if(FileExists("/usr/bin/pkexec") && sudo_user != NULL)
    {
       prefix.push_back("/usr/bin/pkexec");
       prefix.push_back("--user");
       prefix.push_back(sudo_user);
    }
#endif
    if(FileExists("/usr/bin/sudo") && sudo_user != NULL)
    {
       prefix.push_back("/usr/bin/sudo");
       prefix.push_back("-u");
       prefix.push_back(sudo_user);
    }
    // insert the prefix string
    cmd.insert(cmd.begin(), prefix.begin(), prefix.end());

#if 0
    for(std::vector<const gchar*>::iterator it = cmd.begin();
        it != cmd.end(); it++)
       printf("cmd '%s'\n", *it);
#endif

    // build the c way to make g_spawn_async happy
    char **c_cmd = new char*[cmd.size()+1];
    int i;
    for(i=0; i<cmd.size(); i++)
       c_cmd[i] = (gchar*)cmd[i];
    c_cmd[i] = NULL;

    GError *error = NULL;
    g_spawn_async("/", c_cmd, NULL, (GSpawnFlags)0, NULL, NULL, NULL, &error);
    if (error != NULL) {
       std::cerr << "Failed to run cmd: " << cmd[0] << std::endl;
    }

    // and free the memory again
    delete [] c_cmd;

    return true;
}

bool is_binary_in_path(const char *program)
{
   gchar **path = g_strsplit(getenv("PATH"), ":", 0);

   for (int i = 0; path[i] != NULL; i++) {
      char *s = g_strdup_printf("%s/%s", path[i], program);
      if (FileExists(s)) {
         g_free(s);
         g_strfreev(path);
         return true;
      }
      g_free(s);
   }
   g_strfreev(path);
   return false;
}

char *gtk_get_string_from_color(GdkColor * colp)
{
   static char *_str = NULL;

   g_free(_str);
   if (colp == NULL) {
      _str = g_strdup("");
      return _str;
   }
   _str = g_strdup_printf("#%4X%4X%4X", colp->red, colp->green, colp->blue);
   for (char *ptr = _str; *ptr; ptr++)
      if (*ptr == ' ')
         *ptr = '0';

   return _str;
}

void gtk_get_color_from_string(const char *cpp, GdkColor **colp)
{
   GdkColor *new_color;
   gboolean result;

   // "" means no color
   if (cpp == NULL || strlen(cpp) == 0) {
      *colp = NULL;
      return;
   }

   new_color = g_new(GdkColor, 1);
   result = gdk_color_parse(cpp, new_color);
   *colp = new_color;
}

const char *utf8_to_locale(const char *str)
{
   static char *_str = NULL;
   if (str == NULL)
      return NULL;
   if (g_utf8_validate(str, -1, NULL) == false)
      return NULL;
   g_free(_str);
   _str = NULL;
   _str = g_locale_from_utf8(str, -1, NULL, NULL, NULL);
   return _str;
}

const char *utf8(const char *str)
{
   static char *_str = NULL;
   if (str == NULL)
      return NULL;
   if (g_utf8_validate(str, -1, NULL) == true)
      return str;
   g_free(_str);
   _str = NULL;
   _str = g_locale_to_utf8(str, -1, NULL, NULL, NULL);
   return _str;
}

/*
 * MarkupEscapeString: Escape markup from a string
 * ----------------------------------------------------
 * @unescaped: string to escape markup from
 *
 * we use g_markup_escape_text which only support 5 standard entities: 
 * &amp; &lt; &gt; &quot; &apos;
 * We must escape the string used as list item of a GtkTree
 */
 std::string MarkupEscapeString(std::string unescaped) {
   std::string escaped;
   char *c_esc = g_markup_escape_text(unescaped.c_str(), -1);
   escaped = std::string(c_esc);
   g_free(c_esc);
   return escaped;  
}

/*
 * MarkupUnescapeString: Unescape markup from a string
 * ----------------------------------------------------
 * @escaped: string to unescape markup from
 * unescaped entities: &amp; &lt; &gt; &quot; &apos;
 *
 * sadly there is no simple way to unescape a previously escaped string with 
 * g_markup_escape_text
 */
 std::string MarkupUnescapeString(std::string escaped) {
   size_t pos = 0, end = 0;
   std::string entity, str;
   while ((pos = escaped.find("&", pos)) != std::string::npos ) {
      end = escaped.find(";", pos);
      if (end == std::string::npos)
          break;

      entity = escaped.substr(pos,end-pos+1);
      str = "";
      if(entity == "&lt;")
         str = "<";
      else if(entity == "&gt;")
         str = ">";
      else if(entity == "&amp;")
         str = "&";
      else if(entity == "&quot;")
         str = "\"";
      else if(entity == "&apos;")
         str = "'";

      if(!str.empty()) {
          escaped.replace(pos, entity.size(), str);
          pos++; 
      } else {
          pos = end;
      }
   }
   return escaped;
}
// vim:ts=3:sw=3:et
