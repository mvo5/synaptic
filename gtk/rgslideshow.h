/* rgslideshow.h
 *
 * Copyright (c) 2000, 2001 Conectiva S/A
 *
 * Author: Gustavo Niemeyer <niemeyer@conectiva.com>
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


#ifndef _RGSLIDESHOW_H_
#define _RGSLIDESHOW_H_

using namespace std;

class RGSlideShow {

 protected:

   GtkImage * _image;
   int _totalSteps;
   int _currentStep;
   vector<string> _imageFileList;

 public:

   void step();
   void refresh();
   void setTotalSteps(int totalSteps) {
      _totalSteps = totalSteps;
   };

   RGSlideShow(GtkImage * image, string imgPath);
};

#endif

// vim:sts=3:sw=3
