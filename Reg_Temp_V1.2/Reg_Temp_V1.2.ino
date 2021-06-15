/* * * * * * * * * * * * * * * * * * */
/*      Temperature Regulator V 1.2. */
/*      coded by Daniel Đuranović    */
/*             28.3.2020.            */
/* * * * * * * * * * * * * * * * * * */

#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <IRremote.h>
#include <EEPROM.h>

#define RECV_PIN 5 /*DATA PIN ZA RECIEVER*/
#define TPIN A0 /*PIN ZA TEMP*/
#define RELEJ 7 /*RELEJ PIN*/
#define MAXTEMP 50 /*MAX TEMP DO KOJE SE MOZE POSTAVIT DA GRIJALICA GRIJE -ZAŠTITA*/ 
#define MINTEMP 0 /*MINIMALNA TEMP DO KOJE SE MOZE ODREDIT*/
#define Blink_interval  1000  /*vrijeme blinkanja znakica dok je upaljena grijalica u ms*/
#define Sample_interval 1000 /* uzima uzorak temperature odredjen intervalom*/

/*Remote button codes*/
#define POWER 0X87C
#define OPTIONS 0X84D
#define TEMPUP 0X850
#define TEMPDOWN 0X851

LiquidCrystal_I2C lcd(0x27,16,2);  /*deklaracija I2C pinova*/

IRrecv irrecv(RECV_PIN);
decode_results results;
unsigned long key_value = 0;

int Uiz;
float Uul = 1023.0; /* Ulazni napon od 5V*/
float R = 10000; /*Otpor na otpornom djelilu*/
float logRt, Rt, T, Tc, Tf;
float A = 1.009249522e-03, B = 2.378405444e-04, C = 2.019202697e-07; /*konstante za NTC 10K (Stein-Hart jedn.)*/ 
unsigned long previousMillis;  /*Millis od blinkanja*/
unsigned long previousMillisT; /*Millis od uzimanja uzorka temp*/ 
unsigned long previousMillisP; /*Millis od OFF timera*/
bool BlankOnOff = false;
String blnk = "";

extern int setT = 25; 
extern int OFFtimer = 30;
bool OFFtimerState = false;
bool tempValueReachedFT = false; /*Temp reached first time*/
bool powerStatus = LOW;

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

 /*R --> Temp*/
extern int Thermistor(int Uiz) {
  
  Rt = R * (Uul / (float)Uiz - 1.0); /*naponsko djelilo*/
  logRt = log(Rt);
  T = (1.0 / (A + B*logRt + C*logRt*logRt*logRt)); /*Stein-Hart jedn.*/
  Tc = T - 273.15; //Temp u *C
  //Tf = (Tc * 9.0)/ 5.0 + 32.0; /*Temp u Farenheitima*/
 return Tc; /*vrača vrijednost u *C ako se želi u F samo promjenit u Tf */
}

/*Blinkanje texta*/
void BlinkText(byte *msgtxt, int col, int row) {

  if(millis() - previousMillis > Blink_interval)   {
    lcd.setCursor(col, row);
    previousMillis = millis();
    
    if(BlankOnOff){
      lcd.write(msgtxt);
    }
    else {
      /*for loop creates a String of spaces same length as word specified
*/      for(int i = 1; i <= strlen(msgtxt); i++) {
        blnk += " ";
      }
      lcd.print(blnk);
      blnk="";  /*need to reset this back to blank otherwise it'll keep adding spaces to it on next iteration*/
    }
    BlankOnOff = !BlankOnOff;
  }
}
/*Brisanje suvisnog texta-briše suvišni text koji se javlja kada broj predje iz dvoznamenkastog u jednoznamenkasti*/
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
/*Check value*/
extern int checkValue(int value, int minimum, int maximum){
     if (value < minimum){
         value = minimum;
     }
     else if (value > maximum){
         value = maximum;
     }
     else{
     value = value;
     }
     return value;
}

/*Definiranje menu-a*/
class Menu {

   public:

      int menu_selection = 0;

      void select_menu(){
         switch(menu_selection){
            case 0:
               main_menu();
               break;
            case 1:
               setTimer_menu();
               break;
            default:
               menu_selection = 0 ;
               break;
         }
      }
       
      void main_menu(){
         lcd.setCursor(0,0); 
         lcd.print("TEMP: ");
         lcd.print(Thermistor(Uiz)); /*ispisuje trenutnu temp*/
         lcd.print((char)223); /*stupanj*/
         lcd.print("C");
         lcd.setCursor(0,1);
         lcd.print("SET TEMP: ");
         lcd.print(setT); /*ispisuje željenu temp*/
         lcd.print((char)223); /*stupanj*/
         lcd.print("C");

         clearExcessText(); 
       }

      void setTimer_menu(){
         lcd.setCursor(0,0);
         lcd.print("Set");
         lcd.setCursor(0,1);
         lcd.print("Off Timer: ");
         lcd.print(OFFtimer);
         lcd.print("min");

         if(OFFtimer < 10){
            lcd.setCursor(15,1);
            lcd.print(" ");
         }
      } 

};

Menu disp;

class Remote {
   
    public:

    int UPDval = 0;

    void scan(){
      if (irrecv.decode(&results)){
 
        if (results.value == 0XFFFFFFFF)
          results.value = key_value;

        switch(results.value){
          case TEMPUP:
              UPDval += 1;
              break;
          case TEMPDOWN:
              UPDval -= 1;
              break; 
          case OPTIONS:
              lcd.clear();
              disp.menu_selection += 1;
              break;
          case POWER:
              powerStatus -= 1;
              checkValue(powerStatus,0,1);
              break;   
        }
        key_value = results.value;
        irrecv.resume(); 
       }

    }
    
    /*izbor varijable koja se podesava s obzirom na prikazani menu*/
    void check_menu_state(){
       switch (disp.menu_selection){
         case 0:
            UPDval = setT;
            scan();
            setT = UPDval;
            setT = checkValue(setT,MINTEMP,MAXTEMP);
            EEPROM.write(1, setT);
            break;
         case 1:
            UPDval = OFFtimer;
            scan();
            OFFtimer = UPDval;
            OFFtimer = checkValue(OFFtimer,5,60);
            EEPROM.write(0, OFFtimer);
            previousMillisP = millis();
            break;
       
       }
    }
      
};

Remote remote;

/*****************************************SETUP**********************************************/

void setup() {

Serial.begin(9600);

pinMode(RELEJ,OUTPUT);
  
setT = EEPROM.read(1);
setT = checkValue(setT,MINTEMP,setT);

OFFtimer = EEPROM.read(0);
OFFtimer = checkValue(OFFtimer, 5,60);
OFFtimerState = false; 

powerStatus = LOW;

lcd.begin();
lcd.createChar(0, heaterChar); /*stvara custom char, obavezno da bude iza lcd.begin(); inace ce bit kompromiram*/
lcd.setCursor(0,0);           /*stavlja display u DDRAM radi createChar(); (mogu biti lcd.clear() i lcd.home())*/
                          
irrecv.enableIRIn();
irrecv.blink13(true);

Uiz = analogRead(TPIN); /*odmah uzima temperaturu jer inace je "delay" prvih 5 sec nakon paljenja */
}

/*********************************************************LOOP********************************************************/

void loop(){

if(powerStatus == HIGH){
     lcd.backlight();
   
     if(millis() - previousMillisT > Sample_interval){ 
     Uiz = analogRead(TPIN);
     previousMillisT = millis();
     }  

/************************************REGULATION**************************************************/

     if (Thermistor(Uiz) < setT){
           OFFtimerState = false;
           
           if(tempValueReachedFT == false){
                previousMillisP = millis(); /*Mora se dodati radi toga da se resetira odbrojavanje ako temp nije dostignuta prvi puta*/
           }
           
           digitalWrite(RELEJ, HIGH);
           BlinkText(byte(0), 15, 0);
           }

      else{                                                                                           
          OFFtimerState = true;
          tempValueReachedFT = true;
          
          /*Serial.println(millis() - previousMillisP); /*--> Test*/
          if((OFFtimerState == true) && ((millis() - previousMillisP) > (OFFtimer*60000))){
               powerStatus = LOW;
               previousMillisP = millis();
          } 
       
          digitalWrite(RELEJ, LOW); 
          lcd.setCursor(15,0);
          lcd.print(" ");
      }
      
     disp.select_menu();
}

else{
    OFFtimerState = false;
    tempValueReachedFT = false;
    previousMillisP = millis(); /*Mora se dodati radi toga da se resetira odbrojavanje ako padne temp ispod zeljenje, inace nastavlja otkuda je stao*/
    disp.menu_selection = 0;
          
    digitalWrite(RELEJ, LOW);
    lcd.clear();
    lcd.noBacklight();
  }

remote.check_menu_state();

/*end of loop*/
}
