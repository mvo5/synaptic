#ifndef RQPIXMAPS_H
#define RQPIXMAPS_H

#include <qpixmapcache.h>
#include <qstring.h>
#include <qfile.h>

#include <iostream>

class RQPixmaps {

   public:

   static QPixmap find(QString filename) {
      QPixmap pixmap;
      if (!QPixmapCache::find(filename, pixmap)) {
         if (QFile::exists("../pixmaps/"+filename)) {
            pixmap = QPixmap("../pixmaps/"+filename);
            QPixmapCache::insert(filename, pixmap);
         } else if (QFile::exists(SYNAPTIC_PIXMAPDIR "../pixmaps/")) {
            pixmap = QPixmap(SYNAPTIC_PIXMAPDIR+filename);
            QPixmapCache::insert(filename, pixmap);
         } else {
            std::cerr << "Pixmap " << filename << " not found." << std::endl;
         }
      }
      return pixmap;
   };

};

#endif

// vim:ts=3:sw=3:et

