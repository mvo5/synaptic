/* rsource_deb822.h - Deb822 format sources support
 * 
 * Copyright (c) 2025 Synaptic development team          
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License as 
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */

#ifndef RSOURCE_DEB822_H
#define RSOURCE_DEB822_H

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <apt-pkg/error.h>
#include <apt-pkg/sourcelist.h>
#include <apt-pkg/strutl.h>
#include "rsources.h"

class RDeb822Source {
public:
    struct Deb822Entry {
        std::string Types;      // Space-separated list of types
        std::string URIs;       // Space-separated list of URIs
        std::string Suites;     // Space-separated list of suites
        std::string Components; // Space-separated list of components
        std::string SignedBy;   // Path to keyring file
        std::string Architectures; // Space-separated list of architectures
        std::string Languages;  // Space-separated list of languages
        std::string Targets;    // Space-separated list of targets
        bool Enabled;           // Whether the source is enabled
        std::string Comment;    // Any comments associated with this entry
    };

    static bool ParseDeb822File(const std::string& path, std::vector<Deb822Entry>& entries);
    static bool WriteDeb822File(const std::string& path, const std::vector<Deb822Entry>& entries);
    static bool ConvertToSourceRecord(const Deb822Entry& entry, SourcesList::SourceRecord& record);
    static bool ConvertFromSourceRecord(const SourcesList::SourceRecord& record, Deb822Entry& entry);

private:
    static bool ParseStanza(std::wifstream& file, std::map<std::wstring, std::wstring>& fields);
    static void TrimWhitespace(std::string& str);
};

#endif // RSOURCE_DEB822_H 