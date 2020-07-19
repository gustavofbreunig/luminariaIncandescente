#include "Constantes.h"
#include "PinoClass.h"

//*********************************
//#define DEBUGANDO
//*********************************

PinoClass Pinos[QUANTIDADE_PINOS_TRIAC];

int contadorCalcularDelays = 0;
bool devoCalcularDelays = false;
volatile bool cruzou_zero = false;

ISR(INT0_vect)
{
  //a cada semiciclo
  cruzou_zero = true;
}

void setup() {
#ifdef DEBUGANDO
  Serial.begin(9600);
  Serial.print("Frequencia da rede: ");
  Serial.print(FREQUENCIA_REDE);
  Serial.println("Hz.");
  Serial.print("Tempo entre picos AC: ");
  Serial.print(TEMPO_SEMICICLO_SEGUNDOS);
  Serial.println("s.");
  Serial.print("Tempo entre picos AC: ");
  Serial.print(TEMPO_SEMICICLO_US);
  Serial.println("us");
  Serial.print("Tamanho da classe PinoClass: ");
  Serial.print(sizeof(PinoClass));
  Serial.println(" bytes");
  Serial.print("TEMPO_GASTO_DISPARANDO_TRIACS: ");
  Serial.print(TEMPO_GASTO_DISPARANDO_TRIACS);
  Serial.println(" us");
  Serial.print("PROPORCAO_TRANSICAO_TEMPO_TOTAL: ");
  Serial.print(PROPORCAO_TRANSICAO_TEMPO_TOTAL);
  Serial.println("x");
#endif

  //cria as estruturas na classe que representa os pinos
  Pinos[0] = *(new PinoClass(&PORTD, &DDRD, PORTD3, -999, INCREMENTO_BRILHO));
  Pinos[1] = *(new PinoClass(&PORTD, &DDRD, PORTD4, 3500, -INCREMENTO_BRILHO));
  Pinos[2] = *(new PinoClass(&PORTD, &DDRD, PORTD5, -500, INCREMENTO_BRILHO));
  Pinos[3] = *(new PinoClass(&PORTD, &DDRD, PORTD6, 3000, -INCREMENTO_BRILHO));
  Pinos[4] = *(new PinoClass(&PORTD, &DDRD, PORTD7, -100, INCREMENTO_BRILHO));
  Pinos[5] = *(new PinoClass(&PORTB, &DDRB, PORTB0, 2000, -INCREMENTO_BRILHO));
  Pinos[6] = *(new PinoClass(&PORTB, &DDRB, PORTB1, 500, INCREMENTO_BRILHO));
  Pinos[7] = *(new PinoClass(&PORTB, &DDRB, PORTB2, 3900, -INCREMENTO_BRILHO));
  Pinos[8] = *(new PinoClass(&PORTB, &DDRB, PORTB3, -750, INCREMENTO_BRILHO));
  Pinos[9] = *(new PinoClass(&PORTB, &DDRB, PORTB4, 3400, -INCREMENTO_BRILHO));

  //teste de inicialização
  for (uint8_t pin = 0; pin < QUANTIDADE_PINOS_TRIAC; pin++)
  {
    Pinos[pin].calculaEstadoPinoEDelay();

    Pinos[pin].ligaPino();
    _delay_ms(200);
    Pinos[pin].desligaPino();
    _delay_ms(200);
    Pinos[pin].ligaPino();
    _delay_ms(200);
    Pinos[pin].desligaPino();
    _delay_ms(200);
  }

  //seta a INT0 para disparar quando a tensão cai pra zero (falling edge)
  EICRA |= (1 << ISC01);
  EICRA &= ~(1 << ISC00);

  //habilita a interrupção
  EIMSK |= (1 << INT0);
  sei();

#ifdef DEBUGANDO
  Serial.println("Sistema rodando.");
#endif
}

void loop() {

  if (cruzou_zero)
  {

    int16_t menorDelay = INT16_MAX; //valor muito alto, vai buscar o menor depois
    int8_t indexMenorDelay = INT8_MAX; //valor muito alto, vai buscar o menor depois
    int16_t delayAcumulado = 0;

    while (indexMenorDelay > -1)
    {
      menorDelay = INT16_MAX;
      indexMenorDelay = -1;

      //procura o menor delay a ser executado
      for (int8_t j = 0; j < QUANTIDADE_PINOS_TRIAC; j++)
      {
        if (Pinos[j].getEstado() == EstadoPinoEnum::Delay && Pinos[j].delayDaIteracao < menorDelay)
        {
          menorDelay = Pinos[j].delayDaIteracao;
          indexMenorDelay = j;
        }
      }

      if (indexMenorDelay >= 0) //achou o index do menor delay
      {
        //adiciona no delay que já foi executado, por exemplo, se um pino deve ter 100us
        //e o outro 130us, o segundo só deve fazer um delay de 30us
        int16_t delayAExecutar = menorDelay - delayAcumulado;
        delayMicroseconds(delayAExecutar);
        Pinos[indexMenorDelay].geraPulso();
        delayAcumulado += delayAExecutar;

        //coloca um valor muito alto para não passar mais na próxima iteração
        Pinos[indexMenorDelay].delayDaIteracao = INT16_MAX;

#ifdef DEBUGANDO
        Serial.print("indexMenorDelay ");
        Serial.println(indexMenorDelay);

        Serial.print("menorDelay ");
        Serial.println(menorDelay);

        Serial.print("delayAcumulado ");
        Serial.println(delayAcumulado);

        Serial.print("delayAExecutar ");
        Serial.println(delayAExecutar);
#endif

      }

    }

    devoCalcularDelays = contadorCalcularDelays++ % 3 == 0;  //só recalcula a cada 4 semiciclos
    if (devoCalcularDelays)
    {
      contadorCalcularDelays = 1;
    }

    for (int i = 0; i < QUANTIDADE_PINOS_TRIAC; i++)
    {
      if (devoCalcularDelays)
      {
        Pinos[i].calculaEstadoPinoEDelay();
      } else
      {
        Pinos[i].restauraDelaySalvo();
      }

      Pinos[i].fazIncrementoBrilho();
      EstadoPinoEnum estado = Pinos[i].getEstado();

      if (estado == EstadoPinoEnum::Ligado)
      {
#ifdef DEBUGANDO
        Serial.print("ligando pino do index ");
        Serial.println(i);
#endif
        Pinos[i].ligaPino();
      }
      else if (estado == EstadoPinoEnum::Desligado)
      {
#ifdef DEBUGANDO
        Serial.print("desligando pino do index ");
        Serial.println(i);
#endif
        Pinos[i].desligaPino();
      }
    }

#ifdef DEBUGANDO
    Serial.println("-----------fim do cruzou_zero------------");
    //while (true);
#endif

    cruzou_zero = false;
  }


}
