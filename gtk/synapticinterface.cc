/* synapticInterface.cc - SWIG interface for synaptic
 * 
 * Copyright (c) 2002 Michael Vogt <mvo@debian.org>
 * 
 * Author: Michael Vogt <mvo@debian.org>
 *         Gustavo Niemeyer <niemeyer@conectiva.com>
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

#include <iostream>
#include "config.h"
#include "rgmainwindow.h"
#include "synapticinterface.h"
#include "rconfiguration.h"
#include "raptoptions.h"
#include "gsynaptic.h"

using namespace std;

SynapticInterface::SynapticInterface()
{
    if (!RInitConfiguration("synaptic.conf")) {
      cerr << "RInitConfiguration() failed\n";
      exit(1);
    }
    _roptions->restore();
 
    packageLister = new RPackageLister();
    mainWindow = new RGMainWindow(packageLister);

    mainWindow->setTitle(PACKAGE" "VERSION);
    mainWindow->show();
    RGFlushInterface();
    mainWindow->setInterfaceLocked(true);
    
    if (!packageLister->openCache(false)) {
	mainWindow->showErrors();
	exit(1);
    }
    mainWindow->setInterfaceLocked(false);
    mainWindow->restoreState();
    mainWindow->showErrors();
}

void SynapticInterface::update()
{
  //cout << "SynapticInterface::update()" << endl;
  mainWindow->updateClicked(NULL, mainWindow);
}

void SynapticInterface::upgrade() 
{
  //cout << "SynapticInterface::upgrade() " << endl;
  mainWindow->upgradeClicked(NULL, mainWindow);
}

void SynapticInterface::distUpgrade()
{
  //cout << "SynapticInterface::distUpgrade()" << endl;
  mainWindow->distUpgradeClicked(NULL, mainWindow);
}

void SynapticInterface::selectFilter(int i)
{
  //cout << "SynapticInterface::selectFilter(" << i << ")" << endl;
  mainWindow->changeFilter(i);
}

void SynapticInterface::proceed()
{
  //cout << "SynapticInterface::proceed()" << endl;
  mainWindow->proceedClicked(NULL, mainWindow);
}
