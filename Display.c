#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"

#define BUTTON_A 5  // Pino do botão

// Tabela Morse para letras A-Z e números 0-9
typedef struct {
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
char morse_to_char(char *morse) {
    for (int i = 0; i < sizeof(morse_table) / sizeof(MorseCode); i++) {
        if (strcmp(morse_table[i].morse, morse) == 0) {
            return morse_table[i].letter;
        }
    }
    return '?'; // Retorna ? se não encontrado
}

void setup() {
    stdio_init_all();  // Inicializa UART para comunicação serial
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
}

int main() {
    setup();

    char morse_code[10] = "";  // Buffer para armazenar o código Morse de uma letra
    int morse_index = 0;
    uint64_t press_time = 0, release_time = 0;
    uint64_t last_press_time = 0;
    int first_press = 1;

    while (1) {
        if (gpio_get(BUTTON_A) == 0) {  // Botão pressionado
            if (press_time == 0) {
                press_time = time_us_64();  // Marca o tempo inicial do pressionamento
                
                if (!first_press) {
                    uint64_t gap_duration = (press_time - last_press_time) / 1000;  // Tempo desde última liberação
                    
                    if (gap_duration > 500 && gap_duration <= 1500) {
                        printf("Espaço entre letras detectado\n");
                    } else if (gap_duration > 1500) {
                        printf("Espaço entre palavras detectado\n");
                        printf(" ");
                    }
                }
                first_press = 0;
            }
        } else {  // Botão solto
            if (press_time > 0) {
                release_time = time_us_64();
                uint64_t duration = (release_time - press_time) / 1000;  // Converte para ms

                if (duration < 300) {
                    morse_code[morse_index++] = '.';  // Pressão curta → ponto
                } else {
                    morse_code[morse_index++] = '-';  // Pressão longa → traço
                }
                morse_code[morse_index] = '\0';  // Termina a string

                press_time = 0;  // Reseta tempo de pressionamento
                last_press_time = release_time; // Atualiza o último tempo de liberação
            } else if (release_time > 0) {
                uint64_t pause_duration = (time_us_64() - release_time) / 1000;

                if (pause_duration > 500) {  // Se tempo de pausa for maior, processa o código
                    char decoded_char = morse_to_char(morse_code);
                    printf("Letra: %c\n", decoded_char);  // Exibe o caractere decodificado

                    morse_index = 0;
                    memset(morse_code, 0, sizeof(morse_code));  // Limpa o buffer
                    release_time = 0;
                }
            }
        }
        sleep_ms(10);  // Pequeno atraso para evitar leitura errada
    }
}

