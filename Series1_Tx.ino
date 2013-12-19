
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
//Data SP#M0x40762059#A3#I8#
String cycle="SP#M0x40762059#A5#I6#";
String Data="SEND#M0x40762059#C2#DDUBOIS";
int MyLevel = -1;
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
 

/******* Structure de donnee *******/
/*
SP#M_#A_#I_ -> OK

ACK#_

SEND#_

SR#_

SA#_

SETUP#<NIVEAU_SENDER>#

SEND#M<MAC_SENDER>#C<Id_Mess>#D<Mess>#

SEND_ADD#N<No_ANNEAU_SENDER#M<Mac_Sender>#C<Id_Mess>#D<Mess>#
*/
/***
**  J'AI RETIRE L'INACTIVE MODE POUR LE TEST DE SAUT 
***/
void loop() {
 
  if(Initialisation){
    InitMode();
    if(millis() - start > 00000)
      Initialisation = false;
  }
  else{
    /*
    if(isActive)
      ActiveMode();
    else{
      InactiveMode();
      Serial.flush(); //Flush
    }
    */
     ActiveMode();
  }
  //ParserInfo("SEND#M4076205E#N4#DDUBOIS");
  //ParserInfo("SETUP#1#");
}

/*** Fonction qui recoit une donnee et la traite ***/
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
  else if(typeMessage =="SETUP"){
    Serial.println("********ParserInfo TypeMessage : SETUP********");
    TraitementSETUP(data);
  }
  else if(typeMessage =="SEND_ANN"){
    Serial.println("********ParserInfo TypeMessage : SEND_ANN********");
    TraitementSEND_ANN(data);
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

//SEND#M_#C_#D_#
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
  if(data.substring(indPrecedente+1,indPrecedente+2) == "C")
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
  
  String data_ack;
  
  /*
  Faire un retour paquet ACK vers le arduino
  */
  
  Serial.println("");
  
}

//SETUP#M1#
void TraitementSETUP(String data){
  int ReceiveLvl = data.substring(6,7).toInt();
  String data_setup;
  String tempLvl;
  
  if((MyLevel < ReceiveLvl) || (MyLevel != -1))
  {
     MyLevel = ReceiveLvl+1;
     Serial.print("MyLevel :#");
     Serial.print(MyLevel);
     Serial.println("");
     tempLvl = String(MyLevel);
     data_setup = "SETUP#N"+tempLvl+"#";
     
     Serial.print("data_setup :");
     Serial.print(data_setup);
     Serial.println("");
     
     char myStg[10];
     sprintf(myStg, "MyLevel : %d", MyLevel);
     //printLCD(0,"",true);
     printLCD(1,myStg,false);
     
     /** Envoyez des messages de type SETUP durant le temps d'inactivete Max qu'on possede **/
     int tMax = RechercheMaxActivite();
     int cpt_Setup = millis();
     //while(millis() - cpt_active > 3){
      sendData_B(data_setup);
     //}
      
  }
  else
  {
    Serial.print("MyLevel est superieur à ReceiveLvl");
    Serial.println("");  
  }
  
}

void TraitementSEND_ANN(String data){
  
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
/******************************************************* Recherche temps d'inactivité max dans la structure de données et renvoie ce max *******************************************/
int RechercheMaxActivite(){
  int i = 0;
  int max = 0;
  while (i<sizeStruct)
  {
    if (max < tabStruct[i].inactive)
    {
      max = tabStruct[i].inactive;
    }
    i++;
  }
  Serial.print("Le temps d'inactivite max dans notre structure est :");
  Serial.println(max);
  return max;
}
/*** 
****Les differents mode de notre cycle 
***/

/*** Init Mode ***/
void InitMode(){
  if(millis() - cpt_init > 1000) {
    cpt_init = millis();
    printLCD(0,"Mode Init",true);
    sendData_B(cycle);
    receiveData();
    ParserInfo(ParseByteToString());
    Serial.println("");
    Serial.println("Init Mode");
 }
}
/*** Inactive Mode ***/
void InactiveMode(){
   if (millis() - cpt_active > 6000) { //6000
     //printLCD(0,"Mode Inactive",true);
     Serial.println("");
     Serial.print("isInactive : ");
     Serial.println(isActive);
     cpt_active = millis();
     isActive = true;
   }
   receiveData();
}

/*** Active Mode ***/
void ActiveMode(){
  //if (millis() - cpt_active > 5000) { //5000
     //printLCD(0,"Mode Active",false);
     Serial.println("");
     Serial.print("isActive : ");
     Serial.println(isActive);
     cpt_active = millis();
     isActive = false;
  //}
  receiveData();
  ParserInfo(ParseByteToString());
  //sendData_B(Data);
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
     //printLCD(1,charBuf,false);
     
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
     //printLCD(1,charBuf,false);
     
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
      //printLCD(0,"Data received", true);
      //printLCD(1,(const char*)data_response,true);
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
