/* sections_trans.cc - translate debian sections into friendlier names
 *  (c) 2004 Michael Vogt 
 *  
 */

#include <libintl.h>

#include "sections_trans.h"

const char *transtable[][2] = {
   // TRANSLATORS: Alias for the Debian package section "admin"
   {"admin", _("System Administration")},
   // TRANSLATORS: Alias for the Debian package section "base"
   {"base", _("Base System")},
   // TRANSLATORS: Alias for the Debian package section "cli-mono"
   {"cli-mono", _("Mono/CLI Infrastructure")},
   // TRANSLATORS: Alias for the Debian package section "comm"
   {"comm", _("Communication")},
   // TRANSLATORS: Alias for the Debian package section "database"
   {"database", _("Databases")},
   // TRANSLATORS: Alias for the Debian package section "devel"
   {"devel", _("Development")},
   // TRANSLATORS: Alias for the Debian package section "doc"
   {"doc", _("Documentation")},
   // TRANSLATORS: Alias for the Debian package section "debug"
   {"debug", _("Debug")},
   // TRANSLATORS: Alias for the Debian package section "editors"
   {"editors", _("Editors")},
   // TRANSLATORS: Alias for the Debian package section "electronics"
   {"electronics", _("Electronics")},
   // TRANSLATORS: Alias for the Debian package section "embedded"
   {"embedded", _("Embedded Devices")},
   // TRANSLATORS: Alias for the Debian package section "fonts"
   {"fonts", _("Fonts")},
   // TRANSLATORS: Alias for the Debian package section "games"
   {"games", _("Games and Amusement")},
   // TRANSLATORS: Alias for the Debian package section "gnome"
   {"gnome", _("GNOME Desktop Environment")},
   // TRANSLATORS: Alias for the Debian package section "graphics"
   {"graphics", _("Graphics")},
   // TRANSLATORS: Alias for the Debian package section "gnu-r"
   {"gnu-r", _("GNU R statistical system")},
   // TRANSLATORS: Alias for the Debian package section "gnustep"
   {"gnustep", _("Gnustep Desktop Environment")},
   // TRANSLATORS: Alias for the Debian package section "hamradio"
   {"hamradio", _("Amateur Radio")},
   // TRANSLATORS: Alias for the Debian package section "haskell"
   {"haskell", _("Haskell Programming Language")},
   // TRANSLATORS: Alias for the Debian package section "httpd"
   {"httpd", _("Web servers")},
   // TRANSLATORS: Alias for the Debian package section "interpreters"
   {"interpreters", _("Interpreted Computer Languages")},
   // TRANSLATORS: Alias for the Debian package section "java"
   {"java", _("Java Programming Language")},
   // TRANSLATORS: Alias for the Debian package section "KDE"
   {"kde", _("KDE Desktop Environment")},
   // TRANSLATORS: Alias for the Debian package section "kernel"
   {"kernel", _("Kernel and modules")},
   // TRANSLATORS: Alias for the Debian package section "libdevel"
   {"libdevel", _("Libraries - Development")},
   // TRANSLATORS: Alias for the Debian package section "libs"
   {"libs", _("Libraries")},
   // TRANSLATORS: Alias for the Debian package section "lisp"
   {"lisp", _("Lisp Programming Language")},
   // TRANSLATORS: Alias for the Debian package section "localization"
   {"localization", _("Localization")},
   // TRANSLATORS: Alias for the Debian package section "mail"
   {"mail", _("Email")},
   // TRANSLATORS: Alias for the Debian package section "math"
   {"math", _("Mathematics")},
   // TRANSLATORS: Alias for the Debian package section "misc"
   {"misc", _("Miscellaneous - Text Based")},
   // TRANSLATORS: Alias for the Debian package section "net"
   {"net", _("Networking")},
   // TRANSLATORS: Alias for the Debian package section "news"
   {"news", _("Newsgroup")},
   // TRANSLATORS: Alias for the Debian package section "ocaml"
   {"ocaml", _("OCaml Programming Language")},
   // TRANSLATORS: Alias for the Debian package section "oldlibs"
   {"oldlibs", _("Libraries - Old")},
   // TRANSLATORS: Alias for the Debian package section "otherosfs"
   {"otherosfs", _("Cross Platform")},
   // TRANSLATORS: Alias for the Debian package section "perl"
   {"perl", _("Perl Programming Language")},
   // TRANSLATORS: Alias for the Debian package section "php"
   {"php", _("PHP Programming Language")},
   // TRANSLATORS: Alias for the Debian package section "python"
   {"python", _("Python Programming Language")},
   // TRANSLATORS: Alias for the Debian package section "ruby"
   {"ruby", _("Ruby Programming Language")},
   // TRANSLATORS: Alias for the Debian package section "science"
   {"science", _("Science")},
   // TRANSLATORS: Alias for the Debian package section "shells"
   {"shells", _("Shells")},
   // TRANSLATORS: Alias for the Debian package section "sound"
   {"sound", _("Multimedia")},
   // TRANSLATORS: Alias for the Debian package section "task"
   {"tasks", _("Tasks")},
   // TRANSLATORS: Alias for the Debian package section "tex"
   {"tex", _("TeX Authoring")},
   // TRANSLATORS: Alias for the Debian package section "text"
   {"text", _("Word Processing")},
   // TRANSLATORS: Alias for the Debian package section "utils"
   {"utils", _("Utilities")},
   // TRANSLATORS: Alias for the Debian package section "vcs"
   {"vcs", _("Version Control Systems")},
   // TRANSLATORS: Alias for the Debian package section "video"
   {"video", _("Video software")},
   // TRANSLATORS: Alias for the Debian package section "web"
   {"web", _("World Wide Web")},
   // TRANSLATORS: Alias for the Debian package section "x11"
   {"x11", _("Miscellaneous  - Graphical")},
   // TRANSLATORS: Alias for the Debian package section "xfce"
   {"xfce", _("Xfce Desktop Environment")},
   // TRANSLATORS: Alias for the Debian package section "zope"
   {"zope", _("Zope/Plone Environment")},
   // TRANSLATORS: The section of the package is not known
   {"unknown", _("Unknown")},
   // TRANSLATORS: Alias for the Debian package section "alien"
   {"alien", _("Converted From RPM by Alien")},
   // TRANSLATORS: Ubuntu translations section
   {"translations", _("Internationalization and localization")},
   // TRANSLATORS: metapackages section
   {"metapackages", _("Meta Packages")},
   // TRANSLATORS: Alias for the Debian package section "education"
   {"education", _("Education")},
   // TRANSLATORS: Alias for the Debian package section "introspection"
   {"introspection", _("GObject Introspection data")},

   // TRANSLATORS: Alias for the Debian package section "non-US"
   //              Export to the outside of the USA is not allowed
   //              or restricted
   {"non-US", _("Restricted On Export")},
   // TRANSLATORS: Alias for the Debian package section "non free"
   {"non-free", _("non free")},
   // TRANSLATORS: Alias for the Debian package section "contrib"
   //              Free software that depends on non-free software
   {"contrib", _("contrib")},
   //{"non-free",_("<i>(non free)</i>")},
   //{"contrib",_("<i>(contrib)</i>")},
   {NULL, NULL}
};

#ifndef HAVE_RPM
string trans_section(string sec)
{
   string str = sec;
   string suffix;
   // baaaa, special case for stupid debian package naming
   if (str == "non-US/non-free") {
      str = _("Restricted On Export");
      suffix = _("non free");
   }
   if (str == "non-US/non-free") {
      str = _("Restricted On Export");
      suffix = _("contrib");
   }
   // if we have something like "contrib/web", make "contrib" the 
   // suffix and translate it independently
   string::size_type n = str.find("/");
   if (n != string::npos) {
      suffix = str.substr(0, n);
      str.erase(0, n + 1);
      for (int i = 0; transtable[i][0] != NULL; i++) {
         if (suffix == transtable[i][0]) {
            suffix = _(transtable[i][1]);
            break;
         }
      }
   }
   for (int i = 0; transtable[i][0] != NULL; i++) {
      if (str == transtable[i][0]) {
         str = _(transtable[i][1]);
         break;
      }
   }
   // if we have a suffix, add it
   if (!suffix.empty()) {
      ostringstream out;
      ioprintf(out, "%s (%s)", str.c_str(), suffix.c_str());
      str = out.str();
   }
   return str;
}
#else
string trans_section(string sec)
{
   return dgettext("rpm", sec.c_str());
}
#endif
