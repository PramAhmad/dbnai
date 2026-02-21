#include "css.h"
void setup_css(void) {
    GtkCssProvider *provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider,
        ".navbar {"
        "  border-bottom: 1px solid #ddd;"
        "  padding: 0 6px;"
        "}"
        ".buttontransparent {"
        "  border: none;"
        "  background: transparent;"
        "  padding: 0;"
        "  margin: 0;"
        "  box-shadow: none;"
        "  font-weight: normal;"
        "}"
        ".buttontransparent:hover {"
        "  color: #007acc;"
        "  border-radius: 4px;"
        "}"
        ".sidebar {"
        "  background: #f5f5f5;"
        "  border-right: 1px solid #ccc;"
        "  border-bottom: 1px solid #ccc;"
        "  padding: 10px;"
        "}"
        "popover {"
        "  background: #f5f5f5;"
        "  box-shadow: none;"
        "  border-radius: 0;"
        "}"
        ".content-area {"
        "  padding: 10px;"
        "}",
        -1);
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_USER);
    g_object_unref(provider);
}
