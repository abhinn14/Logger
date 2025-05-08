#include <EmonLib.h>
#include <SoftwareSerial.h>
#include <DHT.h>
#include <SD.h>

// Pins
#define DHTPIN1        7
#define DHTPIN2        5
#define DHTTYPE        DHT22
#define SD_CS          10
#define VOLT_SENSOR    A2
#define VIB_PIN        A0
#define NUM_SAMPLES    1000

// Objects
SoftwareSerial BTSerial(9, 6);
EnergyMonitor emon1;
DHT dht1(DHTPIN1, DHTTYPE);
DHT dht2(DHTPIN2, DHTTYPE);

void setup() {
  Serial.begin(9600);
  BTSerial.begin(9600);

  emon1.current(A5, 29);
  dht1.begin();
  dht2.begin();

  Serial.print("Initializing SD...");
  if (!SD.begin(SD_CS)) {
    Serial.println(" failed!"); 
    while (true);
  }
  Serial.println(" OK");
}

float readACVoltage() {
  long sum = 0;
  for (int i = 0; i < NUM_SAMPLES; i++) {
    int v = analogRead(VOLT_SENSOR) - 512;
    sum += (long)v * v;
  }
  float rms = sqrt(sum / (float)NUM_SAMPLES);
  return rms * (5.0 / 1024.0) * 100.0;
}

void loop() {
  // 1. Read sensors
  double       Irms  = emon1.calcIrms(1480);
  float        hum1  = dht1.readHumidity();
  float        t1    = dht1.readTemperature();
  float        hum2  = dht2.readHumidity();
  float        t2    = dht2.readTemperature();
  float        Vrms  = readACVoltage();
  int          vib   = analogRead(VIB_PIN);

  // 2. Send via Bluetooth
  BTSerial.print(Irms,2);  BTSerial.print(' ');
  BTSerial.print(Vrms,2);  BTSerial.print(' ');
  BTSerial.print(hum1,2);  BTSerial.print(' ');
  BTSerial.print(t1,2);    BTSerial.print(' ');
  BTSerial.print(hum2,2);  BTSerial.print(' ');
  BTSerial.print(t2,2);    BTSerial.print(' ');
  BTSerial.println(vib);

  // 3. Ensure CSV exists (with header) before appending
  if (!SD.exists("DataLog.csv")) {
    File hdr = SD.open("DataLog.csv", FILE_WRITE);
    if (hdr) {
      hdr.println(
        "Timestamp(ms),Current(A),Voltage(V),"
        "Humidity1(%),Temp1(C),Humidity2(%),Temp2(C),Vibration"
      );
      hdr.close();
      Serial.println(">> Created DataLog.csv");
    } else {
      Serial.println("!! Header creation failed");
    }
  }

  // 4. Append this sample
  File dataFile = SD.open("Datalog.csv", FILE_WRITE);
  if(dataFile) {
    dataFile.print(Irms,2);   dataFile.print(',');
    dataFile.print(Vrms,2);   dataFile.print(',');
    dataFile.print(hum1,2);   dataFile.print(',');
    dataFile.print(t1,2);     dataFile.print(',');
    dataFile.print(hum2,2);   dataFile.print(',');
    dataFile.print(t2,2);     dataFile.print(',');
    dataFile.println(vib);
    dataFile.close();
    Serial.println("Logged");
  } else {
    Serial.println("!! Write failed");
  }

  delay(1000);
}
