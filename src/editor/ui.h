#pragma once

typedef struct editor editor_t;

// show error message
void editor_show_error(editor_t *ed, const char *fmt, ...);

// show all UI
void editor_show_ui(editor_t *ed);

// show open dialog
void editor_ui_show_open_level(editor_t *ed);

// show saveas dialog
void editor_ui_show_saveas(editor_t *ed);

// show new level dialog
void editor_ui_show_new_level(editor_t *ed);
