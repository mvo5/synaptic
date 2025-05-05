#include "rpackagemanager.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <apt-pkg/configuration.h>
#include <apt-pkg/fileutl.h>
#include <iostream>

// RDeb822Source implementation
RDeb822Source::RDeb822Source() : enabled(true) {}

RDeb822Source::RDeb822Source(const std::string& types, const std::string& uris,
                            const std::string& suites, const std::string& components)
    : types(types), uris(uris), suites(suites), components(components), enabled(true) {}

bool RDeb822Source::isValid() const {
    // Check required fields
    if (types.empty() || uris.empty() || suites.empty()) {
        return false;
    }
    
    // Validate types
    if (types != "deb" && types != "deb-src") {
        return false;
    }
    
    // Split URIs and validate each
    std::istringstream uriStream(uris);
    std::string uri;
    bool hasValidUri = false;
    
    while (std::getline(uriStream, uri, ' ')) {
        // Trim whitespace
        uri.erase(0, uri.find_first_not_of(" \t"));
        uri.erase(uri.find_last_not_of(" \t") + 1);
        
        if (uri.empty()) continue;
        
        // Check URI scheme
        if (uri.starts_with("http://") || 
            uri.starts_with("https://") || 
            uri.starts_with("ftp://") ||
            uri.starts_with("file://") ||
            uri.starts_with("cdrom:")) {
            hasValidUri = true;
        }
    }
    
    if (!hasValidUri) {
        return false;
    }
    
    // Validate suites
    std::istringstream suiteStream(suites);
    std::string suite;
    bool hasValidSuite = false;
    
    while (std::getline(suiteStream, suite, ' ')) {
        // Trim whitespace
        suite.erase(0, suite.find_first_not_of(" \t"));
        suite.erase(suite.find_last_not_of(" \t") + 1);
        
        if (!suite.empty()) {
            hasValidSuite = true;
            break;
        }
    }
    
    return hasValidSuite;
}

std::string RDeb822Source::toString() const {
    std::stringstream ss;
    
    // Add comment if source is disabled
    if (!enabled) {
        ss << "# Disabled: ";
    }
    
    // Format as a single line for compatibility
    ss << types << " " << uris << " " << suites;
    if (!components.empty()) {
        ss << " " << components;
    }
    ss << "\n";
    
    return ss.str();
}

RDeb822Source RDeb822Source::fromString(const std::string& content) {
    RDeb822Source source;
    std::istringstream stream(content);
    std::string line;
    bool inSource = false;
    std::string currentKey;
    std::string currentValue;

    while (std::getline(stream, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') {
            if (inSource && !currentKey.empty()) {
                // Process the last key-value pair
                source.setField(currentKey, currentValue);
                currentKey.clear();
                currentValue.clear();
            }
            inSource = false;
            continue;
        }

        // Check for disabled source
        if (line[0] == '#') {
            source.enabled = false;
            line = line.substr(1);
        }

        // Parse key-value pair
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            if (inSource && !currentKey.empty()) {
                // Process the previous key-value pair
                source.setField(currentKey, currentValue);
            }
            currentKey = line.substr(0, colonPos);
            currentValue = line.substr(colonPos + 1);
            // Trim whitespace
            currentKey.erase(0, currentKey.find_first_not_of(" \t"));
            currentKey.erase(currentKey.find_last_not_of(" \t") + 1);
            currentValue.erase(0, currentValue.find_first_not_of(" \t"));
            currentValue.erase(currentValue.find_last_not_of(" \t") + 1);
            inSource = true;
        } else if (inSource && !currentValue.empty()) {
            // Continuation of previous value
            currentValue += "\n" + line;
        }
    }

    // Process the last key-value pair
    if (inSource && !currentKey.empty()) {
        source.setField(currentKey, currentValue);
    }

    return source;
}

bool RDeb822Source::operator==(const RDeb822Source& other) const {
    return types == other.types &&
           uris == other.uris &&
           suites == other.suites &&
           components == other.components &&
           enabled == other.enabled;
}

bool RDeb822Source::operator!=(const RDeb822Source& other) const {
    return !(*this == other);
}

// RSourceManager implementation
RSourceManager::RSourceManager() : sourcesDir("/etc/apt/sources.list.d") {}

RSourceManager::RSourceManager(const std::string& sourcesDir) : sourcesDir(sourcesDir) {}

bool RSourceManager::addSource(const RDeb822Source& source) {
    if (!source.isValid()) {
        return false;
    }
    
    // Check if source already exists
    for (const auto& existing : sources) {
        if (existing == source) {
            return false;
        }
    }
    
    sources.push_back(source);
    return true;
}

bool RSourceManager::removeSource(const RDeb822Source& source) {
    auto it = std::find(sources.begin(), sources.end(), source);
    if (it == sources.end()) {
        return false;
    }
    
    sources.erase(it);
    return true;
}

bool RSourceManager::updateSource(const RDeb822Source& oldSource, const RDeb822Source& newSource) {
    if (!newSource.isValid()) {
        return false;
    }
    
    auto it = std::find(sources.begin(), sources.end(), oldSource);
    if (it == sources.end()) {
        return false;
    }
    
    *it = newSource;
    return true;
}

std::vector<RDeb822Source> RSourceManager::getSources() const {
    return sources;
}

bool RSourceManager::loadSources() {
    sources.clear();

    try {
        // Read all .sources files in the sources directory
        for (const auto& entry : std::filesystem::directory_iterator(sourcesDir)) {
            if (entry.path().extension() == ".sources") {
                std::cerr << "Loading source file: " << entry.path().string() << std::endl;
                RDeb822Source source = readSourceFile(entry.path().string());
                if (source.isValid()) {
                    std::cerr << "Loaded valid source: " << source.getUris() << " " << source.getSuites() << std::endl;
                    sources.push_back(source);
                } else {
                    std::cerr << "Warning: Invalid source file: " << entry.path().string() << std::endl;
                }
            }
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading sources: " << e.what() << std::endl;
        return false;
    }
}

bool RSourceManager::saveSources() const {
    try {
        for (const auto& source : sources) {
            std::string filename = getSourceFilename(source);
            if (!writeSourceFile(filename, source)) {
                return false;
            }
        }
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

bool RSourceManager::writeSourceFile(const std::string& filename, const RDeb822Source& source) const {
    try {
        // Create parent directories if they don't exist
        std::filesystem::path filePath(filename);
        std::filesystem::create_directories(filePath.parent_path());
        
        std::ofstream file(filename);
        if (!file) {
            return false;
        }
        
        file << source.toString();
        return file.good();
    } catch (const std::exception&) {
        return false;
    }
}

RDeb822Source RSourceManager::readSourceFile(const std::string& filename) const {
    try {
        std::ifstream file(filename);
        if (!file) {
            return RDeb822Source();
        }
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        return RDeb822Source::fromString(content);
    } catch (const std::exception&) {
        return RDeb822Source();
    }
}

bool RSourceManager::updateAptSources() {
    // On Windows, we'll just return true for testing
    return true;
}

bool RSourceManager::reloadAptCache() {
    // On Windows, we'll just return true for testing
    return true;
}

bool RSourceManager::validateSourceFile(const std::string& filename) const {
    RDeb822Source source = readSourceFile(filename);
    return source.isValid();
}

std::string RSourceManager::getSourceFilename(const RDeb822Source& source) const {
    // Generate a filename based on the source URI and suite
    std::string filename = source.getUris();
    filename.erase(0, filename.find("://") + 3);  // Remove protocol
    std::replace(filename.begin(), filename.end(), '/', '-');
    filename += "-" + source.getSuites() + ".sources";
    return (std::filesystem::path(sourcesDir) / filename).string();
} 