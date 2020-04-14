#define PINO_ZERO_CROSS 2
#define QuantidadePinosTriac 10
#define FREQUENCIA_REDE 60L

int PinosTriac[QuantidadePinosTriac] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 12};

//tempo entre cada pico de tensao ou tempo que demora cada vez que cruza o zero, expresso em microssegundos inteiros
const uint32_t TempoPicoMicroSeg = (1.0 / (FREQUENCIA_REDE * 2.0)) * 1000.0 * 1000.0;

#define limiteInferior -1600
#define limiteSuperior 10000
int BrilhoDoPino[QuantidadePinosTriac] = {-1000, -500, 0, 1000, 2000, 3000, 4000, 5000, 6000, 7000}; //brilho inicial de cada triac

//pra saber se incrementa ou decrementa, aí vai fazendo -1 quando bate no limite inferior e vice versa
int DirecaoIncremento[QuantidadePinosTriac] = {1, -1, 1, -1, 1, -1, 1, -1, 1, -1};

volatile uint32_t counter_crossed_zero = 0;
volatile bool ajustar_brilho = false;

ISR(INT0_vect)
{
  //interrupção que ocorre a cada 8,3ms, para uma alteração de brilho sem 
  //piscadas, deve ocorrer o ajuste a cada ~30ms, ou seja, 4 interrupções

  counter_crossed_zero++;
  if (counter_crossed_zero > 4)
  {
      ajustar_brilho = true;
      counter_crossed_zero = 0;
  }
}

void setup() {   
  Serial.begin(9600);
  Serial.print("Frequencia da rede: ");
  Serial.println(FREQUENCIA_REDE);
  Serial.print("Tempo entre picos AC (microsegundos): ");
  Serial.println(TempoPicoMicroSeg);

  for (int pin = 0; pin < QuantidadePinosTriac; pin++)
  {
      pinMode(PinosTriac[pin], OUTPUT);
  }

  //seta a INT0 para disparar quando a tensão cai pra zero (falling edge)
  EICRA |= (1 << ISC01);
  EICRA &= ~(1 << ISC00);

  //habilita a interrupção
  EIMSK |= (1 << INT0);
  sei();

  Serial.print("Sistema rodando.");
}

void loop() {

  if (ajustar_brilho)
  {
      for (int i = 0; i < TempoPicoMicroSeg; i++)
      {
          //controle de intensidade do brilho baseado no momento que deve ligar o TRIAC
          //exemplos:
          //se a intensidade está entre 1 e 7000, o delay será de 8300 - 7000 = 1300, se for 300, será de 8000
          //se a intensidade está >= 8300, bota o pino pra HIGH direto
          //se a intensidade está <= 0, bota o pino low 
        
          for (int iPino = 0; iPino < QuantidadePinosTriac; iPino++)
          {
              digitalWrite(PinosTriac[iPino], HIGH); 
          }

        
      }

      //fim do ciclo, bota tudo pra LOW
      for (int iPino = 0; iPino < QuantidadePinosTriac; iPino++)
      {
        digitalWrite(PinosTriac[iPino], LOW);       
      }
      
      ajustar_brilho = false;
  }

}
