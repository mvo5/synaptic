
#include <apt-pkg/error.h>
#include <apt-pkg/progress.h>
#include <apt-pkg/strutl.h>
#include <apt-pkg/fileutl.h>
#include <apt-pkg/configuration.h>
#include <apt-pkg/tagfile.h>

#include <iostream>
#include <map>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>

#include "rpmindexcopy.h"

#include "i18n.h"

using namespace std;

string RPMIndexCopy::RipComponent(string Path)
{
   const char *begin;
   const char *end;

   end = strrchr(Path.c_str(), '.');
   begin = strchr(strrchr(Path.c_str(), '/'), '.') + 1;
   if (begin < strrchr(end, '/'))
      return string(end + 1);

   return string(begin, end);
}


string RPMIndexCopy::RipDistro(string Path)
{
   return string(Path, 0, Path.find("base") - 1);
}


string RPMIndexCopy::RipDirectory(string Path)
{
   return string(Path, 0, Path.rfind('/'));
}

#if 0
static int strrcmp_(const char *a, const char *b)
{
   int la = strlen(a);
   int lb = strlen(b);

   if (la == 0 || lb == 0)
      return 0;

   if (la > lb)
      return strcmp(&a[la - lb], b);
   else
      return strcmp(&b[lb - la], a);
}
#endif

bool RPMIndexCopy::CopyPackages(string CDROM, string Name,
                                vector<string> &List)
{
   OpTextProgress Progress;

   if (List.size() == 0)
      return true;

   // Prepare the progress indicator
   unsigned long TotalSize = 0;
   for (vector<string>::iterator I = List.begin(); I != List.end(); I++) {
      struct stat Buf;
      if (stat((*I).c_str(), &Buf) != 0)
         return _error->Errno("stat", _("Stat failed for %s"), (*I).c_str());
      TotalSize += Buf.st_size;
   }

   unsigned long CurrentSize = 0;

   // Keep track of global release processing
   map<string, bool> GlobalReleases;

   for (vector<string>::iterator I = List.begin(); I != List.end(); I++) {
      string OrigPath = string(*I, CDROM.length());
      unsigned long FileSize = 0;

      // Open the package file
      FileFd Pkg;
      string File = *I;

      if (strcmp(File.c_str() + File.length() - 4, ".bz2") == 0)
         File = string(File, 0, File.length() - 4);

      if (FileExists(File) == true) {
         Pkg.Open(File, FileFd::ReadOnly);
         FileSize = Pkg.Size();
      } else {
         FileFd From(*I, FileFd::ReadOnly);
         if (_error->PendingError() == true)
            return false;
         FileSize = From.Size();

         // Get a temp file
         FILE *tmp = tmpfile();
         if (tmp == 0)
            return _error->Errno("tmpfile", _("Unable to create a tmp file"));
         Pkg.Fd(dup(fileno(tmp)));
         fclose(tmp);

         // Fork bzip2
         int Process = fork();
         if (Process < 0)
            return _error->Errno("fork",
                                 "Internal Error: couldn't fork bzip2. Please report.");

         // The child
         if (Process == 0) {
            dup2(From.Fd(), STDIN_FILENO);
            dup2(Pkg.Fd(), STDOUT_FILENO);
            SetCloseExec(STDIN_FILENO, false);
            SetCloseExec(STDOUT_FILENO, false);

            const char *Args[3];
            Args[0] = _config->Find("Dir::Bin::bzip2", "bzip2").c_str();
            Args[1] = "-d";
            Args[2] = 0;
            execvp(Args[0], (char **)Args);
            exit(100);
         }
         // Wait for gzip to finish
         if (ExecWait
             (Process, _config->Find("Dir::Bin::bzip2", "bzip2").c_str(),
              false) == false)
            return _error->Error(_("bzip2 failed, perhaps the disk is full."));

         Pkg.Seek(0);
      }
      if (_error->PendingError() == true)
         return false;

      // Open the output file
      char S[400];
      sprintf(S, "cdrom:[%s]/%s", Name.c_str(), File.c_str() + CDROM.length());
      string TargetF = _config->FindDir("Dir::State::lists") + "partial/";
      TargetF += URItoFileName(S);
      if (_config->FindB("APT::CDROM::NoAct", false) == true)
         TargetF = "/dev/null";
      FileFd Target(TargetF, FileFd::WriteEmpty);
      if (_error->PendingError() == true)
         return false;

      // Setup the progress meter
      Progress.OverallProgress(CurrentSize, TotalSize, FileSize,
                               string("Reading Indexes"));

      // Parse
      Progress.SubProgress(Pkg.Size());

      if (!CopyFile(Pkg, Target))
         return false;

      if (_config->FindB("APT::CDROM::NoAct", false) == false) {
         // Move out of the partial directory
         Target.Close();
         string FinalF = _config->FindDir("Dir::State::lists");
         FinalF += URItoFileName(S);
         if (rename(TargetF.c_str(), FinalF.c_str()) != 0)
            return _error->Errno("rename", _("Failed to rename"));

         // Two release steps, one for global, one for component
         string release = "release";
         for (int Step = 0; Step != 2; Step++) {
            if (Step == 0) {
               if (GlobalReleases.find(*I) != GlobalReleases.end())
                  continue;
               GlobalReleases[*I] = true;
            } else
               release += "." + RipComponent(*I);

            // Copy the component release file
            sprintf(S, "cdrom:[%s]/%s/%s", Name.c_str(),
                    RipDirectory(*I).c_str() + CDROM.length(),
                    release.c_str());
            string TargetF =
               _config->FindDir("Dir::State::lists") + "partial/";
            TargetF += URItoFileName(S);
            if (FileExists(RipDirectory(*I) + release) == true) {
               FileFd Target(TargetF, FileFd::WriteEmpty);
               FileFd Rel(RipDirectory(*I) + release, FileFd::ReadOnly);
               if (_error->PendingError() == true)
                  return false;

               if (CopyFile(Rel, Target) == false)
                  return false;
            } else {
               // Empty release file
               FileFd Target(TargetF, FileFd::WriteEmpty);
            }

            // Rename the release file
            FinalF = _config->FindDir("Dir::State::lists");
            FinalF += URItoFileName(S);
            if (rename(TargetF.c_str(), FinalF.c_str()) != 0)
               return _error->Errno("rename", _("Failed to rename"));
         }
      }

      string Prefix = "";
      /* Mangle the source to be in the proper notation with
         prefix dist [component] */
//      *I = string(*I,Prefix.length());
      ConvertToSourceList(CDROM, *I);
      *I = Prefix + ' ' + *I;

      CurrentSize += FileSize;
   }
   Progress.Done();

   return true;
}




void RPMIndexCopy::ConvertToSourceList(string CD, string &Path)
{
   Path = string(Path, CD.length());

   Path = RipDistro(Path) + " " + RipComponent(Path);
}

// vim:sts=3:sw=3
