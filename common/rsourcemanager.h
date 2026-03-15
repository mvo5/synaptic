class RSourceManager {
public:
    RSourceManager();
    ~RSourceManager();

    bool loadSources();
    bool saveSources();
    std::vector<RDeb822Source> getSources() const;
    void addSource(const RDeb822Source& source);
    void removeSource(const RDeb822Source& source);
    void updateSource(const RDeb822Source& oldSource, const RDeb822Source& newSource);

    static RDeb822Source createSourceFromFields(const std::map<std::string, std::string>& fields);
    static std::string trim(const std::string& str);

private:
    bool loadLegacySources();
    bool loadDeb822Sources();
    bool saveLegacySources();
    bool saveDeb822Sources();
    bool shouldConvertToDeb822() const;
    bool askUserAboutConversion();

    std::vector<RDeb822Source> sources;
    bool useDeb822Format;
}; 