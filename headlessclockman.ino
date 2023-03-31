#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <ESP8266WiFi.h> 
#include <ESP8266WebServer.h> 
#include <ESP8266mDNS.h>
#include <time.h>
#include <WiFiClient.h> 
#include <WiFiUdp.h> 
#include <PolledTimeout.h> 
#include "header_defs.h"
unsigned int localPort = 2390; 
const char* ssid=STASSID; 
const char* password=STAPSK;
const char* ntpservername = "time.nist.gov"; 
const int NTP_PACKET_SIZE = 48; 
byte packetbuffer[NTP_PACKET_SIZE];
char* pchostdomain=0;
const char* timestring(void) {
  static char actimestring[32];
  time_t now = time(nullptr); 
  size_t stlength;
  ctime_r(&now, actimestring);
  while (((stlength = strlen(actimestring))) && ('\n' == actimestring[stlength - 1])) {
  actimestring[stlength -1] = 0;   
  }
return actimestring; 
}
bool bhostdomainconfirmed = false; 
bool sethostname(const char* pchostname) {
  if (pchostname) {
    WiFi.hostname(pchostname); 
  }
  return true; 
}
MDNSResponder::hMDNSService hMDNSService=0; 
ESP8266WebServer server(service_port); 
IPAddress timeserverip; 
WiFiUDP udp;
LiquidCrystal_I2C lcd(0x27, 16,2); // 0x3F on some weird chip-sets. 
void setup(void) {
 Serial.begin(115200); 
 #ifndef ESP8266
 while(!Serial); 
 #endif 
 for (int j=0; j<5; j++) { Serial.println("\n"); }
 lcd.init();
 lcd.backlight(); 
 for(int x=0; x<4; x++) {
 lcd.setCursor(0,0); 
 lcd.print("Programed By");
 lcd.setCursor(0,1); 
 lcd.print("Johnny B Stroud");
 delay(450); 
 lcd.clear();   
 }
 lcd.clear();
 lcd.setCursor(0,0); 
 lcd.print("Connecting WiFi..");
 lcd.setCursor(0,1); 
 lcd.print(WiFi.localIP().toString());   
 WiFi.mode(WIFI_STA); 
 WiFi.begin(ssid, password);
 Serial.print("Connecting to local WiFi.."); 
 while (WiFi.status() != WL_CONNECTED) {
  delay(500); 
  Serial.print("."); 
 }
 Serial.println(".Done"); 
 lcd.clear(); 
 lcd.setCursor(0,0); 
 lcd.print("WiFi Device ip:");
 lcd.setCursor(0,1); 
 lcd.print(WiFi.localIP().toString());  
 Serial.print("Connected to "); 
 Serial.println(ssid); 
 Serial.println("IP address:"+WiFi.localIP().toString()); 
 udp.begin(localPort);
 Serial.println("Local port: "+udp.localPort()); 
 delay(660);  
 lcd.clear(); 
 for (int x=0; x<16; x++) { 
 lcd.setCursor(0,0);
 lcd.print(WiFi.SSID());  
 lcd.setCursor(0,1); 
 lcd.print(WiFi.localIP().toString()); 
 delay(560); 
 } 
  for(int x=0; x<12; x++) {
 lcd.clear(); 
 lcd.setCursor(0,0); 
 lcd.print("WiFi device ip:");
 lcd.setCursor(0,1); 
 lcd.print(WiFi.localIP().toString());  
 delay(550); 
 }
 setClock(); 
 MDNS.setHostProbeResultCallback(hostproberesult); 
 if ((!MDNSResponder::indexDomain(pchostdomain, 0, "target")) || (!MDNS.begin(pchostdomain))) {
 Serial.println("\nERROR:setting up MDNS responder!");
 while (1) { delay(1000); Serial.print("."); }  
 }
 server.on("/", handleRequestroot); 
 server.on("/docs", handleRequest_docs);
 server.on("/info", handleRequest_info);  
 server.begin(); 
}

void loop(void) {
 WiFi.hostByName(ntpservername, timeserverip); 
 sendNTPpacket(timeserverip); 
 delay(1000);  
 int cb = udp.parsePacket(); 
 if (!cb) { Serial.println("Sending no packet yet recived!"); } else 
 {
  Serial.print("Packet received, length ");
  Serial.println(cb);  
  udp.read(packetbuffer,  NTP_PACKET_SIZE); 
  unsigned long highword = word(packetbuffer[40], packetbuffer[41]); 
  unsigned long lowword = word(packetbuffer[42],  packetbuffer[43]); 
  unsigned long secssince1900 = highword << 16 | lowword; 
  Serial.print("seconds since Jan 1 1900 ="); 
  Serial.print(secssince1900);
  const unsigned long JAN11970 = 2208988800UL; 
  unsigned long epoch = secssince1900-JAN11970; 
  Serial.println(epoch); 
  Serial.print("UTC TIME "); 
  Serial.print((epoch % 86400L) / 3600);
  Serial.print(":");  
  if(((epoch % 3600) / 60) < 10) { Serial.print('0'); }
  Serial.print((epoch % 3600) / 60);
  Serial.print("."); 
  if(((epoch % 3600) / 60) < 10) { Serial.print('0'); }
  Serial.println(epoch % 60);
 lcd.clear();     
 lcd.setCursor(0,0); 
 lcd.print("UTC TIME "); 
 lcd.print((epoch % 86400L) / 3600);
 lcd.print(":");  
  if(((epoch % 3600) / 60) < 10) { lcd.print('0'); }
 lcd.print((epoch % 3600) / 60);
 lcd.print("."); 
  if(((epoch % 3600) / 60) < 10) { lcd.print('0'); }
 lcd.println(epoch % 60);   
 lcd.setCursor(0,1); 
 lcd.print(timestring());  
 delay(8588); 
 }
 for(int x=0; x<25; x++) { 
 server.handleClient(); 
 MDNS.update(); 
 static esp8266::polledTimeout::periodicMs timeout(update_cycle); 
 if (timeout.expired()) {
  if(hMDNSService) {
    MDNS.announce(); 
  }
 }
 }
  time_t now=time(nullptr);
  Serial.println(timestring()); 
  struct tm timeinfo; 
  gmtime_r(&now, &timeinfo);

 lcd.clear(); 
 for (int x=0; x<16; x++) {
 server.handleClient(); 
 MDNS.update(); 
 lcd.setCursor(0,1); 
 lcd.print(timestring());  
 delay(760); 
 }

}

void setClock(void) {
configTime((timezone_offset * 3600), (dst_offset * 3600), 
"pool.ntp.org", "0.north-america.pool.ntp.org", "us.pool.ntp.org");
Serial.print("Connecting to NTP sync server..");
time_t now = time(nullptr); 
while (now < 8 * 3600 * 2) { 
  delay(500); Serial.print("."); 
  now = time(nullptr); 
}
Serial.println(".Done"); 
}

void MDNSDynamicCallback(const MDNSResponder::hMDNSService p_hservice) {
 if (hMDNSService == p_hservice) {
   MDNS.addDynamicServiceTxt(p_hservice, "curtime", timestring()); 
 }
}

void hostproberesult(String p_pcDomainName, bool p_bproberesult) {
Serial.println("MDNS Probe Result callback:"); 
Serial.printf("Host domain '%s.local' is %s\n", p_pcDomainName.c_str(), (p_bproberesult ? "FREE" : "USED")); 
if (true == p_bproberesult) {
  sethostname(pchostdomain);
  if (!bhostdomainconfirmed) {
    bhostdomainconfirmed = true;
    if(!hMDNSService) {
      hMDNSService = MDNS.addService(0, "espclk", "tcp", service_port);
      if (hMDNSService) {
        MDNS.addServiceTxt(hMDNSService, "port#", service_port); 
        MDNS.setDynamicServiceTxtCallback(MDNSDynamicCallback);
      }
    }
  }
} else {
  if (MDNSResponder::indexDomain(pchostdomain, "-", 0)) {
    MDNS.setHostname(pchostdomain);
  } else {
   Serial.println("MDNS probe result callback:"); 
   Serial.println("Failure to update hostname..");  
  }
}
}

void handleRequestroot() {
  time_t now = time(nullptr);;
  struct tm timeinfo; 
  gmtime_r(&now, &timeinfo); 
  String p;
  p = "<!DOCTYPE html>"; 
  p +="<html><head>";
  p +="<meta http-equiv='refresh' content='6'>";
  p +="<title>Headless Clock Man</title>";
  p +="<style> "; 
  p +="h1 { font-size: 40px; font-color:red; color:red;}"; 
  p +="p { font-color: white; color:white; }"; 
  p +="body { font-size:15px; font-color:black; background-color:black; }";
  p +="</style>";
  p +="</head><body>";   
  p +="<h1><font size='17px' color='white'>";
  p +=timestring(); 
  p +="</h1></font>";
  p +="<p>";
  p +="<font color='yellow'>";
  p +="Local domain -</font><font color='white'> http://"+WiFi.hostname()+".local</font><br/>";
  p +="<font color='yellow'>IP address of server:</font><font color='white'>"+WiFi.localIP().toString()+"<br/>";
  p +="</font></p>"; 
  p +="</body></html>"; 
  Serial.println("Sending 200 html handle request..");
  server.send(200, "text/html", p); 
 
}

void handleRequest_info() {
  time_t now = time(nullptr);;
  struct tm timeinfo; 
  gmtime_r(&now, &timeinfo); 
  String p;
  p +=timestring(); 
    String ssid;
  int32_t rssi;
  uint8_t encryptionType;
  uint8_t* bssid;
  int32_t channel;
  bool hidden;
  int scanResult;
  scanResult = WiFi.scanNetworks(false, true);
  p +="\nLocal domain - http://"+WiFi.hostname()+".local";
  p +="\nIP address of server:"+WiFi.localIP().toString()+"";
  p +="\n\nScanning networks ESSIDS found:";

  if (scanResult == 0) {
    Serial.println(F("No networks found"));
    } else if (scanResult > 0) {
    Serial.printf(PSTR("%d networks found:\n"), scanResult);
    // Print unsorted scan results
    for (int8_t i = 0; i < scanResult; i++) {
      WiFi.getNetworkInfo(i, ssid, encryptionType, rssi, bssid, channel, hidden);

      Serial.printf(PSTR("  %02d: [CH %02d] [%02X:%02X:%02X:%02X:%02X:%02X] %ddBm %c %c %s\n"),
                    i,
                    channel,
                    bssid[0], bssid[1], bssid[2],
                    bssid[3], bssid[4], bssid[5],
                    rssi,
                    (encryptionType == ENC_TYPE_NONE) ? ' ' : '*',
                    hidden ? 'H' : 'V',
                    ssid.c_str());
       p += "\n";
       p += ssid.c_str();             
      delay(0);
    }
  } else {
    Serial.printf(PSTR("WiFi scan error %d"), scanResult);
    p += printf(PSTR("WiFi scan error %d"), scanResult); 
  }

  server.send(200, "text", p); 
 
}

void handleRequest_docs() {
  String p;
  p = "Headless server clockman \n";
  p += "Server is running on A D1 mini ESP8266EX style microcontroller.\n"; 
  p += "The D1 Mini is a breadboard-compatible WiFi development board based on the ESP8266.\n";
  p += " the ESP8266 is a system-on-a-chip (SOC) that has a TCP/IP stack built in. \n";
  p += "It’s sort taking a Wifi controller and squeezing out the extra pins and processing power as a pretty ";
  p += "capable microcontroller.\n";
  p += "\n the ESP8266 has a 80MHz 32-bit processor, with power to spare to drive other functions besides Wifi.\n";
  p += "It offers 16 GPIO pins, 64kB instruction RAM and 96kB of data RAM besides the 4MB external flash.\n";
  p += "\n";  
  server.send(200, "text", p); 
}

void sendNTPpacket(IPAddress& address) {
 Serial.println("now sending NTP packet..."); 
 memset(packetbuffer, 0, NTP_PACKET_SIZE); 
 packetbuffer[0]=0b11100011;
 packetbuffer[1]=0;
 packetbuffer[2]=6;
 packetbuffer[3]=0xEC;
 packetbuffer[12]=49;
 packetbuffer[13]=0x4E;
 packetbuffer[14]=49;
 packetbuffer[15]=52;
 udp.beginPacket(address, 123);  
 udp.write(packetbuffer, NTP_PACKET_SIZE); 
 udp.endPacket();  
}
