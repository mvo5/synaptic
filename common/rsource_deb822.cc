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

bool RDeb822Source::ParseDeb822File(const std::string& path, std::vector<Deb822Entry>& entries) {
    // Open file with UTF-8 encoding
    std::wifstream file(path.c_str());
    if (!file) {
        return _error->Error(_("Cannot open %s"), path.c_str());
    }
    
    // Set UTF-8 locale
    file.imbue(std::locale(file.getloc(), new std::codecvt_utf8<wchar_t>));

    std::map<std::wstring, std::wstring> fields;
    while (ParseStanza(file, fields)) {
        Deb822Entry entry;

        // Required fields
        auto types_it = fields.find(L"Types");
        if (types_it == fields.end()) {
            return _error->Error(_("Missing Types field in %s"), path.c_str());
        }
        entry.Types = std::string(types_it->second.begin(), types_it->second.end());

        auto uris_it = fields.find(L"URIs");
        if (uris_it == fields.end()) {
            return _error->Error(_("Missing URIs field in %s"), path.c_str());
        }
        entry.URIs = std::string(uris_it->second.begin(), uris_it->second.end());

        auto suites_it = fields.find(L"Suites");
        if (suites_it == fields.end()) {
            return _error->Error(_("Missing Suites field in %s"), path.c_str());
        }
        entry.Suites = std::string(suites_it->second.begin(), suites_it->second.end());

        // Optional fields
        entry.Components = std::string(fields[L"Components"].begin(), fields[L"Components"].end());
        entry.SignedBy = std::string(fields[L"Signed-By"].begin(), fields[L"Signed-By"].end());
        entry.Architectures = std::string(fields[L"Architectures"].begin(), fields[L"Architectures"].end());
        entry.Languages = std::string(fields[L"Languages"].begin(), fields[L"Languages"].end());
        entry.Targets = std::string(fields[L"Targets"].begin(), fields[L"Targets"].end());

        // Handle enabled/disabled state
        auto enabled_it = fields.find(L"Enabled");
        entry.Enabled = (enabled_it == fields.end()) || (enabled_it->second != L"no");

        // Store any comment lines
        auto comment_it = fields.find(L"#");
        if (comment_it != fields.end()) {
            entry.Comment = std::string(comment_it->second.begin(), comment_it->second.end());
        }

        entries.push_back(entry);
        fields.clear();
    }

    return true;
}

bool RDeb822Source::WriteDeb822File(const std::string& path, const std::vector<Deb822Entry>& entries) {
    // Open file with UTF-8 encoding
    std::wofstream file(path.c_str());
    if (!file) {
        return _error->Error(_("Cannot write to %s"), path.c_str());
    }
    
    // Set UTF-8 locale
    file.imbue(std::locale(file.getloc(), new std::codecvt_utf8<wchar_t>));

    for (const auto& entry : entries) {
        if (!entry.Comment.empty()) {
            std::wstring wcomment(entry.Comment.begin(), entry.Comment.end());
            file << wcomment << L"\n";
        }

        std::wstring wtypes(entry.Types.begin(), entry.Types.end());
        std::wstring wuris(entry.URIs.begin(), entry.URIs.end());
        std::wstring wsuites(entry.Suites.begin(), entry.Suites.end());
        
        file << L"Types: " << wtypes << L"\n";
        file << L"URIs: " << wuris << L"\n";
        file << L"Suites: " << wsuites << L"\n";
        
        if (!entry.Components.empty()) {
            std::wstring wcomponents(entry.Components.begin(), entry.Components.end());
            file << L"Components: " << wcomponents << L"\n";
        }
        if (!entry.SignedBy.empty()) {
            std::wstring wsignedby(entry.SignedBy.begin(), entry.SignedBy.end());
            file << L"Signed-By: " << wsignedby << L"\n";
        }
        if (!entry.Architectures.empty()) {
            std::wstring warchitectures(entry.Architectures.begin(), entry.Architectures.end());
            file << L"Architectures: " << warchitectures << L"\n";
        }
        if (!entry.Languages.empty()) {
            std::wstring wlanguages(entry.Languages.begin(), entry.Languages.end());
            file << L"Languages: " << wlanguages << L"\n";
        }
        if (!entry.Targets.empty()) {
            std::wstring wtargets(entry.Targets.begin(), entry.Targets.end());
            file << L"Targets: " << wtargets << L"\n";
        }

        if (!entry.Enabled) {
            file << L"Enabled: no\n";
        }

        file << L"\n"; // Empty line between stanzas
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

    std::istringstream uri_stream(entry.URIs);
    std::string uri;
    uri_stream >> uri; // Take first URI
    if (!record.SetURI(uri)) {
        return false;
    }

    std::istringstream suite_stream(entry.Suites);
    std::string suite;
    suite_stream >> suite; // Take first Suite
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

    // Store additional fields in Comment for now
    // TODO: Add proper fields to SourceRecord for these
    std::stringstream additional;
    if (!entry.Architectures.empty()) {
        additional << "Architectures: " << entry.Architectures << "\n";
    }
    if (!entry.Languages.empty()) {
        additional << "Languages: " << entry.Languages << "\n";
    }
    if (!entry.Targets.empty()) {
        additional << "Targets: " << entry.Targets << "\n";
    }
    if (!entry.SignedBy.empty()) {
        additional << "Signed-By: " << entry.SignedBy << "\n";
    }
    if (!entry.Comment.empty()) {
        additional << entry.Comment;
    }
    record.Comment = additional.str();

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

    // Parse additional fields from Comment
    std::istringstream comment_stream(record.Comment);
    std::string line;
    while (std::getline(comment_stream, line)) {
        if (line.empty()) continue;
        
        size_t colon = line.find(':');
        if (colon == std::string::npos) {
            // Regular comment line
            entry.Comment += line + "\n";
            continue;
        }

        std::string key = line.substr(0, colon);
        std::string value = line.substr(colon + 1);
        TrimWhitespace(key);
        TrimWhitespace(value);

        if (key == "Architectures") {
            entry.Architectures = value;
        } else if (key == "Languages") {
            entry.Languages = value;
        } else if (key == "Targets") {
            entry.Targets = value;
        } else if (key == "Signed-By") {
            entry.SignedBy = value;
        } else {
            // Unknown field, treat as comment
            entry.Comment += line + "\n";
        }
    }

    return true;
}

bool RDeb822Source::ParseStanza(std::wifstream& file, std::map<std::wstring, std::wstring>& fields) {
    std::wstring line;
    bool in_stanza = false;

    while (std::getline(file, line)) {
        // Skip empty lines
        if (line.empty()) {
            if (in_stanza) {
                return true; // End of stanza
            }
            continue;
        }

        // Skip comments
        if (line[0] == L'#') {
            fields[L"#"] = line.substr(1);
            continue;
        }

        // Parse field
        size_t colon_pos = line.find(L':');
        if (colon_pos != std::wstring::npos) {
            in_stanza = true;
            std::wstring field = line.substr(0, colon_pos);
            std::wstring value = line.substr(colon_pos + 1);
            
            // Trim whitespace
            while (!field.empty() && std::iswspace(field.back())) field.pop_back();
            while (!value.empty() && std::iswspace(value.front())) value.erase(0, 1);
            
            fields[field] = value;
        }
    }

    return in_stanza; // Return true if we found any fields
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