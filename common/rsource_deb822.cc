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
    std::cout << "DEBUG: [Deb822Parser] Opening file: " << path << std::endl;
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cout << "DEBUG: [Deb822Parser] Failed to open file: " << path << std::endl;
        return false;
    }
    std::string line;
    std::map<std::string, std::string> fields;
    int stanza_count = 0;
    while (std::getline(file, line)) {
        std::cout << "DEBUG: [Deb822Parser] Read line: '" << line << "'" << std::endl;
        if (line.empty()) {
            if (!fields.empty()) {
                std::cout << "DEBUG: [Deb822Parser] End of stanza, fields found:" << std::endl;
                for (const auto& kv : fields) {
                    std::cout << "    '" << kv.first << "': '" << kv.second << "'" << std::endl;
                }
                Deb822Entry entry;
                // Check required fields
                if (fields.find("Types") == fields.end() || fields.find("URIs") == fields.end() || fields.find("Suites") == fields.end()) {
                    std::cout << "DEBUG: [Deb822Parser] Missing required field in stanza, skipping." << std::endl;
                    fields.clear();
                    continue;
                }
                entry.Types = fields["Types"];
                entry.URIs = fields["URIs"];
                entry.Suites = fields["Suites"];
                entry.Components = fields.count("Components") ? fields["Components"] : "";
                entry.SignedBy = fields.count("Signed-By") ? fields["Signed-By"] : "";
                entry.Enabled = true; // Default to enabled
                entries.push_back(entry);
                stanza_count++;
                fields.clear();
            }
            continue;
        }
        if (line[0] == '#') {
            std::cout << "DEBUG: [Deb822Parser] Skipping comment line." << std::endl;
            continue;
        }
        size_t colon = line.find(':');
        if (colon == std::string::npos) {
            std::cout << "DEBUG: [Deb822Parser] No colon found in line, skipping." << std::endl;
            continue;
        }
        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon + 1);
        // Trim whitespace
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);
        std::cout << "DEBUG: [Deb822Parser] Parsed field: '" << key << "' = '" << value << "'" << std::endl;
        fields[key] = value;
    }
    // Handle last stanza if file does not end with blank line
    if (!fields.empty()) {
        std::cout << "DEBUG: [Deb822Parser] End of file, last stanza fields:" << std::endl;
        for (const auto& kv : fields) {
            std::cout << "    '" << kv.first << "': '" << kv.second << "'" << std::endl;
        }
        Deb822Entry entry;
        if (fields.find("Types") == fields.end() || fields.find("URIs") == fields.end() || fields.find("Suites") == fields.end()) {
            std::cout << "DEBUG: [Deb822Parser] Missing required field in last stanza, skipping." << std::endl;
        } else {
            entry.Types = fields["Types"];
            entry.URIs = fields["URIs"];
            entry.Suites = fields["Suites"];
            entry.Components = fields.count("Components") ? fields["Components"] : "";
            entry.SignedBy = fields.count("Signed-By") ? fields["Signed-By"] : "";
            entry.Enabled = true;
            entries.push_back(entry);
            stanza_count++;
        }
    }
    std::cout << "DEBUG: [Deb822Parser] Parsed " << stanza_count << " stanzas from file: " << path << std::endl;
    return true;
}

bool RDeb822Source::WriteDeb822File(const std::string& path, const std::vector<Deb822Entry>& entries) {
    std::ofstream file(path);
    if (!file) {
        return _error->Error(_("Cannot write to %s"), path.c_str());
    }

    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& entry = entries[i];
        
        // Write preserved comments before stanza
        if (!entry.Comment.empty()) {
            file << entry.Comment;
            if (entry.Comment.back() != '\n') file << "\n";
        }

        if (!entry.Enabled) {
            file << "# Disabled: ";
        }

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
        
        // Only add empty line between entries, not after the last one
        if (i < entries.size() - 1) {
            file << std::endl;
        }
    }

    return true;
}

bool RDeb822Source::ConvertToSourceRecord(const Deb822Entry& entry, SourcesList::SourceRecord& record) {
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
    if (has_deb) record.Type |= SourcesList::Deb;
    if (has_deb_src) record.Type |= SourcesList::DebSrc;
    if (!entry.Enabled) record.Type |= SourcesList::Disabled;

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
    std::vector<std::string> sections;
    while (std::getline(compStream, comp, ' ')) {
        TrimWhitespace(comp);
        if (!comp.empty()) {
            sections.push_back(comp);
        }
    }
    
    // Set sections
    if (!sections.empty()) {
        record.NumSections = sections.size();
        record.Sections = new string[record.NumSections];
        for (unsigned short i = 0; i < record.NumSections; i++) {
            record.Sections[i] = sections[i];
        }
    }
    
    // Preserve extra fields in Comment
    std::stringstream commentStream;
    if (!entry.SignedBy.empty()) {
        commentStream << "Signed-By: " << entry.SignedBy << std::endl;
    }
    if (!entry.Architectures.empty()) {
        commentStream << "Architectures: " << entry.Architectures << std::endl;
    }
    if (!entry.Languages.empty()) {
        commentStream << "Languages: " << entry.Languages << std::endl;
    }
    if (!entry.Targets.empty()) {
        commentStream << "Targets: " << entry.Targets << std::endl;
    }
    record.Comment = commentStream.str();
    
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
    for (unsigned short i = 0; i < record.NumSections; i++) {
        compStream << record.Sections[i] << " ";
    }
    entry.Components = compStream.str();
    TrimWhitespace(entry.Components);
    
    // Set enabled state
    entry.Enabled = !(record.Type & SourcesList::Disabled);

    // Parse extra fields from Comment
    if (!record.Comment.empty()) {
        std::istringstream iss(record.Comment);
        std::string line;
        while (std::getline(iss, line)) {
            size_t colon = line.find(":");
            if (colon == std::string::npos) continue;
            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 1);
            TrimWhitespace(key);
            TrimWhitespace(value);
            if (key == "Signed-By") entry.SignedBy = value;
            else if (key == "Architectures") entry.Architectures = value;
            else if (key == "Languages") entry.Languages = value;
            else if (key == "Targets") entry.Targets = value;
        }
    }
    
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