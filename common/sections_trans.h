/* Translation table for debian sections
 * 
 * we could use markup in here, but I fear the performance hit
 * if we go for markup, we only have to change one line in rgmainwindow.cc 
 * (search for '//"markup"'), currently L2391
*/
char *transtable[][2] = {
   {"admin",_("System Administration")},
   {"base",_("Base System")},
   {"comm",_("Communication")},
   {"devel",_("Development")},
   {"doc",_("Documentation")},
   {"editors",_("Text Editing")},
   {"electronics",_("Electronics")},
   {"embedded",_("Embedded Devices")},
   {"games",_("Games and Amusement")},
   {"gnome",_("GNOME Desktop Environment")},
   {"graphics",_("Graphics")},
   {"hamradio",_("Amateur Radio")},
   {"interpreters",_("Interpreted Computer Languages")},
   {"kde",_("KDE Desktop Environment")},
   {"libdevel",_("Libraries - Development")},
   {"libs",_("Libraries - Old")},
   {"mail",_("Email")},
   {"math",_("Mathematics")},
   {"misc",_("Miscellaneous - Text Based")},
   {"net",_("Networking")},
   {"news",_("Newsgroup")},
   {"oldlibs",_("Libraries")},
   {"otherosfs",_("Cross Platform")},
   {"perl",_("Perl Programming Language")},
   {"python",_("Python Programming Language")},
   {"science",_("Science")},
   {"shells",_("Shells")},
   {"sound",_("Multimedia")},
   {"tex",_("TeX Authoring")},
   {"text",_("Word Processing")},
   {"utils",_("Utilities")},
   {"web",_("World Wide Web")},
   {"x11",_("Miscellaneous  - Graphical")},
   {"unknown", _("Unknown")},
   {"non-US",_("Non US")},
   {"non-free",_("(non free)")},
   {"contrib",_("(contrib)")},
   //{"non-free",_("<i>(non free)</i>")},
   //{"contrib",_("<i>(contrib)</i>")},
   {NULL, NULL}
};
