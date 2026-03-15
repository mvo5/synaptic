#include "rdeb822source.h"
#include <algorithm>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <iostream> // Added for debug prints
#include <map> // Added for debug prints

RDeb822Source::RDeb822Source()
    : enabled(true)
{
}

RDeb822Source::RDeb822Source(const std::string& types, const std::string& uris,
                             const std::string& suites, const std::string& components)
    : types(types), uris(uris), suites(suites), components(components), enabled(true)
{
}

bool RDeb822Source::isValid() const {
    return !types.empty() && !uris.empty() && !suites.empty();
}

std::string RDeb822Source::toString() const {
    std::stringstream ss;
    if (!enabled) {
        ss << "Types: " << types << "\n";
        ss << "URIs: " << uris << "\n";
        ss << "Suites: " << suites << "\n";
        if (!components.empty()) {
            ss << "Components: " << components << "\n";
        }
        if (!signedBy.empty()) {
            ss << "Signed-By: " << signedBy << "\n";
        }
    } else {
        ss << "# " << types << " " << uris << " " << suites;
        if (!components.empty()) {
            ss << " " << components;
        }
        if (!signedBy.empty()) {
            ss << " [signed-by=" << signedBy << "]";
        }
    }
    return ss.str();
}

RDeb822Source RDeb822Source::fromString(const std::string& content) {
    RDeb822Source source;
    std::istringstream iss(content);
    std::string line;
    
    while (std::getline(iss, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        
        size_t colon = line.find(':');
        if (colon == std::string::npos) continue;
        
        std::string key = trim(line.substr(0, colon));
        std::string value = trim(line.substr(colon + 1));
        
        if (key == "Types") source.setTypes(value);
        else if (key == "URIs") source.setURIs(value);
        else if (key == "Suites") source.setSuites(value);
        else if (key == "Components") source.setComponents(value);
        else if (key == "Signed-By") source.setSignedBy(value);
    }
    
    return source;
}

bool RDeb822Source::operator==(const RDeb822Source& other) const {
    return types == other.types &&
           uris == other.uris &&
           suites == other.suites &&
           components == other.components &&
           signedBy == other.signedBy &&
           enabled == other.enabled;
}

bool RDeb822Source::operator!=(const RDeb822Source& other) const {
    return !(*this == other);
}

std::string RDeb822Source::trim(const std::string& str) {
    const std::string whitespace = " \t\r\n";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return "";
    }
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
} 

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