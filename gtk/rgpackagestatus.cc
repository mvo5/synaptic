/* rgpackagestatus.cc  - package status UI stuff
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

#include "config.h" // IWYU pragma: associated

#include "rgpackagestatus.h"

#include "rgutils.h"
#include "rpackagestatus.h"

#include <apt-pkg/configuration.h>
#include <cassert>
#include <cstdio>
#include <gtk/gtk.h>
#include <string>

class RPackage;

// RPackageStatus stuff
RGPackageStatus RGPackageStatus::pkgStatus;

void RGPackageStatus::initColorsAndIcons()
{
   const char *default_status_colors[N_STATUS_COUNT] = {
      "#8ae234", // install
      "#4e9a06", // re-install
      "#fce94f", // upgrade
      "#ad7fa8", // downgrade
      "#ef2929", // remove
      "#a40000", // purge
      NULL,      // available
      "#a40000", // available-locked
      NULL,      // installed-updated
      NULL,      // installed-outdated
      "#a40000", // installed-locked
      NULL,      // broken
      NULL       // new
   };

   gchar *config_string;
   for (int i = 0; i < N_STATUS_COUNT; i++) {
      config_string =
         g_strdup_printf("Synaptic::color-%s", PackageStatusShortString[i]);
      gtk_get_color_from_string(
         _config->Find(config_string, default_status_colors[i]).c_str(),
         &StatusColors[i]);
      g_free(config_string);

      gchar *icon_name =
         g_strdup_printf("package-%s", PackageStatusShortString[i]);
      StatusPixbuf[i] = loadStatusIcon(icon_name);
      g_free(icon_name);
   }
   supportedPix = loadStatusIcon("package-supported");
}

// Exact-name lookup only: GTK's GENERIC_FALLBACK (always used by
// icon-name renderers) tries every theme in the chain with the fallback
// names too, so a theme shipping a generic "package" icon (e.g. Tango)
// would shadow the "package-*" icons.
GdkPixbuf *RGPackageStatus::loadStatusIcon(const char *icon_name)
{
   const int statusPixbufSize = 16;
   GError *error = NULL;
   GdkPixbuf *pix = gtk_icon_theme_load_icon(gtk_icon_theme_get_default(),
                                             icon_name,
                                             statusPixbufSize,
                                             GTK_ICON_LOOKUP_FORCE_SIZE,
                                             &error);
   if (pix == NULL) {
      g_warning("failed to load icon %s: %s", icon_name, error->message);
      g_error_free(error);
   }
   return pix;
}

// class that finds out what do display to get user
void RGPackageStatus::init()
{
   RPackageStatus::init();

   initColorsAndIcons();
}

GdkRGBA *RGPackageStatus::getBgColor(RPackage *pkg)
{
   return StatusColors[getStatus(pkg)];
}

GdkPixbuf *RGPackageStatus::getSupportedPix(RPackage *pkg)
{
   if (isSupported(pkg))
      return supportedPix;
   else
      return NULL;
}

GdkPixbuf *RGPackageStatus::getPixbuf(RPackage *pkg)
{
   return getPixbuf(getStatus(pkg));
}

GdkPixbuf *RGPackageStatus::getPixbuf(int i)
{
   assert(0 <= i && i < N_STATUS_COUNT);
   return StatusPixbuf[i];
}

void RGPackageStatus::setColor(int i, GdkRGBA *new_color)
{
   StatusColors[i] = new_color;
}

void RGPackageStatus::saveColors()
{
   gchar *color_string, *config_string;
   for (int i = 0; i < N_STATUS_COUNT; i++) {
      color_string = gtk_get_string_from_color(StatusColors[i]);
      config_string =
         g_strdup_printf("Synaptic::color-%s", PackageStatusShortString[i]);

      _config->Set(config_string, color_string);
      g_free(config_string);
   }
}
