#define PINO_ZERO_CROSS 2
#define QuantidadePinosTriac 10
#define FREQUENCIA_REDE 0.5
#define limiteInferior -20
#define limiteSuperior 180
#define INCREMENTO_BRILHO 1
//quanto deve avançar cada iteracao no loop principal, em microssegundos
#define AVANCO_ITERACAO 500
//tempo entre cada semiciclo, em segundos
#define TEMPO_SEMICICLO_SEGUNDOS 1.0 / (FREQUENCIA_REDE * 2.0)
//tempo entre cada semiciclo, em microssegundos
#define TEMPO_SEMICICLO_US TEMPO_SEMICICLO_SEGUNDOS * 1000 * 1000
//desconto de tempo na iteracao principal para o tempo necessário para executar instruções, 10% do semiciclo
#define DESCONTO_TEMPO TEMPO_SEMICICLO_US * 0.1


//mapeamento dos pinos e dados, cada posicao na array se relaciona com o mesmo dado na mesma posicao da outra array
byte* PinosTriacPORT[QuantidadePinosTriac] = { &PORTD,  &PORTD,  &PORTD, &PORTD, &PORTD, &PORTB, &PORTB, &PORTB, &PORTB, &PORTB }; //registradores
byte PinosTriacPORTN[QuantidadePinosTriac] = { PORTD3,  PORTD4,  PORTD5, PORTD6, PORTD7, PORTB0, PORTB1, PORTB2, PORTB3, PORTB4 }; //valores de de cada pino em binario
byte* PinosTriacDDR[QuantidadePinosTriac] =  { &DDRD,   &DDRD,   &DDRD,  &DDRD,  &DDRD,  &DDRB,  &DDRB,  &DDRB,  &DDRB,  &DDRB  }; //registradores são acessados pelos seus enderecos
int PinosTriac[QuantidadePinosTriac] =       { 3,       4,       5,      6,      7,      8,      9,      10,     11,     12     }; //pinos de acordo com a notação do arduino, só para documentacao
int BrilhoDoPino[QuantidadePinosTriac] =     { 50,      50,      20,     100,    70,     120,    90,     140,    100,    170    }; //brilho inicial de cada triac, em percentual


//pra saber se incrementa ou decrementa, aí vai fazendo -1 quando bate no limite inferior e vice versa
int DirecaoIncremento[QuantidadePinosTriac] = { -INCREMENTO_BRILHO, -INCREMENTO_BRILHO, INCREMENTO_BRILHO, -INCREMENTO_BRILHO, INCREMENTO_BRILHO,
                                                -INCREMENTO_BRILHO, INCREMENTO_BRILHO, -INCREMENTO_BRILHO, INCREMENTO_BRILHO, -INCREMENTO_BRILHO
                                              };

volatile bool ajustar_brilho = false;
volatile bool recalcular_tempos = false;
volatile int recalcular_cont = 0;

ISR(INT0_vect)
{
  //a cada cruzada no zero, dispara os triacs a cada semiciclo
  ajustar_brilho = true;

  if (++recalcular_cont % 4 == 0) //iteração que recalcula os tempos, só precisa ocorrer a cada 4 semiciclos
  {
    recalcular_tempos = true;
    recalcular_cont = 0;
  }
}

void setup() {
  Serial.begin(9600);
  Serial.print("Frequencia da rede: ");
  Serial.print(FREQUENCIA_REDE);
  Serial.println("hz.");
  Serial.print("Tempo entre picos AC: ");
  Serial.print(TEMPO_SEMICICLO_SEGUNDOS);
  Serial.println("s.");
  Serial.print("Tempo entre picos AC: ");
  Serial.print(TEMPO_SEMICICLO_US);
  Serial.println("us");

  //coloca os pinos como saida
  for (int pin = 0; pin < QuantidadePinosTriac; pin++)
  {
    *PinosTriacDDR[pin] |= (1 << PinosTriacPORTN[pin]);
  }

  //seta a INT0 para disparar quando a tensão cai pra zero (falling edge)
  EICRA |= (1 << ISC01);
  EICRA &= ~(1 << ISC00);

  //habilita a interrupção
  EIMSK |= (1 << INT0);
  sei();

  Serial.println("Sistema rodando.");
}

uint32_t tempoDesligado[QuantidadePinosTriac] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

void loop() {

  if (recalcular_tempos)
  {

    for (uint16_t iPino = 0; iPino < QuantidadePinosTriac; iPino++)
    {
      tempoDesligado[iPino] = 0; //limpa memoria

      //calcula o tempo que deve ficar desligado durante o ciclo, somente para aqueles que estão meio ligados
      if (BrilhoDoPino[iPino] >= 0 && BrilhoDoPino[iPino] <= 100)
      {
        long double pctDoTempoDesligado = (100.0 - BrilhoDoPino[iPino]) / 100; //inverso do tempo ligado pra ter o tempo que deve ficar desligado
        tempoDesligado[iPino] = TEMPO_SEMICICLO_US * pctDoTempoDesligado;
      }

      //inverte a direcao do incremento (multiplica por -1)
      if (BrilhoDoPino[iPino] >= limiteSuperior || BrilhoDoPino[iPino] <= limiteInferior)
      {
        DirecaoIncremento[iPino] *= -1;
        Serial.print("Pino ");
        Serial.print(PinosTriac[iPino]);
        Serial.print(" bateu no limite ");
        Serial.print(BrilhoDoPino[iPino]);
        Serial.println(", invertendo ordem de incremento/decremento");
      }

      //altera intensidade do brilho
      BrilhoDoPino[iPino] += DirecaoIncremento[iPino];
    }

    Serial.println("cu");
    recalcular_tempos = false;
  }

  if (ajustar_brilho)
  {
    for (uint32_t i = 0; i < TEMPO_SEMICICLO_US - DESCONTO_TEMPO; i += AVANCO_ITERACAO)
    {
      for (uint16_t iPino = 0; iPino < QuantidadePinosTriac; iPino++)
      {
        //abaixo de 0 ou acima de 100, é low ou high definitivo, respectivamente
        if (BrilhoDoPino[iPino] <= 0)
        {
          *PinosTriacPORT[iPino] &= ~(1 << PinosTriacPORTN[iPino]);
        }
        else if (BrilhoDoPino[iPino] >= 100)
        {
          *PinosTriacPORT[iPino] |= (1 << PinosTriacPORTN[iPino]);
        }
        else
        {
          //deixa ligado por um intervalor equivalente a algumas iteracoes, o suficiente para disparar o TRIAC
          if (i >= tempoDesligado[iPino] && i <= (tempoDesligado[iPino] + (AVANCO_ITERACAO * 2)) )
          {
            *PinosTriacPORT[iPino] |= (1 << PinosTriacPORTN[iPino]);
          }
          else
          {
            *PinosTriacPORT[iPino] &= ~(1 << PinosTriacPORTN[iPino]);
          }
        }
      }

      _delay_us(AVANCO_ITERACAO);
    }

    ajustar_brilho = false;
  }

}
