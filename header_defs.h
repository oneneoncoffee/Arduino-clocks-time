/*
 *  Defines global vars and defined data
 */

#define firmware_var "1.3 Beta"    //  Current firmware version of our program.
#define timezone_offset -7        // NTP pool server timezone and dst offset information. 
#define dst_offset +1
#define update_cycle (1*1000)     /
#define service_port 80
#ifndef STASSID 
#define STASSID "wifi-network-name"         //Edit this for your home use ESSID network name.
#define STAPSK "WiFi-netowrk-password"     // And your ESSID's password here. 
#endif 
