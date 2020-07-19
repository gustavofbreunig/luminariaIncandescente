#ifndef CONSTANTES_H
#define CONSTANTES_H

#define PINO_ZERO_CROSS 2
#define QUANTIDADE_PINOS_TRIAC 10
#define FREQUENCIA_REDE 60
#define limiteInferior -1000
#define limiteSuperior 4000
#define INCREMENTO_BRILHO 1

//tempo entre cada semiciclo, em segundos
#define TEMPO_SEMICICLO_SEGUNDOS (1.0 / (FREQUENCIA_REDE * 2.0))
#define TEMPO_SEMICICLO_US (int16_t)(TEMPO_SEMICICLO_SEGUNDOS * 1000.0 * 1000.0)

//tamanho do pulso para disparo dos triacs, 10us ta bom
#define PULSO_DISPARO_US 5

//tempo total gasto disparando triacs, mais uma folga, precisa levar em conta para ligar-desligar os triacs e o tempo de execução de instruções
#define TEMPO_GASTO_DISPARANDO_TRIACS (120 + (PULSO_DISPARO_US * QUANTIDADE_PINOS_TRIAC))

//quantidade de semiciclos que deve durar a transicao, quanto maior, mais demorada é a transição ligado-desligado
#define TEMPO_TRANSICAO_SEMICICLOS 2080

#define PROPORCAO_TRANSICAO_TEMPO_TOTAL (int16_t)(TEMPO_SEMICICLO_US / TEMPO_TRANSICAO_SEMICICLOS)

#endif
