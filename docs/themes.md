# Themes

This page covers `Theme_Engine`: the theme properties, the built-in themes, theme files in JSON, and the per-widget properties. QtTUI has no style sheets and no `QPalette`; a theme gives each widget type one set of colors and attributes.

## ThemeProperties

`#include <QtTUI/Theme_Engine.h>`

The colors and attributes of one widget type in one theme. `QTuiWidget::themeProperties()` returns the properties for a widget. Use them in `paintEvent()` (see [events and rendering](events-and-rendering.md#custom-paintevent-example)).

### Fields

| Field | Default | Description |
| --- | --- | --- |
| `Color foreground` | (255, 255, 255) | The text color. |
| `Color background` | (0, 0, 0) | The background color. |
| `Color focusForeground` | (255, 255, 255) | The text color of the focus widget, the current item and a selection. |
| `Color focusBackground` | (0, 80, 160) | The background color of the focus widget, the current item and a selection. |
| `Color disabledForeground` | (128, 128, 128) | The text color of a disabled widget. |
| `Color disabledBackground` | (0, 0, 0) | The background color of a disabled widget. |
| `QString borderStyle` | `"single"` | The border characters. See [Border styles](#border-styles). |
| `bool bold` | false | Bold text. |
| `bool italic` | false | Italic text. |
| `bool underline` | false | Underlined text. |

## Theme_Engine

`#include <QtTUI/Theme_Engine.h>` · Inherits: `QObject`

A singleton that holds the themes and gives the properties for each widget type. Use `Theme_Engine::instance()`. A theme change repaints all visible widgets.

### Example

```cpp
#include <QtTUI/QtTUI>

using namespace QtTUI;

int main(int argc, char *argv[])
{
    QTuiApplication app(argc, argv);
    Theme_Engine *engine = Theme_Engine::instance();

    // Load a theme file, then select the theme by the "name" in the file
    if (engine->loadTheme(QStringLiteral("ocean.json")))
        engine->setActiveTheme(QStringLiteral("ocean"));
    else
        engine->setActiveTheme(QStringLiteral("light"));

    QTuiMainWindow window;
    app.setMainWindow(&window);

    auto *title = new QTuiLabel(QStringLiteral("Status"), &window);
    title->setGeometry(QRect(1, 1, 20, 1));

    // Different colors for one widget only
    ThemeProperties header = title->themeProperties();
    header.foreground = engine->resolveColor(QStringLiteral("#FFD700"));
    header.bold = true;
    engine->setWidgetOverride(title, header);

    window.show();
    app.triggerRender();
    return app.exec();
}
```

### Functions

| Function | Description |
| --- | --- |
| `static Theme_Engine *instance()` | Returns the engine. The first call makes it, with the `"default"` theme active. |
| `bool loadTheme(const QString &filePath)` | Reads a theme file and registers the theme under its `"name"`, as `registerTheme()` does. It does not make the theme active. It returns false and shows a warning if the file cannot be opened, is not valid JSON, or has no `"name"`. |
| `void registerTheme(const QString &name, const QHash<QString, ThemeProperties> &properties)` | Registers a theme from C++. The keys are widget type names and `"default"`. A theme with the same name is replaced. It does not make the theme active. If the theme is the active theme, it emits `themeChanged()`. |
| `void setActiveTheme(const QString &name)` | Makes the theme active and emits `themeChanged()`. An unknown name gives a warning and changes nothing. If the theme is already active, nothing occurs. |
| `QString activeTheme() const` | Returns the name of the active theme. |
| `ThemeProperties properties(const QString &widgetType) const` | Returns the properties of a widget type in the active theme. See [Property lookup](#property-lookup). |
| `void setWidgetOverride(QTuiWidget *widget, const ThemeProperties &props)` | Gives one widget its own properties. They replace the theme properties of that widget in all themes. The engine removes the override when the widget is destroyed. The widget repaints. |
| `void clearWidgetOverride(QTuiWidget *widget)` | Removes the override. The widget uses the theme again and repaints. |
| `ThemeProperties effectiveProperties(const QTuiWidget *widget) const` | Returns the override of the widget, or else `properties()` for its class name. A null widget gives the `"default"` properties. `QTuiWidget::themeProperties()` calls this function. |
| `void setColorAlias(const QString &name, const Color &color)` | Adds a color name for `resolveColor()`. |
| `Color resolveColor(const QString &nameOrHex) const` | Returns the color of an alias name or of a `"#RRGGBB"` string. Other values give black and a warning. |
| `static BorderChars borderChars(const QString &style)` | Returns the border characters of a style. |

### Signals

| Signal | Description |
| --- | --- |
| `void themeChanged(const QString &themeName)` | `setActiveTheme()` changes the active theme. The render scheduler then marks all visible widgets dirty. |

### Built-in themes

| Name | Description |
| --- | --- |
| `default` | The `ThemeProperties` defaults: white text on black, a blue focus background (0, 80, 160), single borders. |
| `light` | Dark text (30, 30, 30) on a light background (250, 250, 250), white text on a blue focus background (0, 95, 184), gray disabled text (150, 150, 150). |

Both themes have a `"default"` entry and an entry for 20 widget types. All entries of one theme are equal. Thus all widget types look the same in a built-in theme.

### Property lookup

`properties(widgetType)` uses these steps:

1. Find the active theme. If it is not there, use the `"default"` theme.
2. If the theme has an entry for `widgetType`, return it.
3. If the theme has a `"default"` entry, return it.
4. Return the `ThemeProperties` defaults.

`effectiveProperties()` uses the class name without the namespace, for example `QTuiPushButton`. A subclass without `Q_OBJECT` has the class name of the nearest base class with `Q_OBJECT`. Thus it uses the theme entry of that base class.

### BorderChars

The six code points of a border: `topLeft`, `topRight`, `bottomLeft`, `bottomRight`, `horizontal` and `vertical`, all of type `char32_t`. `drawBorder()` in [RenderUtils](events-and-rendering.md#renderutils) uses them.

### Border styles

| Style | Characters |
| --- | --- |
| `single` | `┌ ┐ └ ┘ ─ │` |
| `double` | `╔ ╗ ╚ ╝ ═ ║` |
| `rounded` | `╭ ╮ ╰ ╯ ─ │` |
| `ascii` | `+ + + + - \|` |
| `none` or an unknown name | Spaces. |

## Theme files

`loadTheme()` reads a JSON object with these keys:

| Key | Description |
| --- | --- |
| `name` | The theme name. It is necessary. Use it with `setActiveTheme()`. |
| `colorAliases` | An object of color names and `"#RRGGBB"` values. A value in another format gives a warning. The engine also adds the aliases to the `resolveColor()` names. |
| `widgets` | An object. Each key is a widget type name (for example `QTuiPushButton`) or `"default"`. Each value is an object with the `ThemeProperties` fields. |

The keys of a property object are `foreground`, `background`, `focusForeground`, `focusBackground`, `disabledForeground`, `disabledBackground`, `borderStyle`, `bold`, `italic` and `underline`. A color is `"#RRGGBB"` or an alias name from the same file. An unknown color gives black without a warning.

The "default" entry rule:

- The engine reads the `"default"` entry first, from the `ThemeProperties` defaults.
- Each other entry starts from the `"default"` entry of the file. Thus an entry needs only the properties that are different.
- A widget type without an entry uses the `"default"` entry.
- If the file has no `"default"` entry, the other entries start from the `ThemeProperties` defaults. A widget type without an entry then gets the `ThemeProperties` defaults, not a color from the file.

### Example

```json
{
    "name": "ocean",
    "colorAliases": {
        "deep": "#0B1D33",
        "foam": "#E0F2F1",
        "coral": "#FF7F50"
    },
    "widgets": {
        "default": {
            "foreground": "foam",
            "background": "deep",
            "focusForeground": "deep",
            "focusBackground": "coral",
            "disabledForeground": "#5F7D95",
            "disabledBackground": "deep",
            "borderStyle": "rounded"
        },
        "QTuiPushButton": {
            "bold": true
        },
        "QTuiGroupBox": {
            "borderStyle": "double"
        }
    }
}
```

In this file, `QTuiPushButton` gets all colors of the `"default"` entry and bold text.

## Notes

- A color alias in a theme file must be in the same file. Aliases from `setColorAlias()` or from an earlier file do not work in `loadTheme()`.
- In `AnsiRenderer::NoColor` mode, the renderer uses the `"default"` entry of the active theme to find the focus and selection cells. See [AnsiRenderer](events-and-rendering.md#ansirenderer).
