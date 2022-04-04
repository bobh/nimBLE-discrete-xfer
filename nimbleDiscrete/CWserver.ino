
/** NimBLE_Server Demo:
 *
 *  Demonstrates many of the available features of the NimBLE server library.
 *  
 *  Created: on March 22 2020
 *      Author: H2zero
 * 
*/

#include <NimBLEDevice.h>

// uses RMTWriteNeoPixel example
//+++++++++++++++++++++++++++++++++++++++++
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "Arduino.h"

#include "esp32-hal.h"

const int keyline = 9;     // the number of the keyer pin
const int ledPin =  19;      // the number of the LED pin

#define NR_OF_LEDS   1*4
#define NR_OF_ALL_BITS 24*NR_OF_LEDS

rmt_data_t led_data[NR_OF_ALL_BITS];

rmt_obj_t* rmt_send = NULL;

int color[] =  { 0x55, 0x11, 0x77 };  // GRB value

typedef enum  { 
  OFF_COLOR = 0,
  DISCONNECT_COLOR,
  CONNECT_COLOR
  } colors_t;

colors_t currentColor = OFF_COLOR;

int led_index = 0;


void shineLED(colors_t displayColor)
{
    // GRB value   
    switch (displayColor) 
    {
      case OFF_COLOR: 
          color[0] =  0x55;  
          color[1] =  0x11;  
          color[2] =  0x77;  
          break;
      case DISCONNECT_COLOR: 
          color[0] =  0x00;  
          color[1] =  0xff;  
          color[2] =  0x0;  
          break;    
       case CONNECT_COLOR: 
          color[0] =  0x77;  
          color[1] =  0x00;  
          color[2] =  0x00;  
          break;          
      default: break;     
    }

    // Init data with only one led ON
    int led, col, bit;
    int i=0;
    for (led=0; led<NR_OF_LEDS; led++) {
        for (col=0; col<3; col++ ) {
            for (bit=0; bit<8; bit++){
                if ( (color[col] & (1<<(7-bit))) && (led == led_index) ) {
                    led_data[i].level0 = 1;
                    led_data[i].duration0 = 8;
                    led_data[i].level1 = 0;
                    led_data[i].duration1 = 4;
                } else {
                    led_data[i].level0 = 1;
                    led_data[i].duration0 = 4;
                    led_data[i].level1 = 0;
                    led_data[i].duration1 = 8;
                }
                i++;
            }
        }
    }
    // make the led travel in the pannel
    if ((++led_index)>=NR_OF_LEDS) {
        led_index = 0;
    }
  
}


//+++++++++++++++++++++++++++++++++++++++++

static NimBLEServer* pServer;

// The remote service we wish to connect to.
static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c3319100");
// The characteristic of the remote service we are interested in.
static BLEUUID characteristicUUID("beb5483e-36e1-4688-b7f5-ea07361b2600");

//static BLEUUID cwkeyserviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c3319555");


/**  None of these are required as they will be handled by the library with defaults. **
 **                       Remove as you see fit for your needs                        */  
class ServerCallbacks: public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer) {
        Serial.println("Client connected");
        //Serial.println("Multi-connect support: start advertising");
        //NimBLEDevice::startAdvertising();
    };
    /** Alternative onConnect() method to extract details of the connection. 
     *  See: src/ble_gap.h for the details of the ble_gap_conn_desc struct.
     */  
    void onConnect(NimBLEServer* pServer, ble_gap_conn_desc* desc) {
        Serial.print("Client address: ");
        Serial.println(NimBLEAddress(desc->peer_ota_addr).toString().c_str());
        /** We can use the connection handle here to ask for different connection parameters.
         *  Args: connection handle, min connection interval, max connection interval
         *  latency, supervision timeout.
         *  Units; Min/Max Intervals: 1.25 millisecond increments.
         *  Latency: number of intervals allowed to skip.
         *  Timeout: 10 millisecond increments, try for 5x interval time for best results.  
         */
        pServer->updateConnParams(desc->conn_handle, 24, 48, 0, 60);
        currentColor = CONNECT_COLOR;
    };
    void onDisconnect(NimBLEServer* pServer) {
        Serial.println("Client disconnected - start advertising");
        NimBLEDevice::startAdvertising();
        currentColor = DISCONNECT_COLOR;
    };
    void onMTUChange(uint16_t MTU, ble_gap_conn_desc* desc) {
        Serial.printf("MTU updated: %u for connection ID: %u\n", MTU, desc->conn_handle);
    };
    
/********************* Security handled here **********************
****** Note: these are the same return values as defaults ********/
    uint32_t onPassKeyRequest(){
        Serial.println("Server Passkey Request");
        /** This should return a random 6 digit number for security 
         *  or make your own static passkey as done here.
         */
        return 123456; 
    };

    bool onConfirmPIN(uint32_t pass_key){
        Serial.print("The passkey YES/NO number: ");Serial.println(pass_key);
        /** Return false if passkeys don't match. */
        return true; 
    };

    void onAuthenticationComplete(ble_gap_conn_desc* desc){
        /** Check that encryption was successful, if not we disconnect the client */  
        if(!desc->sec_state.encrypted) {
            NimBLEDevice::getServer()->disconnect(desc->conn_handle);
            Serial.println("Encrypt connection failed - disconnecting client");
            return;
        }
        Serial.println("Starting BLE work!");
    };
};

/** Handler class for characteristic actions */
class CharacteristicCallbacks: public NimBLECharacteristicCallbacks {
    void onRead(NimBLECharacteristic* pCharacteristic){
      
        //Serial.print(pCharacteristic->getUUID().toString().c_str());
        //Serial.print(": onRead(), value: ");
        //Serial.println(pCharacteristic->getValue().c_str());
    };

    void onWrite(NimBLECharacteristic* pCharacteristic) {
        ///////////Serial.print(pCharacteristic->getUUID().toString().c_str());
        //Serial.print(": onWrite(), value: ");
        //Serial.println(pCharacteristic->getValue().c_str());
        if(pCharacteristic->getValue() == "1")
        {
          ///////////Serial.println(" KEY ON ");
          digitalWrite(keyline, HIGH);// key first
          digitalWrite(ledPin,  HIGH);
        }
        else
        {
          /////////Serial.println(" KEY OFF ");
          digitalWrite(keyline, LOW); //key first
          digitalWrite(ledPin,  LOW);
        }

        
    };
    /** Called before notification or indication is sent, 
     *  the value can be changed here before sending if desired.
     */
    void onNotify(NimBLECharacteristic* pCharacteristic) {
        Serial.println("Sending notification to clients");
    };


    /** The status returned in status is defined in NimBLECharacteristic.h.
     *  The value returned in code is the NimBLE host return code.
     */
    void onStatus(NimBLECharacteristic* pCharacteristic, Status status, int code) {
        String str = ("Notification/Indication status code: ");
        str += status;
        str += ", return code: ";
        str += code;
        str += ", "; 
        str += NimBLEUtils::returnCodeToString(code);
        Serial.println(str);
    };

    void onSubscribe(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc, uint16_t subValue) {
        String str = "Client ID: ";
        str += desc->conn_handle;
        str += " Address: ";
        str += std::string(NimBLEAddress(desc->peer_ota_addr)).c_str();
        if(subValue == 0) {
            str += " Unsubscribed to ";
        }else if(subValue == 1) {
            str += " Subscribed to notfications for ";
        } else if(subValue == 2) {
            str += " Subscribed to indications for ";
        } else if(subValue == 3) {
            str += " Subscribed to notifications and indications for ";
        }
        str += std::string(pCharacteristic->getUUID()).c_str();

        Serial.println(str);
    };
};
    
/** Handler class for descriptor actions */    
class DescriptorCallbacks : public NimBLEDescriptorCallbacks {
    void onWrite(NimBLEDescriptor* pDescriptor) {
        std::string dscVal((char*)pDescriptor->getValue(), pDescriptor->getLength());
        Serial.print("Descriptor witten value:");
        Serial.println(dscVal.c_str());
    };

    void onRead(NimBLEDescriptor* pDescriptor) {
        Serial.print(pDescriptor->getUUID().toString().c_str());
        Serial.println(" Descriptor read");
    };
};


/** Define callback instances globally to use for multiple Charateristics \ Descriptors */ 
static DescriptorCallbacks dscCallbacks;
static CharacteristicCallbacks chrCallbacks;


void setup() {
    Serial.begin(115200);

     // initialize the LED pin as an output:
    pinMode(ledPin, OUTPUT);
    // initialize the pushbutton pin as an input:
    pinMode(keyline, OUTPUT);

    digitalWrite(keyline, LOW);
    digitalWrite(ledPin,  LOW);

    delay(1000);
    Serial.println("Starting NimbleCW Server");
    //+++++++++++++++++++++++++++++++++++++++++
    if ((rmt_send = rmtInit(8, RMT_TX_MODE, RMT_MEM_64)) == NULL)
    {
        Serial.println("init sender failed\n");
    }

    float realTick = rmtSetTick(rmt_send, 100);
    //Serial.printf("real tick set to: %fns\n", realTick);

    currentColor = OFF_COLOR;
    //+++++++++++++++++++++++++++++++++++++++++
    
    /** sets device name */
    NimBLEDevice::init("CWserver");

    /** Optional: set the transmit power, default is 3db */
    //Max power
    NimBLEDevice::setPower(ESP_PWR_LVL_P9); /** +9db */
    
    /** Set the IO capabilities of the device, each option will trigger a different pairing method.
     *  BLE_HS_IO_DISPLAY_ONLY    - Passkey pairing
     *  BLE_HS_IO_DISPLAY_YESNO   - Numeric comparison pairing
     *  BLE_HS_IO_NO_INPUT_OUTPUT - DEFAULT setting - just works pairing
     */
    //NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY); // use passkey
    //NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_YESNO); //use numeric comparison

    /** 2 different ways to set security - both calls achieve the same result.
     *  no bonding, no man in the middle protection, secure connections.
     *   
     *  These are the default values, only shown here for demonstration.   
     */ 
    //NimBLEDevice::setSecurityAuth(false, false, true); 
    //NimBLEDevice::setSecurityAuth(/*BLE_SM_PAIR_AUTHREQ_BOND | BLE_SM_PAIR_AUTHREQ_MITM |*/ BLE_SM_PAIR_AUTHREQ_SC);
    
    //|||||||||||||||||||||||||||||||||||||||||||||
    //|||||||||||||||||||||||||||||||||||||||||||||
    /*
    >Create Server
    >Create Service
    >Create Service Characteristic
    */
    
    pServer = NimBLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());
    
    //NimBLEService* pDeadService = pServer->createService("KEY");
    NimBLEService* pDeadService = pServer->createService(serviceUUID);

    //Characteristic, key line, is write only
    //state is kept in client unless proves unreliable
    //Modified-write only
    NimBLECharacteristic* pBeefCharacteristic = pDeadService->createCharacteristic(
                                               //"BEEF",
                                               characteristicUUID,
                                               NIMBLE_PROPERTY::READ |
                                               NIMBLE_PROPERTY::WRITE // |
                               /** Require a secure connection for read and write access */
                                               //NIMBLE_PROPERTY::READ_ENC |  // only allow reading if paired / encrypted
                                               //NIMBLE_PROPERTY::WRITE_ENC   // only allow writing if paired / encrypted
                                              );
  
    //pBeefCharacteristic->setValue("Burger");
    pBeefCharacteristic->setValue(0);
    pBeefCharacteristic->setCallbacks(&chrCallbacks);

    //|||||||||||||||||||||||||||||||||||||||||||||
    //|||||||||||||||||||||||||||||||||||||||||||||

    /** 2904 descriptors are a special case, when createDescriptor is called with
     *  0x2904 a NimBLE2904 class is created with the correct properties and sizes.
     *  However we must cast the returned reference to the correct type as the method
     *  only returns a pointer to the base NimBLEDescriptor class.
     */
    NimBLE2904* pBeef2904 = (NimBLE2904*)pBeefCharacteristic->createDescriptor("2904"); 
    pBeef2904->setFormat(NimBLE2904::FORMAT_UTF8);
    pBeef2904->setCallbacks(&dscCallbacks);
  

    //NimBLEService* pBaadService = pServer->createService("BAAD");
    //NimBLEService* pBaadService = pServer->createService(cwkeyserviceUUID);
    /*
    NimBLECharacteristic* pFoodCharacteristic = pBaadService->createCharacteristic(
                                               "key_ON_OFF",
                                               NIMBLE_PROPERTY::READ |
                                               NIMBLE_PROPERTY::WRITE |
                                               NIMBLE_PROPERTY::NOTIFY
                                              );

    pFoodCharacteristic->setValue("Fries");
    pFoodCharacteristic->setCallbacks(&chrCallbacks);
    */
    
    /** Note a 0x2902 descriptor MUST NOT be created as NimBLE will create one automatically
     *  if notification or indication properties are assigned to a characteristic.
     */

    /** Custom descriptor: Arguments are UUID, Properties, max length in bytes of the value */
    /*
    NimBLEDescriptor* pC01Ddsc = pFoodCharacteristic->createDescriptor(
                                               "C01D",
                                               NIMBLE_PROPERTY::READ | 
                                               NIMBLE_PROPERTY::WRITE|
                                               NIMBLE_PROPERTY::WRITE_ENC, // only allow writing if paired / encrypted
                                               20
                                              );
    pC01Ddsc->setValue("Send it back!");
    pC01Ddsc->setCallbacks(&dscCallbacks);
    */

    /** Start the services when finished creating all Characteristics and Descriptors */  
    pDeadService->start();
    //pBaadService->start();

    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    /** Add the services to the advertisment data **/
    pAdvertising->addServiceUUID(pDeadService->getUUID());
    //pAdvertising->addServiceUUID(pBaadService->getUUID());
    /** If your device is battery powered you may consider setting scan response
     *  to false as it will extend battery life at the expense of less data sent.
     */
    pAdvertising->setScanResponse(true);
    pAdvertising->start();

    Serial.println("Advertising Started");
}


void loop() {
  /** Do your thing here, this just spams notifications to all connected clients */
    if(pServer->getConnectedCount()) {
        NimBLEService* pSvc = pServer->getServiceByUUID("BAAD");
        if(pSvc) {
            NimBLECharacteristic* pChr = pSvc->getCharacteristic("F00D");
            if(pChr) {
                pChr->notify(true);
            }
        }
    }
    
  delay(1000);
  shineLED(currentColor);
  rmtWrite(rmt_send, led_data, NR_OF_ALL_BITS);
}
