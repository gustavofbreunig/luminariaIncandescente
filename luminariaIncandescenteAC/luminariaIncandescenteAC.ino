#define PINO_ZERO_CROSS 2
#define QuantidadePinosTriac 10
#define FREQUENCIA_REDE 10
#define limiteInferior -20
#define limiteSuperior 180
#define INCREMENTO_BRILHO 1
//quanto deve avançar cada iteracao no loop principal, em microssegundos
#define AVANCO_ITERACAO 500
//tempo entre cada semiciclo, em segundos
#define TEMPO_SEMICICLO_SEGUNDOS 1.0 / (FREQUENCIA_REDE * 2.0)
//tempo entre cada semiciclo, em microssegundos
#define TEMPO_SEMICICLO_US TEMPO_SEMICICLO_SEGUNDOS * 1000 * 1000
//desconto de tempo na iteracao principal para o tempo necessário para executar instruções, em microssegundos
#define DESCONTO_TEMPO 300


//mapeamento dos pinos e dados, cada posicao na array se relaciona com o mesmo dado na mesma posicao da outra array
byte* PinosTriacPORT[QuantidadePinosTriac] = { &PORTD,  &PORTD,  &PORTD, &PORTD, &PORTD, &PORTB, &PORTB, &PORTB, &PORTB, &PORTB }; //registradores
byte PinosTriacPORTN[QuantidadePinosTriac] = { PORTD3,  PORTD4,  PORTD5, PORTD6, PORTD7, PORTB0, PORTB1, PORTB2, PORTB3, PORTB4 }; //valores de de cada pino em binario
byte* PinosTriacDDR[QuantidadePinosTriac] =  { &DDRD,   &DDRD,   &DDRD,  &DDRD,  &DDRD,  &DDRB,  &DDRB,  &DDRB,  &DDRB,  &DDRB  }; //registradores são acessados pelos seus enderecos
uint8_t PinosTriac[QuantidadePinosTriac] =   { 3,       4,       5,      6,      7,      8,      9,      10,     11,     12     }; //pinos de acordo com a notação do arduino, só para documentacao
int16_t BrilhoDoPino[QuantidadePinosTriac] = { 50,      50,      50,     50,     50,     50,     50,     50,     50,     50     }; //brilho inicial de cada triac, em percentual


//pra saber se incrementa ou decrementa, aí vai fazendo *-1 quando bate no limite inferior e vice versa
double DirecaoIncremento[QuantidadePinosTriac] = { -INCREMENTO_BRILHO, -INCREMENTO_BRILHO, INCREMENTO_BRILHO, -INCREMENTO_BRILHO, INCREMENTO_BRILHO,
                                                   -INCREMENTO_BRILHO, INCREMENTO_BRILHO, -INCREMENTO_BRILHO, INCREMENTO_BRILHO, -INCREMENTO_BRILHO
                                                 };

volatile bool ajustar_brilho = false;
volatile bool recalcular_tempos = false;
volatile uint8_t recalcular_cont = 0;

ISR(INT0_vect)
{
  //a cada cruzada no zero, dispara os triacs a cada semiciclo
  ajustar_brilho = true;

  //iteração que recalcula os tempos, só precisa ocorrer a cada 4 semiciclos
  recalcular_cont++;
  if (recalcular_cont % 4 == 0)
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
  for (uint8_t pin = 0; pin < QuantidadePinosTriac; pin++)
  {
    *PinosTriacDDR[pin] |= (1 << PinosTriacPORTN[pin]);
  }

  //teste de inicialização
  for (uint8_t pin = 0; pin < QuantidadePinosTriac; pin++)
  {
    *PinosTriacPORT[pin] |= (1 << PinosTriacPORTN[pin]);
  }
  _delay_ms(1000);
  for (uint8_t pin = 0; pin < QuantidadePinosTriac; pin++)
  {
    *PinosTriacPORT[pin] &= ~(1 << PinosTriacPORTN[pin]);
  }
  _delay_ms(1000);

  //seta a INT0 para disparar quando a tensão cai pra zero (falling edge)
  EICRA |= (1 << ISC01);
  EICRA &= ~(1 << ISC00);

  //habilita a interrupção
  EIMSK |= (1 << INT0);
  sei();

  recalcular_tempos = true;

  Serial.println("Sistema rodando.");
}

//tempo que deve ficar desligado durante o ciclo, somente para aqueles que estão meio ligados
uint32_t tempoDesligado[QuantidadePinosTriac] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

//contador de pinos já setados, quando esse contador chega a mesma quantidade de pinos,
//quer dizer que acabou e pode rearmar a interrupção e sair dos loops
uint16_t cont_pinos_setados = 0;

//array de valores que verifica se precisa iterar sobre o pino ou não
//assim economiza instruções
bool pinoJaSetado[QuantidadePinosTriac] = {false, false, false, false, false, false, false, false, false, false};

//utilizado para iteracao e demais operações no loop principal
uint8_t iPino = 0;
uint32_t i = 0;
int16_t brilho_temp = 0;
uint32_t tempoDesligado_temp = 0;
byte PORTN_temp = 0;

void loop() {

  if (recalcular_tempos)
  {
    for (iPino = 0; iPino < QuantidadePinosTriac; iPino++)
    {
      tempoDesligado[iPino] = 0; //limpa memoria

      //calcula o tempo que deve ficar desligado durante o ciclo, somente para aqueles que estão meio ligados
      brilho_temp = BrilhoDoPino[iPino];

      if (brilho_temp >= 0 && brilho_temp <= 100)
      {
        long double pctDoTempoDesligado = (100.0 - brilho_temp) / 100; //inverso do tempo ligado pra ter o tempo que deve ficar desligado
        tempoDesligado[iPino] = TEMPO_SEMICICLO_US * pctDoTempoDesligado;
      }

      //inverte a direcao do incremento (multiplica por -1)
      if (brilho_temp >= limiteSuperior || brilho_temp <= limiteInferior)
      {
        DirecaoIncremento[iPino] *= -1;

        //log
        Serial.print("Pino ");
        Serial.print(PinosTriac[iPino]);
        Serial.print(" bateu no limite ");
        Serial.print(BrilhoDoPino[iPino]);
        Serial.println(", invertendo ordem de incremento/decremento");
      }

      //altera intensidade do brilho
      BrilhoDoPino[iPino] += DirecaoIncremento[iPino];
    }

    recalcular_tempos = false;
  }

  if (ajustar_brilho)
  {

    //limpa memoria
    cont_pinos_setados = 0;
    for (iPino = 0; iPino < QuantidadePinosTriac; iPino++)
    {
      pinoJaSetado[iPino] = false;
    }

    for (i = 0; i < TEMPO_SEMICICLO_US - DESCONTO_TEMPO; i += AVANCO_ITERACAO)
    {
      for (iPino = 0; iPino < QuantidadePinosTriac; iPino++)
      {
        if (pinoJaSetado[iPino] == true)
        {
          continue;
        }

        //bota tudo em variavel temp para economizar instruções
        brilho_temp = BrilhoDoPino[iPino];
        tempoDesligado_temp = tempoDesligado[iPino];
        PORTN_temp = PinosTriacPORTN[iPino];

        if (brilho_temp <= 10)
        {
          //abaixo de 10% de brilho já desliga
          *PinosTriacPORT[iPino] &= ~(1 << PORTN_temp);
          pinoJaSetado[iPino] = true;
          cont_pinos_setados++;
        }
        else if (brilho_temp >= 90)
        {
          //acima de 90% liga definitivo
          *PinosTriacPORT[iPino] |= (1 << PORTN_temp);
          pinoJaSetado[iPino] = true;
          cont_pinos_setados++;
        }
        else if (i < tempoDesligado_temp)
        {
          //não chegou a hora de ligar ainda
          *PinosTriacPORT[iPino] &= ~(1 << PORTN_temp);
        }
        else if (i >= tempoDesligado_temp && i <= (tempoDesligado_temp + AVANCO_ITERACAO))
        {
          //periodo ligado
          *PinosTriacPORT[iPino] |= (1 << PORTN_temp);
        }
        else
        {
          //passou do tempo ligado, desliga de novo
          *PinosTriacPORT[iPino] &= ~(1 << PORTN_temp);
          pinoJaSetado[iPino] = true;
          cont_pinos_setados++;
        }

      }

      if (cont_pinos_setados >= QuantidadePinosTriac)
      {
        //todos pinos já foram setados se precisava, pode sair do loop
        ajustar_brilho = false;
        break;
      }

      _delay_us(AVANCO_ITERACAO);
    }

    ajustar_brilho = false;
  }

}
