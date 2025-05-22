#include "rsourcemanager.h"
#include <fstream>
#include <filesystem>
#include <gtk/gtk.h>

RSourceManager::RSourceManager() : useDeb822Format(false) {
    // Check if sources.list.d exists and contains .sources files
    useDeb822Format = std::filesystem::exists("/etc/apt/sources.list.d") &&
                      !std::filesystem::is_empty("/etc/apt/sources.list.d");
}

RSourceManager::~RSourceManager() {
}

bool RSourceManager::loadSources() {
    sources.clear();
    
    // Try loading Deb822 sources first
    if (useDeb822Format) {
        if (loadDeb822Sources()) {
            return true;
        }
    }
    
    // Fall back to legacy format
    return loadLegacySources();
}

bool RSourceManager::loadDeb822Sources() {
    const std::string sourcesDir = "/etc/apt/sources.list.d";
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(sourcesDir)) {
            if (entry.path().extension() == ".sources") {
                std::ifstream file(entry.path());
                if (!file.is_open()) continue;
                
                std::string content((std::istreambuf_iterator<char>(file)),
                                   std::istreambuf_iterator<char>());
                
                RDeb822Source source = RDeb822Source::fromString(content);
                if (source.isValid()) {
                    sources.push_back(source);
                }
            }
        }
        return !sources.empty();
    } catch (const std::exception& e) {
        return false;
    }
}

bool RSourceManager::loadLegacySources() {
    std::ifstream file("/etc/apt/sources.list");
    if (!file.is_open()) return false;
    
    std::string line;
    while (std::getline(file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        
        std::istringstream iss(line);
        std::string type, uri, dist, components;
        iss >> type >> uri >> dist;
        
        if (type.empty() || uri.empty() || dist.empty()) continue;
        
        RDeb822Source source;
        source.setTypes(type);
        source.setURIs(uri);
        source.setSuites(dist);
        
        // Read components
        std::string comp;
        while (iss >> comp) {
            if (!components.empty()) components += " ";
            components += comp;
        }
        source.setComponents(components);
        
        sources.push_back(source);
    }
    
    return true;
}

bool RSourceManager::saveSources() {
    if (useDeb822Format) {
        return saveDeb822Sources();
    }
    return saveLegacySources();
}

bool RSourceManager::saveDeb822Sources() {
    const std::string sourcesDir = "/etc/apt/sources.list.d";
    
    try {
        // Create directory if it doesn't exist
        if (!std::filesystem::exists(sourcesDir)) {
            std::filesystem::create_directories(sourcesDir);
        }
        
        // Save to debian.sources
        std::ofstream file(sourcesDir + "/debian.sources");
        if (!file.is_open()) return false;
        
        for (const auto& source : sources) {
            file << source.toString() << "\n\n";
        }
        
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

bool RSourceManager::saveLegacySources() {
    std::ofstream file("/etc/apt/sources.list");
    if (!file.is_open()) return false;
    
    for (const auto& source : sources) {
        if (!source.isEnabled()) continue;
        
        file << source.getTypes() << " " 
             << source.getURIs() << " "
             << source.getSuites();
        
        if (!source.getComponents().empty()) {
            file << " " << source.getComponents();
        }
        
        if (!source.getSignedBy().empty()) {
            file << " [signed-by=" << source.getSignedBy() << "]";
        }
        
        file << "\n";
    }
    
    return true;
}

std::vector<RDeb822Source> RSourceManager::getSources() const {
    return sources;
}

void RSourceManager::addSource(const RDeb822Source& source) {
    sources.push_back(source);
}

void RSourceManager::removeSource(const RDeb822Source& source) {
    sources.erase(
        std::remove_if(sources.begin(), sources.end(),
                      [&source](const RDeb822Source& s) { return s == source; }),
        sources.end()
    );
}

void RSourceManager::updateSource(const RDeb822Source& oldSource, const RDeb822Source& newSource) {
    for (auto& source : sources) {
        if (source == oldSource) {
            source = newSource;
            break;
        }
    }
}

RDeb822Source RSourceManager::createSourceFromFields(const std::map<std::string, std::string>& fields) {
    RDeb822Source source;
    
    if (fields.count("Types")) source.setTypes(fields.at("Types"));
    if (fields.count("URIs")) source.setURIs(fields.at("URIs"));
    if (fields.count("Suites")) source.setSuites(fields.at("Suites"));
    if (fields.count("Components")) source.setComponents(fields.at("Components"));
    if (fields.count("Signed-By")) source.setSignedBy(fields.at("Signed-By"));
    
    return source;
}

std::string RSourceManager::trim(const std::string& str) {
    const std::string whitespace = " \t\r\n";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return "";
    }
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

bool RSourceManager::shouldConvertToDeb822() const {
    return !useDeb822Format && !sources.empty();
}

bool RSourceManager::askUserAboutConversion() {
    GtkWidget* dialog = gtk_message_dialog_new(NULL,
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_YES_NO,
        "Would you like to convert your sources to the new Deb822 format?\n"
        "This will create files in /etc/apt/sources.list.d/");
    
    gint result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    
    return result == GTK_RESPONSE_YES;
} 