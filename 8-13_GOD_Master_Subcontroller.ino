
//LIBRARIES

//7-32-2024

#include <Arduino.h>
#include "soc/rtc_wdt.h"
#include <esp_task_wdt.h>
//#include <ModbusRtu.h>
#include <ModbusMaster.h>
#include "slaver.h"
#include "BluetoothSerial.h"

#include <can_bms.h>
#include "can_server.h"

#include <Preferences.h>
Preferences preferences;





//PINS
#define GEN_RE  0
//#define RXD2 4
//#define TXD2 23



//,ASTER PINS
#define MAX485_DE 25
#define MAX485_RE_NEG 25
#define RXD 16
#define TXD 17
int MaxCurrent = 100;
int MaxStringVolt = 817;
int CalculatedChargeCurrent = 0;
int CalculatedDischargeCurrent = 0;
float String1Volt = 0;
float String2Volt = 0;
float String3Volt = 0;
float String4Volt = 0;
float SystemVolt = 0;


float String1Current = 0;
float String2Current = 0;
float String3Current = 0;
float String4Current = 0;
float SystemCurrent = 0;
float Throttle = 100;




boolean String1Charge = false;
boolean String2Charge = false;
boolean String3Charge = false;
boolean String4Charge = false;
float ChargePercentage = 0;

boolean String1Discharge = false;
boolean String2Discharge = false;
boolean String3Discharge = false;
boolean String4Discharge = false;
float DischargePercentage = 0;

float VoltagePercentage = 0;

float String1SOC = 0;
float String2SOC = 0;
float String3SOC = 0;
float String4SOC = 0;
float SystemSOC = 0;


float String1MaxCell = 0;
float String2MaxCell = 0;
float String3MaxCell = 0;
float String4MaxCell = 0;
float SystemMaxCell = 0;


float String1MinCell = 0;
float String2MinCell = 0;
float String3MinCell = 0;
float String4MinCell = 0;
float SystemMinCell = 0;

float SystemMaxCells[4];
float SystemMinCells[4];



float String1MaxCellModbus = 500;
float String1MinCellModbus = 500;


int BatteryVoltageAverage;
int BatteryCurrentAverage;
int BatteryTemperatureAverage;
int BatterySOCAverage;
int BatterySOHAverage;
int ChargeVoltage;
int DischargeVoltage;
int ChargeCurrent;
int DischargeCurrent;
int Alarm = 0;
int Capacity = 208;


bool RELAYARRAY[] = {0, 0, 0, 0, 0, 0, 0, 0};


String receivedString;
char receivedChar;
boolean newData = false;


//OBJECTS
uint8_t *BMSAR;


uint8_t result = 0;
uint16_t ModbusArray[11] = {500, 600, 700, 800, 900, 1000, 1100, 1200, 1300, 1400, 1500};



uint16_t MasterArray[600];

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

int SubID = 1;
int StringSize = 0;
int ModuleSize = 0;
boolean CANSIM = false;
String SerialProcessor = "";

TaskHandle_t MODBUSTASK_SLAVE;
TaskHandle_t MODBUSTASK_MASTER;
TaskHandle_t CANBUSTASK_INVERTER;
TaskHandle_t BT;
TaskHandle_t SERIALMONITORTASK;


ModbusMaster node;
void preTransmission()
{


  digitalWrite(MAX485_RE_NEG, 1);
  //digitalWrite(MAX485_DE, 1);
}

void postTransmission()
{
  digitalWrite(MAX485_RE_NEG, 0);
  //digitalWrite(MAX485_DE, 0);
}






void setup() {






  //COMMENT IF YOU WANT DEBUG
  RXS = 3;
  TXS = 1;
  en_rs485 = 5;





  //COMMENT IF YOU WANT MODBUS
  //Serial.begin(115200);

  Serial1.begin(9600, SERIAL_8N1, RXD, TXD);
  pinMode(MAX485_RE_NEG, OUTPUT);
  //pinMode(MAX485_DE, OUTPUT);
  digitalWrite(MAX485_RE_NEG, 0);
  //digitalWrite(MAX485_DE, 0);



  delay(500);


  preferences.begin("my-app", false);
  SubID = preferences.getInt("SubID", 1);
  MaxCurrent = preferences.getInt("MaxCurrent", 100);
  StringSize = preferences.getInt("StringSize", 3);
  ModuleSize = preferences.getInt("ModuleSize", 15);
  MaxStringVolt = preferences.getInt("MaxStringVolt", 817);
  CANSIM = preferences.getInt("CANSIM", 0);
  preferences.end();


  //CREATING TASK FOR MODBUS
  xTaskCreatePinnedToCore(
    MODBUSTASK_SLAVE_CODE,   /* Task function. */
    "MODBUSTASK_SLAVE",     /* name of task. */
    10000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    5,           /* priority of the task */
    &MODBUSTASK_SLAVE,      /* Task handle to keep track of created task */
    0);          /* pin task to core 0 */
  delay(50);




  //CREATING TASK FOR MODBUS
  xTaskCreatePinnedToCore(
    MODBUSTASK_MASTER_CODE,   /* Task function. */
    "MODBUSTASK_MASTER",     /* name of task. */
    10000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    6,           /* priority of the task */
    &MODBUSTASK_MASTER,      /* Task handle to keep track of created task */
    0);          /* pin task to core 0 */
  delay(50);


  //CREATING TASK FOR CANBUS_____________________________________________________________________________________________________
  xTaskCreatePinnedToCore(
    CANBUSTASK_INVERTER_CODE,   /* Task function. */
    "CANBUSTASK_INVERTER",     /* name of task. */
    10000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    8,           /* priority of the task */
    &CANBUSTASK_INVERTER,      /* Task handle to keep track of created task */
    1);          /* pin task to core 0 */
  delay(50);

  //CREATING TASK FOR BLUETOOTH_____________________________________________________________________________________________________
  xTaskCreatePinnedToCore(
    BT_CODE,   /* Task function. */
    "BT",     /* name of task. */
    9000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    2,           /* priority of the task */
    &BT,      /* Task handle to keep track of created task */
    1);          /* pin task to core 0 */
  delay(50);

  //   CREATING TASK FOR ANALOG READING_____________________________________________________________________________________________________
  xTaskCreatePinnedToCore(
    SERIALMONITOR_CODE,   /* Task function. */
    "SERIALMONITORTASK",     /* name of task. */
    6000,       /* Stack size of task */
    NULL,        /* parameter of the task */
    2,           /* priority of the task */
    &SERIALMONITORTASK,      /* Task handle to keep track of created task */
    1);          /* pin task to core 0 */
  delay(50);

}



void CANBUSTASK_INVERTER_CODE( void * pvParameters ) {
  Serial.println("CANBUSTASK_INVERTER TASK STARTED");



  can_start(250);
  for (;;) {


    //    if (SystemVolt < 800) {
    //      VoltagePercentage = 100 * 0.01;
    //    }
    //    else {
    //      VoltagePercentage = (10 - (SystemVolt - 800)) * 10 * 0.01;
    //    }

    VoltagePercentage = 1;

    //
    Serial.println(MaxCurrent);
    Serial.println(ChargePercentage);
    Serial.println(DischargePercentage);
    Serial.println(VoltagePercentage);
    CalculatedChargeCurrent = MaxCurrent * ChargePercentage * VoltagePercentage;
    CalculatedDischargeCurrent = MaxCurrent * DischargePercentage * VoltagePercentage;

    if(SystemSOC<25){
      CalculatedDischargeCurrent=CalculatedDischargeCurrent*0.01;
    }


    if (SystemMaxCell * 0.01 < 4.1 && SystemMaxCell * 0.01 > 2.5) {
      String1MaxCellModbus = SystemMaxCell * 0.01;
    }

    if (SystemMinCell * 0.01 < 4.1 && SystemMinCell * 0.01 > 2.5) {
      String1MinCellModbus = SystemMinCell * 0.01;
    }




    if (String1MaxCellModbus < 4.4  && String1MinCellModbus < 4.4 && String1MaxCellModbus > 2.2  && String1MinCellModbus > 2.2) {
      set_maxvoltage(String1MaxCellModbus, String1MinCellModbus, SystemSOC, SystemSOC, 0, 0x180150F1);
      delay(100);


      if ((SystemVolt * 0.1 < MaxStringVolt * 0.980 )|| SystemSOC<80) {
        //FULL THROTTLE
        set_totalvoltage(SystemVolt * 0.1 , SystemCurrent * 0.1 , CalculatedChargeCurrent, CalculatedDischargeCurrent, 0x180250F1);
        delay(100);
        Throttle = 100;
      }

      else if (SystemVolt * 0.1 >= MaxStringVolt * 0.980 && SystemVolt * 0.1 < MaxStringVolt * 0.985) {
        //HALF THROTTLE
        set_totalvoltage(SystemVolt * 0.1 , SystemCurrent * 0.1 , CalculatedChargeCurrent * 0.5, CalculatedDischargeCurrent, 0x180250F1);
        delay(100);
        Throttle = 50;
      }

      else if (SystemVolt * 0.1 >= MaxStringVolt * 0.985 && SystemVolt * 0.1 < MaxStringVolt * 0.990) {
        //QUARTER THROTTLE
        set_totalvoltage(SystemVolt * 0.1 , SystemCurrent * 0.1 , CalculatedChargeCurrent * 0.25, CalculatedDischargeCurrent, 0x180250F1);
        delay(100);
        Throttle = 25;
      }


      else if (SystemVolt * 0.1 >= MaxStringVolt * 0.99 && SystemVolt * 0.1 < MaxStringVolt * 0.995) {
        //TEN PERCENT THROTTLE
        if (SystemSOC < 97) {
          set_totalvoltage(SystemVolt * 0.1 , SystemCurrent * 0.1 , CalculatedChargeCurrent * 0.10, CalculatedDischargeCurrent, 0x180250F1);
          Throttle = 10;
        }
        else {
          set_totalvoltage(SystemVolt * 0.1 , SystemCurrent * 0.1 , CalculatedChargeCurrent * 0.05, CalculatedDischargeCurrent, 0x180250F1);
          Throttle = 5;
        }
        delay(100);
      }

      else if (SystemVolt * 0.1 >= MaxStringVolt * 0.995 && SystemVolt * 0.1 < MaxStringVolt * 1) {
        //FIVE PERCENT THROTTLE
        set_totalvoltage(SystemVolt * 0.1 , SystemCurrent * 0.1 , CalculatedChargeCurrent * 0.05, CalculatedDischargeCurrent, 0x180250F1);
        delay(100);
        Throttle = 5;
      }


      else if (SystemVolt * 0.1 >= MaxStringVolt) {
        //ONE PERCENT THROTTLE
        set_totalvoltage(SystemVolt * 0.1 , SystemCurrent * 0.01 , CalculatedChargeCurrent * 0.01, CalculatedDischargeCurrent, 0x180250F1);
        delay(100);
        Throttle = 1;
      }

       else {
        //ONE PERCENT THROTTLE
        set_totalvoltage(SystemVolt * 0.1 , SystemCurrent * 0.01 , CalculatedChargeCurrent * 0, CalculatedDischargeCurrent, 0x180250F1);
        delay(100);
        Throttle = 0;
      }


      //set_groupnumber(uint8_t maxvolt_grpnm, uint8_t maxvolt_packnm, uint8_t maxvolt_boxnm, uint8_t maxtemp_grpnm, uint8_t maxtemp_packnm, uint8_t maxtemp, 0x180350F1);
      set_groupnumber(1, 3, 4, 1, 3, 25, 0x180350F1);
      delay(100);
      set_groupnumbermin(2, 4, 6, 2, 3, 25,  0x180450F1);
      delay(100);
      set_warnings(4, 0, 0, 0, 0 , 0x180650F1);
      delay(100);
      set_warnings2(0, 0, 0, 0, 0, 0x180750F1);
    }
    else {
      delay(500);
    }

  }
}






void MODBUSTASK_MASTER_CODE( void * pvParameters ) {
  Serial.println("MODBUS MASTER TASK STARTED");

  node.begin(1, Serial1);
  // Callbacks allow us to configure the RS485 transceiver correctly
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);


  for (;;) {

    // DATA ACQUISATION/////////////////////////////////////////////////// START////////////



    if (CANSIM == 0) {

      for (int x = 0; x < StringSize; x++) {
        node.slaves(x + 1); //subcontroller
        delay(50);

        for (int i = 0; i < 5; i++) {

          result = node.readHoldingRegisters(i * 30, 30);
          if (result == node.ku8MBSuccess)
          {
            for (int y = 0; y < 30; y++) {
              MasterArray[i * 30 + y + (150 * x)] = node.getResponseBuffer(y);
              //Serial.println("Value of register#" + String(i * 30 + y + (150 * x)) + ": " + String(MasterArray[i * 30 + y + (150 * x)]));
            }
          }
          delay(50);
        }

      }
    }




    else { //SIMULATION
      for (int x = 0; x < StringSize; x++) {
        for (int i = 0; i < ModuleSize; i++) {

          MasterArray[i + ModuleSize * 0 + ModuleSize * 10 * x] = random(5200, 5400);
          MasterArray[i + ModuleSize * 1 + ModuleSize * 10 * x] = random(1, 10);
          MasterArray[i + ModuleSize * 2 + ModuleSize * 10 * x] = random(200, 300) ;
          MasterArray[i + ModuleSize * 3 + ModuleSize * 10 * x] = random(300, 420);
          MasterArray[i + ModuleSize * 4 + ModuleSize * 10 * x] = random(270, 300);
          MasterArray[i + ModuleSize * 5 + ModuleSize * 10 * x] = random(1, 100);
          MasterArray[i + ModuleSize * 6 + ModuleSize * 10 * x] = random(0, 1);
          MasterArray[i + ModuleSize * 7 + ModuleSize * 10 * x] = random(0, 1);
        }
        MasterArray[ModuleSize * 8 + 0 + ModuleSize * 10 * x] = random(0, 1);
        MasterArray[ModuleSize * 8 + 1 + ModuleSize * 10 * x] = random(0, 1);
        MasterArray[ModuleSize * 8 + 2 + ModuleSize * 10 * x] = random(0, 1);
        MasterArray[ModuleSize * 8 + 3 + ModuleSize * 10 * x] = random(0, 1);
        MasterArray[ModuleSize * 8 + 4 + ModuleSize * 10 * x] = random(0, 1);
        MasterArray[ModuleSize * 8 + 5 + ModuleSize * 10 * x] = random(200, 300);
        MasterArray[ModuleSize * 8 + 6 + ModuleSize * 10 * x] = random(200, 300);
        MasterArray[ModuleSize * 8 + 7 + ModuleSize * 10 * x] = random(7200, 7000);
        MasterArray[ModuleSize * 8 + 8 + ModuleSize * 10 * x] = random(1, 10);
        MasterArray[ModuleSize * 8 + 9 + ModuleSize * 10 * x] = random(1, 10);
        MasterArray[ModuleSize * 9 + 0 + ModuleSize * 10 * x] = random(200, 300);
        MasterArray[ModuleSize * 9 + 1 + ModuleSize * 10 * x] = (random(300, 420) - random(270, 300));
      }
    }



    MasterArray[StringSize * ModuleSize * 10] = MaxCurrent;
    MasterArray[StringSize * ModuleSize * 10 + 1] = ChargePercentage * 100;
    MasterArray[StringSize * ModuleSize * 10 + 2] = DischargePercentage * 100;
    MasterArray[StringSize * ModuleSize * 10 + 3] = VoltagePercentage * 100;
    MasterArray[StringSize * ModuleSize * 10 + 4] = CalculatedChargeCurrent;
    MasterArray[StringSize * ModuleSize * 10 + 5] = CalculatedDischargeCurrent;



    String1Charge = MasterArray[ModuleSize * 6 + (ModuleSize * 10) * 0];
    String2Charge = MasterArray[ModuleSize * 6 + (ModuleSize * 10) * 1];
    String3Charge = MasterArray[ModuleSize * 6 + (ModuleSize * 10) * 2];
    String4Charge = MasterArray[ModuleSize * 6 + (ModuleSize * 10) * 3];
    ChargePercentage = (String1Charge * 33 + String2Charge * 33 + String3Charge * 33) * 0.01;


    String1Discharge = MasterArray[ModuleSize * 7 + (ModuleSize * 10) * 0];
    String2Discharge = MasterArray[ModuleSize * 7 + (ModuleSize * 10) * 1];
    String3Discharge = MasterArray[ModuleSize * 7 + (ModuleSize * 10) * 2];
    DischargePercentage = (String1Discharge * 33 + String2Discharge * 33 + String3Discharge * 33) * 0.01;




    String1SOC = MasterArray[ModuleSize * 5 + (ModuleSize * 10) * 0];
    String2SOC = MasterArray[ModuleSize * 5 + (ModuleSize * 10) * 1];
    String3SOC = MasterArray[ModuleSize * 5 + (ModuleSize * 10) * 2];

    SystemSOC = (String1SOC * 0.1 + String2SOC * 0.1 + String3SOC * 0.1) / 3;








    String1Volt = 0;
    for (int i = 0; i < (ModuleSize * 1 + (ModuleSize * 10) * 0); i++) {
      String1Volt = MasterArray[i] + String1Volt;
    }

    String2Volt = 0;
    for (int i = (ModuleSize * 10) * 1; i < (ModuleSize * 1 + (ModuleSize * 10) * 1); i++) {
      String2Volt = MasterArray[i] + String2Volt;
    }

    String3Volt = 0;
    for (int i = (ModuleSize * 10) * 2; i < (ModuleSize * 1 + (ModuleSize * 10) * 2); i++) {
      String3Volt = MasterArray[i] + String3Volt;
    }

    String4Volt = 0;
    for (int i = (ModuleSize * 10) * 3; i < (ModuleSize * 1 + (ModuleSize * 10) * 3); i++) {
      String4Volt = MasterArray[i] + String4Volt;
    }

    SystemVolt = (String1Volt + String2Volt + String3Volt) / 3;



    String1Current = 0;
    for (int i = (ModuleSize * 1 + (ModuleSize * 10) * 0); i < (ModuleSize * 2 + (ModuleSize * 10) * 0); i++) {
      String1Current = MasterArray[i] + String1Current;
    }
    String1Current = String1Current / ModuleSize;

    String2Current = 0;
    for (int i = (ModuleSize * 1 + (ModuleSize * 10) * 1); i < (ModuleSize * 2 + (ModuleSize * 10) * 1); i++) {
      String2Current = MasterArray[i] + String2Current;
    }
    String2Current = String2Current / ModuleSize;

    String3Current = 0;
    for (int i = (ModuleSize * 1 + (ModuleSize * 10) * 2); i < (ModuleSize * 2 + (ModuleSize * 10) * 2); i++) {
      String3Current = MasterArray[i] + String3Current;
    }
    String3Current = String3Current / ModuleSize;

    String4Current = 0;
    for (int i = (ModuleSize * 1 + (ModuleSize * 10) * 3); i < (ModuleSize * 2 + (ModuleSize * 10) * 3); i++) {
      String4Current = MasterArray[i] + String4Current;
    }
    String4Current = String4Current / ModuleSize;


    SystemCurrent = String1Current + String2Current + String3Current;


    String1MaxCell = 0;
    for (int i = (ModuleSize * 3 + (ModuleSize * 10) * 0); i < (ModuleSize * 4 + (ModuleSize * 10) * 0); i++) {
      if (MasterArray[i] > String1MaxCell) {
        // Serial.println(MasterArray[i]);
        String1MaxCell = MasterArray[i];
      }
    }
    //Serial.println("----");

    String2MaxCell = 0;
    // Serial.println("----");
    for (int i = (ModuleSize * 3 + (ModuleSize * 10) * 1); i < (ModuleSize * 4 + (ModuleSize * 10) * 1); i++) {
      if (MasterArray[i] > String2MaxCell) {
        //  Serial.println(MasterArray[i]);
        String2MaxCell = MasterArray[i];
      }
    }
    // Serial.println("----");

    String3MaxCell = 0;
    for (int i = (ModuleSize * 3 + (ModuleSize * 10) * 2); i < (ModuleSize * 4 + (ModuleSize * 10) * 2); i++) {
      if (MasterArray[i] > String3MaxCell) {
        String3MaxCell = MasterArray[i];
      }
    }

    String4MaxCell = 0;
    for (int i = (ModuleSize * 3 + (ModuleSize * 10) * 3); i < (ModuleSize * 4 + (ModuleSize * 10) * 3); i++) {
      if (MasterArray[i] > String4MaxCell) {
        String4MaxCell = MasterArray[i];
      }
    }

    SystemMaxCells[0] = String1MaxCell;
    SystemMaxCells[1] = String2MaxCell;
    SystemMaxCells[2] = String3MaxCell;
    SystemMaxCells[3] = String4MaxCell;



    SystemMaxCell = 0;
    for (int i = 0; i < 3; i++) {
      if (SystemMaxCells[i] > SystemMaxCell) {
        SystemMaxCell = SystemMaxCells[i];
      }
    }


    String1MinCell = 500;
    for (int i = (ModuleSize * 4 + (ModuleSize * 10) * 0); i < (ModuleSize * 5 + (ModuleSize * 10) * 0); i++) {
      if (MasterArray[i] < String1MinCell) {
        String1MinCell = MasterArray[i];
      }
    }

    String2MinCell = 500;
    for (int i = (ModuleSize * 4 + (ModuleSize * 10) * 1); i < (ModuleSize * 5 + (ModuleSize * 10) * 1); i++) {
      if (MasterArray[i] < String2MinCell) {
        String2MinCell = MasterArray[i];
      }
    }

    String3MinCell = 500;
    for (int i = (ModuleSize * 4 + (ModuleSize * 10) * 2); i < (ModuleSize * 5 + (ModuleSize * 10) * 2); i++) {
      if (MasterArray[i] < String3MinCell) {
        String3MinCell = MasterArray[i];
      }
    }


    String4MinCell = 500;
    for (int i = (ModuleSize * 4 + (ModuleSize * 10) * 3); i < (ModuleSize * 5 + (ModuleSize * 10) * 3); i++) {
      if (MasterArray[i] < String4MinCell) {
        String4MinCell = MasterArray[i];
      }
    }

    SystemMinCells[0] = String1MinCell;
    SystemMinCells[1] = String2MinCell;
    SystemMinCells[2] = String3MinCell;
    SystemMinCells[3] = String4MinCell;


    SystemMinCell = 500;
    for (int i = 0; i < 3; i++) {
      if (SystemMinCells[i] < SystemMinCell) {
        SystemMinCell = SystemMinCells[i];
      }
    }

    delay(50);
  }
}


void MODBUSTASK_SLAVE_CODE( void * pvParameters ) {
  Serial.println("MODBUS SLAVE TASK STARTED");

  baudbaud = 9600;
  //COMMENT IF YOU WANT DEBUG
  Modbus slave(SubID, Serial2, GEN_RE);
  slave.start();


  for (;;) {



    //COMMENT IF YOU WANT DEBUG
    slave.poll(MasterArray, ModuleSize * 10 * StringSize);


    if (CANSIM) {
      for (int i = 0; i < ModuleSize * 10 * StringSize; i++) {
        Serial.println("Register:" + String(i) + ":" + String(MasterArray[i]));
      }
    }
    delay(50);
  }
}



void BT_CODE( void * pvParameters ) {

  String message = "";
  int param_start = 0;
  int param_end = 0;
  String BTProcessor;
  int param_start2 = 0;
  int param_end2 = 0;
  String BTProcessor2;

  String BT_STRING;
  String BT_REMAINING;



  String BT_STATION = "EN-MASTER-UNIFIED-#" + String(SubID);

  BluetoothSerial SerialBT;
  SerialBT.begin(BT_STATION.c_str()); //Bluetooth device name
  Serial.println(F("The device started, now you can pair it with bluetooth!"));

  for (;;) {

    if (SerialBT.available()) {
      char incomingChar = SerialBT.read();
      if (incomingChar != '\n') {
        message += String(incomingChar);
      }
      else {
        message = "";
      }


      if (message != "") {
        Serial.println(message);

      }




      //GET StringSize NUMBER ////////////////////////////////
      param_start2 = message.indexOf("GETSTS");
      param_end2 = message.indexOf("#");
      if (param_start2 != -1 && param_end2 != -1) {
        SerialBT.println("StringSize:" + String(StringSize));
        message = "";
      }

      //SET StringSize NUMBER ////////////////////////////////
      param_start2 = message.indexOf("SETSTS");
      param_end2 = message.indexOf("#");
      if (param_start2 != -1 && param_end2 != -1) {
        BTProcessor2 = message.substring(param_start2 + 6, param_end2);
        StringSize = BTProcessor2.toInt();
        SerialBT.println("StringSize:" + String(StringSize));
        message = "";

        preferences.begin("my-app", false);
        preferences.putInt("StringSize", StringSize);
        preferences.end();
      }

      //GET ModuleSize NUMBER ////////////////////////////////
      param_start2 = message.indexOf("GETMDS");
      param_end2 = message.indexOf("#");
      if (param_start2 != -1 && param_end2 != -1) {
        SerialBT.println("ModuleSize:" + String(ModuleSize));
        message = "";
      }

      //SET ModuleSize NUMBER ////////////////////////////////
      param_start2 = message.indexOf("SETMDS");
      param_end2 = message.indexOf("#");
      if (param_start2 != -1 && param_end2 != -1) {
        BTProcessor2 = message.substring(param_start2 + 6, param_end2);
        ModuleSize = BTProcessor2.toInt();
        SerialBT.println("ModuleSize:" + String(ModuleSize));
        message = "";

        preferences.begin("my-app", false);
        preferences.putInt("ModuleSize", ModuleSize);
        preferences.end();
      }


      //SET ID NUMBER ////////////////////////////////
      param_start2 = message.indexOf("SETID");
      param_end2 = message.indexOf("#");
      if (param_start2 != -1 && param_end2 != -1) {
        BTProcessor2 = message.substring(param_start2 + 5, param_end2);
        SubID = BTProcessor2.toInt();
        SerialBT.println("ID:" + String(SubID));
        message = "";

        preferences.begin("my-app", false);
        preferences.putInt("SubID", SubID);
        preferences.end();
        SerialBT.println("System will restart");
        delay(1000);

        ESP.restart();
      }


      //GET ID NUMBER ////////////////////////////////
      param_start2 = message.indexOf("GETID");
      param_end2 = message.indexOf("#");
      if (param_start2 != -1 && param_end2 != -1) {
        SerialBT.println("ID:" + String(SubID));
        message = "";
      }

      //GET INVERTER ////////////////////////////////
      param_start2 = message.indexOf("GETINV");
      param_end2 = message.indexOf("#");
      if (param_start2 != -1 && param_end2 != -1) {
        SerialBT.println("MAXCELL:" + String(String1MaxCellModbus) + "/MINCELL" + String(String1MinCellModbus) + "/SOC" + String(SystemSOC) +
                         "/SystemVolt" + String(SystemVolt * 0.1) + "/SystemCurrent" + String(SystemCurrent * 0.1) +
                         "/CalculatedChargeCurrent" + String(CalculatedChargeCurrent) + "/CalculatedDischargeCurrent" + String(CalculatedDischargeCurrent) + "/THR" + String(Throttle) + "#");
        message = "";
      }


      //SET MAX CURRENT//////////

      param_start2 = message.indexOf("SETMAXCURRENT");
      param_end2 = message.indexOf("#");
      if (param_start2 != -1 && param_end2 != -1) {
        BTProcessor2 = message.substring(param_start2 + 13, param_end2);
        MaxCurrent = BTProcessor2.toInt();
        SerialBT.println("MaxCurrent:" + String(MaxCurrent));
        message = "";

        preferences.begin("my-app", false);
        preferences.putInt("MaxCurrent", MaxCurrent);
        preferences.end();
      }


      //GET MAXCURRENT ////////////////////////////////
      param_start2 = message.indexOf("GETMAXCURRENT");
      param_end2 = message.indexOf("#");
      if (param_start2 != -1 && param_end2 != -1) {
        SerialBT.println("MAX CURRENT:" + String(MaxCurrent));
        message = "";
      }


      //SET MAX voltage//////////

      param_start2 = message.indexOf("SETMAXVOLTAGE");
      param_end2 = message.indexOf("#");
      if (param_start2 != -1 && param_end2 != -1) {
        BTProcessor2 = message.substring(param_start2 + 13, param_end2);
        MaxStringVolt = BTProcessor2.toInt();
        SerialBT.println("MaxStringVolt:" + String(MaxStringVolt));
        message = "";

        preferences.begin("my-app", false);
        preferences.putInt("MaxStringVolt", MaxStringVolt);
        preferences.end();
      }


      //GET MAXVOLTAGE ////////////////////////////////
      param_start2 = message.indexOf("GETMAXVOLTAGE");
      param_end2 = message.indexOf("#");
      if (param_start2 != -1 && param_end2 != -1) {
        SerialBT.println("MaxStringVolt:" + String(MaxStringVolt));
        message = "";
      }



      //GET chargeCURRENT ////////////////////////////////
      param_start2 = message.indexOf("GETCHARGECURRENT");
      param_end2 = message.indexOf("#");
      if (param_start2 != -1 && param_end2 != -1) {
        SerialBT.println("MAX CHARGE CURRENT:" + String(CalculatedChargeCurrent));
        message = "";
      }


      //GET dischargeCURRENT ////////////////////////////////
      param_start2 = message.indexOf("GETDISCHARGECURRENT");
      param_end2 = message.indexOf("#");
      if (param_start2 != -1 && param_end2 != -1) {
        SerialBT.println("MAX DISCHARGE CURRENT:" + String(CalculatedDischargeCurrent));
        message = "";
      }

    }

  }

}


void SERIALMONITOR_CODE( void * pvParameters ) {

  for (;;) {
    recvOneChar();
    showNewData();
    // Settings();
    delay(1000);
  }
}

void recvOneChar() {
  if (Serial.available() > 0) {
    receivedString = Serial.readStringUntil('\n');
    newData = true;
    delay(100);
  }
}

void showNewData() {
  if (newData == true) {
    Serial.println(receivedString);
    analyseNewData();
    newData = false;
    delay(100);
  }
}


void analyseNewData() {


  if (receivedString == "-SHOWSUMMARY") {

    Serial.println();
    Serial.println("   ---   SYSTEM SUMMARY START   ---   ");
    Serial.println();
    Serial.println("         SubID:" + String(SubID) );
    Serial.println("         ModuleSize:" + String(ModuleSize));
    Serial.println("         StringSize:" + String(StringSize));
    Serial.println("         Simulation Status:" + String(CANSIM));
    Serial.println("         MaxStringVolt:" + String(MaxStringVolt));
    Serial.println("     MaxCurrent:" + String(MaxCurrent));

    Serial.println();

    Serial.println("   ---   SYSTEM SUMMARY END  ---   ");
    Serial.println();
  }



  else if (receivedString.indexOf("SETID") != -1 && receivedString.indexOf("#") != -1) {
    SerialProcessor = receivedString.substring(receivedString.indexOf("SETID") + 5, receivedString.indexOf("#"));
    SubID = SerialProcessor.toInt();
    Serial.println("ID:" + String(SubID));
    receivedString = "";
    preferences.begin("my-app", false);
    preferences.putInt("SubID", SubID);
    preferences.end();
    Serial.println("System will restart");
    delay(1000);
    ESP.restart();
  }

  else if (receivedString.indexOf("SETMDS") != -1 && receivedString.indexOf("#") != -1) {
    SerialProcessor = receivedString.substring(receivedString.indexOf("SETMDS") + 6, receivedString.indexOf("#"));
    ModuleSize = SerialProcessor.toInt();
    Serial.println("ModuleSize:" + String(ModuleSize));
    receivedString = "";
    preferences.begin("my-app", false);
    preferences.putInt("ModuleSize", ModuleSize);
    preferences.end();
  }


  else if (receivedString.indexOf("SETSTS") != -1 && receivedString.indexOf("#") != -1) {
    SerialProcessor = receivedString.substring(receivedString.indexOf("SETSTS") + 6, receivedString.indexOf("#"));
    StringSize = SerialProcessor.toInt();
    Serial.println("StringSize:" + String(StringSize));
    receivedString = "";
    preferences.begin("my-app", false);
    preferences.putInt("StringSize", StringSize);
    preferences.end();
  }

  else if (receivedString.indexOf("CANSIM") != -1 && receivedString.indexOf("#") != -1) {
    SerialProcessor = receivedString.substring(receivedString.indexOf("CANSIM") + 6, receivedString.indexOf("#"));
    CANSIM = SerialProcessor.toInt();
    Serial.println("CANSIM:" + String(CANSIM));
    receivedString = "";
    preferences.begin("my-app", false);
    preferences.putInt("CANSIM", CANSIM);
    preferences.end();
  }

  else if (receivedString.indexOf("GETINV") != -1 && receivedString.indexOf("#") != -1) {
    SerialProcessor = receivedString.substring(receivedString.indexOf("GETINV") + 6, receivedString.indexOf("#"));
    Serial.println("MAXCELL:" + String(SystemMaxCell * 0.01) + "/MINCELL" + String(SystemMinCell * 0.01) + "/SOC" + String(SystemSOC) +
                   "/SystemVolt" + String(SystemVolt * 0.1) + "/SystemCurrent" + String(SystemCurrent * 0.1) +
                   "/CalculatedChargeCurrent" + String(CalculatedChargeCurrent) + "/CalculatedDischargeCurrent" + String(CalculatedDischargeCurrent));
    receivedString = "";
  }



}


void loop() {



}
