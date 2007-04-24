#include <sys/types.h>
#include <dirent.h>

#include <gtk/gtk.h>

#include <algorithm>
#include <string>
#include <vector>

#include "rgslideshow.h"

using namespace std;

RGSlideShow::RGSlideShow(GtkImage * image, string imgPath)
: _image(image), _totalSteps(0), _currentStep(0)
{
   DIR *dir = opendir(imgPath.c_str());
   struct dirent *entry;
   imgPath += '/';
   if (dir != NULL) {
      for (entry = readdir(dir); entry != NULL; entry = readdir(dir)) {
         if (entry->d_name[0] != '.')
            _imageFileList.push_back(imgPath + entry->d_name);
      }
   }
   sort(_imageFileList.begin(), _imageFileList.end());
}

void RGSlideShow::step()
{
   _currentStep += 1;
   refresh();
}

void RGSlideShow::refresh()
{
   if (!_imageFileList.empty()) {
      int current = 0;
      if (_totalSteps) {
	 float stepping = (_totalSteps / (float)_imageFileList.size());
	 current = (int)((_currentStep + (stepping - 1) / 2) / stepping);
	 if (current >= _imageFileList.size())
	    current = _imageFileList.size() - 1;
      }
      gtk_image_set_from_file(_image, _imageFileList[current].c_str());
   }
}

// vim:sts=3:sw=3
