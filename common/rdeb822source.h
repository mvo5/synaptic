class RDeb822Source {
public:
    RDeb822Source();
    RDeb822Source(const std::string& types, const std::string& uris,
                  const std::string& suites, const std::string& components = "");

    bool isValid() const;
    std::string toString() const;
    static RDeb822Source fromString(const std::string& content);
    bool operator==(const RDeb822Source& other) const;
    bool operator!=(const RDeb822Source& other) const;

    // Getters
    std::string getTypes() const { return types; }
    std::string getURIs() const { return uris; }
    std::string getSuites() const { return suites; }
    std::string getComponents() const { return components; }
    std::string getSignedBy() const { return signedBy; }
    bool isEnabled() const { return enabled; }

    // Setters
    void setTypes(const std::string& t) { types = t; }
    void setURIs(const std::string& u) { uris = u; }
    void setSuites(const std::string& s) { suites = s; }
    void setComponents(const std::string& c) { components = c; }
    void setSignedBy(const std::string& s) { signedBy = s; }
    void setEnabled(bool e) { enabled = e; }

    static std::string trim(const std::string& str);

private:
    std::string types;      // Changed from type to types
    std::string uris;       // Changed from uri to uris
    std::string suites;     // Changed from dist to suites
    std::string components; // Changed from sections to components
    std::string signedBy;
    bool enabled;
}; 