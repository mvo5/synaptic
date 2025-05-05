#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/TestFixture.h>
#include <cppunit/TestAssert.h>
#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>
#include <cppunit/TestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestListener.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/XmlOutputter.h>

#include "common/rpackagemanager.h"
#include "gtk/rgrepositorywindow.h"

class TestDeb822Integration : public CppUnit::TestFixture {
    CPPUNIT_TEST_SUITE(TestDeb822Integration);
    CPPUNIT_TEST(testSourceFileOperations);
    CPPUNIT_TEST(testAptIntegration);
    CPPUNIT_TEST(testSourceListRefresh);
    CPPUNIT_TEST(testSourceValidation);
    CPPUNIT_TEST(testSourceParsing);
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp() override {
        // Create temporary test directory
        testDir = std::filesystem::temp_directory_path() / "synaptic_test";
        std::filesystem::create_directories(testDir);
        
        // Initialize source manager with test directory
        sourceManager = new RSourceManager(testDir.string());
    }
    
    void tearDown() override {
        delete sourceManager;
        std::filesystem::remove_all(testDir);
    }
    
    void testSourceFileOperations() {
        // Test valid source
        RDeb822Source source;
        source.setTypes("deb");
        source.setUris("http://example.com");
        source.setSuites("stable");
        source.setComponents("main");
        
        // Write source to file
        std::string filename = testDir / "test.sources";
        CPPUNIT_ASSERT(sourceManager->writeSourceFile(filename.string(), source));
        
        // Read source from file
        RDeb822Source readSource = sourceManager->readSourceFile(filename.string());
        CPPUNIT_ASSERT(readSource.isValid());
        CPPUNIT_ASSERT_EQUAL(std::string("deb"), readSource.getTypes());
        CPPUNIT_ASSERT_EQUAL(std::string("http://example.com"), readSource.getUris());
        CPPUNIT_ASSERT_EQUAL(std::string("stable"), readSource.getSuites());
        CPPUNIT_ASSERT_EQUAL(std::string("main"), readSource.getComponents());
        
        // Test disabled source
        source.setEnabled(false);
        CPPUNIT_ASSERT(sourceManager->writeSourceFile(filename.string(), source));
        readSource = sourceManager->readSourceFile(filename.string());
        CPPUNIT_ASSERT(!readSource.isEnabled());
        
        // Test multiple URIs and suites
        source.setEnabled(true);
        source.setUris("http://example.com ftp://mirror.example.com");
        source.setSuites("stable testing");
        CPPUNIT_ASSERT(sourceManager->writeSourceFile(filename.string(), source));
        readSource = sourceManager->readSourceFile(filename.string());
        CPPUNIT_ASSERT(readSource.isValid());
        CPPUNIT_ASSERT_EQUAL(std::string("http://example.com ftp://mirror.example.com"), 
                           readSource.getUris());
        CPPUNIT_ASSERT_EQUAL(std::string("stable testing"), readSource.getSuites());
    }
    
    void testAptIntegration() {
        // Add a source
        RDeb822Source source;
        source.setTypes("deb");
        source.setUris("http://example.com");
        source.setSuites("stable");
        source.setComponents("main");
        
        CPPUNIT_ASSERT(sourceManager->addSource(source));
        CPPUNIT_ASSERT(sourceManager->saveSources());
        
        // Update APT sources
        CPPUNIT_ASSERT(sourceManager->updateAptSources());
        
        // Reload APT cache
        CPPUNIT_ASSERT(sourceManager->reloadAptCache());
    }
    
    void testSourceListRefresh() {
        // Create repository window
        RGRepositoryWindow* window = new RGRepositoryWindow();
        
        // Add a source
        RDeb822Source source;
        source.setTypes("deb");
        source.setUris("http://example.com");
        source.setSuites("stable");
        source.setComponents("main");
        
        CPPUNIT_ASSERT(sourceManager->addSource(source));
        CPPUNIT_ASSERT(sourceManager->saveSources());
        
        // Refresh source list
        window->refreshSourceList();
        
        // Verify source is in list
        GtkTreeModel* model = gtk_tree_view_get_model(GTK_TREE_VIEW(window->getSourceList()));
        GtkTreeIter iter;
        bool found = false;
        
        if (gtk_tree_model_get_iter_first(model, &iter)) {
            do {
                gchar *type, *uri, *suite, *components;
                gtk_tree_model_get(model, &iter,
                    0, &type,
                    1, &uri,
                    2, &suite,
                    3, &components,
                    -1);
                    
                if (std::string(type) == "deb" &&
                    std::string(uri) == "http://example.com" &&
                    std::string(suite) == "stable" &&
                    std::string(components) == "main") {
                    found = true;
                }
                
                g_free(type);
                g_free(uri);
                g_free(suite);
                g_free(components);
            } while (gtk_tree_model_iter_next(model, &iter));
        }
        
        CPPUNIT_ASSERT(found);
        delete window;
    }
    
    void testSourceValidation() {
        RDeb822Source source;
        
        // Test valid sources
        source.setTypes("deb");
        source.setUris("http://example.com");
        source.setSuites("stable");
        CPPUNIT_ASSERT(source.isValid());
        
        source.setTypes("deb-src");
        source.setUris("https://example.com");
        source.setSuites("testing");
        CPPUNIT_ASSERT(source.isValid());
        
        source.setUris("ftp://example.com");
        CPPUNIT_ASSERT(source.isValid());
        
        source.setUris("file:///media/cdrom");
        CPPUNIT_ASSERT(source.isValid());
        
        source.setUris("cdrom:");
        CPPUNIT_ASSERT(source.isValid());
        
        // Test invalid sources
        source.setTypes("invalid");
        CPPUNIT_ASSERT(!source.isValid());
        
        source.setTypes("deb");
        source.setUris("invalid://example.com");
        CPPUNIT_ASSERT(!source.isValid());
        
        source.setUris("");
        CPPUNIT_ASSERT(!source.isValid());
        
        source.setUris("http://example.com");
        source.setSuites("");
        CPPUNIT_ASSERT(!source.isValid());
    }
    
    void testSourceParsing() {
        // Test single line format
        std::string content = "deb http://example.com stable main";
        RDeb822Source source = RDeb822Source::fromString(content);
        CPPUNIT_ASSERT(source.isValid());
        CPPUNIT_ASSERT_EQUAL(std::string("deb"), source.getTypes());
        CPPUNIT_ASSERT_EQUAL(std::string("http://example.com"), source.getUris());
        CPPUNIT_ASSERT_EQUAL(std::string("stable"), source.getSuites());
        CPPUNIT_ASSERT_EQUAL(std::string("main"), source.getComponents());
        
        // Test disabled source
        content = "# Disabled: deb http://example.com stable main";
        source = RDeb822Source::fromString(content);
        CPPUNIT_ASSERT(!source.isEnabled());
        
        // Test multiple URIs and suites
        content = "deb http://example.com ftp://mirror.example.com stable testing main contrib";
        source = RDeb822Source::fromString(content);
        CPPUNIT_ASSERT(source.isValid());
        CPPUNIT_ASSERT_EQUAL(std::string("http://example.com ftp://mirror.example.com"), 
                           source.getUris());
        CPPUNIT_ASSERT_EQUAL(std::string("stable testing"), source.getSuites());
        CPPUNIT_ASSERT_EQUAL(std::string("main contrib"), source.getComponents());
        
        // Test with comments
        content = "# This is a comment\n"
                 "# Another comment\n"
                 "deb http://example.com stable main\n"
                 "# Trailing comment";
        source = RDeb822Source::fromString(content);
        CPPUNIT_ASSERT(source.isValid());
        CPPUNIT_ASSERT_EQUAL(std::string("deb"), source.getTypes());
    }

private:
    std::filesystem::path testDir;
    RSourceManager* sourceManager;
};

CPPUNIT_TEST_SUITE_REGISTRATION(TestDeb822Integration);

int main(int argc, char* argv[]) {
    // Initialize GTK
    gtk_init(&argc, &argv);
    
    // Create test runner
    CppUnit::TestResultCollector result;
    CppUnit::TestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());
    
    // Add listener
    CppUnit::BriefTestProgressListener progress;
    runner.eventManager().addListener(&progress);
    runner.eventManager().addListener(&result);
    
    // Run tests
    runner.run();
    
    // Output results
    CppUnit::CompilerOutputter outputter(&result, std::cerr);
    outputter.write();
    
    // Clean up GTK
    gtk_main_quit();
    
    return result.wasSuccessful() ? 0 : 1;
} 