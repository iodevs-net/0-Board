#include <stdio.h>
#include <string.h>
#include <X11/keysym.h>
#include "test_common.h"
#include "layout.h"

int main() {
    printf("=== layout key integrity tests ===\n");

    Layout *layout = layout_get_default();
    layout_init(layout);

    TASSERT(layout->num_keys > 0, "layout has keys");
    TASSERT(layout->name != NULL, "layout has name");

    // Every key must have a valid keysym (except special keys)
    for (int i = 0; i < layout->num_keys; i++) {
        KeyDef *k = &layout->keys[i];
        if (k->normal == XK_VoidSymbol) {
            TASSERT(k->label != NULL, "void symbol has label");
        }
        if (k->label == NULL) {
            TASSERT(0, "key has null label");
            continue;
        }
        // Every letter key must have shifted variant
        if (strlen(k->label) == 1 && k->label[0] >= 'a' && k->label[0] <= 'z') {
            TASSERT(k->shifted != 0, "letter key has shifted keysym");
            TASSERT(k->shifted_label != NULL, "letter key has shifted label");
        }
        // Backspace, Enter, Space must be present
        if (strcmp(k->label, "⌫") == 0 || strcmp(k->label, "⏎") == 0 || strcmp(k->label, " ") == 0) {
            TASSERT(k->normal != 0, "special key has valid keysym");
        }
    }

    // Check critical keys exist
    int has_q = 0, has_enter = 0, has_backspace = 0, has_pipe = 0;
    for (int i = 0; i < layout->num_keys; i++) {
        KeyDef *k = &layout->keys[i];
        if (k->normal == XK_q) has_q = 1;
        if (k->normal == XK_Return) has_enter = 1;
        if (k->normal == XK_BackSpace) has_backspace = 1;
        if (k->shifted == XK_bar) has_pipe = 1;
    }
    TASSERT(has_q, "Q key present");
    TASSERT(has_enter, "Enter key present");
    TASSERT(has_backspace, "Backspace key present");
    TASSERT(has_pipe, "Pipe | accessible via shift+backslash");

    // Init idempotency: running layout_init twice shouldn't double-classify
    int shift_count_before = 0;
    for (int i = 0; i < layout->num_keys; i++)
        if (layout->keys[i].flags & KEYFLAG_SHIFT) shift_count_before++;

    layout_init(layout);

    int shift_count_after = 0;
    for (int i = 0; i < layout->num_keys; i++)
        if (layout->keys[i].flags & KEYFLAG_SHIFT) shift_count_after++;

    TASSERT(shift_count_before == shift_count_after, "layout_init is idempotent");

    TEST_REPORT();
}
