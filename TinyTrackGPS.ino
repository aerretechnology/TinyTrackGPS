

/*
     Programa de localizacion por gps que graba las posiciones en
     un fichero de texto de forma diaria. 

     Conectar módulo SD con pin CS (naranja) en pin 10 arduino.

     Conectar módulo NMEA-6M (gps) pines 8,9 (9 - pin rx negro)

     Conectar LCD 16x2 pines 2,3,4,5,6,7 (2-amarillo , 3-azul,
     4-rojo, 5-azul oscuro, 6-verde, 7-blanco)
*/
#include <LCD.h>
#include <LiquidCrystal.h>
#include <SoftwareSerial.h>
#include <TinyGPS.h>
#include <SPI.h>
#include <SD.h>

File GPSFile;
char GPSLogFile[] = "YYYYMMDD.TXT";
float flat, flon;
unsigned long age;

/* Código de demostración uso de librería TinyGPS.
   Requiere uso de librería SoftwareSerial, se presupone que disponemos
   de un dispositivo GPS serie de 9600-bauds conectado en pines 9(rx) y 8(tx).
*/
TinyGPS gps;
SoftwareSerial ss(9, 8);

/* Constantes para declaracion del LCD */
const int LCD_NB_ROWS = 2;
const int LCD_NB_COLUMNS = 16;
/* Crea el objeto lcd */
LiquidCrystal lcd(2, 3, 4, 5, 6, 7);

void setup(void) {
  bool config = false;
  Serial.begin(9600);
  ss.begin(9600);

  pinMode(10, OUTPUT);

  Serial.print("Initializing SD card...");

  if (!SD.begin(10)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");

  /* Initialise l'écran LCD */
  lcd.begin(LCD_NB_COLUMNS, LCD_NB_ROWS);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Waiting for"));
  lcd.setCursor(0, 1);
  lcd.print(F("GPS signal."));

  do {
  while (ss.available())
    {
      if (gps.encode(ss.read())) // Did a new valid sentence come in?
        {
          int year;
          byte month, day, hour, minute, second, hundredths;
          unsigned long age;
          
          gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
          if (sprintf(GPSLogFile, "%04d%02d%02d.txt", year, month, day) > 0) config = true;

          Serial.print("Filename: ");
          Serial.println(GPSLogFile);
        }
    }
  }while(!config);
}

void loop(void) {
//  unsigned long chars;
//  unsigned short sentences, failed;
  int year;
  byte month, day, hour, minute, second, hundredths;
  unsigned long age;
  char timestr[10];
  
  // For one second we parse GPS data and report some key values
  for (unsigned long start = millis(); millis() - start < 1000;)
  {
    while (ss.available())
    {
      if (gps.encode(ss.read())) { // Did a new valid sentence come in?
        gps.f_get_position(&flat, &flon, &age);
        lcd.setCursor(0, 0);
        lcd.print(F("LAT="));
        lcd.print(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6);
        lcd.setCursor(0, 1);
        lcd.print(F("LON="));
        lcd.print(flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flon, 6);
/*        
        gps.stats(&chars, &sentences, &failed);
        Serial.print(" CHARS=");
        Serial.print(chars);
        Serial.print(" SENTENCES=");
        Serial.print(sentences);
        Serial.print(" CSUM ERR=");
        Serial.println(failed);
        if (chars == 0)
          Serial.println("** No characters received from GPS: check wiring **");
*/

        gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
        if (age != TinyGPS::GPS_INVALID_AGE){
          sprintf(timestr, "%02d:%02d:%02d,", hour, minute, second);
        }
  
        if (GPSFile = SD.open(GPSLogFile, FILE_WRITE)) {
          Serial.print("Open GPSLogFile to write...");
          GPSFile.print(timestr);
          GPSFile.print(flat,6);
          GPSFile.print(",");
          GPSFile.println(flon,6);
          GPSFile.close();
          Serial.println("Done.");
        } else {
          Serial.println("** Error opening GPSLogFile. **");
        }
      }
    }
  }
}
