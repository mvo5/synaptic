
/* ######################################################################

   Index Copying - Aid for copying and verifying the index files
   
   ##################################################################### 
 */

#ifndef RPMINDEXCOPY_H
#define RPMINDEXCOPY_H

#include <vector>
#include <string>

class pkgTagSection;
class FileFd;

class RPMIndexCopy {
 protected:

   string RipComponent(string Path);
   string RipDirectory(string Path);
   string RipDistro(string Path);

   void ConvertToSourceList(string CD, string &Path);

 public:

   bool CopyPackages(string CDROM, string Name, vector<string> &List);
};


class RPMPackageCopy:public RPMIndexCopy {
};

class RPMSourceCopy:public RPMIndexCopy {
};

#endif
