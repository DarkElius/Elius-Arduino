// Activate debug on serial port
// #define __DEBUG_SERIAL

// If defined use WiFi (Arduino nano 33 IOT), otherwise Ethernet (Arduino + Ethernet Shield)
#define __USE_WIFI

#include <SPI.h>
#include <OSCMessage.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "ArduinoLowPower.h"

#ifdef __USE_WIFI
  #include <WiFiNINA.h>
  #include <WiFiUdp.h>
#else
  #include <Ethernet.h>
  #include <EthernetUdp.h>
#endif

#ifdef __USE_WIFI
  // SSID network name
  char ssid[] = "<put your SSID>";
  // Network password
  char pass[] = "<put your wifi password>";
  
  // WiFi status
  int status = WL_IDLE_STATUS;
  
  // Initialize the Wifi client
  WiFiSSLClient client;
  
  // WiFi UDP
  WiFiUDP Udp;
#else
  // Network
  EthernetUDP Udp;
  byte mac[] = { 0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x02 };
#endif


// Buttons
const int buttonResume = 2;
const int buttonVolUp = 3;
const int buttonVolDown = 4;
const int buttonMute = 5;
const int buttonSceneUp = 6;
const int buttonSceneDown = 7;


// Delay
const int delayButton = 200;

// Mute
int mute = 0;

//Idle
const int idleTimeout = 5000;
int idleTimer = 0;
volatile boolean resumeLoop = true;

// Display
#define OLED_RESET -1
Adafruit_SSD1306 display(OLED_RESET);

const String displayTitle = "X32 Remote";

// Mixer
IPAddress outIp(192, 168, 0, 78);
const unsigned int outPort = 10023;

// Volume
const int volumeMax = 100;
const int volumeMin = 0;
// X32 DCA = 80 steps
float volumeSteps = 0.0125;
int volume = 10;

// Previous status
int prevStatusMute = LOW;
int prevStatusSceneUp = LOW;
int prevStatusSceneDown = LOW;

// Scene
const int sceneMin = 0;
const int sceneMax = 5;
int scene = sceneMin;
// You can set a description for one or more of the first scenes
String sceneDesc[] = {"Cassette", "SACD", "Cinema"};
// Default
//String sceneDesc[] = {""};


// Utilty to convert ip to string
String ipToString(const IPAddress& address) {
  return String() + address[0] + "." + address[1] + "." + address[2] + "." + address[3];
}


/**
 * Initial setup
 */
void setup() {
#ifdef __DEBUG_SERIAL  
  // Open serial
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
#endif

  // Configure buttons
  pinMode(buttonResume, INPUT_PULLUP);
  pinMode(buttonVolUp, INPUT);
  pinMode(buttonVolDown, INPUT);
  pinMode(buttonMute, INPUT);
  pinMode(buttonSceneUp, INPUT);
  pinMode(buttonSceneDown, INPUT);


  //Init display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

#ifdef __USE_WIFI
  // Try to connect to Wifi network
  while ( status != WL_CONNECTED) {
    logInfo("Attempting to connect to SSID: ", false);
    logInfo(ssid);
    // Connect
    status = WiFi.begin(ssid, pass);
    // Wait
    delay(1000);
  }
#else
  // Configure DHCP
  while (Ethernet.begin(mac) == 0) {
    logInfo("Failed to configure Ethernet using DHCP");
    // Wait
    delay(1000);
  }
#endif

  logInfo("Connected");

  // Debug your local IP address:
  logInfo("Local IP address: ", false);
#ifdef __USE_WIFI 
  logInfo(ipToString(WiFi.localIP()));
#else
  logInfo(ipToString(Ethernet.localIP()));
#endif

  // Configure UDP Port
  Udp.begin(outPort);

  // Set default volume
  setVolume();

  // Set idle timer
  idleTimer = millis();

  // Attach a wakeup interrupt 
  LowPower.attachInterruptWakeup(digitalPinToInterrupt(buttonResume), resumeFromIdle, RISING);  
  
  // Debug messages
  logInfo("Setup completed");

}


/**
 * Program main loop
 */ 
void loop() {
  // Set idle after timeout
  if ((millis() - idleTimer) >= idleTimeout) {
    logInfo("Going to idle...");
    // Reset timer
    idleTimer = millis();
    
    // Stop loop
    resumeLoop = false;

    // Power off display
    display.ssd1306_command(SSD1306_DISPLAYOFF);

    // Set idle
    LowPower.deepSleep();
  }

  while(!resumeLoop);
    
  

  // Button volume up pressed
  if (HIGH == digitalRead(buttonVolUp)) {
    // Manage volume
    if (volume < volumeMax) {
      volume++;
      setVolume();
    }
    // Delay
    delay(delayButton);
  }

  // Button volume down pressed
  if (HIGH == digitalRead(buttonVolDown)) {
    if (volume > volumeMin) {
      volume--;
      setVolume();
    }
    // Delay
    delay(delayButton);
  }

  // Button mute pressed
  if ((LOW == prevStatusMute) && (HIGH == digitalRead(buttonMute))) {
    // Set new  button status
    prevStatusMute = HIGH;
    // Set mute
    setMute();
    // Delay
    delay(delayButton);
  } else if ((HIGH == prevStatusMute) && (LOW == digitalRead(buttonMute))) {
    // Reset previous status
    prevStatusMute = LOW;
  }

  // Button scene up pressed
  if ((LOW == prevStatusSceneUp) && (HIGH == digitalRead(buttonSceneUp))) {
    // Set new  button status
    prevStatusSceneUp = HIGH;
    // Get next
    if (scene < sceneMax) {
      scene++;
      // Set scene
      setScene();
    }
    // Delay
    delay(delayButton);
  } else if ((HIGH == prevStatusSceneUp) && (LOW == digitalRead(buttonSceneUp))) {
    // Reset previous status
    prevStatusSceneUp = LOW;
  }

  // Button scene down pressed
  if ((LOW == prevStatusSceneDown) && (HIGH == digitalRead(buttonSceneDown))) {
    // Set new  button status
    prevStatusSceneDown = HIGH;
    // Get prev
    if (scene > sceneMin) {
      scene--;
      // Set scene
      setScene();
    }
    // Delay
    delay(delayButton);
  } else if ((HIGH == prevStatusSceneDown) && (LOW == digitalRead(buttonSceneDown))) {
    // Reset previous status
    prevStatusSceneDown = LOW;
  }

}


/**
 * Resume routine
 */
void resumeFromIdle() {
  // Resume loop
  resumeLoop = true;
  // Reset timeout
  idleTimer = millis();
  // Power on display
  display.ssd1306_command(SSD1306_DISPLAYON);
}


/**
 * Set volume
 */
void setVolume() {
  // Reset timeout
  idleTimer = millis();
  
  // Set message
  OSCMessage msg("/dca/1/fader");
  // Set value
  msg.add(float(volume) * volumeSteps);

  // Set packet
  Udp.beginPacket(outIp, outPort);
  // Send packet
  msg.send(Udp);
  // End packet
  Udp.endPacket();
  // Free
  msg.empty();

  // Refresh display
  refreshDisplay();

  // Debug messages
  logInfo("Set volume to: ", false);
  logInfo(String(volume));
}


/**
 * Set mute
 */
void setMute() {
  // Reset timeout
  idleTimer = millis();
  
  // Set message
  OSCMessage msg("/dca/1/on");
  // Set value
  msg.add(mute);
  // Revert mute
  mute = 1 % mute;

  // Set packet
  Udp.beginPacket(outIp, outPort);
  // Send packet
  msg.send(Udp);
  // End packet
  Udp.endPacket();
  // Free
  msg.empty();

  // Refresh display
  refreshDisplay();

  // Debug messages
  logInfo("Set Mute");
}


/**
 * Set scene
 */
void setScene() {
  // Reset timeout
  idleTimer = millis();
    
  // Set message
  OSCMessage msg("/-action/goscene");
  // Set value
  msg.add(scene);

  // Set packet
  Udp.beginPacket(outIp, outPort);
  // Send packet
  msg.send(Udp);
  // End packet
  Udp.endPacket();
  // Free
  msg.empty();

  // Set the current volume to the selected scene
  setVolume();

  // Refresh display
  refreshDisplay();

  // Debug messages
  logInfo("Set scene to: ", false);
  logInfo(String(scene));
}


/**
 * Refresh display info
 */
void refreshDisplay() {
  // Set params
  display.setTextSize(1);
  display.setTextColor(WHITE);

  // Clear display
  display.clearDisplay();

  // Write title
  display.setCursor(centerString(displayTitle), 0);
  display.println(displayTitle);

  // Write scene
  String buf = getSceneDesc();
  display.setCursor(centerString(getSceneDesc()), 8);
  display.println(buf);

  // Write Volume
  display.setCursor(centerString("Volume: 00"), 24);
  display.print("Volume: ");
  display.println(volume);

  // Write Mute
  if (1 == mute) {
    display.setCursor(centerString("MUTE"), 16);
    display.print("MUTE");
  }

  // Display
  display.display();
}


/**
 * Get the starting x to center text
 */
int centerString(String text) {
  int16_t  x1, y1;
  uint16_t w, h;

  // Get text boundaries
  display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);

  // Return the x where start
  return ((124 - w) / 2);
}


/**
 * Get scene description if available
 */
String getSceneDesc() {
  // Check if there is a description
  if (scene < (int)sizeof(sceneDesc) / sizeof(sceneDesc[0])) {
    // Skip blanks descriptions
    if (!sceneDesc[scene].equals("")) {
      return sceneDesc[scene];
    }
  }
  // Return default
  return "Scene: " + String(scene);
}


/**
 * Print log messages
 */
void logInfo(String text) {
  return logInfo(text, true);
}


/**
 * Print log messages
 */
void logInfo(String text, boolean nl) {
#ifdef __DEBUG_SERIAL
  // New line
  if(nl)
    Serial.println(text);
  else
    Serial.print(text);    
#endif  
}
