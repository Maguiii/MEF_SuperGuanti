#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>
#include <TimerOne.h>
#include <Wire.h>
/* 
  Retencion de inicio para pulsadores inicio e incremento 
  Pantalla lcd funcionando con todos los mensajes         
  Deteccion de los cambios de estado de los infras        
  Con la deteccion de los viajes se cambia el led
  Al cambiar el led se esperan instrucciones para la grua 
  Programa en bucle (al finalizar el juego se puede volver a empezar)

  Probar con la grua

  Agregar contador de pulsaciones por dedo
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

Servo miservo_1, miservo_2, miservo_3;

LiquidCrystal_I2C lcd(0x3F, 16, 2);

int tIncremento = 0;
int tInicio = 0;
int tInfras = 0;
int taux = 0;
int tauxmili = 0;
int tlcd = 0;
int tmin = 0;
int tseg = 0;
int thora = 0;

int estadoPrograma = 1;
int estadoRetencionIncremento = 1;
int estadoRetencionInicio = 1;
int estadoInfras = 0;
int estadoLcd = 0;
int estadoBluetooth = 0;

int numViajes = 0;
int contadorViajes = 0;
int aleatorio = 0;
int numAnterior = 0;

int menique = 0;
int indice = 0;
int anular = 0;
int mayor = 0;
int pulgar = 0;

int grados1 = 0;
int grados2 = 0;
int grados3 = 9
0;

bool flagPulsoIncremento = FALSE;
bool flagPulsoInicio = FALSE;
bool flagMensajeFinal = FALSE;
bool flagHabilitacionInicio = FALSE;

void actualizarLcd();
void juego();
void retencionInicio();

void setup(){
  //Inicializacion del Timer2
  cli(); 
  TCCR2A = 0; 
  TCCR2B = 0; 
  TCNT2 = 0;  

  OCR2A = 255; 
  TCCR2A |= (1 << WGM21);
  TCCR2B |= (0b00000111); //1024 (preescala)
  TIMSK2 |= (1 << OCIE2A);
  sei(); 

  Serial.begin(57600); 

  miservo_1.attach(9, 350, 2900); //servo base, derecha-izquierda
  miservo_1.write(grados1); 

  miservo_2.attach(6, 1000, 2000); //servo de la derecha, adelante-atras
  miservo_2.write(grados2); 

  miservo_3.attach(11, 1000, 2000); //servo de la izqueirda, abajo
  miservo_3.write(grados3);
  delay(500);

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
  lcd.clear();

  pinMode(incremento, INPUT);
  pinMode(inicio, INPUT);

  pinMode(pinLatch, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);

  //Sin estas lineas hay un led que empieza encendido, copiar esto tmb al final del programa
  digitalWrite(pinLatch, LOW);              
  shiftOut(dataPin, clockPin, MSBFIRST, 0); 
  digitalWrite(pinLatch, HIGH);
}

ISR(TIMER2_COMPA_vect){ 
/* Esta funcion se interrumpe cada 32.77us
   La cuenta no es exacta. Salida ejemplo: 4:30(salida arduino) 4:26(cronometro)
*/
  tauxmili++;
  if (tauxmili >= 30) {
    tIncremento++;
    tInicio++;
    taux++;
    
    if(taux >= 60){
      tlcd--;
      taux = 0;
      if(estadoLcd == 2){
        tseg++;
        if(tseg >= 60){
          tmin++;
          tseg = 0;
          if(tmin >= 60){
            thora++;
            tmin = 0;
          }
        }
      }
    }
    //tauxmili = 0; //esto no estaba antes no esta probado
  } 
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
            estadoRetencionIncremento = 2;

          if(digitalRead(incremento) == LOW){
            tIncremento = 0;
            estadoRetencionIncremento = 2;
          }
        break;
        case 2:
          if(tIncremento < 20)
            estadoRetencionIncremento = 2;
          if(tIncremento >= 20)
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
      retencionInicio(); //probar si esto funciona antes el switch estaba aca
      
      /*Si el pulsador verdaderamente esta presionado, se incrementa una vez la variable
        Se puede dar inicio al juego luego de haber seleccionado minimo un viaje, entonces se habilita el boton inicio
      */
      if(flagPulsoIncremento == TRUE){
        numViajes++;
        flagHabilitacionInicio = TRUE;
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

        ///CONDICION 0 CUANDO NO SE ESTA PULSANDO NADA ///
        if(estadoBluetooth == '0'){
          miservo_1.write(grados1);
          miservo_2.write(grados2);
          miservo_3.write(grados3);
        }

        ///SERVO 1 -- DERECHA IZQUIERDA -- 9///
        if(estadoBluetooth == '1'){
          grados1++;
          if(grados1 >= 180){
            grados1 = 180;
          }
          miservo_1.write(grados1); //,0 para velocidad 
        }

        if(estadoBluetooth == '3'){
          grados1--;
          if(grados1 <= 0){
            grados1 = 0;
          }
          miservo_1.write(grados1);
        }
        ///SERVO 2 -- ADELANTE ATRAS -- 6///
        if(estadoBluetooth == '5'){
          grados2++;
          if(grados2 >= 180){
            grados2 = 180;
          }
          miservo_2.write(grados2);
        }

        if(estadoBluetooth == '7'){
          grados2--;
          if(grados2 <= 0){
            grados2 = 0;
          }
          miservo_2.write(grados2);
        }
        ///SERVO 3 -- ABAJO -- 11///
        if(estadoBluetooth == '9'){    
          grados3--;        
          if(grados3<=0){
            grados3 = 90;
          }
          miservo_3.write(grados3);
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
            if(estadoLcd == 2){
              juego();
            }
            estadoInfras = 0;
          }
        break;
      }
    break;
    case 4:
    /*En este estado se entra desde la condicion anterior y desde las condiciones del lcd al llegar al ultimo caso
      Se reinician todas las varibles definidas en el inicio   
      La variable flagHabilitacionInicio se desactiva para volver a ingresar la cantidad de viajes que se quieren
    */
      tmin = 0;
      tseg = 0;
      thora = 0;

      numViajes = 0;
      contadorViajes = 0;
      aleatorio = 0;
      numAnterior = 0;

      estadoLcd = 0;
      flagHabilitacionInicio = FALSE;
      estadoPrograma = 1;
    break;
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

      if(flagPulsoInicio == TRUE && flagHabilitacionInicio == TRUE){ //se cambia de estado si esta habilitado el boton inicio, es decir que ya hay viajes seleccionados
        tlcd = 5;
        estadoLcd = 1;
      }
      else{
        estadoLcd = 0;
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
      else{
        tlcd = 5;//este tiempo es corto porque se estan haciendo pruebas, despues se puede modificar
        lcd.clear();
        estadoLcd = 3;
      }
        
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

      if(tlcd <= 0){
        tlcd = 5;
        lcd.clear();
        estadoLcd = 4;
      }
    break;
    case 4:
    /*Por ahora aca se muestra todo en cero, falta desarrollar la suma de estas variables*/
      lcd.setCursor(0,0);
      lcd.print("1:");
      lcd.print(menique);
      lcd.print(" 2:");
      lcd.print(indice);
      lcd.print(" 3:");
      lcd.print(pulgar);
      lcd.setCursor(0, 1);
      lcd.print("4:");
      lcd.print(anular);
      lcd.print(" 5:");
      lcd.print(mayor);

      if(tlcd <= 0){
        tlcd = 10;
        lcd.clear();
        estadoLcd = 5;
      }
    break;
    case 5:
      lcd.setCursor(0,0);
      lcd.print(" Para reiniciar ");
      lcd.setCursor(0,1);
      lcd.print("presionar inicio");

      retencionInicio();
      if(flagPulsoInicio == TRUE){
        lcd.clear();
        estadoPrograma = 4;
      }
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
void retencionInicio(){
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
      if(tInicio < 7)
        estadoRetencionInicio = 2;
      if(tInicio >= 7)
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
}