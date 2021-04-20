// based on https://qiita.com/coppercele/items/fef9eacee05b752ed982#m5stickc%E3%83%90%E3%83%BC%E3%82%B8%E3%83%A7%E3%83%B3

// #define ENABLE_M5STACK_UPDATER

#include <Arduino.h>
#include <M5Stack.h>
#ifdef ENABLE_M5STACK_UPDATER
#include <M5StackUpdater.h>
#endif
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

int scanTime = 4; //BLEタイムアウト
BLEScan* pBLEScan;

// 接触確認アプリのUUID
const char* uuid = "0000fd6f-0000-1000-8000-00805f9b34fb";

int deviceNum = 0;
int brightness = 200;

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
       if(advertisedDevice.haveServiceUUID()){
            if(strncmp(advertisedDevice.getServiceUUID().toString().c_str(),uuid, 36) == 0){
                int rssi = advertisedDevice.getRSSI();
                Serial.print("RSSI: ");
                Serial.println(rssi);
                Serial.print("ADDR: ");
                Serial.println(advertisedDevice.getAddress().toString().c_str());
                Serial.println("Found!");
                deviceNum++;
            }
        }

    }
};

void drawScreenHeader() {
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setTextSize(2);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.setTextColor(RED);
    M5.Lcd.print("COCOA Counter\n");
}

void drawScreen() {
    drawScreenHeader();
    M5.Lcd.setCursor(0, 30);
    M5.Lcd.setTextSize(7);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.printf(" %2d",deviceNum);
    if (M5.Power.canControl()) {
      M5.Lcd.setTextSize(1);
      M5.Lcd.setCursor(0, M5.Lcd.height() - 10);
      M5.Lcd.printf("Battery:%d%% ", M5.Power.getBatteryLevel());
      if (M5.Power.isCharging()) {
        M5.Lcd.printf("(Charging)");
      }
    }
}
void Task1(void *pvParameters) {
  // loop()書くとBLEスキャン中M5.update()が実行されなくてボタンが取れないのでマルチスレッド化している
  while(1) {
    deviceNum = 0;
    BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
    Serial.print("Devices found: ");
    Serial.println(deviceNum);
    Serial.println("Scan done!");
    pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
    drawScreen();
  }
}


void setup() {
  M5.begin();
  Serial.begin(115200);

#ifdef ENABLE_M5STACK_UPDATER
  if (digitalRead(BUTTON_A_PIN) == 0) {
    Serial.println("Will Load menu binary");
    updateFromFS(SD);
    ESP.restart();
  }
#endif

  M5.Power.begin();

  M5.Lcd.setRotation(1);
  M5.Lcd.setBrightness(brightness);
  drawScreenHeader();

  Serial.println("Scanning...");

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(5000); // スキャン間隔5秒
  pBLEScan->setWindow(4999);  // less or equal setInterval value
  xTaskCreatePinnedToCore(Task1,"Task1", 4096, NULL, 3, NULL, 1);
}

void loop() {
  M5.update();
  if ( M5.BtnA.wasReleased() ) {
    if (brightness >= 200) {
      brightness = 50;
    } else if (brightness > 0) {
      M5.Lcd.setBrightness(0);
      M5.Lcd.sleep();
      brightness = 0;
    } else {
      M5.Lcd.wakeup();
      brightness = 200;
    }
  }
  M5.Lcd.setBrightness(brightness);
}