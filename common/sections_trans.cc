/* sections_trans.cc - translate debian sections into friendlier names
 *  (c) 2004 Michael Vogt 
 *  
 */

#include "sections_trans.h"

char *transtable[][2] = {
   {"admin", _("System Administration")},
   {"base", _("Base System")},
   {"comm", _("Communication")},
   {"devel", _("Development")},
   {"doc", _("Documentation")},
   {"editors", _("Editors")},
   {"electronics", _("Electronics")},
   {"embedded", _("Embedded Devices")},
   {"games", _("Games and Amusement")},
   {"gnome", _("GNOME Desktop Environment")},
   {"graphics", _("Graphics")},
   {"hamradio", _("Amateur Radio")},
   {"interpreters", _("Interpreted Computer Languages")},
   {"kde", _("KDE Desktop Environment")},
   {"libdevel", _("Libraries - Development")},
   {"libs", _("Libraries")},
   {"mail", _("Email")},
   {"math", _("Mathematics")},
   {"misc", _("Miscellaneous - Text Based")},
   {"net", _("Networking")},
   {"news", _("Newsgroup")},
   {"oldlibs", _("Libraries - Old")},
   {"otherosfs", _("Cross Platform")},
   {"perl", _("Perl Programming Language")},
   {"python", _("Python Programming Language")},
   {"science", _("Science")},
   {"shells", _("Shells")},
   {"sound", _("Multimedia")},
   {"tex", _("TeX Authoring")},
   {"text", _("Word Processing")},
   {"utils", _("Utilities")},
   {"web", _("World Wide Web")},
   {"x11", _("Miscellaneous  - Graphical")},
   {"unknown", _("Unknown")},
   {"alien", _("Converted From RPM by Alien")},

   {"non-US", _("Restricted On Export")},
   {"non-free", _("non free")},
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
      str = _("Non US");
      suffix = _("non free");
   }
   if (str == "non-US/non-free") {
      str = _("Non US");
      suffix = _("contrib");
   }
   // if we have something like "contrib/web", make "contrib" the 
   // suffix and translate it independently
   unsigned int n = str.find("/");
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
      ioprintf(out, "%s <i>(%s)</i>", str.c_str(), suffix.c_str());
      str = out.str();
   }
   return str;
}
#else
string trans_section(string sec)
{
   return sec;
}
#endif
