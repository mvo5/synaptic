// pkg_acqfile.cc
//
//  Copyright 2002 Daniel Burrows
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; see the file COPYING.  If not, write to
//  the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
//  Boston, MA 02111-1307, USA.

// (based on pkg_changelog)
//
// mvo: taken from aptitude with a big _thankyou_ 

#include "pkg_acqfile.h"

#include "config.h"
#include "i18n.h"

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>

#include <apt-pkg/error.h>
#include <apt-pkg/configuration.h>
#include <apt-pkg/acquire-item.h>
#include <apt-pkg/sourcelist.h>
#include <apt-pkg/strutl.h>
#include <apt-pkg/pkgrecords.h>
#include <apt-pkg/fileutl.h>
#include <apt-pkg/indexfile.h>

#if APT_PKG_MAJOR < 5

// Let's all sing a song about apt-pkg's brokenness..
pkgAcqFileSane::pkgAcqFileSane(pkgAcquire *Owner,
                               std::string URI,
                               HashStringList const &hsl,
                               unsigned long long const Size,
                               std::string Description,
                               std::string ShortDesc,
                               std::string const &DestDir,
                               std::string const &filename)
 : Item(Owner)
{
  Retries=_config->FindI("Acquire::Retries",0);
  DestFile=filename;

  Desc.URI=URI;
  Desc.Description=Description;
  Desc.Owner=this;
  Desc.ShortDesc=ShortDesc;

  QueueURI(Desc);
}

// Straight from acquire-item.cc
/* Here we try other sources */
void pkgAcqFileSane::Failed(std::string Message,pkgAcquire::MethodConfig *Cnf)
{
  ErrorText = LookupTag(Message,"Message");

  // This is the retry counter
  if (Retries != 0 &&
      Cnf->LocalOnly == false &&
      StringToBool(LookupTag(Message,"Transient-Failure"),false) == true)
    {
      Retries--;
      QueueURI(Desc);
      return;
    }

  Item::Failed(Message,Cnf);
}

#endif
