#include "rdeb822source.h"
#include <algorithm>
#include <sstream>
#include <fstream>
#include <filesystem>

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