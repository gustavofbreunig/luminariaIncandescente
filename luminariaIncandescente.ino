const uint16_t QuantidadePinosPwm = 6;
uint16_t PinosComPWM[QuantidadePinosPwm] = {3,5,6,9,10,11};

//o brilho dos pinos vai de -10000 a 25500 (por exemplo), sendo que 
//quando o PWM < 0 é 0, e quando > 255 é 255
//quando bate o limite, inverte o sinal
const int limiteInferior = -100;
const int limiteSuperior = 500;
int BrilhoDoPino[QuantidadePinosPwm] = {0,0,0,0,0,0};

//pra saber se incrementa ou decrementa, aí vai fazendo -1 quando bate no limite inferior e vice versa
int DirecaoIncremento[QuantidadePinosPwm] = {1, -1, 1, -1, 1, -1};

void loop();
void sorteiaValoresParaOsPinos();

void setup() {
  Serial.begin(115200);
  
  //declara os pinos como saída
  for (int i = 0; i < QuantidadePinosPwm; i++)
  {
      pinMode(PinosComPWM[i], OUTPUT); 
  }

  randomSeed(analogRead(0));
  sorteiaValoresParaOsPinos();

}

void sorteiaValoresParaOsPinos()
{
  for (int i = 0; i < QuantidadePinosPwm; i++)
  {
      //faz metade dos pinos estar no lado negativo (desligados) e a outra metade estar no lado positivo (ligados)
      if (i % 2 == 0)
      {
         BrilhoDoPino[i] = random(limiteInferior, 0);
      }
      else
      {
        BrilhoDoPino[i] = random(0, limiteSuperior);
      }      

      Serial.print("Pino ");
      Serial.print(PinosComPWM[i]);
      Serial.print(" tera valor ");
      Serial.println(BrilhoDoPino[i]);
  }   

  //verifica se algum valor ficou muito proximo de outro e refaz sorteio quantas vezes for necessario
  for (int i = 0; i < QuantidadePinosPwm; i++)
  {
    for (int j = 0; j < QuantidadePinosPwm; j++)
    {
      //diferenca absoluta entre o brilho de qualquer pino com qualquer outro
      if (i != j)
      {
        int diferenca = abs(BrilhoDoPino[i] - BrilhoDoPino[j]);
        if (diferenca < 30)
        {
          //chama a função recursivamente
          sorteiaValoresParaOsPinos();
        }
      }
    }
  }
  
}

void loop() {

  for (int i = 0; i < QuantidadePinosPwm; i++)
  {
      //pega o brilho que é pra setar no pino (porcentagem do pwm)
      //e ajusta para 0 a 255
      int brilho = BrilhoDoPino[i];
      if (brilho > 255)
      {
        brilho = 255;
      } else
      if (brilho < 0)
      {
        brilho = 0;
      }

      //seta no PWM
      analogWrite(PinosComPWM[i], brilho);

      //faz o incremento ou decremento
      BrilhoDoPino[i] += DirecaoIncremento[i];

      //se bateu no limite, troca a direcao do incremento
      if (BrilhoDoPino[i] > limiteSuperior || BrilhoDoPino[i] < limiteInferior)
      {
        DirecaoIncremento[i] *= -1;
        
        Serial.print("Pino ");
        Serial.print(PinosComPWM[i]);
        Serial.print(" bateu no limte ");
        Serial.print(BrilhoDoPino[i]);   
        Serial.println(", a direcao sera invertida.");     
        
      }
  }

  delay(30);
}
