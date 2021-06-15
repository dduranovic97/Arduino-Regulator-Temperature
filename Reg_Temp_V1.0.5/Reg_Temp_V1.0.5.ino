//VERZIJA 1.0.5
//BLINKANJE
//POTENCOMETAR ZA ODREDIT TEMP
// BEZ MEMOMIRANIH TEMP
//UZIMANJE UZORKA TEMP SVAKIH 5 SEC
//rješio problem sa Ostajanjem slova nakon ispisivanja 
//Custom character se vise ne corrupta nakon istekavanja napajanja 
//program napisan bez ijednog delay();

#include <LiquidCrystal_I2C.h>
#include <Wire.h>

#define PR1 A2 //POTENCIOMETAR
#define TPIN A0 //PIN ZA TEMP
#define RELEJ 7 //RELEJ PIN
#define MAXTEMP 50 //MAX TEMP DO KOJE SE MOZE POSTAVIT DA GRIJALICA GRIJE -ZAŠTITA 
#define MINTEMP 0 //MINIMALNA TEMP DO KOJE SE MOZE ODREDIT
#define Blink_interval  1000  //vrijeme blinkanja znakica dok je upaljena grijalica u ms
#define Sample_interval 10000 // uzima uzorak temperature odredjen intervalom

 //deklaracija I2C pinova
LiquidCrystal_I2C lcd(0x27,16,2); 

int Uiz;//cita Uiz sa A0
float Uul = 1023.0; // Ulazni napon od 5V
float R = 10000; // otpor trimera
float logRt, Rt, T, Tc, Tf;
float A = 1.009249522e-03, B = 2.378405444e-04, C = 2.019202697e-07;//Sa interneta konstante za NTC 10K (Stein-Hart jedn.)
int setT; //Temperatura koja je podešena 
unsigned long previousMillis = 0; //Millis od blinkanja
unsigned long previousMillisT = 0;//Millis od uzimanja uzorka temp 
unsigned long currentMillis;
unsigned long currentMillisT;
bool BlankOnOff = false;
String blnk = "";

//----CUSTOM CHARACTER-------
byte heaterChar[] = {
  B01001,
  B10010,
  B10010,
  B01001,
  B01001,
  B10010,
  B00000,
  B11111
};
 //------PRETVARANJE OTPORA U TEMP-------------
float Thermistor(int Uiz) {
  
  Rt = R * (Uul / (float)Uiz - 1.0); //naponsko djelilo
  logRt = log(Rt);
  T = (1.0 / (A + B*logRt + C*logRt*logRt*logRt)); //Stein-Hart jedn.
  Tc = T - 273.15; //Temp u *C
  //Tf = (Tc * 9.0)/ 5.0 + 32.0; //Temp u Farenheitima
 return Tc; //vrača vrijednost u *C ako se želi u F samo promjenit u Tf 
}
//-------BLINKANJE TEXTA-----------
void BlinkText(byte *msgtxt, int col, int row) {
  currentMillis = millis();

  if(currentMillis - previousMillis > Blink_interval)   {
    lcd.setCursor(col, row);
    previousMillis = currentMillis;
    
    if(BlankOnOff){
      lcd.write(msgtxt);
    }
    else {
      //for loop creates a String of spaces same length as word specified
      for(int i = 1; i <= strlen(msgtxt); i++) {
        blnk += " ";
      }
      lcd.print(blnk);
      blnk="";  //need to reset this back to blank otherwise it'll keep adding spaces to it on next iteration.
    }
    BlankOnOff = !BlankOnOff;
  }
}
//-----------------------BRISANJE SUVISNOG TEXTA---------------------------
//briše suvišni text koji se javlja kada broj predje iz dvoznamenkastog u jednoznamenkasti
void clearExcessText(){                 
     if (setT < 10){
        lcd.setCursor(13,1);
        lcd.print("   ");
    }

     if (Thermistor(Uiz) < 10){
        lcd.setCursor(12,0);
        lcd.print("   ");
    }
}

//--------------SETUP----------------------------------------------
void setup() 
{
pinMode(RELEJ,OUTPUT);
  
lcd.begin();
lcd.backlight();//Pali backlight
//lcd.noBacklight();// To Power OFF the back light

lcd.createChar(0, heaterChar); //stvara custom char, obavezno da bude iza lcd.begin(); inace ce bit kompromiram
lcd.setCursor(0,0); //stavlja display u DDRAM radi createChar(); (mogu biti lcd.clear() i lcd.home())

Uiz = analogRead(TPIN); //odmah uzima temperaturu jer inace je "delay" prvih 5 sec nakon paljenja 
}
//---------------LOOP-------------------------------------------------
void loop() 
{
currentMillis = millis();
currentMillisT = millis();

if(currentMillisT - previousMillisT > Sample_interval){ //UZIMA UZORAK TEMPERATURE ODREDJEN INTERVALOM
Uiz = analogRead(TPIN);
previousMillisT = currentMillisT;
}
 
//postavljanje željene temp sa potenciometrom
setT = analogRead(PR1); //potenciometar spojen na A2
setT = map(setT, 0, 1023, MINTEMP, MAXTEMP); //podjeli korak potenciometra od min do max temp

lcd.setCursor(0,0); 
lcd.print("TEMP: ");
lcd.print(Thermistor(Uiz)); //ispisuje trenutnu temp
lcd.print((char)223);//stupanj
lcd.print("C");
lcd.setCursor(0,1);
lcd.print("SET TEMP: ");
lcd.print(setT); //ispisuje željenu temp
lcd.print((char)223);//stupanj
lcd.print("C");

clearExcessText(); //cisti suvisna slova

//--------------REGULACIJA------------------------
if (Thermistor(Uiz) < setT){
  digitalWrite(RELEJ, HIGH);
  BlinkText(byte(0), 15, 0);
}

 else{
   digitalWrite(RELEJ, LOW); 
   lcd.setCursor(15,0);
   lcd.print(" ");
}

//END OF LOOP
}
