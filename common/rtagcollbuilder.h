#ifndef RTAGCOLL_BUILDER_H
#define RTAGCOLL_BUILDER_H

/*
 * TagcollConsumer that builds a tagged collection
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

#if 0 // PORTME
#ifdef HAVE_DEBTAGS
//#pragma interface

#include <HandleMaker.h>
#include <TagcollConsumer.h>
#include <TagCollection.h>
#include "rpackage.h"
#include "rpackagelister.h"

// TagcollConsumer that builds a tagged collection for synaptic
class RTagcollBuilder:public TagcollConsumer<std::string> {
 protected:
   HandleMaker<RPackage *> &handleMaker;
   TagCollection<int> coll;
   RPackageLister *_lister;

   int getHandle(const std::string &str) throw() {
      RPackage *pkg = _lister->getElement(str);
      return handleMaker.getHandle(pkg);
   }

   OpSet<int> itemsToHandles(const OpSet<std::string> &ts) throw() {
      OpSet<int> res;
      for (OpSet<std::string>::const_iterator i = ts.begin();
           i != ts.end(); i++)
         res += getHandle(*i);
      return res;
   }

 public:
   RTagcollBuilder(HandleMaker<RPackage *> &handleMaker,
                   RPackageLister *l) throw()
 :   handleMaker(handleMaker), _lister(l) {
   }
   virtual ~ RTagcollBuilder()throw() {
   }

   virtual void consume(const std::string &item) throw() {
      coll.add(getHandle(item));
   }

   virtual void consume(const std::string &item,
                        const OpSet<std::string> &tags) throw() {
      coll.add(tags, getHandle(item));
   }

   virtual void consume(const OpSet<std::string> &items) throw() {
      coll.add(itemsToHandles(items));
   }

   virtual void consume(const OpSet<std::string> &items,
                        const OpSet<std::string> &tags) throw() {
      coll.add(tags, itemsToHandles(items));
   }

   // Retrieve the resulting collection
   TagCollection<int> collection() throw() {
      return coll;
   }
   const TagCollection<int> collection() const throw() {
      return coll;
   }
};

#endif //HAVE_DEBTAGS

// vim:set ts=4 sw=4:
#endif
#endif
