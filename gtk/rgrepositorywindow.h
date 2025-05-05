#ifndef RGREPOSITORYWINDOW_H
#define RGREPOSITORYWINDOW_H

#include <gtk/gtk.h>
#include "../common/rpackagemanager.h"

class RGRepositoryWindow {
public:
    RGRepositoryWindow();
    ~RGRepositoryWindow();
    
    // Window management
    void show();
    void hide();
    bool isVisible() const;
    
    // Source management
    void refreshSourceList();
    bool addSource();
    bool editSource(const RDeb822Source& source);
    bool removeSource(const RDeb822Source& source);
    
    // Signal handlers
    void onAddClicked();
    void onEditClicked();
    void onRemoveClicked();
    void onSourceSelected();
    
private:
    GtkWidget* window;
    GtkWidget* sourceList;
    GtkWidget* addButton;
    GtkWidget* editButton;
    GtkWidget* removeButton;
    GtkListStore* listStore;
    
    RSourceManager sourceManager;
    
    void createWindow();
    void createSourceList();
    void createButtons();
    void updateButtonStates();
    bool showSourceDialog(RDeb822Source* source = nullptr);
    bool confirmSourceRemoval(const RDeb822Source& source);
};

#endif // RGREPOSITORYWINDOW_H 