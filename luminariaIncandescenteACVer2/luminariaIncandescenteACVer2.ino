#define PINO_ZERO_CROSS 2
#define QUANTIDADE_PINOS_TRIAC 10
#define FREQUENCIA_REDE 60
#define limiteInferior -1000
#define limiteSuperior 4000
#define INCREMENTO_BRILHO 1
//tempo entre cada semiciclo, em segundos
#define TEMPO_SEMICICLO_SEGUNDOS 1.0 / (FREQUENCIA_REDE * 2.0)
//tempo entre cada semiciclo, em microssegundos
#define TEMPO_SEMICICLO_US TEMPO_SEMICICLO_SEGUNDOS * 1000.0 * 1000.0
//tamanho do pulso para disparo dos triacs, 10us ta bom
#define PULSO_DISPARO_US 10
//tempo total gasto disparando triacs, mais uma folga, precisa levar em conta para ligar-desligar os triacs
#define TEMPO_GASTO_DISPARANDO_TRIACS 10 + (PULSO_DISPARO_US * QUANTIDADE_PINOS_TRIAC)
//quantidade de semiciclos que deve durar a transicao, quanto maior, mais demorada é a transição ligado-desligado
#define TEMPO_TRANSICAO_SEMICICLOS 2400

//mapeamento dos pinos e dados, cada posicao na array se relaciona com o mesmo dado na mesma posicao da outra array
byte* PinosTriacPORT[QUANTIDADE_PINOS_TRIAC] = { &PORTD,  &PORTD,  &PORTD, &PORTD, &PORTD, &PORTB, &PORTB, &PORTB, &PORTB, &PORTB }; //registradores
byte  PinosTriacPORTN[QUANTIDADE_PINOS_TRIAC] = { PORTD3,  PORTD4,  PORTD5, PORTD6, PORTD7, PORTB0, PORTB1, PORTB2, PORTB3, PORTB4 }; //valores de de cada pino em binario
byte* PinosTriacDDR[QUANTIDADE_PINOS_TRIAC] =  { &DDRD,   &DDRD,   &DDRD,  &DDRD,  &DDRD,  &DDRB,  &DDRB,  &DDRB,  &DDRB,  &DDRB  }; //registradores são acessados pelos seus enderecos
uint8_t PinosTriac[QUANTIDADE_PINOS_TRIAC] =   { 3,       4,       5,      6,      7,      8,      9,      10,     11,     12     }; //pinos de acordo com a notação do arduino, só para documentacao
int16_t BrilhoDoPino[QUANTIDADE_PINOS_TRIAC] = { -19,      0,      180,     20,     130,     60,     110,     90,     -10,     80     }; //brilho inicial de cada triac, em percentual


//pra saber se incrementa ou decrementa, aí vai fazendo *-1 quando bate no limite inferior e vice versa
int DirecaoIncremento[QUANTIDADE_PINOS_TRIAC] = { INCREMENTO_BRILHO, -INCREMENTO_BRILHO, INCREMENTO_BRILHO, -INCREMENTO_BRILHO, INCREMENTO_BRILHO,
                                                   -INCREMENTO_BRILHO, INCREMENTO_BRILHO, -INCREMENTO_BRILHO, INCREMENTO_BRILHO, -INCREMENTO_BRILHO
                                                 };

volatile bool cruzou_zero = false;

ISR(INT0_vect)
{
  //a cada semiciclo
  cruzou_zero = true;
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
  for (uint8_t pin = 0; pin < QUANTIDADE_PINOS_TRIAC; pin++)
  {
    *PinosTriacDDR[pin] |= (1 << PinosTriacPORTN[pin]);
  }

  //teste de inicialização
  for (uint8_t pin = 0; pin < QUANTIDADE_PINOS_TRIAC; pin++)
  {
    *PinosTriacPORT[pin] |= (1 << PinosTriacPORTN[pin]);
  }
  _delay_ms(5000);

  //seta a INT0 para disparar quando a tensão cai pra zero (falling edge)
  EICRA |= (1 << ISC01);
  EICRA &= ~(1 << ISC00);

  //habilita a interrupção
  EIMSK |= (1 << INT0);
  sei();


  Serial.println("Sistema rodando.");
}

void geraPulso()
{
  for (uint8_t pin = 0; pin < QUANTIDADE_PINOS_TRIAC; pin++)
  {
    *PinosTriacPORT[pin] |= (1 << PinosTriacPORTN[pin]);
    _delay_us(PULSO_DISPARO_US);
    *PinosTriacPORT[pin] &= ~(1 << PinosTriacPORTN[pin]);
  }
}

void ligaPino()
{
  for (uint8_t pin = 0; pin < QUANTIDADE_PINOS_TRIAC; pin++)
  {
    *PinosTriacPORT[pin] |= (1 << PinosTriacPORTN[pin]);
  }
}

void desligaPino()
{
  for (uint8_t pin = 0; pin < QUANTIDADE_PINOS_TRIAC; pin++)
  {
    *PinosTriacPORT[pin] &= ~(1 << PinosTriacPORTN[pin]);
  }
}

int obtemDelayParaMomentoTransicao(int momentoTransicao)
{
  if (momentoTransicao <= TEMPO_GASTO_DISPARANDO_TRIACS)
  {
    return -1; //desligado
  }
  else if (momentoTransicao > TEMPO_GASTO_DISPARANDO_TRIACS && momentoTransicao <= TEMPO_TRANSICAO_SEMICICLOS)
  {
    //retorna o tempo que deve ficar desligado até disparar o triac
    int tempoDelay = momentoTransicao * (TEMPO_SEMICICLO_US / TEMPO_TRANSICAO_SEMICICLOS);
    return TEMPO_SEMICICLO_US - tempoDelay;
  }
  else
  {
    return -2; //ligado
  }
}

int c = -300;
int direcaoIncremento = INCREMENTO_BRILHO;

void loop() {

  if (cruzou_zero)
  {
    int d = obtemDelayParaMomentoTransicao(c);

    if (d == -1)
    {
      desligaPino();
    }
    else if (d == -2)
    {
      ligaPino();
    }
    else
    {
      delayMicroseconds(d);
      geraPulso();
    }

    if (c < limiteInferior || c > limiteSuperior)
    {
      direcaoIncremento *= -INCREMENTO_BRILHO;
    }

    c += direcaoIncremento;
    cruzou_zero = false;
  }


}
