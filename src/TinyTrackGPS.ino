/*
TinyTrackGPS.ino - A simple track GPS to SD card logger.
TinyTrackGPS v0.3

Copyright © 2019-2021 Francisco Rafael Reyes Carmona.
All rights reserved.

rafael.reyes.carmona@gmail.com

  This file is part of TinyTrackGPS.

  TinyTrackGPS is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  TinyTrackGPS is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with TinyTrackGPS.  If not, see <https://www.gnu.org/licenses/>.
*/

/*
     Programa de localizacion por gps que graba las posiciones en
     un fichero de texto cada segundo, de forma diaria.

     Conectar módulo SD con pin CS (naranja) en pin 10 arduino.

     Conectar módulo NMEA-6M (gps) pines 8,9 (9 - pin rx negro)

     Conectar LCD 16x2 pines 2,3,4,5,6,7 (2-amarillo , 3-azul,
     4-rojo, 5-azul oscuro, 6-verde, 7-blanco)
*/
// Include libraries.
#include <LiquidCrystal.h>
#include <SoftwareSerial.h>
#include <TinyGPS.h>
#include <SD.h>
#include "UTMconversion.h"

File GPSFile;
char GPSLogFile[] = "YYYYMMDD.csv"; // Formato de nombre de fichero. YYYY-Año, MM-Mes, DD-Día.
boolean SDReady;
float flat, flon;
unsigned long age;

/* Código de demostración uso de librería TinyGPS.
   Requiere uso de librería SoftwareSerial, se presupone que disponemos
   de un dispositivo GPS serie de 9600-bauds conectado en pines 9(rx) y 8(tx).
*/
TinyGPS gps;
GPS_UTM utm;
#define UTM_LCD
#define PIN_SELECT 8
bool pin = LOW;
SoftwareSerial gps_serial(9, 8);
int year_actual;
byte month_actual, day_actual;

inline void GPSData(unsigned long ms);

/* Constantes para declaracion del LCD */
const int LCD_NB_ROWS = 2;
const int LCD_NB_COLUMNS = 16;
/* Crea el objeto lcd */
LiquidCrystal lcd(2, 3, 4, 5, 6, 7);

void setup(void) {
  boolean config = 0;
  Serial.begin(9600);
  gps_serial.begin(9600);

  pinMode(10, OUTPUT);

  Serial.print(F("Initializing SD card..."));

  SDReady = SD.begin(10);
  if (!SDReady) {
    Serial.println(F("FAILED!"));
  }
  Serial.println(F("Done."));

  /* Declaramos pin para selector visor coordenadas */
  pinMode(PIN_SELECT,INPUT_PULLUP);

  /* Iniciaización del LCD */
  lcd.begin(LCD_NB_COLUMNS, LCD_NB_ROWS);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("Waiting for"));
  lcd.setCursor(0, 1);
  lcd.print(F("GPS signal."));

  Serial.print(F("Waiting for GPS signal..."));

  do {
    while (gps_serial.available())
    {
      if (gps.encode(gps_serial.read())) // Comprueba que ha recibido una sentencia del GPS.
        {
          int year;
          byte month, day, hour, minute, second, hundredths;
          unsigned long age;

          gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
          if (sprintf(GPSLogFile, "%04d%02d%02d.csv", year, month, day) > 0) {
            config = true;
            Serial.println(F("Done."));
            }
          year_actual = year;
          month_actual = month;
          day_actual = day;

          // Si no existe el fichero lo crea y añade las cabeceras.
          if (SDReady & !SD.exists(GPSLogFile)) {
             if (GPSFile = SD.open(GPSLogFile, FILE_WRITE)) {
               Serial.print(F("New GPSLogFile, adding heads..."));
               GPSFile.println(F("Time,latitude,longitude,alt,utm"));
               Serial.println(F("Done."));
               GPSFile.close();
             } else {
               Serial.println(F("** Error creating GPSLogFile. **"));
             }
          }
        }
    }
  }while(!config);

  if (SDReady) {
    Serial.print(F("Filename: "));
    Serial.println(GPSLogFile);
  }
  Serial.println(F("Configuration ended."));
  lcd.clear();
}

void loop(void) {
  int year;
  byte month, day, hour, minute, second, hundredths;
  unsigned long age;
  int elevation;
  float f_elevation;
  char timestr[10];
  char utmstr[] = "30S 123456 1234567";

  // For one second we parse GPS data and report some key values

  GPSData(1000);
  gps.f_get_position(&flat, &flon, &age);
  f_elevation = gps.f_altitude();
  elevation = abs((int)f_elevation);
  utm.UTM(flat, flon);
  sprintf(utmstr, "%02d%c %ld %ld", utm.zone(), utm.band(), utm.X(), utm.Y());
  if (pin != digitalRead(PIN_SELECT)) {
    lcd.clear();
    pin = digitalRead(PIN_SELECT);
    }
  if (pin == LOW) {
    lcd.setCursor(0, 0);
    if (utm.zone()<10) lcd.print("0");
    lcd.print(utm.zone());
    lcd.setCursor(3,0);
    lcd.print(utm.band());
    lcd.setCursor(5, 0);
    lcd.print(F("Alt: "));
    lcd.print(elevation);
    lcd.print(F(" m"));
    lcd.setCursor(0, 1);
    //lcd.print(F("X="));
    lcd.print(utm.X());
    lcd.setCursor(8, 1);
    //lcd.print(F("Y="));
    lcd.print(utm.Y());
    }
  else {
    lcd.setCursor(0, 0);
    lcd.print(F("LAT="));
    lcd.print(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6);
    lcd.setCursor(0, 1);
    lcd.print(F("LON="));
    lcd.print(flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flon, 6);
    };

  gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
  if (age != TinyGPS::GPS_INVALID_AGE){
    sprintf(timestr, "%02d:%02d:%02d,", hour, minute, second);
  }
  if (year != year_actual || month != month_actual || day != day_actual) {
    sprintf(GPSLogFile, "%04d%02d%02d.csv", year, month, day);
    year_actual = year;
    month_actual = month;
    day_actual = day;

    // Si no existe el fichero lo crea y añade las cabeceras.
    if (SDReady & !SD.exists(GPSLogFile)) {
      if (GPSFile = SD.open(GPSLogFile, FILE_WRITE)) {
        Serial.print(F("New GPSLogFile, adding heads..."));
        GPSFile.println(F("Time,latitude,longitude,alt,utm"));
        Serial.println(F("Done."));
        GPSFile.close();
        }
        else {
        Serial.println(F("** Error creating GPSLogFile. **"));
        }
    }
  }

  if (SDReady & (GPSFile = SD.open(GPSLogFile, FILE_WRITE))) {
    Serial.print(F("Open GPSLogFile to write..."));
    GPSFile.print(timestr);
    GPSFile.print(flat,6);
    GPSFile.print(",");
    GPSFile.print(flon,6);
    GPSFile.print(",");
    GPSFile.print(elevation);
    GPSFile.print(",");
    GPSFile.println(utmstr);
    GPSFile.close();
    Serial.println(F("Done."));
    } else {
    Serial.println(F("** Error opening GPSLogFile. **"));
  }
}

inline void GPSData(unsigned long ms)
  {
    unsigned long start = millis();
    do
    {
      while (gps_serial.available())
        gps.encode(gps_serial.read());
    } while (millis() - start < ms);
  }
