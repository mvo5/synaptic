/* rsource_deb822.cc - Deb822 format sources support
 * 
 * Copyright (c) 2025 Synaptic development team          
 */

#include "rsource_deb822.h"
#include <apt-pkg/configuration.h>
#include <apt-pkg/fileutl.h>
#include <apt-pkg/strutl.h>
#include <apt-pkg/error.h>
#include <fstream>
#include <sstream>
#include "i18n.h"

bool RDeb822Source::ParseDeb822File(const std::string& path, std::vector<Deb822Entry>& entries) {
    std::ifstream file(path.c_str());
    if (!file) {
        return _error->Error(_("Cannot open %s"), path.c_str());
    }

    std::map<std::string, std::string> fields;
    while (ParseStanza(file, fields)) {
        Deb822Entry entry;
        
        // Required fields
        auto types_it = fields.find("Types");
        if (types_it == fields.end()) {
            return _error->Error(_("Missing Types field in %s"), path.c_str());
        }
        entry.Types = types_it->second;

        auto uris_it = fields.find("URIs");
        if (uris_it == fields.end()) {
            return _error->Error(_("Missing URIs field in %s"), path.c_str());
        }
        entry.URIs = uris_it->second;

        auto suites_it = fields.find("Suites");
        if (suites_it == fields.end()) {
            return _error->Error(_("Missing Suites field in %s"), path.c_str());
        }
        entry.Suites = suites_it->second;

        // Optional fields
        entry.Components = fields["Components"];
        entry.SignedBy = fields["Signed-By"];
        
        // Handle enabled/disabled state
        auto enabled_it = fields.find("Enabled");
        entry.Enabled = (enabled_it == fields.end() || enabled_it->second != "no");

        // Store any comment lines
        auto comment_it = fields.find("#");
        if (comment_it != fields.end()) {
            entry.Comment = comment_it->second;
        }

        entries.push_back(entry);
        fields.clear();
    }

    return true;
}

bool RDeb822Source::WriteDeb822File(const std::string& path, const std::vector<Deb822Entry>& entries) {
    std::ofstream file(path.c_str());
    if (!file) {
        return _error->Error(_("Cannot write to %s"), path.c_str());
    }

    for (const auto& entry : entries) {
        if (!entry.Comment.empty()) {
            file << entry.Comment << "\n";
        }

        file << "Types: " << entry.Types << "\n";
        file << "URIs: " << entry.URIs << "\n";
        file << "Suites: " << entry.Suites << "\n";
        
        if (!entry.Components.empty()) {
            file << "Components: " << entry.Components << "\n";
        }
        
        if (!entry.SignedBy.empty()) {
            file << "Signed-By: " << entry.SignedBy << "\n";
        }
        
        if (!entry.Enabled) {
            file << "Enabled: no\n";
        }
        
        file << "\n";  // Empty line between stanzas
    }

    return true;
}

bool RDeb822Source::ConvertToSourceRecord(const Deb822Entry& entry, SourcesList::SourceRecord& record) {
    std::istringstream types(entry.Types);
    std::string type;
    bool has_deb = false, has_deb_src = false;
    
    while (types >> type) {
        if (type == "deb") has_deb = true;
        else if (type == "deb-src") has_deb_src = true;
    }

    record.Type = 0;
    if (has_deb) record.Type |= SourcesList::Deb;
    if (has_deb_src) record.Type |= SourcesList::DebSrc;
    if (!entry.Enabled) record.Type |= SourcesList::Disabled;

    // Split URIs into vector
    std::istringstream uri_stream(entry.URIs);
    std::string uri;
    uri_stream >> uri;  // Take first URI
    record.URI = uri;

    // Split Suites into vector
    std::istringstream suite_stream(entry.Suites);
    std::string suite;
    suite_stream >> suite;  // Take first Suite
    record.Dist = suite;

    // Handle Components
    if (!entry.Components.empty()) {
        std::istringstream comp_stream(entry.Components);
        std::vector<std::string> components;
        std::string comp;
        while (comp_stream >> comp) {
            components.push_back(comp);
        }
        
        record.NumSections = components.size();
        record.Sections = new std::string[record.NumSections];
        for (size_t i = 0; i < components.size(); i++) {
            record.Sections[i] = components[i];
        }
    }

    record.Comment = entry.Comment;
    
    return true;
}

bool RDeb822Source::ConvertFromSourceRecord(const SourcesList::SourceRecord& record, Deb822Entry& entry) {
    std::string types;
    if (record.Type & SourcesList::Deb) {
        types += "deb ";
    }
    if (record.Type & SourcesList::DebSrc) {
        types += "deb-src ";
    }
    TrimWhitespace(types);
    entry.Types = types;

    entry.URIs = record.URI;
    entry.Suites = record.Dist;
    
    std::string components;
    for (unsigned short i = 0; i < record.NumSections; i++) {
        components += record.Sections[i] + " ";
    }
    TrimWhitespace(components);
    entry.Components = components;

    entry.Enabled = !(record.Type & SourcesList::Disabled);
    entry.Comment = record.Comment;
    
    return true;
}

bool RDeb822Source::ParseStanza(std::istream& input, std::map<std::string, std::string>& fields) {
    std::string line;
    std::string current_field;
    std::string current_value;
    bool in_stanza = false;
    
    while (std::getline(input, line)) {
        TrimWhitespace(line);
        
        // Skip empty lines between stanzas
        if (line.empty()) {
            if (in_stanza) {
                // End of stanza
                if (!current_field.empty()) {
                    TrimWhitespace(current_value);
                    fields[current_field] = current_value;
                }
                return true;
            }
            continue;
        }

        // Handle comments
        if (line[0] == '#') {
            if (!in_stanza) {
                // Comment before stanza belongs to next stanza
                fields["#"] = fields["#"].empty() ? line : fields["#"] + "\n" + line;
            }
            continue;
        }

        in_stanza = true;
        
        // Handle field continuation
        if (line[0] == ' ' || line[0] == '\t') {
            if (!current_field.empty()) {
                current_value += "\n" + line;
            }
            continue;
        }

        // New field
        if (!current_field.empty()) {
            TrimWhitespace(current_value);
            fields[current_field] = current_value;
        }

        size_t colon = line.find(':');
        if (colon == std::string::npos) {
            return _error->Error(_("Invalid line format: %s"), line.c_str());
        }

        current_field = line.substr(0, colon);
        TrimWhitespace(current_field);
        current_value = line.substr(colon + 1);
        TrimWhitespace(current_value);
    }

    // Handle last field of last stanza
    if (in_stanza && !current_field.empty()) {
        TrimWhitespace(current_value);
        fields[current_field] = current_value;
        return true;
    }

    return false;
}

void RDeb822Source::TrimWhitespace(std::string& str) {
    // Trim leading whitespace
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        str.clear();
        return;
    }
    
    // Trim trailing whitespace
    size_t end = str.find_last_not_of(" \t\r\n");
    str = str.substr(start, end - start + 1);
} 