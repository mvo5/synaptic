#include "rpackagemanager.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <apt-pkg/configuration.h>
#include <apt-pkg/fileutl.h>
#include <iostream>

// RPackageManager implementation
RPackageManager::RPackageManager(pkgPackageManager *pm) : pm(pm) {}

pkgPackageManager::OrderResult RPackageManager::DoInstallPreFork() {
   Res = pm->OrderInstall();
   return Res;
}

#ifdef WITH_DPKG_STATUSFD
pkgPackageManager::OrderResult RPackageManager::DoInstallPostFork(int statusFd) {
   return (pm->Go(statusFd) == false) ? pkgPackageManager::Failed : Res;
}
#else
pkgPackageManager::OrderResult RPackageManager::DoInstallPostFork() {
   return (pm->Go() == false) ? pkgPackageManager::Failed : Res;
}
#endif

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
    bool hasTypes = false;
    bool hasUris = false;
    bool hasSuites = false;
    bool hasComponents = false;

    while (std::getline(stream, line)) {
        // Skip comments and empty lines
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Parse key-value pairs
        size_t colonPos = line.find(':');
        if (colonPos == std::string::npos) {
            continue;
        }

        std::string key = line.substr(0, colonPos);
        std::string value = line.substr(colonPos + 1);
        
        // Trim whitespace
        key = std::string(APT::String::Strip(key));
        value = std::string(APT::String::Strip(value));

        if (key == "Types") {
            source.types = value;
            hasTypes = true;
        } else if (key == "URIs") {
            source.uris = value;
            hasUris = true;
        } else if (key == "Suites") {
            source.suites = value;
            hasSuites = true;
        } else if (key == "Components") {
            source.components = value;
            hasComponents = true;
        } else if (key == "Signed-By") {
            source.signedBy = value;
        }
    }

    // Validate required fields
    if (!hasTypes || !hasUris || !hasSuites || !hasComponents) {
        std::cerr << "Warning: Missing required fields in source" << std::endl;
        return RDeb822Source(); // Return invalid source
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
                
                // Read the entire file content
                std::ifstream file(entry.path());
                if (!file) {
                    std::cerr << "Error: Could not open file: " << entry.path().string() << std::endl;
                    continue;
                }
                
                std::string content((std::istreambuf_iterator<char>(file)),
                                  std::istreambuf_iterator<char>());
                
                // Parse sources from the file content
                std::vector<RDeb822Source> fileSources = parseSources(content);
                sources.insert(sources.end(), fileSources.begin(), fileSources.end());
            }
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading sources: " << e.what() << std::endl;
        return false;
    }
}

std::vector<RDeb822Source> RSourceManager::parseSources(const std::string& content)
{
    std::vector<RDeb822Source> sources;
    std::map<std::string, std::string> currentFields;
    
    std::istringstream iss(content);
    std::string line;
    bool inSourceBlock = false;
    
    while (std::getline(iss, line)) {
        // Trim whitespace
        line = trim(line);
        
        // Skip empty lines
        if (line.empty()) {
            if (!currentFields.empty()) {
                RDeb822Source source = createSourceFromFields(currentFields);
                if (source.isValid()) {
                    sources.push_back(source);
                }
                currentFields.clear();
            }
            continue;
        }
        
        // Check for source block start
        if (line.starts_with("#") && line.find("Modernized") != std::string::npos) {
            if (!currentFields.empty()) {
                RDeb822Source source = createSourceFromFields(currentFields);
                if (source.isValid()) {
                    sources.push_back(source);
                }
                currentFields.clear();
            }
            inSourceBlock = true;
            continue;
        }
        
        // Skip other comments
        if (line.starts_with("#")) {
            continue;
        }
        
        // Parse key-value pairs
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string key = trim(line.substr(0, colonPos));
            std::string value = trim(line.substr(colonPos + 1));
            
            // Look ahead for continuation lines
            while (std::getline(iss, line)) {
                line = trim(line);
                if (line.empty() || line.starts_with("#") || line.find(':') != std::string::npos) {
                    // Put the line back for the next iteration
                    iss.seekg(-static_cast<long>(line.length() + 1), std::ios_base::cur);
                    break;
                }
                value += " " + line;
            }
            
            currentFields[key] = value;
        }
    }
    
    // Handle the last source
    if (!currentFields.empty()) {
        RDeb822Source source = createSourceFromFields(currentFields);
        if (source.isValid()) {
            sources.push_back(source);
        }
    }
    
    return sources;
}

RDeb822Source RSourceManager::createSourceFromFields(const std::map<std::string, std::string>& fields)
{
    RDeb822Source source;
    
    // Set required fields
    source.types = fields.count("Types") ? fields.at("Types") : "";
    source.uris = fields.count("URIs") ? fields.at("URIs") : "";
    source.suites = fields.count("Suites") ? fields.at("Suites") : "";
    source.components = fields.count("Components") ? fields.at("Components") : "";
    
    // Set optional fields
    if (fields.count("Signed-By")) {
        source.signedBy = fields.at("Signed-By");
    }
    
    return source;
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

std::string RSourceManager::trim(const std::string& str) const
{
    const std::string whitespace = " \t\r\n";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return "";
    }
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
} 