#include <Arduino.h>
#include <Adafruit_Fingerprint.h>
#include <LiquidCrystal_I2C.h>
#include <SimpleTimer.h>

/*Cài đặt cổng giao tiếp với các thiết bị*/
#if (defined(__AVR__) || defined(ESP8266)) && !defined(__AVR_ATmega2560__)
// For UNO and others without hardware serial, we must use software serial...
// pin #2 is IN from sensor (GREEN wire)
// pin #3 is OUT from arduino  (WHITE wire)
// Set up the serial port to use softwareserial..
SoftwareSerial mySerial(2, 3); 
#else
// On Leonardo/M0/etc, others with hardware serial, use hardware serial!
// #0 is green wire, #1 is white
#define mySerial Serial1
#endif

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
LiquidCrystal_I2C lcd(0x3F, 16, 2); 
#define Relay 12 
#define redLed 9
#define button 8
SimpleTimer timer;
unsigned long times = millis();
char setupState = 0;
uint8_t id = 0;

/*Hàm hiển thị lcd đợi quét vân tay*/
void waitFinger()
{
  lcd.clear(); 
  lcd.setCursor(1,0);
  lcd.print("READY TO SCAN");
}
/*Hàm hiển thị lên lcd không tìm thấy vân tay*/
void notFoundFinger()
{
  lcd.clear(); 
  lcd.setCursor(5, 0);
  lcd.print("FINGER ");
  lcd.setCursor(3, 1);
  lcd.print("NOT FOUND!!!");
}
/*Hàm hiển thị lên lcd khi quét đúng vân tay*/
void fingerOK()
{
  lcd.clear(); 
  lcd.setCursor(5,0);
  lcd.print("SUCCESS");
  lcd.setCursor(3,1);
  lcd.print("DOOR UNLOCK");
}
void scanOldFinger()
{
  lcd.clear();
  lcd.setCursor(6,0);
  lcd.print("SCAN");
  lcd.setCursor(2,1);
  lcd.print("OLD FINGER!!!");
}
void enrollFinger()
{
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("ADD FINGER!!!");
}
void enrollFingerAgain()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SCAN SAME FINGER");
  lcd.setCursor(5, 1);
  lcd.print("AGAIN");
}
void addFingerSuccess()
{
  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("SUCCESS");
}
uint8_t getFingerprintEnroll() {
  int p = -1;
  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      if(digitalRead(button)==LOW){
        setupState=0;
        waitFinger();
        return true;
      }else{
        times=millis();
      }
      break;
    default:
      Serial.println("Error");
      break;
    }
  }
  // OK success!
  p = finger.image2Tz(1);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    default:
      Serial.println("Error");
      return p;
  }

  Serial.println("Remove finger");
  enrollFingerAgain();
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID "); Serial.println(id);
  p = -1;
  Serial.println("Place same finger again");
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.print(".");
      break;
    default:
      Serial.println("Error");
      break;
    }
  }
  // OK success!
  p = finger.image2Tz(2);
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    default:
      Serial.println("Unknown error");
      return p;
  }
  // OK converted!
  Serial.print("Creating model for #");  Serial.println(id);
  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  }else {
    Serial.println("Error");
    return p;
  }
  Serial.print("ID "); Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
  }else {
    Serial.println("Error");
    return p;
  }
  id++;
  addFingerSuccess();
  delay(1000);
  enrollFinger();
  return true;
}
/*Hàm kiểm tra lỗi, vân tay đúng, sai hoặc chưa quét*/
uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("No finger detected");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    notFoundFinger(); /*Thông báo lên lcd vân tay sai*/
    digitalWrite(redLed, HIGH);
    delay(2000);
    digitalWrite(redLed, LOW);
    waitFinger();     /*Tiếp tục đợi quét vân tay*/
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  // found a match!
  Serial.print("Found ID #"); Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); Serial.println(finger.confidence);

  return finger.fingerID;
}

/*Nếu có lỗi, vân tay sai hoặc chưa quét sẽ trả về -1*/
int getFingerprintIDez() {
  uint8_t p = finger.getImage();

  if (p != FINGERPRINT_OK)  return -1;

  p = finger.image2Tz();
  if (p != FINGERPRINT_OK)  return -1;

  p = finger.fingerFastSearch();
  if (p != FINGERPRINT_OK)  return -1;

  // found a match!
  Serial.print("Found ID #"); 
  Serial.print(finger.fingerID);
  Serial.print(" with confidence of "); 
  Serial.println(finger.confidence);
  return finger.fingerID;
}

void setup()
{
  lcd.init();
  lcd.backlight();
  pinMode(Relay, OUTPUT);
  pinMode(redLed, OUTPUT);
  pinMode(button, INPUT_PULLUP);
  waitFinger();
  Serial.begin(9600);
  while (!Serial);  // For Yun/Leo/Micro/Zero/...
  delay(100);
  Serial.println("\n\nAdafruit finger detect test");

  // set the data rate for the sensor serial port
  finger.begin(57600);
  delay(5);
  if (finger.verifyPassword()) 
  {
    Serial.println("Found fingerprint sensor!");
  } 
  else 
  {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) { delay(1); }
  }

  finger.getTemplateCount();

  Serial.print("Sensor contains "); 
  Serial.print(finger.templateCount); 
  Serial.println(" templates");
  Serial.println("Waiting for valid finger...");
}

void loop()                     // run over and over again
{ 
  timer.run();
  id = finger.fingerID + 1; 
  if(digitalRead(button)==LOW){ 
    if(millis() - times > 3000){ 
      setupState = 1;
    }
  }else{
    times = millis();
  }

  if(setupState == 1)
  {
    if(id > 1)
    {
      scanOldFinger();
      delay(2000);
      if(getFingerprintIDez() == -1)
      {
        getFingerprintID();
      }
      else
      {
        finger.emptyDatabase();
        Serial.println("Now database is empty : ");
        enrollFinger();
        id = 1;
        setupState = 2;
        times = millis();
        delay(2000);
      }
    }
    else
    {
      finger.emptyDatabase();
      Serial.println("Now database is empty : ");
      enrollFinger();
      id = 1;
      setupState = 2;
      times = millis();
      delay(2000);
    }
  }
  else if(setupState == 2)
  {
    while (!getFingerprintEnroll() );
  }
  else{
    if(getFingerprintIDez() == -1) /*Kiêm tra vân tay nếu sai thì kiểm 
                                  tra xem do vân tay sai hay chưa quét*/
    {
      getFingerprintID();
    }
    else /*Nếu đúng*/
    {
      digitalWrite(Relay, HIGH); /*bật relay*/
      fingerOK();                /*Thông báo thành công lên lcd*/
      delay(2000);
      digitalWrite(Relay, LOW);
      delay(200);  /*tắt relay*/
      waitFinger();              /*Tiếp tục đợi kiếm tra quét vân tay*/ 
    } 
    delay(50);
  }
  delay(50);
}