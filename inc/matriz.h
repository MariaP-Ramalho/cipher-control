#include "inc/numbers.h"
#include "display.pio.h"
#include "hardware/pio.h"

#define WS2812_PIN 7  // Pino onde os LEDs estão conectados
#define IS_RGBW false // Define se os LEDs possuem um canal branco adicional

int getIndex(int x, int y)
{
    // Se a linha for par (0, 2, 4), percorremos da esquerda para a direita.
    // Se a linha for ímpar (1, 3), percorremos da direita para a esquerda.
    if (y % 2 == 0)
    {
        return 24 - (y * 5 + x); // Linha par (esquerda para direita).
    }
    else
    {
        return 24 - (y * 5 + (4 - x)); // Linha ímpar (direita para esquerda).
    }
}


uint matrix_rgb(float r, float g, float b)
{
    unsigned char R, G, B;
    R = r * 255;
    G = g * 255;
    B = b * 255;
    return (G << 24) | (R << 16) | (B << 8);
}

void desenho_pio(bool *desenho, uint32_t valor_led, PIO pio, uint sm, double r, double g, double b)
{
    for (int y = 0; y < 5; y++) // Percorre as linhas da matriz
    {
        for (int x = 0; x < 5; x++) // Percorre as colunas da matriz
        {
            int real_index = getIndex(x, y); // Obtém o índice correto com base na posição
            valor_led = matrix_rgb(desenho[real_index] * r, desenho[real_index] * g, desenho[real_index] * b);
            pio_sm_put_blocking(pio, sm, valor_led);
        }
    }
}

void imprime_numeros_letras(char index, PIO pio, uint sm)
{
    static const bool* caracteres[36] = {
        numero_0, numero_1, numero_2, numero_3, numero_4,
        numero_5, numero_6, numero_7, numero_8, numero_9,
        letra_A, letra_B, letra_C, letra_D, letra_E,
        letra_F, letra_G, letra_H, letra_I, letra_J,
        letra_K, letra_L, letra_M, letra_N, letra_O,
        letra_P, letra_Q, letra_R, letra_S, letra_T,
        letra_U, letra_V, letra_W, letra_X, letra_Y, letra_Z
    };

    uint32_t valor_led = 0; // Inicializa valor_led antes de passar para desenho_pio

    if (index >= '0' && index <= '9') {
        desenho_pio((bool*)caracteres[index - '0'], valor_led, pio, sm, 0.0, 0.0, 0.1);
    } 
    else if (index >= 'A' && index <= 'Z') {
        desenho_pio((bool*)caracteres[index - 'A' + 10], valor_led, pio, sm, 0.0, 0.0, 0.1);
    }
}

PIO init_pio()
{
  // Inicializa a PIO
  PIO pio = pio0;
  int sm = 0;
  uint offset = pio_add_program(pio, &ws2812_program);

  ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

  desenho_pio(numero_0, 0, pio, sm, 0.0, 0.0, 0.1);

  return pio;
}