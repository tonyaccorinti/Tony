 /* -----------------------------------------------------------------------------------
 Accende e spegne un RELE a secondo dell'SMS inviato. Questa la lista comandi da SMS
 ON  => Accende il RELE della Caldaia
 OFF => Spegne il RELE della Caldaia                    
 STATO => Comunica stato della caldaia/rele
 TEMP => Comunica la temperatura in casa
 Altro => Messaggio di errore sul Serial Monitor
 Fuga di Gas => invio sms con allarme
 ---------------------------------------------------------------------------------- */
#include "SIM900.h"
#include <SoftwareSerial.h>                             // necessaria alla libreria SIM900.h richiamata in sms.h
#include "sms.h"                                        // Libreria per la gestione degli SMS
#include <cactus_io_DHT22.h>                            //Library for DHT22 Temperature sensor

SMSGSM sms;
boolean started = false;                                // GSM start state
int GSM_POW = 9;
int numdata;
int RCald = 13;                                         // Il Pin 13 è quello connesso al rele Caldaia
int RedLed = 6;                                         // Red led: GSM board is powered ON
int BlueLed = 5;                                        // Blue led: GSM board is READY
int GreenLed = 4;                                       // Green led: la caldaia è accesa
int CountLoop = 0;
int DelTime = 3;
char smsbuffer[60];
char Mittente[13];
char Mittente1[13];
float FloatTemp;                                        //Temperatura 
char CharTemp[10];                                      //temporarily holds data from vals 

#define DHT22Sens 2
DHT22 dht(DHT22Sens);

void setup(){
  pinMode(GSM_POW, OUTPUT);
  pinMode (RCald, OUTPUT);
  pinMode (RedLed, OUTPUT);
  pinMode (BlueLed, OUTPUT);
  pinMode (GreenLed, OUTPUT);

  digitalWrite(RCald, HIGH);                                        // spegne inizialmente il Rele Caldaia
  
  Serial.begin(9600);
  Serial.println("Debug Programma Gestione Caldaia");
  
  Serial.println("Switching ON GSM Module");
  SIM900power();                                                    //Sequenza di accensione GSM board
  
  strcpy(Mittente1,"3332148764");
  delay(4000);
  
//Inizializzo la connessione GSM impostando il baudrate. Per l'utilizzo di HTTP è opportuno usare 4800 baud o meno
  if (gsm.begin(9600)) {
     delay(15*1000);
     started = true;
     Serial.println("STATUS Modulo GSM = PRONTO");
      digitalWrite(RedLed, HIGH);
  }
  else
     Serial.println("STATUS Modulo GSM = INATTIVO");
  
  Serial.println("Primo Controllo Registrazione GSM");
//  SIM900CheckReg();
  SIM900IsReg();
  delay(5*1000);
  
  dht.begin();
  dht.readTemperature();                                                                           // inizializza sensore DHT22
  Serial.println("Stato Sensore Temperatura"); 
  Serial.println(dht.temperature_C);    
}
void SIM900power(){                                   // Power up GSM shield via software
  digitalWrite(GSM_POW, HIGH);
  delay(1100);
  digitalWrite(GSM_POW, LOW);  
  delay(5000);
  Serial.println("Modulo GPRS is Power on");          // messaggio su serial monitor
  delay(4000);
  }
/*void SIM900CheckReg(){
  int reg;
    reg = gsm.CheckRegistration();
    delay(30*1000);
    switch (reg){    
      case REG_NOT_REGISTERED:
        Serial.println("GSM module is not registered");
//        digitalWrite(BlueLed, LOW);
        break;
      case REG_REGISTERED:
        Serial.println("GSM module is registered");
        digitalWrite(BlueLed, HIGH);     
        break;
      case REG_NO_RESPONSE:
        Serial.println("GSM doesn't response");
//        digitalWrite(BlueLed, LOW);
        break;
      case REG_COMM_LINE_BUSY:
        Serial.println("comm line is not free");
//        digitalWrite(BlueLed, LOW);
        break;
    }
    delay(2000);
*/
void SIM900IsReg(){
   int reg=0;
   reg = gsm.IsRegistered();
    Serial.print("Registration ");
    Serial.println(reg);
    if (reg > 0) 
      digitalWrite(BlueLed, HIGH);
    delay(10*1000);
    }
    
void loop(){
  char position;
    if (started){
    CountLoop++;                                                  // contatore dei cicli loop
    Serial.print("Ciclo n.");
    Serial.println(CountLoop);                                    // stampa il numero dei cicli su seral monitor
        if (CountLoop == 10){                                     // Solo ogni n cicli ontrollo se la sim è connessa alla rete
//          Serial.println("Routine Controllo Registrazione GSM");
//          SIM900CheckReg();
          SIM900IsReg();
          CountLoop = 0;                                          // azzero il contatore
        }
/* Legge se ci sono messaggi disponibili sulla SIM Card e li visualizza sul Serial Monitor.*/
    position = sms.IsSMSPresent(SMS_UNREAD); // Valore da 1..20
      if (position){
      sms.GetSMS(position, Mittente, smsbuffer, 80);               // Leggo il messaggio SMS e stabilisco chi sia il mittente
//      Serial.print("Comando Ricevuto [tel. "+String(Mittente)+String("]: ") + String(smsbuffer));
      
        if (strcmp(smsbuffer,"Accendi Caldaia")==0){                                                      // Se riceve il messaggio Accendi Caldaia via sms
        digitalWrite(RCald, LOW);                                                                         // Accende il Rele Caldaia
        digitalWrite(GreenLed, HIGH);
//        Serial.println(" => Accendo la Caldaia");
        sms.SendSMS(Mittente, "Caldaia Accesa");
        }
      else if (strcmp(smsbuffer,"Spegni Caldaia")==0){                                                    // se riveve il messaggio Spegni Caldaia via sms
        digitalWrite(RCald, HIGH);                                                                        // Spengo il Rele Caldaia
        digitalWrite(GreenLed, LOW);
//        Serial.println(" => Spengo la caldaia");
        sms.SendSMS(Mittente, "Caldaia Spenta");
        }
      else if (strcmp(smsbuffer,"Temperatura")==0){                                                      // se riceve il messaggio Temperatura
        FloatTemp = (dht.temperature_C - 1.5);                                                           // Leggo la Temp in degC dal sensore e sottraggo 1 grado (riscaldamento interno)
        dtostrf(FloatTemp, 4, 2, CharTemp);                                                              // converti il valore da Float a Char
//        Serial.print("CharTemp: ");
        sms.SendSMS(Mittente, CharTemp);                                                                 // invia sms con valore temperatura - restituisce true se invia l'SMS
        delay(5000);
        }
      else if (strcmp(smsbuffer,"Stato Caldaia")==0){                                                             // altrimenti se riceve il messaggio STATO
        if (digitalRead(RCald)==LOW){                                                                     // se il rele è acceso (stato invertito..)
          sms.SendSMS(Mittente, "STATO:  Caldaia Accesa");                                                // invia sms con caldaia accesa - restituisce true se invia l'SMS
//          Serial.println(" => La Caldaia e' Accesa");
          delay(5000);
          }
        else {
          sms.SendSMS(Mittente, "STATO:  Caldaia Spenta");                                                //  invia sms con caldaia spenta - restituisce true se invia l'SMS
//          Serial.println(" => La Caldaia e' Spenta");
          delay(5000);
          }
        }
        else
          sms.SendSMS(Mittente, "Comando non riconosciuto");
        sms.DeleteSMS(position);                                                                            // Elimina l'SMS appena analizzato
    }
  }
delay(DelTime*1000);
}

