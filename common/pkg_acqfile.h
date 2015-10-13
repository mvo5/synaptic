// pkg_acqfile.h
//

#include <apt-pkg/acquire-item.h>

// new APT has a proper pkgAcqFile so all good
#if APT_PKG_MAJOR >= 5
 #define pkgAcqFileSane pkgAcqFile
#else
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

#endif
