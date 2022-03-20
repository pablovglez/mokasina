// Incluye:
// indicador de porción completa sin 'delay', usa millis()
// protección para giros infinitos con un contador de millis() en activaMotor()
// lámpara con timer de activación y duración aleatoria

#include <Wire.h>
//#include <LiquidCrystal.h>  // Libreria para LCD sin I2C
#include <LiquidCrystal_I2C.h> // Library for I2C  LCD 
#define PCF8563address 0x51

// Declaración de entradas y salidas
//ALIMENTADOR
const int pinmotor = 7;
const int pinsensor = 6;
const int pinsel = 8;
const int pinreg = 9;
const int pinsube = 12;
const int pinbaja = 10;
String sT = ":";
String sP = "/";
String etiqueta2 = " ";
//LiquidCrystal lcd(13, 11, 5, 4, 3, 2); //lcd sin I2C
LiquidCrystal_I2C lcd(0x3F, 16, 2); // I2C address 0x3f, 16 column and 2 rows

// Declaración de variables del alimentador
int tproteccion = 4020;
int menu = 0;
int racion = 8;
int edoreg = 0;
int edosube = 0;
int edobaja = 0;
int edosel = 0;
int AH = 18;
int AM = 30;
int TH = 00;
int TM = 30;
int giros = 0; //testigo de ración
long marcadetiempo1, marcadetiempo2;
unsigned long dTiempo1, dTiempo2;
char puntero;
bool hora_minuto = 0; // 0:horas 1:minutos
bool edosensor = false;  // estado del sensor ( false enc / true apag )
bool porcion_servida = 0; // indicador de porción completamente servida (0:no servida 1: servida)

// Declaracion de variables del reloj
byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
String days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };

// Declaracion de variables de la lámpara
// long TH = 0; //previamente declarada
// long TM = 0; //previamente declarada
//int TH , TM;
int TS, set;
long ran1, ran2;
long AHL, AML; //hora y minuto de lámpara
long lapso, lapsoh, frecCambio;
unsigned long currMillis_Al, currMillis_Ti;
const int pinLampara = 11;
int pinSemilla = A0;
int previous_millis, current_millis; // Posiblemente sin uso
int testigoLampara = 13;
long prevMillis_Ti = 0;
long prevMillis_Al = 0;


//////////////////// A P A R T A D O  D E L  R E L O J ///////////////////////

void falsoReloj () {
  if (random() >= 5) {
    TH = TH + 1;
  }
  if (TH == 24) {
    TH = 0;
  }
}


byte bcdToDec(byte value) {
 return ((value / 16) * 10 + value % 16);
}

byte decToBcd(byte value){
 return (value / 10 * 16 + value % 10);
}

void setPCF8563()
// this sets the time and date to the PCF8563
{
  Wire.beginTransmission(PCF8563address);
  Wire.write(0x02);
  Wire.write(decToBcd(second));  
  Wire.write(decToBcd(minute));
  Wire.write(decToBcd(hour));     
  Wire.write(decToBcd(dayOfMonth));
  Wire.write(decToBcd(dayOfWeek));  
  Wire.write(decToBcd(month));
  Wire.write(decToBcd(year));
  Wire.endTransmission();
}

void readPCF8563()
// this gets the time and date from the PCF8563
{
  Wire.beginTransmission(PCF8563address);
  Wire.write(0x02);
  Wire.endTransmission();
  Wire.requestFrom(PCF8563address, 7);
  second     = bcdToDec(Wire.read() & B01111111); // remove VL error bit
  minute     = bcdToDec(Wire.read() & B01111111); // remove unwanted bits from MSB
  hour       = bcdToDec(Wire.read() & B00111111); 
  dayOfMonth = bcdToDec(Wire.read() & B00111111);
  dayOfWeek  = bcdToDec(Wire.read() & B00000111);  
  month      = bcdToDec(Wire.read() & B00011111);  // remove century bit, 1999 is over
  year       = bcdToDec(Wire.read());
  // traduce hora reloj a hora display
  TH = hour;
  TM = minute; 
}

//////////////////////////// F I N   R E L O J /////////////////////////////

void setup () {
 //  set up the LCD's number of columns and rows:
  lcd.init();
  lcd.begin(16,2);
  lcd.backlight();
  Serial.begin(9600);
  pinMode(pinmotor, OUTPUT);
  pinMode(pinsensor, INPUT);
  pinMode(pinsel, INPUT);
  pinMode(pinreg, INPUT);
  pinMode(pinsube, INPUT);
  pinMode(pinbaja, INPUT);
  
// Ajuste de la hora inicial del reloj
  Wire.begin();
  second = 0;
  minute = 30;
  hour = 12;
  dayOfWeek = 3; //0-domingo  7-sabado
  dayOfMonth = 30;
  month = 12;
  year = 20;
  // comment out the next line and upload again to set and keep the time from resetting every reset
  //setPCF8563();

// Ajuste de la lámpara
  pinMode(pinLampara, OUTPUT);
  pinMode(testigoLampara, OUTPUT);
  digitalWrite(pinLampara, HIGH);

  randomSeed(analogRead(pinSemilla));
  AHL = random(19,22);      // valores iniciales de alarma
  AML = random(0,59);      // valores iniciales de alarma
  lapso = random(6,11)*1620000;
//  frecCambio = 86400000; // 86400000 ms = 24h  // Será removido de aqui, hacia alarma_aleatoria
}

void botones () {
  // Revisa el estado de los botones
  edoreg = digitalRead(pinreg);
  delay(100);
  // Si presionas, avisa:
  if (edoreg == 1) {
    Serial.println((String)"boton regresa: " + edoreg);
  }
  edosube = digitalRead(pinsube);
  delay(100);
  // Si presionas, avisa:
  if (edosube == 1) {
    Serial.println((String)"boton sube: " + edosube);
  }
  edobaja = digitalRead(pinbaja);
  delay(100);
  // Si presionas, avisa:
  if (edobaja == 1) {
    Serial.println((String)"boton baja: " + edobaja);
  }
  edosel = digitalRead(pinsel);
  delay(100);
  // Si presionas, avisa:
  if (edosel == 1) {
    Serial.println((String)"boton seleccion: " + edosel);
  }
  delay(500);
}


void ajustarRacion () {
  Serial.println((String)" Racion:" + racion);
  String linea2 = ("    R" + sT + racion);
  lcd.setCursor (0, 1);
  lcd.print(linea2);
  if (edosube == 1) {
    delay(200);
    racion = racion + 1;
  }
  if (edobaja == 1) {
    delay(200);
    racion = racion - 1;
  }
}

void ajustarAlarma () {
  Serial.println((String)"Horario " + AH + ":" + AM + " P:" + puntero);
  String linea2 = (+ AH + sT + AM + "          " + puntero);
  lcd.setCursor (0, 1);
  lcd.print(linea2);
  if (hora_minuto == 0) {
    puntero = 'H';
  }
  if (hora_minuto == 1) {
    puntero = 'M';
  }
  if (edosel == 1) {
    delay(200);
    hora_minuto = !hora_minuto;
  }
  if (puntero == 'M') {
    if (edosube == 1) {
      delay(200);
      AM = AM + 5;
    }
    if (edobaja == 1) {
      delay(200);
      AM = AM - 5;
    }
    if (AM == 60) {
      AM = 00;
    }
    if (AM == -5) {
      AM = 55;
    }
  }
  if (puntero == 'H') {
    if (edosube == 1) {
      delay(200);
      AH = AH + 1;
    }
    if (edobaja == 1) {
      delay(200);
      AH = AH - 1;
    }
    if (AH == 24) {
      AH = 00;
    }
    if (AH == -1) {
      AH = 00;
    }
  }
}


void activaMotor () {
  // Revisa el estado del sensor
  // porcion_servida = 0;
  edosensor = digitalRead(pinsensor);
  // Revisa por si no se ha servido ya
  if (porcion_servida == 0) {
    // Pon el contador en ceros y gira el motor 'racion' veces
    lcd.clear();
    String linea1 = ("Vamos a comer");
    lcd.setCursor (0, 0);
    lcd.print(linea1);
    giros = 0;
    digitalWrite(pinmotor, LOW);  // Se cambia la polaridad de pinmotor, dado que el relevador se acciona con 'tierra'.
    marcadetiempo2 = millis();
    while (giros < racion) {
      dTiempo2 = (millis() - marcadetiempo2);           // impide que el motor de surtido gire por más de (racion*tproteccion) segundos
      if ( dTiempo2 >= (racion * tproteccion)){         // es decir, 'tproteccion' por cada vuelta
        giros = racion;
      }
      if (digitalRead(pinsensor) == HIGH) {
        delay(200);                                     // tiempo que tarda el sensor en desactivarse después de haber leído
        giros = giros + 1;
        Serial.println(giros);
        delay(10);
      }
      if (giros == racion) {
        marcadetiempo1 = millis();
        digitalWrite(pinmotor, HIGH);
        
        String linea1 = ("Servido!   :-)");
        lcd.setCursor (0, 0);
        lcd.print(linea1);
        Serial.println((String)"racion completa");
        //Serial.println((String)"servido"+ sT +porcion_servida);       // contador de vueltas no funciona
        porcion_servida = 1;
        delay(10);
 //     if (porcion_servida == 1) {
 //       delay (60000);
 //       porcion_servida = 0;
 //     }
    }
      String linea2 = ("Sirviendo...  ");
      //String linea2 = ("Sirviendo: " + giros + sP + racion );  // contador de vueltas no funciona
      lcd.setCursor (0, 1);
      lcd.print(linea2);
    }
  }
  else {
    digitalWrite(pinmotor, HIGH);
  }
}



void selectorMenu () {
  if (edoreg == 1) {
    delay(200);
    menu = menu + 1;
  }
  if (menu == 0) {
    Serial.println((String)+TH+":"+TM+"  [" + AH + ":" + AM + "]  M:" + menu);
    Serial.println((String)"LMMJVSD - Racion:" + racion);
    lcd.clear(); // mas 16 caracteres
//    String linea1 = ( + TH + sT + TM + "    d  " + (AH-10) + sT + AM );
    String linea1 = ( + TH + sT + TM + "  r"+racion+"  " + AH + sT + AM );
    lcd.setCursor (0, 0);
    lcd.print(linea1);
//    String linea2 = ("R" + sT + racion + "      c " + (AH) + sT + AM );
    String linea2 = (etiqueta2 + AHL + sT + AML + " / " + lapsoh + "min");
    lcd.setCursor (0, 1);
    lcd.print(linea2);
  }
  if (menu == 1) {
    Serial.println((String)" AJUSTA PORCION  M:" + menu);
    lcd.clear();
    String linea1 = ("Nueva racion");
    lcd.setCursor (0, 0);
    lcd.print(linea1);
    ajustarRacion ();
  }
  if (menu == 2) {
    Serial.println((String)" AJUSTA ALARMA  M:" + menu);
    lcd.clear();
    String linea1 = ("Hora de comida");
    lcd.setCursor (0, 0);
    lcd.print(linea1);
    ajustarAlarma ();
  }
//  if (menu == N) {
//    Serial.println((String)" AJUSTA TIEMPO  M:" + menu);
//  }
  if (menu == 3) {
    menu = 0;
  }
}

void alarmaAleatoria () {
  currMillis_Al = millis();
  if ((currMillis_Al - prevMillis_Al) > frecCambio) { 
    Serial.print(" c a m b i o    d e    a l a r m a ");
    prevMillis_Al = currMillis_Al;
                // VALORES DE PRUEBA
 //  frecCambio = 4     000; // 86400000 ms = 24h
 //   ran1 = random(0,4);
 //   ran2 = random(0,4);
 //   lapso = 20000; // random(1,3)*10000;     // 1 a 10 seg
//    lapsoh = lapso/60000  // traduce la duracion de la alarma a minutos
 //   AHL = ran1;
 //   AML = AM;

                // VALORES DE PRODUCCION
    frecCambio = 86400000; // 86400000 ms = 24h
    ran1 = random(20,22);
    ran2 = random(0,59);
//    lapso = random(2,6)*1620000; // Dura de 2 a 6 veces 45min (1.5h a 4.5h)
    lapso = random(5400000,10800000); // Dura de 2 a 6 veces 45min (1.5h a 4.5h)
    lapsoh = lapso/60000;  // traduce la duracion de la alarma a minutos
    AHL = ran1;
    AML = ran2;

  }
}

void prendeLampara () {
  digitalWrite(pinLampara,LOW); 
  digitalWrite(testigoLampara, HIGH);
  prevMillis_Ti = millis();
  Serial.print ("              L A M P A R A    P R E N D I D A ");
}

void loop() {
//  falsoReloj();
  readPCF8563();

///////////////// L A M P A R A
  alarmaAleatoria();
  Serial.println((String) " ");
  Serial.println((String) "prevMillis_Tiempo: "+prevMillis_Ti+" Millis : "+millis());
  Serial.println((String) "Tiempo: "+TH+" : "+TM);
  Serial.println((String) "Alarma: "+AHL+" : "+AML+" durante "+lapsoh+" min");
 // Serial.println((String)"frecCambio: "+frecCambio+" currMilis: "+currMillis_Al+" divisor: "+(3600000)+" 1/hr*min*seg");  
  if (TH == AHL) {
   if (TM == AML) {
//    if (TS <= 5) {       //Probablemente no sea necesario medir el tiempo hasta 'segundos'
      prendeLampara();
//    }                   // complemento de TS
   }
  }
  if ((millis() - prevMillis_Ti) >= lapso) {
//    prevMillis_Ti = millis();
    digitalWrite(pinLampara, HIGH);
    digitalWrite(testigoLampara, LOW);
    Serial.print ("                    L A M P A R A    O F F ");
  }

///////////////// A L I M E N T A D O R
  botones();
  selectorMenu();
  dTiempo1 = (millis() - marcadetiempo1);  // impide que se repita el ciclo de surtido por durar menos de un minuto
  if ( dTiempo1 >= 60000){
    porcion_servida = 0;
  }

///////////////// D E B U G

//  Serial.println((String)"horasssss: "+TH);
//  Serial.println((String)"minutosss: "+TM);
    Serial.println("= - = - = - = - = - = - = - = - = - = - = - ");
    Serial.println((String) " Millis : "+millis());
    Serial.println((String) " dTiempo1 : "+dTiempo1);
    Serial.println((String) " porcion servida : "+porcion_servida);
    Serial.println((String) " dTiempo2 : "+dTiempo2);



 
//////////////////////
  if (TH == AH) {
    if (TM == AM) {
      Serial.println((String)"Vamos a cenar!");
      activaMotor ();
    }
  }
  if (TH == (AH - 10)) { // hace girar el motor 10 horas más temprano, para desayunar
    if (TM == AM) {
    Serial.println((String)"A desayunar!");
      activaMotor ();
    }
  }
  else {
    digitalWrite(pinmotor, HIGH);
  }

}
