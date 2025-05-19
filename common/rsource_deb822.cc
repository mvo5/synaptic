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

bool RDeb822Source::ParseDeb822File(const std::string& path, std::vector<Deb822Entry>& entries) {
    std::wifstream file(path);
    if (!file) {
        return _error->Error(_("Cannot open %s"), path.c_str());
    }

    std::map<std::wstring, std::wstring> fields;
    while (ParseStanza(file, fields)) {
        Deb822Entry entry;
        
        // Check required fields
        if (fields.find(L"Types") == fields.end()) {
            return _error->Error(_("Missing Types field in %s"), path.c_str());
        }
        entry.Types = APT::String::FromUTF8(fields[L"Types"]);
        
        if (fields.find(L"URIs") == fields.end()) {
            return _error->Error(_("Missing URIs field in %s"), path.c_str());
        }
        entry.URIs = APT::String::FromUTF8(fields[L"URIs"]);
        
        if (fields.find(L"Suites") == fields.end()) {
            return _error->Error(_("Missing Suites field in %s"), path.c_str());
        }
        entry.Suites = APT::String::FromUTF8(fields[L"Suites"]);
        
        // Optional fields
        if (fields.find(L"Components") != fields.end()) {
            entry.Components = APT::String::FromUTF8(fields[L"Components"]);
        }
        if (fields.find(L"Signed-By") != fields.end()) {
            entry.SignedBy = APT::String::FromUTF8(fields[L"Signed-By"]);
        }
        if (fields.find(L"Architectures") != fields.end()) {
            entry.Architectures = APT::String::FromUTF8(fields[L"Architectures"]);
        }
        if (fields.find(L"Languages") != fields.end()) {
            entry.Languages = APT::String::FromUTF8(fields[L"Languages"]);
        }
        if (fields.find(L"Targets") != fields.end()) {
            entry.Targets = APT::String::FromUTF8(fields[L"Targets"]);
        }
        
        entry.Enabled = true; // Default to enabled
        entries.push_back(entry);
        fields.clear();
    }
    
    return true;
}

bool RDeb822Source::WriteDeb822File(const std::string& path, const std::vector<Deb822Entry>& entries) {
    std::wofstream file(path);
    if (!file) {
        return _error->Error(_("Cannot write to %s"), path.c_str());
    }
    
    for (const auto& entry : entries) {
        if (!entry.Enabled) {
            file << L"# Disabled: ";
        }
        
        file << L"Types: " << APT::String::ToUTF8(entry.Types) << std::endl;
        file << L"URIs: " << APT::String::ToUTF8(entry.URIs) << std::endl;
        file << L"Suites: " << APT::String::ToUTF8(entry.Suites) << std::endl;
        
        if (!entry.Components.empty()) {
            file << L"Components: " << APT::String::ToUTF8(entry.Components) << std::endl;
        }
        if (!entry.SignedBy.empty()) {
            file << L"Signed-By: " << APT::String::ToUTF8(entry.SignedBy) << std::endl;
        }
        if (!entry.Architectures.empty()) {
            file << L"Architectures: " << APT::String::ToUTF8(entry.Architectures) << std::endl;
        }
        if (!entry.Languages.empty()) {
            file << L"Languages: " << APT::String::ToUTF8(entry.Languages) << std::endl;
        }
        if (!entry.Targets.empty()) {
            file << L"Targets: " << APT::String::ToUTF8(entry.Targets) << std::endl;
        }
        
        file << std::endl;
    }
    
    return true;
}

bool RDeb822Source::ConvertToSourceRecord(const Deb822Entry& entry, pkgSourceList::SourceRecord& record) {
    // Parse types
    bool has_deb = false;
    bool has_deb_src = false;
    std::istringstream typeStream(entry.Types);
    std::string type;
    while (std::getline(typeStream, type, ' ')) {
        TrimWhitespace(type);
        if (type == "deb") has_deb = true;
        if (type == "deb-src") has_deb_src = true;
    }
    
    record.Type = 0;
    if (has_deb) record.Type |= pkgSourceList::Deb;
    if (has_deb_src) record.Type |= pkgSourceList::DebSrc;
    if (!entry.Enabled) record.Type |= pkgSourceList::Disabled;
    
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
    while (std::getline(compStream, comp, ' ')) {
        TrimWhitespace(comp);
        if (!comp.empty()) {
            record.Comps.push_back(comp);
        }
    }
    
    return true;
}

bool RDeb822Source::ConvertFromSourceRecord(const pkgSourceList::SourceRecord& record, Deb822Entry& entry) {
    // Set types
    std::stringstream typeStream;
    if (record.Type & pkgSourceList::Deb) {
        typeStream << "deb ";
    }
    if (record.Type & pkgSourceList::DebSrc) {
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
    entry.Enabled = !(record.Type & pkgSourceList::Disabled);
    
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

bool RDeb822Source::ParseStanza(std::wifstream& file, std::map<std::wstring, std::wstring>& fields) {
    std::wstring line;
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
        if (line[0] == L'#') {
            continue;
        }
        
        // Check for stanza start
        if (line.find(L"Types:") != std::wstring::npos) {
            inStanza = true;
        }
        
        if (inStanza) {
            size_t colonPos = line.find(L':');
            if (colonPos != std::wstring::npos) {
                std::wstring key = line.substr(0, colonPos);
                std::wstring value = line.substr(colonPos + 1);
                
                // Trim whitespace
                key.erase(0, key.find_first_not_of(L" \t"));
                key.erase(key.find_last_not_of(L" \t") + 1);
                value.erase(0, value.find_first_not_of(L" \t"));
                value.erase(value.find_last_not_of(L" \t") + 1);
                
                fields[key] = value;
            }
        }
    }
    
    return !fields.empty();
} 