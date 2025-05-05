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

#include <string>
#include <apt-pkg/pkgcache.h>
#include <vector>
#include <map>
#include <apt-pkg/sourcelist.h>

#define protected public
#include <apt-pkg/packagemanager.h>
#undef protected

#ifndef RPACKAGEMANAGER_H
#define RPACKAGEMANAGER_H

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
      return (pm->Go() == false) ? pkgPackageManager::Failed : Res;
   }
#endif

   RPackageManager(pkgPackageManager *pm) : pm(pm) {}

};

/**
 * @class RDeb822Source
 * @brief Represents a Deb822 format source entry
 *
 * This class handles the parsing, validation, and serialization of Deb822 format
 * source entries. It supports multiple URIs and suites in a single source, as well
 * as source enabling/disabling.
 *
 * Example usage:
 * @code
 * RDeb822Source source;
 * source.setTypes("deb");
 * source.setUris("http://example.com");
 * source.setSuites("stable");
 * source.setComponents("main");
 * if (source.isValid()) {
 *     // Use the source
 * }
 * @endcode
 */
class RDeb822Source {
public:
    RDeb822Source();
    RDeb822Source(const std::string& types, const std::string& uris, 
                  const std::string& suites, const std::string& components = "");
    
    // Getters
    std::string getTypes() const { return types; }
    std::string getUris() const { return uris; }
    std::string getSuites() const { return suites; }
    std::string getComponents() const { return components; }
    bool isEnabled() const { return enabled; }
    
    // Setters
    void setTypes(const std::string& t) { types = t; }
    void setUris(const std::string& u) { uris = u; }
    void setSuites(const std::string& s) { suites = s; }
    void setComponents(const std::string& c) { components = c; }
    void setEnabled(bool e) { enabled = e; }
    
    /**
     * @brief Validates the source entry
     * @return true if the source is valid, false otherwise
     *
     * Checks:
     * - Required fields (types, uris, suites) are not empty
     * - Type is either "deb" or "deb-src"
     * - At least one URI has a valid scheme
     * - At least one suite is specified
     */
    bool isValid() const;
    
    /**
     * @brief Converts the source to its string representation
     * @return String representation of the source
     */
    std::string toString() const;
    
    /**
     * @brief Creates a source from its string representation
     * @param content The string content to parse
     * @return A new RDeb822Source instance
     */
    static RDeb822Source fromString(const std::string& content);
    
    // Comparison operators
    bool operator==(const RDeb822Source& other) const;
    bool operator!=(const RDeb822Source& other) const;

private:
    std::string types;
    std::string uris;
    std::string suites;
    std::string components;
    bool enabled;
};

/**
 * @class RSourceManager
 * @brief Manages Deb822 format source files
 *
 * This class handles the management of source files in the sources.list.d
 * directory, including reading, writing, and validating source files.
 *
 * Example usage:
 * @code
 * RSourceManager manager("/etc/apt/sources.list.d");
 * RDeb822Source source = manager.readSourceFile("example.sources");
 * if (source.isValid()) {
 *     manager.addSource(source);
 *     manager.saveSources();
 * }
 * @endcode
 */
class RSourceManager {
public:
    RSourceManager();
    explicit RSourceManager(const std::string& sourcesDir);
    
    /**
     * @brief Adds a new source
     * @param source The source to add
     * @return true if added successfully, false if invalid or duplicate
     */
    bool addSource(const RDeb822Source& source);
    
    /**
     * @brief Removes a source
     * @param source The source to remove
     * @return true if removed successfully, false if not found
     */
    bool removeSource(const RDeb822Source& source);
    
    /**
     * @brief Updates an existing source
     * @param oldSource The source to update
     * @param newSource The new source data
     * @return true if updated successfully, false if invalid or not found
     */
    bool updateSource(const RDeb822Source& oldSource, const RDeb822Source& newSource);
    
    /**
     * @brief Gets all managed sources
     * @return Vector of sources
     */
    std::vector<RDeb822Source> getSources() const;
    
    /**
     * @brief Loads sources from the sources directory
     * @return true if loaded successfully
     */
    bool loadSources();
    
    /**
     * @brief Saves all sources to files
     * @return true if saved successfully
     */
    bool saveSources() const;
    
    /**
     * @brief Writes a source to a file
     * @param filename The target file
     * @param source The source to write
     * @return true if written successfully
     */
    bool writeSourceFile(const std::string& filename, const RDeb822Source& source) const;
    
    /**
     * @brief Reads a source from a file
     * @param filename The source file
     * @return The read source
     */
    RDeb822Source readSourceFile(const std::string& filename) const;
    
    /**
     * @brief Updates APT sources
     * @return true if updated successfully
     */
    bool updateAptSources();
    
    /**
     * @brief Reloads the APT cache
     * @return true if reloaded successfully
     */
    bool reloadAptCache();

private:
    std::vector<RDeb822Source> sources;
    std::string sourcesDir;
    
    bool validateSourceFile(const std::string& filename) const;
    std::string getSourceFilename(const RDeb822Source& source) const;
};

#endif

// vim:ts=3:sw=3:et
