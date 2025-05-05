/* rsource_deb822.h - Deb822 format sources support
 * 
 * Copyright (c) 2025 Synaptic development team          
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License as 
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 */

#ifndef _RSOURCE_DEB822_H
#define _RSOURCE_DEB822_H

#include "rsources.h"
#include <string>
#include <vector>
#include <map>

class RDeb822Source {
public:
    struct Deb822Entry {
        std::string Types;        // Can be "deb" and/or "deb-src"
        std::string URIs;         // Space-separated list of URIs
        std::string Suites;       // Space-separated list of suites
        std::string Components;   // Space-separated list of components
        std::string SignedBy;     // Path to keyring file
        bool Enabled;             // Whether the source is enabled
        std::string Comment;      // Any comments associated with this entry
    };

    static bool ParseDeb822File(const std::string& path, std::vector<Deb822Entry>& entries);
    static bool WriteDeb822File(const std::string& path, const std::vector<Deb822Entry>& entries);
    static bool ConvertToSourceRecord(const Deb822Entry& entry, SourcesList::SourceRecord& record);
    static bool ConvertFromSourceRecord(const SourcesList::SourceRecord& record, Deb822Entry& entry);

private:
    static bool ParseStanza(std::istream& input, std::map<std::string, std::string>& fields);
    static void TrimWhitespace(std::string& str);
};

#endif // _RSOURCE_DEB822_H 