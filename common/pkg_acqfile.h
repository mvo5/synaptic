// pkg_acqfile.h
//

#ifndef _COMMON_PKG_ACQFILE_H
#define _COMMON_PKG_ACQFILE_H

#include "config.h"

#include <apt-pkg/macros.h>   // Provides the definition of APT_PKG_MAJOR.

#if APT_PKG_MAJOR >= 5

// new APT has a proper pkgAcqFile so all good
#include <apt-pkg/acquire-item.h>
#define pkgAcqFileSane pkgAcqFile

#else

#include <apt-pkg/acquire.h>
#include <apt-pkg/acquire-item.h>

#include <string>

class pkgAcqFileSane : public pkgAcquire::Item
// This is frustrating: pkgAcqFile is **almost** good enough, but has some
// hardcoded stuff that makes it not quite work.
//
//  Based heavily on that class, though.
{
  pkgAcquire::ItemDesc Desc;
  std::string Md5Hash;
  unsigned int Retries;

public:
  pkgAcqFileSane(pkgAcquire *Owner,
                 std::string URI,
                 HashStringList const &hsl,
                 unsigned long long const Size,
                 std::string Description,
                 std::string ShortDesc,
                 std::string const &DestDir,
                 std::string const &DestFilename);

  void Failed(std::string Message, pkgAcquire::MethodConfig *Cnf);
  std::string MD5Sum() {return Md5Hash;}
  std::string DescURI() {return Desc.URI;}
  virtual ~pkgAcqFileSane() {}
};

#endif // APT_PKG_MAJOR >= 5

#endif
