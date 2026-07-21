#ifndef QTTUI_GALLERYBUILDER_H
#define QTTUI_GALLERYBUILDER_H

namespace QtTUI {
class QTuiMainWindow;
}

// The gallery theme: a dark background with one accent color
void applyGalleryTheme();

// Builds the menu bar, the status bar and the category tabs in window.
// The tab bar gets focus, so the arrow keys switch tabs.
void buildGallery(QtTUI::QTuiMainWindow &window);

#endif // QTTUI_GALLERYBUILDER_H
