
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

   std::string RipComponent(std::string Path);
   std::string RipDirectory(std::string Path);
   std::string RipDistro(std::string Path);

   void ConvertToSourceList(std::string CD, std::string &Path);

 public:

   bool CopyPackages(std::string CDROM, 
                     std::string Name, 
                     std::vector<std::string> &List);
};


class RPMPackageCopy:public RPMIndexCopy {
};

class RPMSourceCopy:public RPMIndexCopy {
};

#endif
