#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

// Simulación de la lógica ORIGINAL (strcmp + cálculos en caliente)
typedef struct {
    char label[16];
    int width_weight;
} KeyDefOld;

typedef struct {
    int x, y, w, h;
    KeyDefOld *key;
    int flags;
} KeyRectOld;

void precompute_old(KeyRectOld *rects, KeyDefOld *keys, int num_keys) {
    for (int i = 0; i < num_keys; i++) {
        rects[i].key = &keys[i];
        // Batería de strcmp ineficiente
        if (strcmp(keys[i].label, "shift") == 0) rects[i].flags = 1;
        else if (strcmp(keys[i].label, "ctrl") == 0) rects[i].flags = 4;
        else if (strcmp(keys[i].label, "alt") == 0) rects[i].flags = 4;
        else if (strcmp(keys[i].label, "del") == 0) rects[i].flags = 4;
        else rects[i].flags = 0;
        // Cálculos de geometría omitidos por brevedad, centrados en el cuello de botella
    }
}

// Simulación de la lógica NUEVA (flags pre-calculados O(1))
typedef struct {
    char label[16];
    int width_weight;
    int flags;
} KeyDefNew;

typedef struct {
    int x, y, w, h;
    KeyDefNew *key;
    int flags;
} KeyRectNew;

void precompute_new(KeyRectNew *rects, KeyDefNew *keys, int num_keys) {
    for (int i = 0; i < num_keys; i++) {
        rects[i].key = &keys[i];
        rects[i].flags = keys[i].flags; // Acceso directo a memoria
    }
}

int main() {
    int num_keys = 100;
    int iterations = 1000000;
    
    KeyDefOld keys_old[100];
    KeyRectOld rects_old[100];
    KeyDefNew keys_new[100];
    KeyRectNew rects_new[100];
    
    for(int i=0; i<100; i++) {
        strcpy(keys_old[i].label, (i % 10 == 0) ? "shift" : "a");
        strcpy(keys_new[i].label, keys_old[i].label);
        keys_new[i].flags = (i % 10 == 0) ? 1 : 0;
    }

    printf("--- Benchmark de Geometría (1,000,000 iteraciones) ---\n");

    clock_t start = clock();
    for (int i = 0; i < iterations; i++) precompute_old(rects_old, keys_old, num_keys);
    clock_t end = clock();
    double time_old = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Versión ORIGINAL (strcmp): %.4f segundos\n", time_old);

    start = clock();
    for (int i = 0; i < iterations; i++) precompute_new(rects_new, keys_new, num_keys);
    end = clock();
    double time_new = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Versión NUEVA (flags):    %.4f segundos\n", time_new);

    printf("Mejora de rendimiento:    %.1f%%\n", (time_old / time_new - 1.0) * 100.0);
    
    printf("\n--- Análisis de Estructura (Padding) ---\n");
    printf("Tamaño KeyRectOld: %lu bytes\n", sizeof(KeyRectOld));
    printf("Tamaño KeyRectNew: %lu bytes\n", sizeof(KeyRectNew));

    return 0;
}
