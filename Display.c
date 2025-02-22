#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "inc/matriz.h"

#define BUTTON_A 5 // Pino do botão

char morse_code[10] = "";         // Buffer para armazenar o código Morse de uma letra
char message[100] = "";           // Buffer para armazenar a palavra completa
volatile char last_letter = '\0'; // Variável para armazenar a última letra digitada (sem exibir)
volatile int morse_index = 0;
volatile int msg_index = 0;
volatile uint64_t press_time = 0;
volatile uint64_t release_time = 0;
volatile uint64_t last_press_time = 0;
volatile int first_press = 1;
volatile int new_word = 0;
volatile bool callback_a = 0;

void gpio_irq_handler(uint gpio, uint32_t events);

// Tabela Morse para letras A-Z e números 0-9
typedef struct
{
    const char *morse;
    char letter;
} MorseCode;

MorseCode morse_table[] = {
    {".-", 'A'},   {"-...", 'B'}, {"-.-.", 'C'}, {"-..", 'D'}, 
    {".", 'E'},    {"..-.", 'F'}, {"--.", 'G'},  {"....", 'H'}, 
    {"..", 'I'},   {".---", 'J'}, {"-.-", 'K'},  {".-..", 'L'}, 
    {"--", 'M'},   {"-.", 'N'},   {"---", 'O'},  {".--.", 'P'}, 
    {"--.-", 'Q'}, {".-.", 'R'},  {"...", 'S'},  {"-", 'T'}, 
    {"..-", 'U'},  {"...-", 'V'}, {".--", 'W'},  {"-..-", 'X'}, 
    {"-.--", 'Y'}, {"--..", 'Z'}, {"-----", '0'}, {".----", '1'}, 
    {"..---", '2'}, {"...--", '3'}, {"....-", '4'}, {".....", '5'}, 
    {"-....", '6'}, {"--...", '7'}, {"---..", '8'}, {"----.", '9'}
};

// Converte sequência Morse para caractere
char morse_to_char(char *morse)
{
    for (int i = 0; i < sizeof(morse_table) / sizeof(MorseCode); i++)
    {
        if (strcmp(morse_table[i].morse, morse) == 0)
        {
            return morse_table[i].letter;
        }
    }
    return '?'; // Retorna '?' se não encontrado
}

void init_hardware()
{
    stdio_init_all(); // Inicializa UART para comunicação serial
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
}

void gpio_irq_handler(uint gpio, uint32_t events)
{
    callback_a = true;
}

void morse_converter()
{
    if (gpio_get(BUTTON_A) == 0)
    { // Botão pressionado
        if (press_time == 0)
        {
            press_time = time_us_64(); // Marca o tempo inicial do pressionamento

            if (!first_press)
            {
                uint64_t gap_duration = (press_time - last_press_time) / 1000; // Tempo desde última liberação

                if (gap_duration > 1500)
                { // Espaço entre palavras (tempo longo)
                    printf(" ");
                    strcat(message, " ");
                    new_word = 1;
                }
            }
            first_press = 0;
        }
    }
    else
    { // Botão solto
        if (press_time > 0)
        {
            release_time = time_us_64();
            uint64_t duration = (release_time - press_time) / 1000; // Converte para ms

            if (duration < 300)
            {
                morse_code[morse_index++] = '.'; // Pressão curta → ponto
            }
            else
            {
                morse_code[morse_index++] = '-'; // Pressão longa → traço
            }
            morse_code[morse_index] = '\0'; // Termina a string

            press_time = 0;                 // Reseta tempo de pressionamento
            last_press_time = release_time; // Atualiza o último tempo de liberação
        }
        else if (release_time > 0)
        {
            uint64_t pause_duration = (time_us_64() - release_time) / 1000;

            if (pause_duration > 500)
            {                                                  // Se tempo de pausa for maior, processa o código
                char decoded_char = morse_to_char(morse_code); // Decodifica a letra
                last_letter = decoded_char;                    // Armazena a última letra digitada

                printf("%c", decoded_char); // Imprime letra imediatamente

                if (msg_index < sizeof(message) - 2)
                {
                    if (new_word)
                    { // Se for uma nova palavra, adiciona um espaço antes da letra
                        message[msg_index++] = ' ';
                        new_word = 0;
                    }
                    message[msg_index++] = decoded_char;
                    message[msg_index] = '\0';
                }

                morse_index = 0;
                memset(morse_code, 0, sizeof(morse_code)); // Limpa o buffer
                release_time = 0;
            }
        }
    }

    sleep_ms(10); // Pequeno atraso para evitar leitura errada
}

int main()
{
    PIO pio = init_pio();
    init_hardware();
    sleep_ms(50);
    desenho_pio(apagado, 0, pio, 0, 0.0, 0.0, 0.1);

    desenho_pio(apagado, 0, pio, 0, 0.0, 0.0, 0.1);

    while (1)
    {
        if (callback_a == 1)
        {
            morse_converter();
            imprime_numeros_letras(last_letter, pio, 0);
        }
    }
}