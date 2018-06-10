

//Code written by Max Bäckström, copy as you want, no guarantees. Most comments and names for variables (and such) are in Swedish.
//Code is based upon the work of:
//Marshall Taylor @ SparkFun Electronics released under the MIT License (http://opensource.org/licenses/MIT)
//Bibliotek "SPI" är kanske onödig
#include <stdint.h>
#include <SparkFunBME280.h>
#include <Sodaq_DS3231.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <LiquidCrystal_I2C.h>

//****************************************************************//

// [Tid och datum] värde 0-6 översatt till text + alt. !OBS! bör bytas mot en serie "if" eller liknande (hanterar inte "åäö")
char weekDay[][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
//char weekDay[][4] = {"Sön", "Mån", "Tis", "Ons", "Tor", "Fre", "Lör" };
//char weekDay[][4] = {"Söndag", "Måndag", "Tisdag", "Onsdag", "Torsdag", "Fredag", "Lördag" };

// [Loggning] CS pin på SD-läsaren
const int chipSelect = 4;


// [Temp/Fukt/Tryck mätaren] Sparar värden för färre avläsningar
float temp;
float tryck;
float fuktighet;
BME280 mySensor;    //Namn på sensor i koden

// [Fotocell] Konfiguration och variabel för lättligänglighet (ljus = 0-1023)
int fotocellPin = 0;
int ljus;

// [Timing] variabler

long paus;
unsigned long paus2;
unsigned long paus3; 

// Variabel som växlar mellan 0 och 1 varje sekund, inte exakt (ej från RTC'n)
int sekblink = 1;

//[User input] Klappsensor
int knockSensor = 1; // Pin
byte val = 0;       // Senaste värdet
int THRESHOLD = 2; // Aktiverings gräns
  //Knapp
  int knapp1 = 8;

//[Kommunikation IO]
  //Röd Led
  boolean rodled = LOW;
  int rodledPin = 2;

//[Skärm] Bakrundsbelysnings variabel samt adress inst.
int belysn = 1;
LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address

  // Egna karaktärer
    // Grader ( ° )
     byte grader[8] = { 
       B00010,
       B00101,
       B00010,
     };

//**************************************************************************//
// 

void setup() {
  // put your setup code here, to run once:

  
//***[Temp/Fukt/Tryck mätaren] inställningar********************************//
  
  // Adress och koummunikations metod
  // commInterface can be I2C_MODE or SPI_MODE
  // specify chipSelectPin using arduino pin names
  // specify I2C address.  Can be 0x77(default) or 0x76
  // For I2C, enable the following and disable the SPI section
  mySensor.settings.commInterface = I2C_MODE;
  mySensor.settings.I2CAddress = 0x76; //Ska vara 0x76
  // For SPI enable the following and dissable the I2C section
  // mySensor.settings.commInterface = SPI_MODE;
  // mySensor.settings.chipSelectPin = 10;


  // Körsätt. BOSH rekomenderar FORCED mode för temp läsare men ingen observerad skillnad
  // Sleep = måste väckas
  // Forced = är i ngt hybrid sovläge väcks och gör en läsning
  // Normal = regelbunda avläsningar
  
  // runMode can be:
  //  0, Sleep mode
  //  1 or 2, Forced mode
  //  3, Normal mode
  mySensor.settings.runMode = 3; //"Tvingat" läge
  // Ifall NORMAL mode är detta tiden mellan läsningar
  // tStandby can be:
  //  0, 0.5ms
  //  1, 62.5ms
  //  2, 125ms
  //  3, 250ms
  //  4, 500ms
  //  5, 1000ms
  //  6, 10ms
  //  7, 20ms
  mySensor.settings.tStandby = 5;

  // Filter inst. berör endast tryck, motverkar "noise" eller temp luft "puffar"
  // filter can be off or number of FIR coefficients to use:
  //  0, filter off
  //  1, coefficients = 2
  //  2, coefficients = 4
  //  3, coefficients = 8
  //  4, coefficients = 16
  mySensor.settings.filter = 2;

  // Oversampling
  // Värden kan vara 0 = ingen, 1 till 5 är *1, *2, *4, *8, *16 (gånger mätningen görs, kan ses som snitt)
  mySensor.settings.tempOverSample = 5;
  mySensor.settings.pressOverSample = 3;
  mySensor.settings.humidOverSample = 5;

//*****[Pin lägen]***********************************************************//

pinMode(knapp1, INPUT);
pinMode(rodledPin, OUTPUT);

//***************************************************************************//

Serial.begin(57600);
Wire.begin();
rtc.begin(); //RTC klockan

//**********[Temp/Fukt/Tryck sensorn]*****************************************************//
delay(50);
mySensor.begin(); // Startar BME sensorn samt laddar inställningar
delay(10);

Serial.print("BME startad");

  // Alternativt starta med debuginfo till seriell port: (glöm inte att "//" mySensor.begin();)
  /*
  //Calling .begin() causes the settings to be loaded
  delay(10);  //Make sure sensor had enough time to turn on. BME280 requires 2ms to start up.
  Serial.println(mySensor.begin(), HEX);

  Serial.print("Displaying ID, reset and ctrl regs\n");
  
  Serial.print("ID(0xD0): 0x");
  Serial.println(mySensor.readRegister(BME280_CHIP_ID_REG), HEX);
  Serial.print("Reset register(0xE0): 0x");
  Serial.println(mySensor.readRegister(BME280_RST_REG), HEX);
  Serial.print("ctrl_meas(0xF4): 0x");
  Serial.println(mySensor.readRegister(BME280_CTRL_MEAS_REG), HEX);
  Serial.print("ctrl_hum(0xF2): 0x");
  Serial.println(mySensor.readRegister(BME280_CTRL_HUMIDITY_REG), HEX);

  Serial.print("\n\n");

  Serial.print("Displaying all regs\n");
  uint8_t memCounter = 0x80;
  uint8_t tempReadData;
  for(int rowi = 8; rowi < 16; rowi++ )
  {
    Serial.print("0x");
    Serial.print(rowi, HEX);
    Serial.print("0:");
    for(int coli = 0; coli < 16; coli++ )
    {
      tempReadData = mySensor.readRegister(memCounter);
      Serial.print((tempReadData >> 4) & 0x0F, HEX);//Print first hex nibble
      Serial.print(tempReadData & 0x0F, HEX);//Print second hex nibble
      Serial.print(" ");
      memCounter++;
    }
    Serial.print("\n");
  }
  
  
  Serial.print("\n\n");
  
  Serial.print("Displaying concatenated calibration words\n");
  Serial.print("dig_T1, uint16: ");
  Serial.println(mySensor.calibration.dig_T1);
  Serial.print("dig_T2, int16: ");
  Serial.println(mySensor.calibration.dig_T2);
  Serial.print("dig_T3, int16: ");
  Serial.println(mySensor.calibration.dig_T3);
  
  Serial.print("dig_P1, uint16: ");
  Serial.println(mySensor.calibration.dig_P1);
  Serial.print("dig_P2, int16: ");
  Serial.println(mySensor.calibration.dig_P2);
  Serial.print("dig_P3, int16: ");
  Serial.println(mySensor.calibration.dig_P3);
  Serial.print("dig_P4, int16: ");
  Serial.println(mySensor.calibration.dig_P4);
  Serial.print("dig_P5, int16: ");
  Serial.println(mySensor.calibration.dig_P5);
  Serial.print("dig_P6, int16: ");
  Serial.println(mySensor.calibration.dig_P6);
  Serial.print("dig_P7, int16: ");
  Serial.println(mySensor.calibration.dig_P7);
  Serial.print("dig_P8, int16: ");
  Serial.println(mySensor.calibration.dig_P8);
  Serial.print("dig_P9, int16: ");
  Serial.println(mySensor.calibration.dig_P9);
  
  Serial.print("dig_H1, uint8: ");
  Serial.println(mySensor.calibration.dig_H1);
  Serial.print("dig_H2, int16: ");
  Serial.println(mySensor.calibration.dig_H2);
  Serial.print("dig_H3, uint8: ");
  Serial.println(mySensor.calibration.dig_H3);
  Serial.print("dig_H4, int16: ");
  Serial.println(mySensor.calibration.dig_H4);
  Serial.print("dig_H5, int16: ");
  Serial.println(mySensor.calibration.dig_H5);
  Serial.print("dig_H6, uint8: ");
  Serial.println(mySensor.calibration.dig_H6);
    
  Serial.println();
  */
//**************[Display]***************************************************//
 //setup_display();
 lcd.begin(20,4);   // Konfigurerar display som 20 tecken x 4 rader samt slår på bakrundsljus
 delay(50);         // Säkerställa att displayen har startat (borde funka utan, men för säkerhets skull)
 
 lcd.createChar(0, grader);  // Skapar karaktären "grader" 
 
 for(int i = 0; i< 2; i++)  // Blinkar 2 gånger med skärmen som "User input" om en reset/uppladdning
  {
    lcd.backlight();
    delay(250);
    lcd.noBacklight();
    delay(250);
  }
  lcd.backlight(); // Avslutar med bakrundsbelysning påslagen
delay(100);

//**********[Loggning]**************************************************//

Serial.println();
Serial.println();
Serial.println();
Serial.print("Initializing SD card...");

  // Om ett minneskort finns och kan användas
  if (!SD.begin(chipSelect)) {
    Serial.println("Kort ej läsbart eller ej närvarande");
    //Stanna här
    return;
  }
  Serial.println("Kort initialiserat");
  Serial.println();
  
// Markering av en omstart i loggfilen  
File dataFile = SD.open("datalog.txt", FILE_WRITE); //Öppna eller skapa fil
if (dataFile) { //Om fil tillgänglig
  dataFile.print("//********************Ny loggning********************//");
  dataFile.println();
  dataFile.close();
}
//Så att if1MIN kör första rundan
paus = -60030;
rodled = HIGH;
} //void setup

//****************************************************************************************************************************************************************************************************************************************************************//

uint32_t old_ts; // Används av RTC biblioteks exemplet ?Hindra för att överbelasta med frekventa läsningar?

void loop() {
  // put your main code here, to run repeatedly:

//**************************[User input]**************************//
/*
  //Knocksensor
 val = analogRead(knockSensor);     
  if (val >= THRESHOLD) {
      //Debugg
      /*
      Serial.print("Knock! = ");
      Serial.println(val);
      
    if (belysn == 1){
      belysn = 0;
      }
      else{
        belysn = 1;
      }
  delay(100);
}
*/
if (digitalRead(knapp1) == HIGH) {
  if (belysn == 1) {
    belysn = 0; 
  }
  else {
    belysn = 1; 
  }
delay(350); //Dåligt sätt att hindra knappen från att dubbelslå (ÄNDRA!)  + skapar en delay mellan knapptrycking och händelse
}

//************[Kommunikations IO]********************************//
/* OBS! PAJ!
//Röd varningslysdiod = Fel på loggning
if (rodled == HIGH) {   //Blinkande röd led om det är problem med loggningen 
  if (sekblink == 1) {
    if (rodledPin == HIGH) {
      digitalWrite(rodledPin, LOW);
    }
    else {
      digitalWrite(rodledPin, HIGH);
      
    }
  }
}
*/

// LCD bakrundsbelysn

if (belysn == 1){ // Släcker resp. ränder bakrunds belysningen beroende på inst.
  lcd.backlight();
  }
  else{
    lcd.noBacklight();
    }

//***************[Klocka + sekblink (DISPLAY)]*******************//

if ((millis() - paus2) >= 1000) {   //En sekunds intervall

Serial.print(analogRead(3));
Serial.print(" | ");
Serial.println(digitalRead(7));

  DateTime now = rtc.now(); //hämta nuvarande datum och tid
    uint32_t ts = now.getEpoch();
  
  lcd.setCursor(0, 3);    //setCursor(plats, rad) 
  lcd.print("Tid:");
  if(now.hour() < 10) {
    lcd.setCursor(5, 3);
    lcd.print("0");
    lcd.setCursor(6, 3);
    lcd.print(now.hour());
  }
  else {
    lcd.setCursor(5, 3);
    lcd.print(now.hour());
  }
 if(now.minute() < 10) {
  lcd.setCursor(8, 3);
  lcd.print("0");
  lcd.setCursor(9, 3);
  lcd.print(now.minute());
 }
 else {
  lcd.setCursor(8, 3);
  lcd.print(now.minute());
 }

if(now.date() < 10) {
  lcd.setCursor(12, 3);
  lcd.print("0");
  lcd.setCursor(13, 3);
  lcd.print(now.date(), DEC);
}
    else {
      lcd.setCursor(12, 3);
      lcd.print(now.date(), DEC);
    }
lcd.setCursor(14, 3);
lcd.print("/");

if(now.month() < 10){
  lcd.setCursor(15, 3);
  lcd.print("0");
  lcd.setCursor(16, 3);
  lcd.print(now.month(), DEC);
}
    else{
      lcd.setCursor(15, 3);
      lcd.print(now.month(), DEC);
    }
lcd.setCursor(17, 3);
lcd.print("-");
lcd.setCursor(18, 3);
lcd.print(now.year() % 100);

  
  if (sekblink == 1) {   //Sekblink
    lcd.setCursor(7, 3); //Blinkande ":"
    lcd.print(":");
    sekblink = 0;
  }
    else {
      lcd.setCursor(7, 3); //Blinkande ":"
      lcd.print(" ");
      sekblink = 1;
    }
paus2 = millis();
}

//****************[Temp/Fukt/Tryck - sensor (+loggning)]*************************************//

if ((millis() - paus) >= 60000)  {   // Mät en gång per minut. Denna if funktion kommer referas till som "if1MIN"
  
  DateTime now = rtc.now(); // Hämta nuvarande datum och tid
  uint32_t ts = now.getEpoch();
   //Osäker på varför jag lagt till (float) här nedan:
  temp = (float)(mySensor.readTempC());            // Temperatur måste läsas först (°C)
  tryck = (float)(mySensor.readFloatPressure());   // Mät tryck (Pa)
  fuktighet = (float)(mySensor.readFloatHumidity()); // Mät Relativ luftfuktighet (%)
  
  rtc.convertTemperature();
  //Serial.println(rtc.getTemperature(), 2); -------------------------------------------------------
  
  ljus = analogRead(fotocellPin);     // Fotodiod

    // Loggning till sd

    File dataFile = SD.open("datalog.txt", FILE_WRITE); // Notera att endast en fil kan vara öppen samtidigt
                                                        // Så du man måste stänga denna fil innan du öppnar en annan.
    if (dataFile) {         // Om datafilen är tillgänglig, öppna den. 
      
    //rodled = LOW;   //Se till att varningslysdioden sloknar.
    
    // Datum och tid
    dataFile.print(now.year(), DEC);
    dataFile.print("/");
    if (now.month() < 10) {
      dataFile.print("0");
      dataFile.print(now.month(), DEC);
    }
    else {
      dataFile.print(now.month(), DEC);
    }
    dataFile.print("/");
    if (now.date() < 10) {
      dataFile.print("0");
      dataFile.print(now.date(), DEC);
    }
    else {
      dataFile.print(now.date(), DEC);
    }
    dataFile.print(" "); // Övergång från datum till tid
    if (now.hour() < 10) {
      dataFile.print("0");
      dataFile.print(now.hour(), DEC);
    }
    else {
      dataFile.print(now.hour(), DEC);
    }
    dataFile.print(":");
    if (now.minute() < 10) {
      dataFile.print("0");
      dataFile.print(now.minute(), DEC);
    }
    else {
      dataFile.print(now.minute(), DEC);
    }
    dataFile.print(",");
    
    // BME 280 sensor värden
    dataFile.print(temp, 2);
    dataFile.print(",");
    dataFile.print(tryck, 2);
    dataFile.print(",");
    dataFile.print(fuktighet, 2);
    dataFile.print(",");
    // Fotodiod värden
    dataFile.print(ljus);
    
    dataFile.println(); // Skapa en ny rad
    dataFile.close(); // Stäng filen
  }
  // Om det är något som är fel (minneskortet är inte tillgängligt)
  else {
    Serial.println("error opening datalog.txt");
    rodled = HIGH;
    paus3 = millis();
  }
  
//*******************[Display mätvärden]********************************//

//Rad-titlarna
lcd.setCursor(0, 0);
lcd.print("Temperatur:");
lcd.setCursor(0, 1);
lcd.print("Relativ LF:");
lcd.setCursor(0, 2);
lcd.print("Tryck:");

//Mätvärnerna
//Temperatur i grader C
lcd.setCursor(12, 0);
lcd.print(temp, 2);
lcd.setCursor(17, 0);
lcd.write(byte(0));
lcd.setCursor(18, 0);
lcd.print("C");

//Relativ fuktighet i %
lcd.setCursor(12, 1);
lcd.print(fuktighet, 2);
lcd.setCursor(17, 1);
lcd.print("%");

//Tryck i Pa
lcd.setCursor(7, 2);
lcd.print(tryck, 1);
lcd.setCursor(15, 2);
lcd.print("Pa");

//*******************************************************************//

paus = millis();
  
} //if1MIN





} //void_loop

void setup_display() {
 lcd.begin(20,4);   // Konfigurerar display som 20 tecken x 4 rader samt slår på bakrundsljus
 delay(50);         // Säkerställa att displayen har startat (borde funka utan, men för säkerhets skull)
 
 lcd.createChar(0, grader);  // Skapar karaktären "grader" 
 
 for(int i = 0; i< 2; i++)  // Blinkar 2 gånger med skärmen som "User input" om en reset/uppladdning
  {
    lcd.backlight();
    delay(250);
    lcd.noBacklight();
    delay(250);
  }
  lcd.backlight(); // Avslutar med bakrundsbelysning påslagen
 delay(100);
}



