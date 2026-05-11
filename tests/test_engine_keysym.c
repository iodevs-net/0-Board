#include <stdio.h>
#include "test_common.h"
#include "keysym_util.h"

int main() {
    printf("=== keysym utility tests ===\n");

    // Letter detection
    TASSERT(keysym_is_letter(XK_a), "lowercase a is letter");
    TASSERT(keysym_is_letter(XK_Z), "uppercase Z is letter");
    TASSERT(keysym_is_letter(XK_ntilde), "n tilde is letter");
    TASSERT(!keysym_is_letter(XK_1), "1 is not letter");
    TASSERT(!keysym_is_letter(XK_Return), "Return is not letter");

    // Shift pair mappings
    TASSERT(keysym_get_base(XK_exclam) == XK_1, "exclam -> 1");
    TASSERT(keysym_get_base(XK_bar) == XK_backslash, "bar -> backslash");
    TASSERT(keysym_get_base(XK_braceleft) == XK_bracketleft, "braceleft -> bracketleft");
    TASSERT(keysym_get_base(XK_braceright) == XK_bracketright, "braceright -> bracketright");
    TASSERT(keysym_get_base(XK_asciitilde) == XK_grave, "asciitilde -> grave");
    TASSERT(keysym_get_base(XK_colon) == XK_semicolon, "colon -> semicolon");
    TASSERT(keysym_get_base(XK_quotedbl) == XK_apostrophe, "quotedbl -> apostrophe");
    TASSERT(keysym_get_base(XK_question) == XK_slash, "question -> slash");

    // Non-shifted symbols
    TASSERT(keysym_get_base(XK_a) == 0, "a has no base");
    TASSERT(keysym_get_base(XK_1) == 0, "1 has no base");
    TASSERT(keysym_get_base(0) == 0, "null keysym has no base");

    TEST_REPORT();
}
