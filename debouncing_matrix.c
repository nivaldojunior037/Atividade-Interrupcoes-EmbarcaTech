// Definição das biliotecas a serem utilizadas na execução
#include <stdio.h> 
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "debouncing_matrix.pio.h" 

#define NUM_PIXELS 25       // Definição do número de LEDs da matriz 5X5
#define LEDS_PIN 7          // Definição da porta GPIO da matriz de LEDs 5X5
#define LED_PIN_RED 13      // Definição da porta GPIO do led vermelho do conjunto RGB 
#define BUTTON_A 5          // Definição da porta GPIO do botão A
#define BUTTON_B 6          // Definição da porta GPIO do botão B
#define tempo 100           // Definição do intervalo de acordo com o qual o LED piscará

// Definição do contador e da marcação de tempo utilizados para efetuar o debouncing
static volatile uint num = 0;
static volatile uint32_t ultimo_tempo = 0;

// Definição de variáveis a serem utilizadas na função que cria os desenhos na matriz de LEDs
// Foram declaradas fora da main para que fosse possível chamar os desenhos na função que administra as interrupções 
static volatile double r = 0.0, b = 0.0, g = 0.0;
static volatile uint32_t valor_led;
static volatile PIO pio = pio0;
static volatile uint offset;
static volatile uint sm;

// Vetores usados para criar os desenhos na matriz de LEDs
double numero0[NUM_PIXELS] = {0.0, 0.5, 0.5, 0.5, 0.0,
                              0.0, 0.5, 0.0, 0.5, 0.0,
                              0.0, 0.5, 0.0, 0.5, 0.0,
                              0.0, 0.5, 0.0, 0.5, 0.0,
                              0.0, 0.5, 0.5, 0.5, 0.0}; 

double numero1[NUM_PIXELS] = {0.0, 0.0, 0.5, 0.0, 0.0,
                              0.0, 0.0, 0.5, 0.0, 0.0,
                              0.0, 0.0, 0.5, 0.0, 0.0,
                              0.0, 0.0, 0.5, 0.0, 0.0,
                              0.0, 0.0, 0.5, 0.0, 0.0}; 

double numero2[NUM_PIXELS] = {0.0, 0.5, 0.5, 0.5, 0.0,
                              0.0, 0.5, 0.0, 0.0, 0.0,
                              0.0, 0.5, 0.5, 0.5, 0.0,
                              0.0, 0.0, 0.0, 0.5, 0.0,
                              0.0, 0.5, 0.5, 0.5, 0.0}; 

double numero3[NUM_PIXELS] = {0.0, 0.5, 0.5, 0.5, 0.0,
                              0.0, 0.5, 0.0, 0.0, 0.0,
                              0.0, 0.5, 0.5, 0.5, 0.0,
                              0.0, 0.5, 0.0, 0.0, 0.0,
                              0.0, 0.5, 0.5, 0.5, 0.0}; 

double numero4[NUM_PIXELS] = {0.0, 0.5, 0.0, 0.5, 0.0,
                              0.0, 0.5, 0.0, 0.5, 0.0,
                              0.0, 0.5, 0.5, 0.5, 0.0,
                              0.0, 0.5, 0.0, 0.0, 0.0,
                              0.0, 0.0, 0.0, 0.5, 0.0}; 

double numero5[NUM_PIXELS] = {0.0, 0.5, 0.5, 0.5, 0.0,
                              0.0, 0.0, 0.0, 0.5, 0.0,
                              0.0, 0.5, 0.5, 0.5, 0.0,
                              0.0, 0.5, 0.0, 0.0, 0.0,
                              0.0, 0.5, 0.5, 0.5, 0.0}; 

double numero6[NUM_PIXELS] = {0.0, 0.5, 0.5, 0.5, 0.0,
                              0.0, 0.0, 0.0, 0.5, 0.0,
                              0.0, 0.5, 0.5, 0.5, 0.0,
                              0.0, 0.5, 0.0, 0.5, 0.0,
                              0.0, 0.5, 0.5, 0.5, 0.0}; 

double numero7[NUM_PIXELS] = {0.0, 0.5, 0.5, 0.5, 0.0,
                              0.0, 0.5, 0.0, 0.0, 0.0,
                              0.0, 0.0, 0.0, 0.5, 0.0,
                              0.0, 0.5, 0.0, 0.0, 0.0,
                              0.0, 0.0, 0.0, 0.5, 0.0}; 

double numero8[NUM_PIXELS] = {0.0, 0.5, 0.5, 0.5, 0.0,
                              0.0, 0.5, 0.0, 0.5, 0.0,
                              0.0, 0.5, 0.5, 0.5, 0.0,
                              0.0, 0.5, 0.0, 0.5, 0.0,
                              0.0, 0.5, 0.5, 0.5, 0.0}; 

double numero9[NUM_PIXELS] = {0.0, 0.5, 0.5, 0.5, 0.0,
                              0.0, 0.5, 0.0, 0.5, 0.0,
                              0.0, 0.5, 0.5, 0.5, 0.0,
                              0.0, 0.5, 0.0, 0.0, 0.0,
                              0.0, 0.0, 0.0, 0.5, 0.0};

// Função para converter os parâmetros r, g, b em um valor de 32 bits
uint32_t matrix_rgb(double b, double r, double g)
{
    unsigned char R, G, B;
    R = r * 255;
    G = g * 255;
    B = b * 255;
    return (G << 24) | (R << 16) | (B << 8);
}

// Função para formar os desenhos na matriz de LEDs 5x5
void desenhos(double *desenho, uint32_t valor_led, PIO pio, uint sm, double r, double g, double b)
{
    // O loop aciona cada LED da matriz com base em um valor de cor 
    for (int i = 0; i < NUM_PIXELS; i++){
        // Determinação da cor de cada LED
        uint32_t valor_led = matrix_rgb(desenho[24 - i]*r, desenho[24 - i]*g, desenho[24 - i]*b);
        // O valor é enviado ao LED para ser exibido
        pio_sm_put_blocking(pio, sm, valor_led);
    }
}

// Função que administrará a interrupção
static void gpio_irq_handler(uint gpio, uint32_t events);

// Função principal
int main()
{   
    // Inicialização dos comandos e variáveis necessárias
    stdio_init_all();
    offset = pio_add_program(pio, &debouncing_matrix_program);
    sm = pio_claim_unused_sm(pio, true);
    debouncing_matrix_program_init(pio, sm, offset, LEDS_PIN);

    // Inicialização do LED e definição como saída
    gpio_init(LED_PIN_RED);
    gpio_set_dir(LED_PIN_RED, GPIO_OUT);

    //Inicialização dos botões A e B, definição como entrada e acionamento do pull-up interno
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);

    // Interrupção com callback para cada um dos botões 
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    // A mtraiz de LEDs é inicialmente acionada com o desenho 0
    b = 1.0;
    r = 1.0;
    g = 1.0;
    desenhos(numero0, valor_led, pio, sm, b, r ,g);

    //Loop infinito de repetições 
    while (true) {
        
        //O LED vermelho do conjunto RGB é ligado 5 vezes em um segundo (acende por 100ms e desliga por 100ms)
        gpio_put(LED_PIN_RED, true);
        sleep_ms(tempo);
        gpio_put(LED_PIN_RED, false);
        sleep_ms(tempo);
    }

    return 0;
}

// Função de interrupção com debouncing
static void gpio_irq_handler(uint gpio, uint32_t events){
    // Criação de booleanos para obter o estado de cada botão durante a interrupção
    bool incremento = gpio_get(BUTTON_A);
    bool decremento = gpio_get(BUTTON_B);

    // Obtenção do tempo em que ocorre a interrupção desde a inicialização
    uint32_t tempo_atual = to_us_since_boot(get_absolute_time());
    // Impressão do contador no momento da interrupção
    printf("Contador atual: %d\n", num);

    // Verificação de alteração em um intervalo maior que 300ms (debouncing)
    if(tempo_atual - ultimo_tempo > 300000){
        // Determinação de qual botão foi ativado 
        if(incremento == false && decremento == true && num < 9){
            //Se o botão A foi pressionado, o tempo é atualizado e o contador incrementa (se for menor que 9)
            ultimo_tempo = tempo_atual;
            num++;
    } else if (incremento == true && decremento == false && num > 0){
        // Se o botão B foi pressionado, o tempo é atualizado e o contador decrementa (se for maior que 0)
        ultimo_tempo = tempo_atual;
        num--;
    }
    // O contador é impresso ao ser atualizado com o incremento ou decremento
    printf("Botão pressionado. Contador atualizado: %d\n", num);
        // O menu switch vai chamar a função que criará o desenho de um número na matriz de LEDs de acordo com o valor do contador atualizado
        // Cada número terá uma cor de luz diferente (os parâmetros r, g, b variam para cada número)
        switch (num){
            case 0:
            // Os parâmetros para cada número são definidos 
            b = 1.0;
            r = 1.0;
            g = 1.0;
            // A função de desenho é chamada e os parâmetros são passados a ela
            desenhos(numero0, valor_led, pio, sm, b, r ,g);
            // O caso se encerra e o número fica exposto até uma nova interrupção ser detectada
            break;

            case 1:
            b = 0.5;
            r = 1.0;
            g = 1.0;
            desenhos(numero1, valor_led, pio, sm, b, r ,g);
            break;

            case 2:
            b = 1.0;
            r = 0.5;
            g = 1.0;
            desenhos(numero2, valor_led, pio, sm, b, r ,g);
            break;

            case 3:
            b = 1.0;
            r = 1.0;
            g = 0.5;
            desenhos(numero3, valor_led, pio, sm, b, r ,g);
            break;

            case 4:
            b = 0.5;
            r = 0.5;
            g = 1.0;
            desenhos(numero4, valor_led, pio, sm, b, r ,g);
            break;

            case 5:
            b = 0.5;
            r = 1.0;
            g = 0.5;
            desenhos(numero5, valor_led, pio, sm, b, r ,g);
            break;

            case 6:
            b = 1.0;
            r = 0.5;
            g = 0.5;
            desenhos(numero6, valor_led, pio, sm, b, r ,g);
            break;

            case 7:
            b = 0.0;
            r = 1.0;
            g = 1.0;
            desenhos(numero7, valor_led, pio, sm, b, r ,g);
            break;

            case 8:
            b = 1.0;
            r = 0.0;
            g = 1.0;
            desenhos(numero8, valor_led, pio, sm, b, r ,g);
            break;

            case 9:
            b = 1.0;
            r = 1.0;
            g = 0.0;
            desenhos(numero9, valor_led, pio, sm, b, r ,g);
            break;
        }
    }
}
