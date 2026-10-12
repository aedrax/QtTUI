# QtTUI documentation

QtTUI copies the Qt Widgets API for the terminal. Most classes have the same
name as the Qt class, with the prefix `QTui` in place of `Q`, and live in
the `QtTUI` namespace. These pages tell how each class works in QtTUI and
where it is different from Qt.

## Start

- [Getting started](getting-started.md): build, install, the first program,
  and `QTuiApplication`.
- [Widget basics](widget.md): `QTuiWidget`, the focus, the hover, tooltips
  and custom widgets. Also `QTuiClipboard`.

## Widgets

- [Buttons](buttons.md): push buttons, check boxes, radio buttons, tool
  buttons and button groups.
- [Text input](text-input.md): labels, line edits, text edits, validators,
  completers and combo boxes.
- [Numbers and dates](numbers-and-dates.md): spin boxes, date and time
  edits, sliders, scroll bars, dials, the calendar, LCD numbers and
  progress bars.
- [Containers](containers.md): frames, group boxes, tabs, stacked widgets,
  tool boxes, splitters, scroll areas, MDI windows, dock widgets and the
  size grip.
- [Layouts](layouts.md): box, grid, form and stacked layouts, and size
  policies.

## Data

- [Model/view](model-view.md): the list, table, tree and column views,
  selection, editing, delegates and the data widget mapper.
- [Item widgets](item-widgets.md): the list, table and tree widgets and
  their items.
- [Undo](undo.md): the undo stack and the undo view.

## Application

- [Main window](main-window.md): the main window, menus, actions,
  shortcuts, tool bars, the status bar and tooltips.
- [Dialogs](dialogs.md): dialogs, message boxes, input dialogs, progress
  dialogs, file dialogs and wizards.

## Internals

- [Events and rendering](events-and-rendering.md): the input path, the
  render path, the screen buffer, the paint helpers and the environment
  variables.
- [Themes](themes.md): `Theme_Engine`, the built-in themes and JSON themes.
- [UI loader](ui-loader.md): widget trees from JSON.

## Keyboard

Each mouse action in QtTUI also has a key. The keys that work in all
windows:

| Key | Action |
| --- | --- |
| Tab, Shift+Tab | Move the focus to the next or the previous widget. |
| F1 | Show the tooltip of the focus widget. |
| F10 | Open the first menu of the menu bar. |
| Shift+F10, Menu | Open the context menu of the focus widget. |
| Ctrl+Z | Suspend the application, as in a shell. In a text editor, Ctrl+Z is undo. |

Each page has a "Keys" table for the widgets on it.

## Examples

Each C++ example on these pages is a complete file, and it compiles. The
`demos/` directory has 4 programs. `demo_gallery` shows all widgets.
