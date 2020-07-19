#ifndef PINO_CLASS_H
#define PINO_CLASS_H

enum class EstadoPinoEnum { Ligado, Desligado, Delay };

class PinoClass {
  public:
    int16_t delayDaIteracao;

    PinoClass()
    {

    }

    PinoClass(byte* PORT, byte* DDR, byte PORTN, int16_t Brilho, int8_t direcaoIncremento)
    {
      _AtmegaPORT = PORT;
      _AtmegaDDR = DDR;
      _AtmegaPORTN = PORTN;
      _brilhoDoPino = Brilho;
      _direcaoIncremento = direcaoIncremento;

      //coloca o pino como saída
      *_AtmegaDDR |= (1 << _AtmegaPORTN);
    }

    EstadoPinoEnum getEstado()
    {
      return _estadoPino;
    }

    void restauraDelaySalvo()
    {
      delayDaIteracao = _delayDaIteracao_temp;
    }

    void ligaPino()
    {
      *_AtmegaPORT |= (1 << _AtmegaPORTN);
    }

    void desligaPino()
    {
      *_AtmegaPORT &= ~(1 << _AtmegaPORTN);
    }

    void geraPulso()
    {
      *_AtmegaPORT |= (1 << _AtmegaPORTN);
      _delay_us(PULSO_DISPARO_US);
      *_AtmegaPORT &= ~(1 << _AtmegaPORTN);
    }

    void calculaEstadoPinoEDelay()
    {
      if (_brilhoDoPino <= TEMPO_GASTO_DISPARANDO_TRIACS)
      {
        _estadoPino = EstadoPinoEnum::Desligado;
      }
      else if (_brilhoDoPino > TEMPO_GASTO_DISPARANDO_TRIACS && _brilhoDoPino < TEMPO_TRANSICAO_SEMICICLOS - TEMPO_GASTO_DISPARANDO_TRIACS)
      {
        _estadoPino = EstadoPinoEnum::Delay;
        // faz um calculo de proporção para adequar o delay
        // ex:         proporcao = 8333 (tempo semiciclo) / 2085 (tempo transição)
        //     depois, tempo =     1000 (brilho) * 4 (proporcao)
        //     depois, delay =     8333 (tempo semiciclo) - 4000 (tempo)
        int16_t tempo  = _brilhoDoPino * PROPORCAO_TRANSICAO_TEMPO_TOTAL;
        delayDaIteracao = TEMPO_SEMICICLO_US - tempo - TEMPO_GASTO_DISPARANDO_TRIACS;
        _delayDaIteracao_temp = delayDaIteracao;
      }
      else
      {
        _estadoPino = EstadoPinoEnum::Ligado;
      }
    }

    void fazIncrementoBrilho()
    {
      //faz o incremento do brilho e altera a direção do incremento, se precisar
      _brilhoDoPino += _direcaoIncremento;

      if (_brilhoDoPino <= limiteInferior || _brilhoDoPino >= limiteSuperior)
      {
        _direcaoIncremento *= -1;
      }
    }

  private:
    //registradores da porta, conjunto de 8 pinos
    byte* _AtmegaPORT;

    //registrador que identifica se o pino é input ou output
    byte* _AtmegaDDR;

    //registrador do pino
    byte _AtmegaPORTN;

    int16_t _brilhoDoPino;
    int8_t _direcaoIncremento;
    int16_t _delayDaIteracao_temp;
    EstadoPinoEnum _estadoPino;
};


#endif
