#include "engine.h"
#include "layout.h"
#include "ui.h"
#include <stdio.h>

extern Layout apple_layout;

int main() {
    printf("Iniciando xvkbd_pro (Motor Cairo)...\n");
    
    EngineConfig config = {NULL, true, false};
    engine_init(&config);
    
    UIState ui;
    ui_init(&ui, &apple_layout);
    ui_loop(&ui);
    
    return 0;
}
