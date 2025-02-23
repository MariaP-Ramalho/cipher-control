#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/timer.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "inc/matriz.h"
#include "hardware/pwm.h"

// Definição dos pinos
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define BUTTON_A 5 // Pino do botão
#define BUTTON_B 6 // Pino do botão
#define BUZZER 10  // Defina o pino do buzzer

char morse_code[10] = "";         // Buffer para armazenar o código Morse de uma letra
char message[100] = "";           // Buffer para armazenar a palavra completa
static volatile uint a = 0;
static volatile uint b = 0;
volatile char last_letter = '\0'; // Variável para armazenar a última letra digitada (sem exibir)
volatile int morse_index = 0;
volatile int msg_index = 0;
volatile uint64_t press_time = 0;
volatile uint64_t release_time = 0;
volatile uint64_t last_press_time = 0;
volatile int first_press = 1;
volatile int new_word = 0;
volatile bool callback_a = 0;
volatile bool callback_b = 0;
ssd1306_t ssd;

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
    return 'a'; // Retorna '?' se não encontrado
}

void init_hardware()
{
    stdio_init_all(); // Inicializa UART para comunicação serial

    // Inicializa botao A
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // Inicializa botao B
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler); // Habilita interrupção para o botão B

    // I2C Initialisation. Using it at 400Khz.
    i2c_init(I2C_PORT, 400 * 1000);

    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
    gpio_pull_up(I2C_SDA);                                        // Pull up the data line
    gpio_pull_up(I2C_SCL);                                        // Pull up the clock line
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd);                                         // Configura o display
    ssd1306_send_data(&ssd);

    // Envia os dados para o display
    // Limpa o display. O display inicia com todos os pixels apagados.
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    // Configuração do Buzzer
    gpio_set_function(BUZZER, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(BUZZER);
    pwm_set_wrap(slice_num, 50000);                                // Define um período para o PWM (ajustável)
    pwm_set_chan_level(slice_num, pwm_gpio_to_channel(BUZZER), 0); // Começa com volume 0
    pwm_set_enabled(slice_num, true);                              // Habilita PWM
}

void alterar_display()
{
    bool cor = true;
    cor = !cor;

    ssd1306_fill(&ssd, !cor);                     // Limpa o display
    ssd1306_rect(&ssd, 3, 3, 122, 58, cor, !cor); // Desenha um retângulo

    // Define os parâmetros do display
    const int max_chars_per_line = 14; // Limite de caracteres por linha
    const int max_lines = 5;           // Número máximo de linhas que cabem no display
    int message_length = strlen(message);

    for (int i = 0; i < max_lines; i++)
    {
        if (i * max_chars_per_line >= message_length)
            break; // Sai do loop se não houver mais caracteres para exibir

        // Calcula a posição inicial do trecho da string
        char line_buffer[max_chars_per_line + 1];
        strncpy(line_buffer, &message[i * max_chars_per_line], max_chars_per_line);
        line_buffer[max_chars_per_line] = '\0'; // Garante o término da string

        // Exibe a linha correspondente no display
        ssd1306_draw_string(&ssd, line_buffer, 10, 10 + (i * 10));
    }

    ssd1306_send_data(&ssd);
}

void gpio_irq_handler(uint gpio, uint32_t events)
{

    static absolute_time_t last_time = {0}; // Garante que a variável mantenha seu valor entre chamadas
    absolute_time_t current_time = get_absolute_time();

    a++;
    printf("Sem debounce: %d\n", a);

    // Calcula a diferença de tempo em microsegundos
    int64_t diff = absolute_time_diff_us(last_time, current_time);
    printf("Tempo decorrido: %lld us\n", diff);

    if (diff > 250000) // 250ms
{
    if (!gpio_get(BUTTON_A) || !gpio_get(BUTTON_B)) // Verifica se ainda está pressionado
    {
        last_time = current_time; // Atualiza o tempo da última interrupção válida

        if (gpio == BUTTON_A)
        {
            callback_a = true;
            b++;
            printf("Com debounce: %d\n", b);
        }
        else if (gpio == BUTTON_B)
        {
            callback_b = true;
            printf("Com debounce: %d\n", b);
        }
    }
}
}

void morse_converter()
{
    static bool buzzer_on = false; // Estado do buzzer

    if (gpio_get(BUTTON_A) == 0)
    { // Botão pressionado
        if (press_time == 0)
        {
            press_time = time_us_64(); // Marca o tempo inicial do pressionamento

            uint slice_num = pwm_gpio_to_slice_num(BUZZER);
            pwm_set_chan_level(slice_num, pwm_gpio_to_channel(BUZZER), 12000); // Define volume do buzzer
            buzzer_on = true;

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
        if (buzzer_on)
        {
            uint slice_num = pwm_gpio_to_slice_num(BUZZER);
            pwm_set_chan_level(slice_num, pwm_gpio_to_channel(BUZZER), 0); // Para o som
            buzzer_on = false;
        }

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
            alterar_display();
        }

        if (callback_b == 1)
        {
            if (msg_index > 0)
            {
                msg_index--;               // Remove o último caractere
                message[msg_index] = '\0'; // Atualiza a string
                printf("%s\n", message);
                alterar_display(); // Atualiza o display após backspace
            }
            callback_b = false;
        }
    }
}