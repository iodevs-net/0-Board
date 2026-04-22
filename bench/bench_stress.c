#include <stdio.h>
#include <time.h>
#include "ui.h"
#include "layout.h"

int main() {
    UIState state;
    // Mock de inicialización mínima
    state.display = XOpenDisplay(NULL);
    if (!state.display) return 1;
    layout_init(layout_get_default());
    ui_init(&state, layout_get_default());
    
    clock_t start = clock();
    for(int i=0; i<100; i++) {
        ui_set_size(&state, i % 3); // Cambiamos tamaño (Gatilla precompute_grid)
    }
    clock_t end = clock();
    
    printf("Tiempo para 100 resizes (OPTIMIZADO): %.4f segundos\n", (double)(end-start)/CLOCKS_PER_SEC);
    return 0;
}
