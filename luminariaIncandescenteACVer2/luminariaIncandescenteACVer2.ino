//#define DEBUGANDO
#define PINO_ZERO_CROSS 2
#define QUANTIDADE_PINOS_TRIAC 10
#define FREQUENCIA_REDE 60
#define limiteInferior -1000
#define limiteSuperior 4000
const int16_t INCREMENTO_BRILHO = 1;
//tempo entre cada semiciclo, em segundos
#define TEMPO_SEMICICLO_SEGUNDOS (1.0 / (FREQUENCIA_REDE * 2.0))
//tempo entre cada semiciclo, em microssegundos
#define TEMPO_SEMICICLO_US (TEMPO_SEMICICLO_SEGUNDOS * 1000.0 * 1000.0)
//converte em um inteiro de 16bits, assim não fica fazendo instruções de ponto flutuante por todo lado
const int16_t TEMPO_SEMICICLO_US_INT16 = TEMPO_SEMICICLO_US;
//tamanho do pulso para disparo dos triacs, 10us ta bom
#define PULSO_DISPARO_US 10
//tempo total gasto disparando triacs, mais uma folga, precisa levar em conta para ligar-desligar os triacs e o tempo de execução de instruções
const int16_t TEMPO_GASTO_DISPARANDO_TRIACS = 150 + (PULSO_DISPARO_US * QUANTIDADE_PINOS_TRIAC);
//quantidade de semiciclos que deve durar a transicao, quanto maior, mais demorada é a transição ligado-desligado
#define TEMPO_TRANSICAO_SEMICICLOS 2400

//mapeamento dos pinos e dados, cada posicao na array se relaciona com o mesmo dado na mesma posicao da outra array
byte* PinosTriacPORT[QUANTIDADE_PINOS_TRIAC] =  { &PORTD,  &PORTD,  &PORTD, &PORTD, &PORTD, &PORTB, &PORTB, &PORTB, &PORTB, &PORTB }; //registradores
byte  PinosTriacPORTN[QUANTIDADE_PINOS_TRIAC] = { PORTD3,  PORTD4,  PORTD5, PORTD6, PORTD7, PORTB0, PORTB1, PORTB2, PORTB3, PORTB4 }; //valores de de cada pino em binario
byte* PinosTriacDDR[QUANTIDADE_PINOS_TRIAC] =   { &DDRD,   &DDRD,   &DDRD,  &DDRD,  &DDRD,  &DDRB,  &DDRB,  &DDRB,  &DDRB,  &DDRB  }; //registradores são acessados pelos seus enderecos
uint8_t PinosTriac[QUANTIDADE_PINOS_TRIAC] =    { 3,       4,       5,      6,      7,      8,      9,      10,     11,     12     }; //pinos de acordo com a notação do arduino, só para documentacao
int16_t BrilhoDoPino[QUANTIDADE_PINOS_TRIAC] =  { -700,      2100,      2500,     -500,     3100,     -200,     3400,     -100,     3900,     2000     }; //brilho inicial de cada triac, em percentual


//pra saber se incrementa ou decrementa, aí vai fazendo *-1 quando bate no limite inferior e vice versa
int8_t DirecaoIncremento[QUANTIDADE_PINOS_TRIAC] = { INCREMENTO_BRILHO, -INCREMENTO_BRILHO, INCREMENTO_BRILHO, -INCREMENTO_BRILHO, INCREMENTO_BRILHO,
                                                     -INCREMENTO_BRILHO, INCREMENTO_BRILHO, -INCREMENTO_BRILHO, INCREMENTO_BRILHO, -INCREMENTO_BRILHO
                                                   };

//armazena o delay para os pinos
int16_t delayDaIteracao[QUANTIDADE_PINOS_TRIAC] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int16_t delayDaIteracao_temp[QUANTIDADE_PINOS_TRIAC] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int16_t devoCalcularDelay = 0;

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
  Serial.println("hz.");
  Serial.print("Tempo entre picos AC: ");
  Serial.print(TEMPO_SEMICICLO_SEGUNDOS);
  Serial.println("s.");
  Serial.print("Tempo entre picos AC: ");
  Serial.print(TEMPO_SEMICICLO_US_INT16);
  Serial.println("us");
#endif

  //coloca os pinos como saida
  for (uint8_t pin = 0; pin < QUANTIDADE_PINOS_TRIAC; pin++)
  {
    *PinosTriacDDR[pin] |= (1 << PinosTriacPORTN[pin]);
  }

  //teste de inicialização
  for (uint8_t pin = 0; pin < QUANTIDADE_PINOS_TRIAC; pin++)
  {
    ligaPino(pin);
    _delay_ms(200);
    desligaPino(pin);
    _delay_ms(200);
    ligaPino(pin);
    _delay_ms(200);
    desligaPino(pin);
  }

  //valores iniciais
  for (uint8_t i = 0; i < QUANTIDADE_PINOS_TRIAC; i++)
  {
    delayDaIteracao_temp[i] = delayDaIteracao[i] = obtemDelayParaMomentoTransicao(BrilhoDoPino[i]);
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

void geraPulso(uint8_t pin)
{
  *PinosTriacPORT[pin] |= (1 << PinosTriacPORTN[pin]);
  _delay_us(PULSO_DISPARO_US);
  *PinosTriacPORT[pin] &= ~(1 << PinosTriacPORTN[pin]);
}

void ligaPino(uint8_t pin)
{
  *PinosTriacPORT[pin] |= (1 << PinosTriacPORTN[pin]);
}

void desligaPino(uint8_t pin)
{
  *PinosTriacPORT[pin] &= ~(1 << PinosTriacPORTN[pin]);
}

int16_t obtemDelayParaMomentoTransicao(int16_t momentoTransicao)
{
  if (momentoTransicao <= TEMPO_GASTO_DISPARANDO_TRIACS)
  {
    return -1; //desligado
  }
  else if (momentoTransicao > TEMPO_GASTO_DISPARANDO_TRIACS && momentoTransicao <= TEMPO_TRANSICAO_SEMICICLOS)
  {
    //retorna o tempo que deve ficar desligado até disparar o triac
    int16_t ret = TEMPO_SEMICICLO_US_INT16 - (momentoTransicao * (TEMPO_SEMICICLO_US_INT16 / TEMPO_TRANSICAO_SEMICICLOS));
    return ret;
  }
  else
  {
    return -2; //ligado
  }
}

void loop() {

  if (cruzou_zero)
  {

    //só calcula delays a cada 4 semiciclos, se não, usa o temp
    if (devoCalcularDelay++ % 5 == 0)
    {
      devoCalcularDelay = 1;

      for (uint8_t i = 0; i < QUANTIDADE_PINOS_TRIAC; i++)
      {
        delayDaIteracao_temp[i] = delayDaIteracao[i] = obtemDelayParaMomentoTransicao(BrilhoDoPino[i]);
      }
    }
    else
    {
      for (uint8_t i = 0; i < QUANTIDADE_PINOS_TRIAC; i++)
      {
        delayDaIteracao[i] = delayDaIteracao_temp[i];
      }
    }


    int16_t menorDelay = TEMPO_SEMICICLO_US_INT16 + 1; //valor muito alto, vai buscar o menor depois
    int8_t indexMenorDelay = QUANTIDADE_PINOS_TRIAC + 1; //valor muito alto, vai buscar o menor depois
    int16_t delayAcumulado = 0;

    while (indexMenorDelay > -1)
    {
      menorDelay = TEMPO_SEMICICLO_US_INT16 + 1;
      indexMenorDelay = -1;
      //procura o menor delay a ser executado
      for (int8_t j = 0; j < QUANTIDADE_PINOS_TRIAC; j++)
      {
        if (delayDaIteracao[j] < menorDelay)
        {
          menorDelay = delayDaIteracao[j];
          indexMenorDelay = j;
        }
      }

      if (indexMenorDelay >= 0) //achou o index do menor delay
      {
        //coloca um valor muito alto para não passar mais na próxima iteração
        delayDaIteracao[indexMenorDelay] = TEMPO_SEMICICLO_US_INT16 + 1;

#ifdef DEBUGANDO
        Serial.print("indexMenorDelay ");
        Serial.println(indexMenorDelay);

        Serial.print("menorDelay ");
        Serial.println(menorDelay);
#endif

        //liga/desliga pino ou faz delay e dispara o pino
        if (menorDelay == -1)
        {
#ifdef DEBUGANDO
          Serial.print("desligando pino do index ");
          Serial.println(indexMenorDelay);
#endif
          desligaPino(indexMenorDelay);
        }
        else if (menorDelay == -2)
        {
#ifdef DEBUGANDO
          Serial.print("ligando pino do index ");
          Serial.println(indexMenorDelay);
#endif
          ligaPino(indexMenorDelay);
        }
        else
        {
          //adiciona no delay que já foi executado, por exemplo, se um pino deve ter 100us
          //e o outro 130us, o segundo só deve fazer um delay de 30us
          uint16_t delayAExecutar = menorDelay - delayAcumulado;
          delayMicroseconds(delayAExecutar);
          delayAcumulado += delayAExecutar;
          geraPulso(indexMenorDelay);

#ifdef DEBUGANDO
          Serial.print("delayAcumulado ");
          Serial.println(delayAcumulado);

          Serial.print("delayAExecutar ");
          Serial.println(delayAExecutar);
#endif

        }
      }

    }

    //faz o incremento do brilho e altera a direção do incremento, se precisar
    for (uint8_t i = 0; i < QUANTIDADE_PINOS_TRIAC; i++)
    {
      BrilhoDoPino[i] += DirecaoIncremento[i];
      int16_t brilhoTemp = BrilhoDoPino[i];

      if (brilhoTemp <= limiteInferior || brilhoTemp >= limiteSuperior)
      {
        DirecaoIncremento[i] *= -INCREMENTO_BRILHO;
      }
    }

#ifdef DEBUGANDO
    Serial.println("-----------fim do cruzou_zero------------");
    //while (true);
#endif

    cruzou_zero = false;
  }


}
