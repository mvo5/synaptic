/* rconfiguration.h
 *
 * Copyright (c) 2000, 2001 Conectiva S/A
 *               2002 Michael Vogt <mvo@debian.org>
 *
 * Author: Alfredo K. Kojima <kojima@conectiva.com.br>
 *         Michael Vogt <mvo@debian.org>
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


#ifndef _RCONFIGURATION_H_
#define _RCONFIGURATION_H_

#include <string>
#include <fstream>

using namespace std;

class Configuration;

bool RWriteConfigFile(Configuration &Conf);

bool RInitConfiguration(string confFileName);

bool RReadFilterData(Configuration &config);
bool RFilterDataOutFile(ofstream &out);

bool RPackageOptionsFile(ofstream &out);
bool RPackageOptionsFile(ifstream &in);


// get the default conf dir
string RConfDir();

// this needs to be a safe tmp dir (like /root/.synaptic/tmp) to store
// small files like changelogs or pinfiles (preferences)
string RTmpDir();

// state dir - we store the locked packages (preferences file) here
//             possible other stuff in the future
string RStateDir();

// we store the commit history here
string RLogDir();

#endif
