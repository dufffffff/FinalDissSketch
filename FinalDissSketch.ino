// Includes libraries
#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>


#define RST_PIN         D0           // Defines pins used by the microcontroller to communicate with the MRFC522 Module
#define SS_PIN          D8 




MFRC522 mfrc522(SS_PIN, RST_PIN);



/*Specifying the SSID and Password of the AP*/
 
const char* ap_ssid = "Cloner"; //Access Point SSID
const char* ap_password= "12345678"; //Access Point Password
uint8_t max_connections=1;//Maximum Connection Limit for AP
int current_stations=0, new_stations=0;
 
//Specifying the Webserver instance to connect with HTTP Port: 80
ESP8266WebServer server(80);
 
 
//Specifying the boolean variables indicating the Attack Modes.
bool read_status=false, clone_status=false;

//Defines the variables needed to manipulate the UIDs
String nout = "";
String str = "";
bool written;



void setup() {

  
  //Start the serial communication channel
  Serial.begin(115200);
  Serial.println();
  while (!Serial);   // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  delay(4);       // Optional delay. Some board do need more time after init to be ready, see Readme
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));

 
   
  //Setting the AP Mode with SSID, Password, and Max Connection Limit
  if(WiFi.softAP(ap_ssid,ap_password,1,false,max_connections)==true)
  {
    Serial.print("Access Point is Created with SSID: ");
    Serial.println(ap_ssid);
    Serial.print("Max Connections Allowed: ");
    Serial.println(max_connections);
    Serial.print("Access Point IP: ");
    Serial.println(WiFi.softAPIP());
  }
  else
  {
    Serial.println("Unable to Create Access Point");
  }
 
  //Specifying the functions which will be executed upon corresponding GET request from the client
  server.on("/",handle_OnConnect);
  server.on("/readon",handle_readon);
  server.on("/readoff",handle_readoff);
  server.on("/cloneon",handle_cloneon);
  server.on("/cloneoff",handle_cloneoff);
  server.onNotFound(handle_NotFound);
   
  //Starting the Server
  server.begin();
  Serial.println("HTTP Server Started");
}




void loop() {
  //Assign the server to handle the clients
  server.handleClient();
     
  //Continuously check how many stations are connected to Soft AP and notify whenever a new station is connected or disconnected
  new_stations=WiFi.softAPgetStationNum();
   
  if(current_stations<new_stations)//Device is Connected
  {
    current_stations=new_stations;
    Serial.print("New Device Connected to SoftAP... Total Connections: ");
    Serial.println(current_stations);
  }
   
  if(current_stations>new_stations)//Device is Disconnected
  {
    current_stations=new_stations;
    Serial.print("Device disconnected from SoftAP... Total Connections: ");
    Serial.println(current_stations);
  }
 
  //Turn the Attack Modes ON/OFF as per their status set by the connected client
   
  //Read Mode
  if(read_status==false)
  {
    //Serial.println("read mode off");
  }
  else
  {
    //Serial.println("read mode on");
  
    readUID();
  }
 

 
  //Clone Mode
  if(clone_status==false)
  {
    //Serial.println("clone mode off");
  }
  else
  { 
    cloneUID();
   // Serial.println("clone mode on");
  }

 
}

String byte_array_engine(byte *buffer, byte bufferSize) {
 str = "";
  for (byte i = 0; i < bufferSize; i++) {
    str = str + ((((buffer[i] & 0xF0) >> 4) <= 9) ? (char)(((buffer[i] & 0xF0) >> 4) + '0') : (char)(((buffer[i] & 0xF0) >> 4) + 'A' - 10));
    str = str + (((buffer[i] & 0x0F) <= 9) ? (char)((buffer[i] & 0x0F) + '0') : (char)((buffer[i] & 0x0F) + 'A' - 10));
  }
  Serial.println(str);           //For debugging
  return str;
}



void hexCharacterStringToBytes(byte *byteArray, const char *hexString)  //This code takes an array of characters and turns it to an array of hex bytes
{
  bool oddLength = strlen(hexString) & 1;

  byte currentByte = 0;
  byte byteIndex = 0;

  for (byte charIndex = 0; charIndex < strlen(hexString); charIndex++)
  {
    bool oddCharIndex = charIndex & 1;

    if (oddLength)
    {
      // If the length is odd
      if (oddCharIndex)
      {
        // odd characters go in high nibble
        currentByte = nibble(hexString[charIndex]) << 4;
      }
      else
      {
        // Even characters go into low nibble
        currentByte |= nibble(hexString[charIndex]);
        byteArray[byteIndex++] = currentByte;
        currentByte = 0;
      }
    }
    else
    {
      // If the length is even
      if (!oddCharIndex)
      {
        // Odd characters go into the high nibble
        currentByte = nibble(hexString[charIndex]) << 4;
      }
      else
      {
        // Odd characters go into low nibble
        currentByte |= nibble(hexString[charIndex]);
        byteArray[byteIndex++] = currentByte;
        currentByte = 0;
      }
    }
  }
}


byte nibble(char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';

  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;

  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;

  return 0;  // Not a valid hexadecimal character
}

void readUID(){ //This function reads the card
  
  //Serial.println("Insert card...");
  // Look for new cards
    if ( ! mfrc522.PICC_IsNewCardPresent() || ! mfrc522.PICC_ReadCardSerial() ) {
    return;
  }

    // Show details of the card
    //Serial.print(F("UID:"));
    byte_array_engine(mfrc522.uid.uidByte, mfrc522.uid.size);
  
   mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
    
}  

bool cloneUID(){//This function takes the UID read by the readCard() function and writes it to another card.

 
  byte uidArray[4]={ 0 };
  
  char buf[16];
  str.toCharArray(buf, 16);
  
  hexCharacterStringToBytes(uidArray, buf);
  //otherDumpByteArray(uidArray , 4);
  
//  Serial.println("Size of uid array:");
//  for (int i = 0; i < 4; i++) Serial.println(uidArray[i], HEX); //Debugging code
//    Serial.println(sizeof(uidArray));


    
 if ( ! mfrc522.PICC_IsNewCardPresent() || ! mfrc522.PICC_ReadCardSerial() ) {
    delay(50);
    return written;
    
  }

  if ( mfrc522.MIFARE_SetUid(uidArray, (byte)4, true) ) {
    Serial.println(F("Wrote new UID to card."));
    return written = true;
  } else { 
    return written = false;
  }
  
  // Halt PICC and re-select it so DumpToSerial doesn't get confused
  mfrc522.PICC_HaltA();
  if ( ! mfrc522.PICC_IsNewCardPresent() || ! mfrc522.PICC_ReadCardSerial() ) {
   return written;
    
  }
  

  return written;
  delay(2000);
}




void handle_OnConnect()
{
  Serial.println("Client Connected");
  server.send(200, "text/html", HTML(nout)); 
}
 
void handle_readon()
{
  Serial.println("Read mode: ON");

  clone_status=false;
  read_status=true;
  
  server.send(200, "text/html", HTML(str));
}
 
void handle_readoff()
{
  Serial.println("Read mode: OFF");
  read_status=false;
  server.send(200, "text/html", HTML(nout));
}
 
 
void handle_cloneon()
{
  Serial.println("Clone mode: ON");
  clone_status=true;
  read_status=false;
  server.send(200, "text/html", HTML(str));
}
 
void handle_cloneoff()
{
  Serial.println("Clone mode: OFF");
  clone_status=false;
  server.send(200, "text/html", HTML(nout));
}
 
 
void handle_NotFound()
{
  server.send(404, "text/plain", "Not found");
}
 
String HTML(String disp)
{
  String msg="<!DOCTYPE html> <html>\n";
  msg+="<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0, user-scalable=no\">\n";
  msg+="<title>Cloner Control</title>\n";
  msg+="<style>html{font-family:Helvetica; display:inline-block; margin:0px auto; text-align:center;}\n";
  msg+="body{margin-top: 50px;} h1{color: #444444; margin: 50px auto 30px;} h3{color:#444444; margin-bottom: 50px;}\n";
  msg+=".button1{display:block; width:80px; background-color:#f48100; border:none; color:white; padding: 13px 30px; text-decoration:none; font-size:25px; margin: 0px auto 35px; cursor:pointer; border-radius:4px;}\n";
  msg+=".button1-on{background-color:#37E842;}\n";
  msg+=".button1-on:active{background-color:#37E842;}\n";
  msg+=".button1-off{background-color:#E7300F;}\n";
  msg+=".button1-off:active{background-color:#E7300F;}\n";
  msg+="</style>\n";
  msg+="</head>\n";
  msg+="<body>\n";
  msg+="<h1>RFID Cloner Web Server</h1>\n";
  msg+="<h3> Access Point Active</h3>\n";
  msg+="<p>Console</p>\n";
   

  if(read_status==false)
  {

    msg+="<p>Read Mode: OFF</p><a class=\"button1 button1-on\" href=\"/readon\">ACTIVE</a>\n";    
  }
  else
  {
    msg+="<p style=\"color:White; background-color:Black;\">";  
    msg+=  "Read UID :" + disp;    //displays UID on console
    msg+= "</p>\n"; 
   
    msg+="<p>Read Mode: ACTIVE</p><a class=\"button1 button1-off\" href=\"/readoff\">OFF</a>\n";
    msg+="<a class=\"button1 button-on\" href=\"/readon\">refresh</a>\n";
  }
 

 
  if(clone_status==false)
  {
 
    msg+="<p>Clone Mode: OFF</p><a class=\"button1 button1-on\" href=\"/cloneon\">ACTIVE</a>\n";
        
  }
  else
  {
    msg+="<p style=\"color:White; background-color:Black;\">";  
    if (written == true){
      msg+=  "Written UID: " + disp;    //displays UID on console
    }
    else
    { msg+= "Unable to write UID";
    }
    msg+= "</p>\n"; 
    msg+="<p>Clone Mode: ACTIVE</p><a class=\"button1 button1-off\" href=\"/cloneoff\">OFF</a>\n";
    msg+="<a class=\"button1 button-on\" href=\"/cloneon\">Refresh</a>\n";
  }
 
  
  
  msg+="</body>\n";
  msg+="</html>\n";
  return msg;
}
