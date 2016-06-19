#include <SPI.h>
#include <Ethernet.h>
#include <EEPROM.h>

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network.
// gateway and subnet are optional:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192, 168, 178, 120);
IPAddress gateway(192, 168, 178, 1);
IPAddress subnet(255, 255, 255, 0);

char linebuffer[80];
byte lp;
#define eepromtempaddr 10
#define eepromdurationaddr 20
#define heizungspin 2
#define pumpenpin 3

// telnet defaults to port 23
EthernetServer server(23);
boolean alreadyConnected = false; // whether or not the client was connected previously

void heatup(int solltemperatur, EthernetClient client)
{
  int lookup[] = {1260, 1226, 1192, 1158, 1124, 1090, 1056, 1022, 988, 954, 920, 896, 872, 848, 824, 800, 776, 752, 728, 704, 680};

  float ntctmp = lookup[solltemperatur - 80] / 1000.0;
  int temperature = 255;
  float res = 10.0 + ntctmp;
  float volts = (5 / res) * ntctmp;
  int heateddigits = volts / (5.0 / 1023.0);
//  Serial.print("Solltemperatur digits: ");
//  Serial.print(heateddigits, DEC);

  while (temperature > heateddigits) {
    temperature = analogRead(A0);
    client.print("temperature: ");
    client.println(temperature, DEC);
    delay(500);
  }
}


void switchheatoff()
{
  Serial.println("Heizung AUS");
  digitalWrite(heizungspin, HIGH);
}

void switchheaton()
{
  Serial.println("Heizung AN");
  digitalWrite(heizungspin, LOW);
}

void switchpumpoff()
{
  Serial.println("Pumpe AUS");
  digitalWrite(pumpenpin, HIGH);
}

void switchpumpon()
{
  Serial.println("Pumpe AN");
  digitalWrite(pumpenpin, LOW);
}

void setup() {
  // initialize the ethernet device
  Ethernet.begin(mac, ip, gateway, subnet);
  // start listening for clients
  server.begin();
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for Leonardo only
  }
  pinMode(heizungspin, OUTPUT);
  pinMode(pumpenpin, OUTPUT);
  digitalWrite(heizungspin, HIGH);
  digitalWrite(pumpenpin, HIGH);

  Serial.print("cofe server address:");
  Serial.println(Ethernet.localIP());
  memset(linebuffer, '\0', 80);
  lp = 0;
}
int i = 0;
int temp = 0;
int duration = 0;

void loop() {
  // wait for a new client:
  EthernetClient client = server.available();

  // when the client sends the first byte, say hello:
  if (client) {
    if (!alreadyConnected) {
      // clead out the input buffer:
      client.flush();
      Serial.println("We have a new client");
      client.println("Hello, client!");
      temp = EEPROM.read(eepromtempaddr);
      duration = EEPROM.read(eepromdurationaddr);

      char carr[5];
      itoa(duration, carr, 10);
      client.print("duration: ");
      client.println(carr);
      itoa(temp, carr, 10);
      client.print("temperature: ");
      client.println(carr);

      alreadyConnected = true;
    }

    if (client.available() > 0) {
      // read the bytes incoming from the client:
      linebuffer[lp] = client.read();
      if ((linebuffer[lp] == 0x20) && (lp == 0)) {
        linebuffer[lp] = client.read();
      }
      if ((linebuffer[lp] == '\r') || (linebuffer[lp] == '\n')) {
        linebuffer[lp] = '\0';
        if (lp > 1) {

          if (strncmp(linebuffer, "set", 3) == 0) {
            if ( strncmp(&linebuffer[4], "temperature", strlen("temperature")) == 0) {
              char * eqptr = &linebuffer[4 + strlen("temperature")];
              while ((*eqptr != '=') && (*eqptr != '\0')) {
                eqptr++;
              }
              if (*eqptr != '\0') {
                eqptr++;
                temp = atoi(eqptr);
                EEPROM.write(eepromtempaddr, temp);

              }
              else {
                client.println("missing argument/=");
              }
            } else if (strncmp(&linebuffer[4], "duration", strlen("duration")) == 0) {
              char * eqptr = &linebuffer[4 + strlen("duration")];
              while ((*eqptr != '=') && (*eqptr != '\0')) {
                eqptr++;
              }
              if (*eqptr != '\0') {
                eqptr++;
                duration = atoi(eqptr);
                EEPROM.write(eepromdurationaddr, duration);

              }
              else {
                client.println("missing argument/=");
              }
            }
          }
          else if (strncmp(linebuffer, "get", 3) == 0) {
            if ( strncmp(&linebuffer[4], "temperature", strlen("temperature")) == 0) {
              char carr[5];
              itoa(temp, carr, 10);
              client.print("temperature is set to: ");
              client.println(carr);

            } else if (strncmp(&linebuffer[4], "duration", strlen("duration")) == 0) {
              char carr[5];
              itoa(duration, carr, 10);
              client.print("duration is set to: ");
              client.println(carr);
            }
          }
          else if ((strncmp(linebuffer, "cafe", 4) == 0)|| (strncmp(linebuffer, "cofe", 4) == 0)) {
            client.println("cafe machen:");
            if ((temp >= 80) and (temp < 100)) {
              switchheaton();
              heatup(temp, client);
              switchheatoff();
              switchpumpon();
              delay(duration * 1000);
              switchpumpoff();
            } else {
              client.println("temperatur ausserhalb des erlaubten Bereiches");
            }
          } else {
            client.print("hab da was kann aber nix mit anfangen:");
            client.println(linebuffer);
          }

          lp = 0;
        }
      } else if (((linebuffer[lp] >= 0x30) && (linebuffer[lp] <= 0x7e)) || (linebuffer[lp] == 0x20)) {
        lp++;
      }
    }
  }
}

