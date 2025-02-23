#include "hardware/gpio.h"
#include "inc/ssd1306.h"
#include "inc/font.h"
#include "inc/matriz.h"
#include "inc/morse.h"

// Definição dos pinos
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define BUTTON_B 6 // Pino do botão

ssd1306_t ssd;

void init_hardware();
void alterar_display();
void gpio_irq_handler(uint gpio, uint32_t events);

int main()
{
    PIO pio = init_pio();
    init_hardware();
    sleep_ms(50);
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
            backspace();
            alterar_display();
            callback_b = false;
        }
    }
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