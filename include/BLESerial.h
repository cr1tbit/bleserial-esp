

/*
    Video: https://www.youtube.com/watch?v=oCMOYS71NIU
    Based on Neil Kolban example for IDF: 
    https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleNotify.cpp
    Ported to Arduino ESP32 by Evandro Copercini

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
   Has a characteristic of: 6E400002-B5A3-F393-E0A9-E50E24DCCA9E - used for receiving data with "WRITE" 
   Has a characteristic of: 6E400003-B5A3-F393-E0A9-E50E24DCCA9E - used to send data with  "NOTIFY"

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   In this example rxValue is the data received (only accessible inside that function).
   And txValue is the data to be sent, in this example just a byte incremented every second. 
*/

#include <Arduino.h>
#include <NimBLEDevice.h>


class BLESerial : public BLEServerCallbacks, public BLECharacteristicCallbacks {
    const char *SERVICE_UUID           = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"; // UART service UUID
    const char *CHARACTERISTIC_UUID_RX = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
    const char *CHARACTERISTIC_UUID_TX = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";

    const int RX_BUF = 100;

public:
    BLESerial(){};

    void begin(char* name, bool mock = false){
        (void)mock;
        
        receiveQueue = xQueueCreate(RX_BUF, sizeof(char));
        if(receiveQueue == NULL) {
            Serial.println("This is so sad");
            Serial.flush();
        }

        BLEDevice::init(name);

        // Create the BLE Server
        pServer = BLEDevice::createServer();
        pServer->setCallbacks(this);

        // NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
        // pAdvertising->setAppearance(GENERIC_COMPUTER);

        // Create the BLE Service
        BLEService *pService = pServer->createService(SERVICE_UUID);

        pTxCharacteristic = pService->createCharacteristic(
            CHARACTERISTIC_UUID_TX, NIMBLE_PROPERTY::NOTIFY );

        BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
            CHARACTERISTIC_UUID_RX, NIMBLE_PROPERTY::WRITE );

        pRxCharacteristic->setCallbacks(this);

        pService->start();
        pServer->getAdvertising()->start();
        Serial.println("BLESerial init complete");
    }

    void loop(){
        if (deviceConnected) {
            if (loopTest){
                sendTest();
            }            
            delay(10); // bluetooth stack will go into congestion, if too many packets are sent
        }

        // disconnecting
        if (!deviceConnected && oldDeviceConnected) {
            delay(500); // give the bluetooth stack the chance to get things ready
            pServer->startAdvertising(); // restart advertising
            Serial.println("start advertising");
            oldDeviceConnected = deviceConnected;
        }
        // connecting
        if (deviceConnected && !oldDeviceConnected) {
            // do stuff here on connecting
            oldDeviceConnected = deviceConnected;
        }
    }

    bool available(){
        if (!deviceConnected) {
            return false;
        }
        if(uxQueueMessagesWaiting(receiveQueue) == 0){
            return false;
        }
        return true;
    }

    String readStringUntil(char c){
        String s = "";
        char _c;
        while(uxQueueMessagesWaiting(receiveQueue) > 0){
            if (xQueueReceive(receiveQueue, &_c, 0) != pdTRUE) {
                Serial.println("Failed to receive from queue");
                return s;
            }
            if(_c == c){
                return s;
            }
            s += _c;
        }
        return s;
    }

    void flush(){
        char _c;
        while(uxQueueMessagesWaiting(receiveQueue) > 0){
            if (xQueueReceive(receiveQueue, &_c, 0) != pdTRUE) {
                Serial.println("Failed to flush lel");
                return;
            }
        }
    }

    void sendTest(){
        static char text[7] = "test0\n";
        static int counter = 0;
        text[4] = (counter++)%10 + 0x30;

        pTxCharacteristic->setValue((uint8_t*)text, 6);
        pTxCharacteristic->notify();
    }

    //
    // void println(String s);
    
    //BLEServerCallbacks
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
    }

    /*
    uint32_t onPassKeyRequest(){
        Serial.println("Server PassKeyRequest");
        return 123456; 
    }
    bool onConfirmPIN(uint32_t pass_key){
        Serial.print("The passkey YES/NO number: ");Serial.println(pass_key);
        return true; 
    }
    void onAuthenticationComplete(ble_gap_conn_desc desc){
        Serial.println("Starting BLE work!");
    }
    */

    //BLECharacteristicCallbacks
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string rxValue = pCharacteristic->getValue();

        if (rxValue.length() > 0) {
            for (int i = 0; i < rxValue.length(); i++){
                // Serial.print(rxValue[i]);
                xQueueSend(receiveQueue, &rxValue[i], 0); //TODO handle full queue
            }
        }
    }

private:
    bool deviceConnected = false;
    bool oldDeviceConnected = false;

    bool loopTest = false;
    uint8_t txValue = 0;

    QueueHandle_t receiveQueue;

    BLEServer *pServer = NULL;
    BLECharacteristic * pTxCharacteristic;
};