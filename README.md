# CipherControl - Sistema de Controle Codificado

## 📖 Sobre o Projeto
O **CipherControl** é um sistema de controle com suporte a **codificação configurável**, permitindo que o usuário envie comandos codificados para controlar periféricos e transmitir dados.  
Inicialmente foi implementado com **código Morse**, mas o desenvolvedor pode configurar outros métodos de codificação durante o desenvolvimento do firmware.

## 🚀 Funcionalidades
- Codificação configurável entre código e letra.
- Controle de periféricos com comandos codificados (ex.: `RED ON`, `GREEN OFF`).
- Exibição de mensagens decodificadas em **Display OLED SSD1306**.
- Exibição do último caractere em **matriz de LEDs WS2812**.
- Função **backspace** via botão dedicado.
- Transmissão de dados em cenários de **baixa largura de banda ou alta latência**.

## 🛠️ Hardware Utilizado
- **Microcontrolador**: Raspberry Pi Pico W  
- **Display OLED** SSD1306 (I²C)  
- **Matriz de LEDs** WS2812 (Neopixel)  
- **LED RGB** de cátodo comum  
- **Buzzer** ativo  
- **Botões** (A e B)  
- **Bateria** + módulo carregador  

> Para detalhes completos de pinagem e diagramas, consulte a [📄 Documentação do Projeto](./DocumentaçãoCipherControl.pdf).

## ⚙️ Firmware
- Implementação em **C/C++** para Raspberry Pi Pico.  
- Uso de interrupções para leitura dos botões.  
- Comunicação via **I²C** para controle do display OLED.  
- PWM para acionamento do buzzer.  
- PIO para controle da matriz de LEDs WS2812.  

## 🔄 Fluxo de Funcionamento
1. Entrada do usuário via botão A (código Morse).  
2. Decodificação e armazenamento da mensagem.  
3. Exibição no display OLED e na matriz de LEDs.  
4. Possibilidade de correção via botão B (backspace).  
5. Envio de comandos codificados para controle de LEDs RGB.

## ✅ Testes e Resultados
- Tradução confiável de todos os caracteres Morse.  
- Exibição correta em **OLED** e **matriz de LEDs**.  
- Função backspace validada.  
- LED RGB respondeu a todos os comandos (`ON/OFF`).  
- Sistema estável e eficiente em ambiente de testes.

## 📎 Documentação Completa
Para informações detalhadas sobre especificações, firmware, fluxogramas e testes, acesse o PDF completo:  
[📄 Documentação Projeto Final EmbarcaTech](./DocumentaçãoCipherControl.pdf)

---

## 👨‍💻 Autores
Maria Eduarda Pamponet Ramalho
>Sistema desenvolvido como projeto final na Residência Tecnológica em Sistemas Embarcados "Embarca Tech" idealizado pelo CEPEDI
