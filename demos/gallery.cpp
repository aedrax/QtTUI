// Widget Gallery Demo, showcases every public QtTUI widget organized by category.
// Navigate tabs with arrow keys, widgets with Tab/Shift+Tab. Ctrl+Q to quit.
// The tabs are built in GalleryBuilder.cpp, which tests/test_gallery.cpp also uses.

#include "GalleryBuilder.h"
#include <QtTUI/QtTUI>

using namespace QtTUI;

int main(int argc, char *argv[])
{
    QTuiApplication app(argc, argv);
    applyGalleryTheme();

    QTuiMainWindow window;
    app.setMainWindow(&window);
    buildGallery(window);

    window.show();
    app.triggerRender();

    return app.exec();
}
