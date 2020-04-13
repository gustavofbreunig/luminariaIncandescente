#define PINO_ZERO_CROSS 2
#define PINO_LED 3

const uint16_t QuantidadePinosTriac = 10;
int PinosTriac[QuantidadePinosTriac] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 12};

//tempo entre cada cruzada no zero, considerando 60Hz de frequencia e retirando algum tempo que é gasto executando instruções
//acima desse tempo, o pino deve ficar em HIGH
const uint32_t TempoCicloMicroSeg = 8300;

const int limiteInferior = -1600;
const int limiteSuperior = 10000;
int BrilhoDoPino[QuantidadePinosTriac] = {-1000, -500, 0, 1000, 2000, 3000, 4000, 5000, 6000, 7000}; //brilho inicial de cada triac

//pra saber se incrementa ou decrementa, aí vai fazendo -1 quando bate no limite inferior e vice versa
int DirecaoIncremento[QuantidadePinosTriac] = {1, -1, 1, -1, 1, -1, 1, -1, 1, -1};

volatile uint32_t temp = 0;

ISR(INT0_vect)
{
  temp++;
}

void setup() {  
  pinMode(PINO_LED, OUTPUT);

  //seta a INT0 para disparar quando a tensão cai pra zero (falling edge)
  EICRA |= (1 << ISC01);
  EICRA &= ~(1 << ISC00);

  //habilita a interrupção
  EIMSK |= (1 << INT0);
  sei();
}

void loop() {

  if (temp % 10 == 0)
  {
        digitalWrite(PINO_LED, 1);
        delay(500);
        digitalWrite(PINO_LED, 0);
        delay(500);
        digitalWrite(PINO_LED, 1);
        delay(500);
        digitalWrite(PINO_LED, 0);        
  }

}
