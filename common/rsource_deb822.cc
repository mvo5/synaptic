/* rsource_deb822.cc - Deb822 format sources support
 * 
 * Copyright (c) 2025 Synaptic development team          
 */

#include "rsource_deb822.h"
#include <apt-pkg/configuration.h>
#include <apt-pkg/fileutl.h>
#include <apt-pkg/strutl.h>
#include <fstream>
#include <sstream>
#include <locale>
#include <codecvt>
#include "i18n.h"
#include <apt-pkg/error.h>
#include <iostream>
#include <algorithm>
#include "rsources.h"  // Add this for SourcesList::SourceRecord

bool RDeb822Source::ParseDeb822File(const std::string& path, std::vector<Deb822Entry>& entries) {
    std::ifstream file(path);
    if (!file.is_open()) {
        _error->Error(_("Could not open file %s"), path.c_str());
        return false;
    }

    std::map<std::string, std::string> fields;
    while (ParseStanza(file, fields)) {
        Deb822Entry entry;
        entry.Types = fields["Types"];
        entry.URIs = fields["URIs"];
        entry.Suites = fields["Suites"];
        if (fields.find("Components") != fields.end()) {
            entry.Components = fields["Components"];
        }
        if (fields.find("Signed-By") != fields.end()) {
            entry.SignedBy = fields["Signed-By"];
        }
        if (fields.find("Architectures") != fields.end()) {
            entry.Architectures = fields["Architectures"];
        }
        if (fields.find("Languages") != fields.end()) {
            entry.Languages = fields["Languages"];
        }
        if (fields.find("Targets") != fields.end()) {
            entry.Targets = fields["Targets"];
        }
        entry.Enabled = true;
        entries.push_back(entry);
    }

    return true;
}

bool RDeb822Source::WriteDeb822File(const std::string& path, const std::vector<Deb822Entry>& entries) {
    std::ofstream file(path);
    if (!file.is_open()) {
        _error->Error(_("Could not open file %s for writing"), path.c_str());
        return false;
    }

    for (const auto& entry : entries) {
        file << "Types: " << entry.Types << std::endl;
        file << "URIs: " << entry.URIs << std::endl;
        file << "Suites: " << entry.Suites << std::endl;
        if (!entry.Components.empty()) {
            file << "Components: " << entry.Components << std::endl;
        }
        if (!entry.SignedBy.empty()) {
            file << "Signed-By: " << entry.SignedBy << std::endl;
        }
        if (!entry.Architectures.empty()) {
            file << "Architectures: " << entry.Architectures << std::endl;
        }
        if (!entry.Languages.empty()) {
            file << "Languages: " << entry.Languages << std::endl;
        }
        if (!entry.Targets.empty()) {
            file << "Targets: " << entry.Targets << std::endl;
        }
        file << std::endl;
    }

    return true;
}

bool RDeb822Source::ConvertToSourceRecord(const Deb822Entry& entry, SourcesList::SourceRecord& record) {
    // Parse types
    std::istringstream typeStream(entry.Types);
    std::string type;
    record.Type = 0;
    
    while (std::getline(typeStream, type, ' ')) {
        TrimWhitespace(type);
        if (type == "deb") record.Type |= SourcesList::Deb;
        if (type == "deb-src") record.Type |= SourcesList::DebSrc;
    }
    
    if (!entry.Enabled) record.Type |= SourcesList::Disabled;
    record.Type |= SourcesList::Deb822;  // Mark as Deb822 format

    // Parse URIs
    std::istringstream uriStream(entry.URIs);
    std::string uri;
    while (std::getline(uriStream, uri, ' ')) {
        TrimWhitespace(uri);
        if (!uri.empty()) {
            record.URI = uri;
            break;
        }
    }

    // Parse suites
    std::istringstream suiteStream(entry.Suites);
    std::string suite;
    while (std::getline(suiteStream, suite, ' ')) {
        TrimWhitespace(suite);
        if (!suite.empty()) {
            record.Dist = suite;
            break;
        }
    }

    // Parse components
    std::istringstream compStream(entry.Components);
    std::string comp;
    record.Comps.clear();
    while (std::getline(compStream, comp, ' ')) {
        TrimWhitespace(comp);
        if (!comp.empty()) {
            record.Comps.push_back(comp);
        }
    }
    
    return true;
}

bool RDeb822Source::ConvertFromSourceRecord(const SourcesList::SourceRecord& record, Deb822Entry& entry) {
    // Set types
    std::stringstream typeStream;
    if (record.Type & SourcesList::Deb) {
        typeStream << "deb ";
    }
    if (record.Type & SourcesList::DebSrc) {
        typeStream << "deb-src ";
    }
    entry.Types = typeStream.str();
    TrimWhitespace(entry.Types);

    // Set URI
    entry.URIs = record.URI;
    
    // Set suite
    entry.Suites = record.Dist;
    
    // Set components
    std::stringstream compStream;
    for (const auto& comp : record.Comps) {
        compStream << comp << " ";
    }
    entry.Components = compStream.str();
    TrimWhitespace(entry.Components);
    
    // Set enabled state
    entry.Enabled = !(record.Type & SourcesList::Disabled);
    
    return true;
}

void RDeb822Source::TrimWhitespace(std::string& str) {
    const std::string whitespace = " \t\r\n";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        str.clear();
        return;
    }
    size_t end = str.find_last_not_of(whitespace);
    str = str.substr(start, end - start + 1);
}

bool RDeb822Source::ParseStanza(std::ifstream& file, std::map<std::string, std::string>& fields) {
    std::string line;
    bool inStanza = false;
    
    while (std::getline(file, line)) {
        // Skip empty lines
        if (line.empty()) {
            if (inStanza) {
                return true;
            }
            continue;
        }

        // Skip comments
        if (line[0] == '#') {
            continue;
        }

        // Check for stanza start
        if (line.find("Types:") != std::string::npos) {
            inStanza = true;
        }

        if (inStanza) {
            size_t colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                std::string key = line.substr(0, colonPos);
                std::string value = line.substr(colonPos + 1);
                
                // Trim whitespace
                key.erase(0, key.find_first_not_of(" \t"));
                key.erase(key.find_last_not_of(" \t") + 1);
                value.erase(0, value.find_first_not_of(" \t"));
                value.erase(value.find_last_not_of(" \t") + 1);
                
                fields[key] = value;
            }
        }
    }
    
    return !fields.empty();
} 