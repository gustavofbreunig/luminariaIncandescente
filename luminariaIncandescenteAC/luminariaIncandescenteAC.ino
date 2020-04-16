#define PINO_ZERO_CROSS 2
#define QuantidadePinosTriac 10
#define FREQUENCIA_REDE 0.5
#define limiteInferior -20
#define limiteSuperior 180
#define INCREMENTO_BRILHO 1
//multiplicador de tempo para ajuste fino, se ficar em 1000, cada iteracao tem 1ms
#define MULTIPLICADOR_TEMPO 1000

//mapeamento dos pinos e dados, cada posicao na array se relaciona com o mesmo dado na mesma posicao da outra array
byte* PinosTriacPORT[QuantidadePinosTriac] = { &PORTD,  &PORTD,  &PORTD, &PORTD, &PORTD, &PORTB, &PORTB, &PORTB, &PORTB, &PORTB }; //registradores 
byte PinosTriacPORTN[QuantidadePinosTriac] = { PORTD3,  PORTD4,  PORTD5, PORTD6, PORTD7, PORTB0, PORTB1, PORTB2, PORTB3, PORTB4 }; //valores de de cada pino em binario
byte* PinosTriacDDR[QuantidadePinosTriac] =  { &DDRD,   &DDRD,   &DDRD,  &DDRD,  &DDRD,  &DDRB,  &DDRB,  &DDRB,  &DDRB,  &DDRB  }; //registradores são acessados pelos seus enderecos
int PinosTriac[QuantidadePinosTriac] =       { 3,       4,       5,      6,      7,      8,      9,      10,     11,     12     }; //pinos de acordo com a notação do arduino, só para documentacao
int BrilhoDoPino[QuantidadePinosTriac] =     { 50,       50,      20,     100,    70,     120,    90,     140,    100,    170   }; //brilho inicial de cada triac, em percentual


//pra saber se incrementa ou decrementa, aí vai fazendo -1 quando bate no limite inferior e vice versa
int DirecaoIncremento[QuantidadePinosTriac] ={ -INCREMENTO_BRILHO,-INCREMENTO_BRILHO,INCREMENTO_BRILHO,-INCREMENTO_BRILHO,INCREMENTO_BRILHO,
                                               -INCREMENTO_BRILHO,INCREMENTO_BRILHO,-INCREMENTO_BRILHO,INCREMENTO_BRILHO,-INCREMENTO_BRILHO}; 

//tempo entre cada pico de tensao ou tempo que demora cada vez que cruza o zero, em microssegundos, aí divide pelo multiplicador se quiser em millisegundos
const uint32_t TempoPicoMicroSeg = ((1.0 / (FREQUENCIA_REDE * 2.0)) * 1000.0 * 1000.0) / MULTIPLICADOR_TEMPO;
volatile bool ajustar_brilho = false;

ISR(INT0_vect)
{
  //a cada cruzada no zero, dispara atualização do brilho
  ajustar_brilho = true;
}

void setup() {   
  Serial.begin(9600);
  Serial.print("Frequencia da rede: ");
  Serial.println(FREQUENCIA_REDE);
  Serial.print("Tempo entre picos AC: ");
  Serial.println(TempoPicoMicroSeg);
  
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

uint32_t TempoIteracao = TempoPicoMicroSeg - 100; //descontar mais ou menos o tempo de execução das instrucoes para o loop nao ficar mais de um semiciclo executando

void loop() {

  if (ajustar_brilho)
  {    
      uint32_t tempoDesligado[QuantidadePinosTriac] = {0,0,0,0,0,0,0,0,0,0};

      //calcula o tempo que deve ficar desligado durante o ciclo
      for (int iPino = 0; iPino < QuantidadePinosTriac; iPino++)
      {    
          if (BrilhoDoPino[iPino] >= 0 && BrilhoDoPino[iPino] <= 100)
          {
              long double pctDoTempoDesligado = (100.0 - BrilhoDoPino[iPino]) / 100; //inverso do tempo ligado pra ter o tempo que deve ficar desligado
              tempoDesligado[iPino] = TempoIteracao * pctDoTempoDesligado;
          }
      }
    
      for (uint32_t i = 0; i < TempoIteracao; i++) 
      {       
          for (int iPino = 0; iPino < QuantidadePinosTriac; iPino++)
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
                  if (i > tempoDesligado[iPino]) //liga se chegou na hora (o contador de microsegundos passou o tempo desligado
                  {
                      *PinosTriacPORT[iPino] |= (1 << PinosTriacPORTN[iPino]);
                  } else
                  {
                      *PinosTriacPORT[iPino] &= ~(1 << PinosTriacPORTN[iPino]);   
                  }
              }
              
          }

        _delay_us(MULTIPLICADOR_TEMPO);
      }

      for (int iPino = 0; iPino < QuantidadePinosTriac; iPino++)
      {
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

/*
      Serial.print("Tempo desligado: [");
      for (int iPino = 0; iPino < QuantidadePinosTriac; iPino++)
      {                         
          Serial.print(tempoDesligado[iPino]);
          Serial.print(" , ");
      }
      Serial.println("]");
*/
      ajustar_brilho = false;
  }

}
