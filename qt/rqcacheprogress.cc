/* rgcacheprogress.cc
 *
 * Copyright (c) 2000, 2001 Conectiva S/A
 *
 * Author: Alfredo K. Kojima <kojima@conectiva.com.br>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA
 */

#include <rqcacheprogress.h>
#include <qstatusbar.h>
#include <qapplication.h>

RQCacheProgress::RQCacheProgress(QMainWindow *main)
   : _main(main)
{
   _prog = new QProgressBar(_main);
   _prog->setMaximumWidth(150);
   _prog->setTotalSteps(100);
   _prog->show();
   QStatusBar *status = _main->statusBar();
   status->addWidget(_prog, 0, true);
   status->message(Op.c_str());
}


RQCacheProgress::~RQCacheProgress()
{
   //delete _prog;
}


void RQCacheProgress::Update()
{
   if (!CheckChange()) {
      qApp->processEvents();
      return;
   }

   if (!_prog->isVisible()) {
      _prog->show();
      qApp->processEvents();
   }

   if (MajorChange)
      _main->statusBar()->message(Op.c_str());
   
   _prog->setProgress((int)Percent);
   qApp->processEvents();
}


void RQCacheProgress::Done()
{
   _prog->setProgress(100);
   qApp->processEvents();
   _prog->hide();
   _main->statusBar()->clear();
}

// vim:ts=3:sw=3:et
