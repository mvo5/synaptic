#include "rgrepositorywindow.h"
#include <glib/gi18n.h>

enum {
    COL_ENABLED,
    COL_TYPE,
    COL_URI,
    COL_SUITE,
    COL_COMPONENTS,
    COL_SOURCE_PTR,
    N_COLUMNS
};

RGRepositoryWindow::RGRepositoryWindow() {
    createWindow();
    createSourceList();
    createButtons();
    refreshSourceList();
}

RGRepositoryWindow::~RGRepositoryWindow() {
    if (window) {
        gtk_widget_destroy(window);
    }
}

void RGRepositoryWindow::createWindow() {
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Software Sources");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 400);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    
    GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    
    // Create source list
    sourceList = gtk_tree_view_new();
    gtk_box_pack_start(GTK_BOX(vbox), sourceList, TRUE, TRUE, 0);
    
    // Create button box
    GtkWidget* buttonBox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_button_box_set_layout(GTK_BUTTON_BOX(buttonBox), GTK_BUTTONBOX_END);
    gtk_box_pack_end(GTK_BOX(vbox), buttonBox, FALSE, FALSE, 0);
    
    // Create buttons
    addButton = gtk_button_new_with_label("Add");
    editButton = gtk_button_new_with_label("Edit");
    removeButton = gtk_button_new_with_label("Remove");
    
    gtk_container_add(GTK_CONTAINER(buttonBox), addButton);
    gtk_container_add(GTK_CONTAINER(buttonBox), editButton);
    gtk_container_add(GTK_CONTAINER(buttonBox), removeButton);
    
    // Connect signals
    g_signal_connect(addButton, "clicked", G_CALLBACK(onAddClicked), this);
    g_signal_connect(editButton, "clicked", G_CALLBACK(onEditClicked), this);
    g_signal_connect(removeButton, "clicked", G_CALLBACK(onRemoveClicked), this);
    
    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(sourceList));
    g_signal_connect(selection, "changed", G_CALLBACK(onSourceSelected), this);
    
    updateButtonStates();
}

void RGRepositoryWindow::createSourceList() {
    // Create list store
    listStore = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    gtk_tree_view_set_model(GTK_TREE_VIEW(sourceList), GTK_TREE_MODEL(listStore));
    
    // Create columns
    GtkCellRenderer* renderer = gtk_cell_renderer_text_new();
    
    GtkTreeViewColumn* typeCol = gtk_tree_view_column_new_with_attributes(
        "Type", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(sourceList), typeCol);
    
    GtkTreeViewColumn* uriCol = gtk_tree_view_column_new_with_attributes(
        "URI", renderer, "text", 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(sourceList), uriCol);
    
    GtkTreeViewColumn* suiteCol = gtk_tree_view_column_new_with_attributes(
        "Suite", renderer, "text", 2, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(sourceList), suiteCol);
    
    GtkTreeViewColumn* compCol = gtk_tree_view_column_new_with_attributes(
        "Components", renderer, "text", 3, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(sourceList), compCol);
}

void RGRepositoryWindow::createButtons() {
    gtk_widget_set_sensitive(addButton, TRUE);
    gtk_widget_set_sensitive(editButton, FALSE);
    gtk_widget_set_sensitive(removeButton, FALSE);
}

void RGRepositoryWindow::refreshSourceList() {
    gtk_list_store_clear(listStore);
    
    for (const auto& source : sourceManager.getSources()) {
        GtkTreeIter iter;
        gtk_list_store_append(listStore, &iter);
        gtk_list_store_set(listStore, &iter,
            0, source.getTypes().c_str(),
            1, source.getUris().c_str(),
            2, source.getSuites().c_str(),
            3, source.getComponents().c_str(),
            -1);
    }
}

void RGRepositoryWindow::show() {
    gtk_widget_show_all(window);
}

void RGRepositoryWindow::hide() {
    gtk_widget_hide(window);
}

bool RGRepositoryWindow::isVisible() const {
    return gtk_widget_get_visible(window);
}

void RGRepositoryWindow::onAddClicked() {
    if (showSourceDialog()) {
        refreshSourceList();
        sourceManager.saveSources();
        sourceManager.updateAptSources();
    }
}

void RGRepositoryWindow::onEditClicked() {
    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(sourceList));
    GtkTreeModel* model;
    GtkTreeIter iter;
    
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gchar *type, *uri, *suite, *components;
        gtk_tree_model_get(model, &iter,
            0, &type,
            1, &uri,
            2, &suite,
            3, &components,
            -1);
            
        RDeb822Source source;
        source.setTypes(type);
        source.setUris(uri);
        source.setSuites(suite);
        source.setComponents(components);
        
        g_free(type);
        g_free(uri);
        g_free(suite);
        g_free(components);
        
        if (editSource(source)) {
            refreshSourceList();
            sourceManager.saveSources();
            sourceManager.updateAptSources();
        }
    }
}

void RGRepositoryWindow::onRemoveClicked() {
    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(sourceList));
    GtkTreeModel* model;
    GtkTreeIter iter;
    
    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        gchar *type, *uri, *suite, *components;
        gtk_tree_model_get(model, &iter,
            0, &type,
            1, &uri,
            2, &suite,
            3, &components,
            -1);
            
        RDeb822Source source;
        source.setTypes(type);
        source.setUris(uri);
        source.setSuites(suite);
        source.setComponents(components);
        
        g_free(type);
        g_free(uri);
        g_free(suite);
        g_free(components);
        
        if (removeSource(source)) {
            refreshSourceList();
            sourceManager.saveSources();
            sourceManager.updateAptSources();
        }
    }
}

void RGRepositoryWindow::onSourceSelected() {
    GtkTreeSelection* selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(sourceList));
    bool hasSelection = gtk_tree_selection_get_selected(selection, NULL, NULL);
    
    gtk_widget_set_sensitive(editButton, hasSelection);
    gtk_widget_set_sensitive(removeButton, hasSelection);
}

bool RGRepositoryWindow::showSourceDialog(RDeb822Source* source) {
    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        source ? "Edit Source" : "Add Source",
        GTK_WINDOW(window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        "Cancel", GTK_RESPONSE_CANCEL,
        source ? "Save" : "Add", GTK_RESPONSE_ACCEPT,
        NULL);
        
    GtkWidget* content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 5);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 5);
    gtk_container_add(GTK_CONTAINER(content), grid);
    
    // Type field
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Type:"), 0, 0, 1, 1);
    GtkWidget* typeEntry = gtk_entry_new();
    if (source) gtk_entry_set_text(GTK_ENTRY(typeEntry), source->getTypes().c_str());
    gtk_grid_attach(GTK_GRID(grid), typeEntry, 1, 0, 1, 1);
    
    // URI field
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("URI:"), 0, 1, 1, 1);
    GtkWidget* uriEntry = gtk_entry_new();
    if (source) gtk_entry_set_text(GTK_ENTRY(uriEntry), source->getUris().c_str());
    gtk_grid_attach(GTK_GRID(grid), uriEntry, 1, 1, 1, 1);
    
    // Suite field
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Suite:"), 0, 2, 1, 1);
    GtkWidget* suiteEntry = gtk_entry_new();
    if (source) gtk_entry_set_text(GTK_ENTRY(suiteEntry), source->getSuites().c_str());
    gtk_grid_attach(GTK_GRID(grid), suiteEntry, 1, 2, 1, 1);
    
    // Components field
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Components:"), 0, 3, 1, 1);
    GtkWidget* compEntry = gtk_entry_new();
    if (source) gtk_entry_set_text(GTK_ENTRY(compEntry), source->getComponents().c_str());
    gtk_grid_attach(GTK_GRID(grid), compEntry, 1, 3, 1, 1);
    
    gtk_widget_show_all(dialog);
    
    bool result = false;
    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        RDeb822Source newSource;
        newSource.setTypes(gtk_entry_get_text(GTK_ENTRY(typeEntry)));
        newSource.setUris(gtk_entry_get_text(GTK_ENTRY(uriEntry)));
        newSource.setSuites(gtk_entry_get_text(GTK_ENTRY(suiteEntry)));
        newSource.setComponents(gtk_entry_get_text(GTK_ENTRY(compEntry)));
        
        if (newSource.isValid()) {
            if (source) {
                result = sourceManager.updateSource(*source, newSource);
            } else {
                result = sourceManager.addSource(newSource);
            }
        }
    }
    
    gtk_widget_destroy(dialog);
    return result;
}

bool RGRepositoryWindow::confirmSourceRemoval(const RDeb822Source& source) {
    GtkWidget* dialog = gtk_message_dialog_new(
        GTK_WINDOW(window),
        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_YES_NO,
        "Are you sure you want to remove this source?\n\n"
        "Type: %s\n"
        "URI: %s\n"
        "Suite: %s",
        source.getTypes().c_str(),
        source.getUris().c_str(),
        source.getSuites().c_str());
        
    bool result = gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES;
    gtk_widget_destroy(dialog);
    return result;
}

void RGRepositoryWindow::updateSource(const RDeb822Source& oldSource,
                                    const RDeb822Source& newSource) {
    if (_sourceManager->updateSource(oldSource, newSource)) {
        refreshSourceList();
    } else {
        GtkWidget *dialog = gtk_message_dialog_new(
            GTK_WINDOW(_window),
            GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
            GTK_MESSAGE_ERROR,
            GTK_BUTTONS_OK,
            _("Failed to update source. Please check the format."));
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
    }
}

// Static signal handlers
void RGRepositoryWindow::onAddClicked(GtkWidget*, gpointer data) {
    RGRepositoryWindow *self = static_cast<RGRepositoryWindow*>(data);
    self->onAddClicked();
}

void RGRepositoryWindow::onEditClicked(GtkWidget*, gpointer data) {
    RGRepositoryWindow *self = static_cast<RGRepositoryWindow*>(data);
    self->onEditClicked();
}

void RGRepositoryWindow::onRemoveClicked(GtkWidget*, gpointer data) {
    RGRepositoryWindow *self = static_cast<RGRepositoryWindow*>(data);
    self->onRemoveClicked();
}

void RGRepositoryWindow::onSourceSelected(GtkTreeSelection *selection, gpointer data) {
    RGRepositoryWindow *self = static_cast<RGRepositoryWindow*>(data);
    bool hasSelection = gtk_tree_selection_get_selected(selection, NULL, NULL);
    
    gtk_widget_set_sensitive(self->editButton, hasSelection);
    gtk_widget_set_sensitive(self->removeButton, hasSelection);
} 