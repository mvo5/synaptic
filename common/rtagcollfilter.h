#ifndef RTAGCOLFILTER_H
#define RTAGCOLFILTER_H

#ifdef HAVE_DEBTAG

/*
 * Represent a list of tag substitutions to apply as a TagcollFilter
 *
 * Copyright (C) 2003  Enrico Zini <enrico@debian.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 */

//#pragma interface

#include <TagcollConsumer.h>
#include <TagcollFilter.h>

#include <map>
#include <string>
#include "rpackage.h"
#include "rpackagelister.h"

class RTagcollFilter:public TagcollFilter<std::string> {
 protected:
   RPackageLister *_lister;

 public:
   RTagcollFilter(RPackageLister *lister) throw()
 :   _lister(lister) {
   }

   virtual void consume(const std::string &item) throw() {
      // make sure that only elements we know about are displayed
      // this should really use getElementInDisplayList
      if (_lister->getElement(item) != NULL)
         consumer->consume(item);
   }

   virtual void consume(const std::string &item,
                        const OpSet<std::string> &tags) throw() {
      // make sure that only elements we know about are displayed
      // this should really use getElementInDisplayList
      if (_lister->getElement(item) != NULL)
         consumer->consume(item, tags);
   }
};

// vim:set ts=4 sw=4:

#endif // HAVE_DEBTAGS
#endif
