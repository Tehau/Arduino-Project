
//Projet Réseau TSING Tehau & DUBOIS Nicolas

#include <XBee.h>
#include <NewSoftSerial.h>
#include <string.h>
//LCD
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

XBee xbee = XBee();

unsigned long start = millis();

uint8_t option = 0;
uint8_t* data_response = NULL;
uint8_t payload[] = { 0, 0 };
// 16-bit addressing: Enter address of remote XBee, typically the coordinator
Tx16Request tx = Tx16Request(0x1874, payload, sizeof(payload));
TxStatusResponse txStatus = TxStatusResponse();

AtCommandRequest atRequest = AtCommandRequest();
AtCommandResponse atResponse = AtCommandResponse();
// create reusable response objects for responses we expect to handle 
Rx16Response rx16 = Rx16Response();
Rx64Response rx64 = Rx64Response();

//Structure de donnee des differents capteur Arduino
struct CycleArduino{
  String adresseMac;
  int active;
  int inactive;
};

int sizeStruct = 0;
const int maxStruct = 7;
CycleArduino CycleGlobal;
//Tableau de structure
struct CycleArduino tabStruct[maxStruct];
//Data SP#M0x40922053#A3#I8#
String cycle="SP#M0x406FB3E9#A5#I6#";
String Data="SEND#M0x406FB3E9#N2#DDUBOIS";
//Boolean for active
boolean isActive = true;
boolean Initialisation = true;
//Compteur
long cpt = 0;
long cpt_active = 0;
long cpt_init = 0;

LiquidCrystal_I2C lcd(0x20,16,2);  // set the LCD address to 0x20 for a 16 chars and 2 line display

void printLCD(int theLine, const char* theInfo, boolean clearFirst=true) {
        if (clearFirst)
              lcd.clear();
        lcd.setCursor(0,theLine);
        lcd.print(theInfo);
}

//to run once
void setup() {
  
  xbee.begin(38400);
  
  Serial.begin(38400);
  Serial.println("Arduino. Will send packets.");    

  lcd.init();
  lcd.backlight();
  lcd.home();
  
  start = millis();
  Serial.println("***************** INITIALISATION *****************");
}

//TODO LIST
 /*
 à faire des ack au lieu du broadcast
 xbee.flush() -> retire le cache dans le messages lors du sommeil //Done
 voir le protocole de synchronisation
 
 envoyez un send sur une adresse Mac (UNE SEULE) et attendre de recevoir le Ack de l'adresse mac
 
 à modifier le sendData_C pour le foutre en monocast
 
 faire une fonction pour les infos
 
 //SR Schedule Request
 Si on recoit un message SR
   Envoie le cycle du noeud en broadcast
 
 */

void loop() {
 
  if(Initialisation){
    InitMode();
    if(millis() - start > 20000)
      Initialisation = false;
  }
  else{
    if(isActive)
      ActiveMode();
    else{
      InactiveMode();
      Serial.flush(); //Flush
    }
  }
  //ParserInfo("SEND#M4076205E#N4#DDUBOIS");
}



//Ne pas oublier de changer les variables à mettre en accord avec la structure
/*
SP#M_#A_#I_ -> OK

ACK#_

SEND#_

SR#_

SA#_
*/
void ParserInfo(String data){
  String typeMessage, active, inactive, adresse;
  CycleArduino cycleA;
  
  int i=0;
  do{
    i++;
  }while( i < data.length() && data.substring(i,i+1) != "#");
  typeMessage = data.substring(0,i); //On recupere le type de Message jusqu'au #
  
  //Serial.println("Data recu : " + data);  
  if(typeMessage == "SP" && Initialisation){
    Serial.println("********ParserInfo TypeMessage : SP********");
    TraitementSP(data);
  }
  else if(typeMessage =="ACK"){
    Serial.println("********ParserInfo TypeMessage : ACK********");
  }
  else if(typeMessage =="SEND"){
    Serial.println("********ParserInfo TypeMessage : SEND********");
    TraitementSEND(data);
  }
  else if(typeMessage =="SR"){
    Serial.println("********ParserInfo TypeMessage : SR********");
  }
  else if(typeMessage =="SA"){
    Serial.println("********ParserInfo TypeMessage : SA********");
  }
  else{
    //Serial.println("Erreur : Aucun type de message correspondant.");
  }
}

/*** Traitement des paquets de type SP ***/
//SP#M0x40922053#A3#I8#
void TraitementSP(String data){
  String adressemac;
  int active, inactive, indPrecedente;
  int i=3;
  Serial.println("");
  Serial.println("***** Traitement Paquet type SP *****");
  //Recuperation de l'adresse Mac
  do{
    i++;
  }while( i < data.length() && data.substring(i,i+1) != "#");
  
  if(data.substring(3,4) == "M")
    adressemac = data.substring(4,i);
  else
    Serial.println("Erreur : Paquet non conforme.");
  //Affichage adresseMac
  /*
  Serial.println("Adresse Mac : " + adressemac);
  */
  indPrecedente = i;
  do{
    i++;
  }while( i < data.length() && data.substring(i,i+1) != "#");
  if(data.substring(indPrecedente+1,indPrecedente+2) == "A")
    active = data.substring(indPrecedente+2,i).toInt();
  else
    Serial.println("Erreur : Paquet non conforme");
  //Affichage temps actif
  /*
  Serial.println("Active Time : " + active);
  */
  indPrecedente = i;
  do{
    i++;
  }while( i < data.length() && data.substring(i,i+1) != "#");
  //Serial.println("Data : ");
  //Serial.print(data.substring(indPrecedente,i));
  if(data.substring(indPrecedente+1,indPrecedente+2) == "I")
    inactive = data.substring(indPrecedente+2,i).toInt();
  else
    Serial.println("Erreur : Paquet non conforme");
  //Affichage temps actif
  /*
  Serial.println("Inactive Time : " +inactive);
  */
  //On test si le tableau n'est pas remplie
  boolean isPresent = RechercheMac(adressemac);
  if (sizeStruct < maxStruct && !isPresent )
  {
    tabStruct[sizeStruct].adresseMac = adressemac;
    tabStruct[sizeStruct].active = active;
    tabStruct[sizeStruct].inactive = inactive;
    Serial.println("Arduino bien rajoute | Mac : " + tabStruct[sizeStruct].adresseMac + " | Active : " + tabStruct[sizeStruct].active +" | Inactive : " +tabStruct[sizeStruct].inactive);
    sizeStruct += 1;
  }
  else
  {
    if (sizeStruct == maxStruct)
    {
    Serial.println("Trop d'Arduino");
    }
    if (isPresent)
    {
    Serial.println("Arduino deja present");
    }
  }
}

//SEND#M_#N_#D_#
//ACK#M_
//Retourner type paquet : "ACK#N_#
void TraitementSEND(String data){
  String adressemac, numero, donnee;
  int i = 5; //On commence a partir de l'indice du 1er #
  int indPrecedente;
  
  Serial.println("");
  Serial.println("***** Traitement Paquet type SEND *****");
  
  //Recuperation de l'adresse Mac
  do{
    i++;
  }while( i < data.length() && data.substring(i,i+1) != "#");
  
  if(data.substring(5,6) == "M")
    adressemac = data.substring(6,i);
  else
    Serial.println("Erreur : Paquet non conforme.");
  
  indPrecedente = i;
  do{
    i++;
  }while( i < data.length() && data.substring(i,i+1) != "#");
  if(data.substring(indPrecedente+1,indPrecedente+2) == "N")
    numero = data.substring(indPrecedente+2,i);
  else
    Serial.println("Erreur : Paquet non conforme");
  
  indPrecedente = i;
  do{
    i++;
  }while( i < data.length() && data.substring(i,i+1) != "#");
  //Serial.println("Data : ");
  //Serial.print(data.substring(indPrecedente,i));
  if(data.substring(indPrecedente+1,indPrecedente+2) == "D")
    donnee = data.substring(indPrecedente+2,i);
  else
    Serial.println("Erreur : Paquet non conforme");
  
  Serial.println("Adresse Mac : "+adressemac+" Numero message : "+numero+" Donnee message : "+donnee);
  
  Serial.println("");
  
}


/*** Transformation des paquets recu de type uint_8 en String ***/
String ParseByteToString()
{
  String string;
  for (int i=0; i<rx16.getDataLength(); i++) 
  {
    string += (char)data_response[i];
    //Serial.print((char)data_response[i]);
    //Serial.print(string[i]);
  }
  return (string);
}

/*** Recherche adresse Mac dans notre tableau de structure ***/
boolean RechercheMac(String mac)
{
  int i =0;
  boolean reponse = false;
  while (i<sizeStruct && reponse != true)
  {
    if (mac == tabStruct[i].adresseMac)
    {
      reponse=true;
    }
    i++;
  }
  return(reponse);
  
}

/*** Les differents mode de notre cycle ***/

void InitMode(){
  if(millis() - cpt_init > 1000) {
    cpt_init = millis();
    printLCD(0,"Mode Init",true);
    sendData_B(cycle);
    receiveData();
    ParserInfo(ParseByteToString());
    //Serial.println("");
    //Serial.println("Init Mode");
 }
}

void InactiveMode(){
   if (millis() - cpt_active > 6000) {
     cpt_active = millis();
     isActive = true;
     printLCD(0,"Mode Inactive",true);
     Serial.println("");
     Serial.print("isActive : " + isActive);
     Serial.println(isActive);
   }
   receiveData();
}

void ActiveMode(){
  if (millis() - cpt_active > 5000) {
     cpt_active = millis();
     isActive = false;
     printLCD(0,"Mode Active",true);
     Serial.println("");
     Serial.print("isActive : " + isActive);
     //Serial.println(isActive);
  }
  receiveData();
  sendData_B(Data);
}
 
/*** Send Data ***/

void sendData_B(String data){
   if (millis() - cpt > 1000) {
     cpt = millis();
     // 64-bit addressing: This is the SH + SL address of remote XBee
     XBeeAddress64 addr64 = XBeeAddress64(0x00000000, 0x0000FFFF);
     // unless you have MY on the receiving radio set to FFFF, this will be received as a RX16 packet
     
     for (int i; i<data.length(); i++)
         payload[i]=(uint8_t)data.charAt(i);
     
     // in this way, we know the exact size of the payload
     Tx64Request tx = Tx64Request(addr64, payload, data.length());
     Serial.println("");
     Serial.print("Data send");
     char charBuf[data.length()+1];
     data.toCharArray(charBuf, data.length()+1);
     
     //printLCD(0,"Data send", true);
     printLCD(1,charBuf,false);
     
     //Send data
     xbee.send(tx);
    }
}

void sendData_C(String data,String mac){
    
   if (millis() - cpt > 1000) {
     cpt = millis();
     // 64-bit addressing: This is the SH + SL address of remote XBee
     XBeeAddress64 addr64 = XBeeAddress64(0x0013A200, mac.toInt());
     // unless you have MY on the receiving radio set to FFFF, this will be received as a RX16 packet
      
     for (int i; i<data.length(); i++)
         payload[i]=(uint8_t)data.charAt(i);
     
     // in this way, we know the exact size of the payload
     Tx64Request tx = Tx64Request(addr64, payload, data.length());
     Serial.println("");
     Serial.print("Data send : " + data);
     Serial.println("");
     char charBuf[data.length()+1];
     data.toCharArray(charBuf, data.length()+1);
     
     //printLCD(0,"Data send", true);
     printLCD(1,charBuf,false);
     
     //Send data
     xbee.send(tx);
    }
}

//reads packets, looking for RX16 or RX64
void receiveData(){
    xbee.readPacket();
    
    if (xbee.getResponse().isAvailable()) {
    // got something
      if (xbee.getResponse().getApiId() == RX_16_RESPONSE || xbee.getResponse().getApiId() == RX_64_RESPONSE) {
        // got a rx packet
        printLCD(0,"Data received", true);
        
        if (xbee.getResponse().getApiId() == RX_16_RESPONSE) {
          xbee.getResponse().getRx16Response(rx16);
          option = rx16.getOption();
          data_response = rx16.getData();
          
          Serial.println("");
          Serial.print("RECEIVED rx16 : ");
          for (int i=0; i<rx16.getDataLength(); i++) {
            Serial.print((char)data_response[i]);
	  }  
  	  Serial.println("");
          
        } else {
          xbee.getResponse().getRx64Response(rx64);
          option = rx64.getOption();
          data_response = rx64.getData();
          
          Serial.println("");
          Serial.print("RECEIVED rx64 : ");
          for (int i=0; i<rx64.getDataLength(); i++) {
            Serial.print((char)data_response[i]);
	  }  
  	  Serial.println("");
          
        }
         
      }
    }
}
 

    /*
    xbee.readPacket();
    // got a response!
    if (xbee.getResponse().isAvailable()) {
        // got something
	//if (xbee.getResponse().getApiId() == RX_64_RESPONSE) {
        if (xbee.getResponse().getApiId() == RX_16_RESPONSE) {
  	  Serial.println("RECEIVED");
          xbee.getResponse().getRx16Response(rx16);
	  //xbee.getResponse().getRx64Response(rx64);
	  //data_response = rx64.getData();
          String dataR = rx16.getData();
  	  printLCD(0,"Data received", true);
	  //printLCD(1,data_response.c_str(),false);
          //data_response.Length();
	  for (int i=0; i<rx64.getDataLength(); i++) {
            Serial.print((char)data_response[i]);
	  }  
  	  Serial.println("");	 
      }
   }
   */
