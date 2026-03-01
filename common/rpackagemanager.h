/* rpackagemanager.h
 *
 * Copyright (c) 2000, 2001 Conectiva S/A
 *                     2002 Michael Vogt
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

#ifndef RPACKAGEMANAGER_H
#define RPACKAGEMANAGER_H

#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <apt-pkg/pkgcache.h>
#include <apt-pkg/sourcelist.h>

// We need a different package manager, since we need to do the
// DoInstall() process in two steps when forking. Without that,
// the forked package manager would be updated with the new
// information, and the original Synaptic process wouldn't be
// aware about it.
//
// Very unfortunately, we *do* need to access stuff which is
// declared as protected in the packagemanager.h header. To do
// that, we need this awful hack. Make sure to include all headers
// included by packagemanager.h before the hack, so that we reduce
// the impact of it. In the future, we must ask the APT maintainers
// to export the functionality we need, so that we may avoid this
// ugly hack.

#define protected public
#include <apt-pkg/packagemanager.h>
#undef protected

class RDeb822Source {
public:
    RDeb822Source();
    RDeb822Source(const std::string& types, const std::string& uris,
                  const std::string& suites, const std::string& components = "");

    bool isValid() const;
    std::string toString() const;
    static RDeb822Source fromString(const std::string& content);
    bool operator==(const RDeb822Source& other) const;
    bool operator!=(const RDeb822Source& other) const;

    std::string getTypes() const { return types; }
    std::string getUris() const { return uris; }
    std::string getSuites() const { return suites; }
    std::string getComponents() const { return components; }
    std::string getSignedBy() const { return signedBy; }
    bool isEnabled() const { return enabled; }

    void setTypes(const std::string& t) { types = t; }
    void setUris(const std::string& u) { uris = u; }
    void setSuites(const std::string& s) { suites = s; }
    void setComponents(const std::string& c) { components = c; }
    void setSignedBy(const std::string& s) { signedBy = s; }

    static std::string trim(const std::string& str);

private:
    std::string types;
    std::string uris;
    std::string suites;
    std::string components;
    std::string signedBy;
    bool enabled;
};

class RSourceManager {
public:
    RSourceManager();
    explicit RSourceManager(const std::string& sourcesDir);

    bool addSource(const RDeb822Source& source);
    bool removeSource(const RDeb822Source& source);
    bool updateSource(const RDeb822Source& oldSource, const RDeb822Source& newSource);
    std::vector<RDeb822Source> getSources() const;
    bool loadSources();
    bool saveSources() const;
    bool updateAptSources();
    bool reloadAptCache();
    bool validateSourceFile(const std::string& filename) const;

private:
    std::string sourcesDir;
    std::vector<RDeb822Source> sources;

    std::vector<RDeb822Source> parseSources(const std::string& content);
    RDeb822Source createSourceFromFields(const std::map<std::string, std::string>& fields);
    bool writeSourceFile(const std::string& filename, const RDeb822Source& source) const;
    RDeb822Source readSourceFile(const std::string& filename) const;
    std::string getSourceFilename(const RDeb822Source& source) const;
    std::string trim(const std::string& str) const;
};

class RPackageManager {
protected:
    pkgPackageManager::OrderResult Res;

public:
    pkgPackageManager *pm;

    pkgPackageManager::OrderResult DoInstallPreFork() {
        Res = pm->OrderInstall();
        return Res;
    }

#ifdef WITH_DPKG_STATUSFD
    pkgPackageManager::OrderResult DoInstallPostFork(int statusFd=-1) {
        return (pm->Go(statusFd) == false) ? pkgPackageManager::Failed : Res;
    }
#else
    pkgPackageManager::OrderResult DoInstallPostFork() {
        if (pm == NULL)
            return pkgPackageManager::Failed;
        return (pm->Go(NULL) == false) ? pkgPackageManager::Failed : Res;
    }
#endif

    RPackageManager(pkgPackageManager *pm) : pm(pm) {}
};

#endif // RPACKAGEMANAGER_H

// vim:ts=3:sw=3:et
