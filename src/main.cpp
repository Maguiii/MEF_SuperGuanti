#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <VarSpeedServo.h>
#include <TimerOne.h>
#include <Wire.h>
/* 
  Retencion de inicio para pulsadores inicio e incremento MEF
  Pantalla lcd funcionando con todos los mensajes         MEF
  Deteccion de los cambios de estado de los infras        MEF
  Con la deteccion de los viajes se cambia el led
  Al cambiar el led se esperan instrucciones para la grua 

  Probar codigo con las instrucciones reales de la grua
  Pensar bien como es el final del programa

  Agregar accesorios al final, ej contador de pulsaciones por dedo
*/

#define FALSE 0
#define TRUE 1

#define incremento 10
#define inicio 12

#define infra1 4
#define infra2 A3
#define infra3 A2
#define infra4 A1
#define infra5 A0

#define pinLatch 7  // 74hc595
#define clockPin 8 
#define dataPin 5  

VarSpeedServo miservo_1, miservo_2, miservo_3;

LiquidCrystal_I2C lcd(0x3F, 16, 2);

volatile int tIncremento = 0;
volatile int tInicio = 0;
volatile int tInfras = 0;
volatile int taux = 0;
volatile int tlcd = 0;
volatile int tmin = 0;
volatile int tseg = 0;
volatile int thora = 0;

volatile byte estadoPrograma = 1;
volatile byte estadoRetencionIncremento = 1;
volatile byte estadoRetencionInicio = 1;
volatile byte estadoInfras = 0;
volatile byte estadoLcd = 0;
volatile byte estadoBluetooth = 0;

volatile byte numViajes = 0;
volatile byte contadorViajes = 0;
volatile byte aleatorio = 0;
volatile byte numAnterior = 0;

volatile int grados1 = 0;
volatile int grados2 = 0;
volatile int grados3 = 0;

volatile bool flagPulsoIncremento = FALSE;
volatile bool flagPulsoInicio = FALSE;


void tiempo();
void actualizarLcd();
void juego();

void setup(){
  Serial.begin(57600); 

  lcd.init();
  lcd.backlight();
  //Mensaje de bienvenida
  lcd.setCursor(0, 0);
  lcd.print("  Bienvenido a  ");
  lcd.setCursor(0, 1);
  lcd.print("  Super Guanti  ");
  delay(1000);  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Una creacion de:");
  lcd.setCursor(0, 1);
  lcd.print("     M.A.L.     ");
  delay(1000);

  pinMode(incremento, INPUT);
  pinMode(inicio, INPUT);

  pinMode(pinLatch, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);

  miservo_1.attach(9, 350, 2900); //servo base, derecha-izquierda
  miservo_1.write(grados1); 

  miservo_2.attach(6, 1000, 2000); //servo de la derecha, adelante-atras
  miservo_2.write(grados2); 

  miservo_3.attach(11, 1000, 2000); //servo de la izqueirda, abajo
  miservo_3.write(grados3);

  //Sin estas lineas hay un led que empieza encendido, copiar esto tmb al final del programa
  digitalWrite(pinLatch, LOW);              
  shiftOut(dataPin, clockPin, MSBFIRST, 0); 
  digitalWrite(pinLatch, HIGH);

  Timer1.initialize(1000);//1ms
  Timer1.attachInterrupt(tiempo);
}

void loop(){
  
  actualizarLcd();

  switch(estadoPrograma){
  case 1:
  /* En este caso se hace la eleccion de la cantidad de viajes a realizar y se da inicio al juego
   * Las MEF son para la retencion de los pulsadores de incremento de viajes y de inicio 
  */
    switch(estadoRetencionIncremento){
      case 1:
        flagPulsoIncremento = FALSE;

        if(digitalRead(incremento) == HIGH)
          estadoRetencionIncremento = 1;

        if(digitalRead(incremento) == LOW){
          tIncremento = 0;
          estadoRetencionIncremento = 2;
        }
      break;
      case 2:
        if(tIncremento < 100)
          estadoRetencionIncremento = 2;
        if(tIncremento >= 100)
          estadoRetencionIncremento = 3;
      break;
      case 3: 
        if(digitalRead(incremento) == LOW){
          flagPulsoIncremento  = TRUE;
          estadoRetencionIncremento  = 1;
        }
        else{
          flagPulsoIncremento = FALSE;
          estadoRetencionIncremento = 1;
        }
      break;
    }
    switch(estadoRetencionInicio){
      case 1:
        flagPulsoInicio = FALSE;

        if(digitalRead(inicio) == HIGH)
          estadoRetencionInicio = 1;

        if(digitalRead(inicio) == LOW){
          tInicio = 0;
          estadoRetencionInicio = 2;
        }
      break;
      case 2:
        if(tInicio < 100)
          estadoRetencionInicio = 2;
        if(tInicio >= 100)
          estadoRetencionInicio = 3;
      break;
      case 3: 
        if(digitalRead(inicio) == LOW){
          flagPulsoInicio = TRUE;
          estadoRetencionInicio = 1;
        }
        else{
          flagPulsoInicio = FALSE;
          estadoRetencionInicio = 1;
        }
      break;
    }
    /*Si el pulsador verdaderamente esta presionado, se incrementa una vez la variable*/
    if(flagPulsoIncremento == TRUE){
      numViajes++;
    }
    if(estadoLcd == 2){ //condicion para salir de este estado, le puse esta para no repetir la condicion del estado del lcd
      juego(); //llamo para encender el primer led
      estadoPrograma = 2;
      tmin = 0;
      tseg = 0;
      thora = 0;
    }
  break;
  case 2:
    /*Cuando se detecta algun infra se va al sig estado del programa donde se cuentan bien la cantidad de viajes 
      Despues de contar la cantidad de viajes, se llama a la funcion juego, la cual despues de prender el sig led viene a este estado de programa
    */
    if(digitalRead(infra1) == LOW || digitalRead(infra2) == LOW || digitalRead(infra3) == LOW || digitalRead(infra4) == LOW || digitalRead(infra5) == LOW){
      estadoPrograma = 3;
    }

    if (Serial.available()){

      estadoBluetooth = Serial.read(); 

      ///SERVO 1 -- DERECHA IZQUIERDA -- 9///
      if(estadoBluetooth == '1'){
        grados1++;
        if(grados1 >= 180){
          grados1 = 180;
        }
        
        miservo_1.write(grados1, 0);
        
      }
      if(estadoBluetooth == '2'){
        grados1--; 
        if(grados1 <= 0){
          grados1 = 0;
        }
        
        miservo_1.write(grados1, 0);
        
      }

      ///SERVO 2 -- ADELANTE ATRAS -- 6///
      if(estadoBluetooth == '3'){
        grados2++;
        if(grados2 >= 180){
          grados2 == 180;
        }
        
        miservo_2.write(grados2, 200);
        
      }
      if(estadoBluetooth == '4'){
        grados2--;
        if(grados2 >= 0){
          grados2 == 0;
        }
        
        miservo_2.write(grados2, 200);
        
      }

      ///SERVO 3 -- ABAJO -- 11///
      if(estadoBluetooth == '5'){    
        grados3--;        
        if(grados3<=0){
          grados3 = 90;
        }
          
        miservo_3.write(grados3, 0);
        
      }  
    }
    }
  break;
  case 3:
  /*  Se detectan los viajes 
   *  La deteccion se produce cuando los infra cambian de estado, es decir que un viaje
   *  se considera valido cuando el bloque se levanta de la plataforma
  */
    switch (estadoInfras)
    {
      case 0:
        if(digitalRead(infra1) == HIGH && digitalRead(infra2) == HIGH && digitalRead(infra3) == HIGH && digitalRead(infra4) == HIGH && digitalRead(infra5) == HIGH){
          estadoInfras = 0;
        }
        if(digitalRead(infra1) == LOW || digitalRead(infra2) == LOW || digitalRead(infra3) == LOW || digitalRead(infra4) == LOW || digitalRead(infra5) == LOW){
          estadoInfras = 1;
        }
      break;
      case 1:
        if(digitalRead(infra1) == LOW || digitalRead(infra2) == LOW || digitalRead(infra3) == LOW || digitalRead(infra4) == LOW || digitalRead(infra5) == LOW){
          estadoInfras = 1;
        }
        if(digitalRead(infra1) == HIGH && digitalRead(infra2) == HIGH && digitalRead(infra3) == HIGH && digitalRead(infra4) == HIGH && digitalRead(infra5) == HIGH){
          contadorViajes++;
          juego();
          estadoInfras = 0;
        }
      break;
    }
  break;
  }
}

void tiempo(){
  /*Esta funcion cuenta cada 1ms 
   * tIncremento y tInicio se utilizan para la retencion de los pulsadores
   * taux se utiliza para contar cada 1seg
   * Luego esa misma se usa para la cuanta regresiva con tlcd y para la cuenta general del programa con tseg, tmin y thora
  */
  tIncremento++;
  tInicio++;
  taux++;
  
  if(taux >= 1000){
    tlcd--;
    taux = 0;

    if(estadoLcd == 2){
      tseg++;
      if(tseg >= 60){
        tmin++;
        tseg = 00;
        if(tmin >= 60){
          thora++;
          tmin = 00;
        }
      }
    }
  }
}

void actualizarLcd(){
  /* En esta MEF estan agrupadas todas las salidas en pantalla con sus respectivas condiciones  
   * para cambiar de estado
  */
  switch (estadoLcd)
  {
    case 0:
      lcd.setCursor(0,0);
      lcd.print("Cant de viajes: ");
      lcd.setCursor(0,1);
      lcd.print(numViajes);

      if(flagPulsoInicio == FALSE){
        estadoLcd = 0;
      }
      else{
        tlcd = 5;
        estadoLcd = 1;
      }
    break;
    case 1:
      lcd.setCursor(0,0);
      lcd.print("El juego inicia");
      lcd.setCursor(0, 1);
      lcd.print("     en: ");
      lcd.print(tlcd);

      if(tlcd > 0)
        estadoLcd = 1;
      else{
        lcd.clear();
        estadoLcd = 2;
      }
    break;
    case 2:
      lcd.setCursor(0, 0);
      lcd.print("    A JUGAR!    ");
      lcd.setCursor(4, 1);
      lcd.print(thora);
      lcd.print(":");
      lcd.print(tmin);
      lcd.print(":");
      lcd.print(tseg);

      if(contadorViajes != numViajes)
        estadoLcd = 2;
      else
        estadoLcd = 3;
    break;
    case 3:
      lcd.setCursor(0, 0);
      lcd.print("  Felicidades!  ");
      lcd.setCursor(4, 1);
      lcd.print(thora);
      lcd.print(":");
      lcd.print(tmin);
      lcd.print(":");
      lcd.print(tseg);

      // if(finMensajeFinal == FALSE) //esta flag se usaria para demostrar que se termino de imprimir el mensaje final
      //   estadoLcd = 4;
      // else
      //   estadoLcd = 1;
    break;
  }
}
void juego(){
  /* En esta funcion se cambia el led que esta encendido, con la condicion de que no se prenda dos veces el mismo
   * Esta funcion es llamada cuando se detecta como valido un viaje  
   * Luego de encender el led se va al estadoPrograma 2 donde se reciben instrucciones para la grua
   */
  while(aleatorio == numAnterior){
    aleatorio = random(0, 5);
  }
  
  switch (aleatorio)
  {
    case 0:
      digitalWrite(pinLatch, LOW);              
      shiftOut(dataPin, clockPin, MSBFIRST, 1); 
      digitalWrite(pinLatch, HIGH);
      numAnterior = 0;
      estadoPrograma = 2;
    break;
    case 1:
      digitalWrite(pinLatch, LOW);              
      shiftOut(dataPin, clockPin, MSBFIRST, 2); 
      digitalWrite(pinLatch, HIGH);
      numAnterior = 1;
      estadoPrograma = 2;
    break;
    case 2:
      digitalWrite(pinLatch, LOW);              
      shiftOut(dataPin, clockPin, MSBFIRST, 4); 
      digitalWrite(pinLatch, HIGH);
      numAnterior = 2;
      estadoPrograma = 2;
    break;
    case 3:
      digitalWrite(pinLatch, LOW);              
      shiftOut(dataPin, clockPin, MSBFIRST, 8);
      digitalWrite(pinLatch, HIGH);
      numAnterior = 3;
      estadoPrograma = 2;
    break;
    case 4:
      digitalWrite(pinLatch, LOW);               
      shiftOut(dataPin, clockPin, MSBFIRST, 16); 
      digitalWrite(pinLatch, HIGH);
      numAnterior = 4;
      estadoPrograma = 2;
    break;
  }
}