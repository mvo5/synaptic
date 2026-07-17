/* ######################################################################

   Index Copying - Aid for copying and verifying the index files

   #####################################################################
 */

#pragma once

#include "config.h" // IWYU pragma: associated

#include <string>
#include <vector>

class RPMIndexCopy
{
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


class RPMPackageCopy : public RPMIndexCopy
{};

class RPMSourceCopy : public RPMIndexCopy
{};
