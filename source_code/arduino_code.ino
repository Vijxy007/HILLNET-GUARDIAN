#include <Wire.h>
#include <MPU6050.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

MPU6050 mpu;
TinyGPSPlus gps;

// GPS
SoftwareSerial gpsSerial(4, 3); // RX, TX

// GSM
SoftwareSerial gsmSerial(7, 8); // RX, TX

#define SOS_BUTTON 2
#define BUZZER 9

String phoneNumber = "+91 9080531817";

// Accident threshold
const float ACCIDENT_THRESHOLD = 2.5;

bool alertSent = false;

void setup()
{
  Serial.begin(9600);

  gpsSerial.begin(9600);
  gsmSerial.begin(9600);

  pinMode(SOS_BUTTON, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);

  Wire.begin();
  mpu.initialize();

  Serial.println("HillNet Guardian Started");

  delay(2000);
}

void loop()
{
  checkGPS();
  checkAccident();
  checkSOSButton();

  delay(100);
}

void checkGPS()
{
  while (gpsSerial.available())
  {
    gps.encode(gpsSerial.read());
  }
}

void checkAccident()
{
  int16_t ax, ay, az;
  int16_t gx, gy, gz;

  mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

  float x = ax / 16384.0;
  float y = ay / 16384.0;
  float z = az / 16384.0;

  float totalAccel = sqrt(x*x + y*y + z*z);

  if (totalAccel > ACCIDENT_THRESHOLD && !alertSent)
  {
    Serial.println("Accident Detected!");

    digitalWrite(BUZZER, HIGH);
    delay(3000);
    digitalWrite(BUZZER, LOW);

    sendEmergencySMS("AUTOMATIC ACCIDENT DETECTED");

    alertSent = true;
  }
}

void checkSOSButton()
{
  if (digitalRead(SOS_BUTTON) == LOW)
  {
    delay(50);

    if (digitalRead(SOS_BUTTON) == LOW)
    {
      Serial.println("SOS Button Pressed");

      digitalWrite(BUZZER, HIGH);
      delay(1000);
      digitalWrite(BUZZER, LOW);

      sendEmergencySMS("MANUAL SOS ALERT");

      while(digitalRead(SOS_BUTTON) == LOW);
      delay(500);
    }
  }
}

void sendEmergencySMS(String alertType)
{
  String latitude = "0.000000";
  String longitude = "0.000000";

  if (gps.location.isValid())
  {
    latitude = String(gps.location.lat(), 6);
    longitude = String(gps.location.lng(), 6);
  }

  String message = "";

  message += "HILLNET GUARDIAN ALERT\n";
  message += alertType;
  message += "\nLocation:\n";
  message += "Lat:";
  message += latitude;
  message += "\nLng:";
  message += longitude;
  message += "\nGoogle Maps:\n";
  message += "https://maps.google.com/?q=";
  message += latitude;
  message += ",";
  message += longitude;

  Serial.println("Sending SMS...");
  Serial.println(message);

  gsmSerial.println("AT");
  delay(1000);

  gsmSerial.println("AT+CMGF=1");
  delay(1000);

  gsmSerial.print("AT+CMGS=\"");
  gsmSerial.print(phoneNumber);
  gsmSerial.println("\"");

  delay(1000);

  gsmSerial.print(message);

  delay(500);

  gsmSerial.write(26);

  delay(5000);

  Serial.println("SMS Sent");
}
