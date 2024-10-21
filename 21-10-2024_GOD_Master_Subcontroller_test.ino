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
int SOCOverlook = 80;
int MaxCurrent = 100;
int MaxStringVolt = 817;
int CalculatedChargeCurrent = 0;
int CalculatedDischargeCurrent = 0;
float String1Volt = 0;
float String2Volt = 0;
float String3Volt = 0;
float String4Volt = 0;
float SystemVolt = 0;
String InverterStatus1 = "None";
String InverterStatus2 = "None";
String InverterStatus3 = "None";
String InverterStatus4 = "None";

float String1Current = 0;
float String2Current = 0;
float String3Current = 0;
float String4Current = 0;
float SystemCurrent = 0;
float Throttle = 100;


//INVERTER LIMIT VALUES
float INVTempLim = 50;
float INVMaxCellLim = 50;
float INVSOCLim = 50;
float INVSOCLimMax = 99;


bool TemperatureRule = false;
bool CellVoltageRule = false;
bool MaxSOCRule = false;
bool MinSOCRule = false;
bool SOCOverlookRule = false;

int MaxSystemVoltageRule = 100;


float StringVolt = 0;
float StringCurrent = 0;
float StringMaxVolt = 0;
float StringMinVolt = 0;

float StringTemp = 0;



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

float VoltagePercentage = 1;

float String1SOC = 0;
float String2SOC = 0;
float String3SOC = 0;
float String4SOC = 0;
float SystemSOC = 0;
float SystemTemp = 0;

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

float StringChargeArray[8] = {1, 1, 1, 1, 1, 1, 1, 1};
float StringDischargeArray[8] = {1, 1, 1, 1, 1, 1, 1, 1};
float StringSOCArray[8];
float StringVoltageArray[8];
float StringCurrentArray[8];
float StringMaxVoltArray[8];
float StringMinVoltArray[8];
float StringTempArray[8];

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
String FirmwareVer = "21-10-2024 Ver1";


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


//CHATGPT//////////////////

int *numericArray;   // Pointer for numeric array
bool *booleanArray;  // Pointer for boolean array
int arraySize;       // Variable to store array size

void initializeArrays(int size) {
  arraySize = size;  // Set the array size based on user input
  numericArray = new int[arraySize];  // Dynamically allocate memory for numeric array
  booleanArray = new bool[arraySize]; // Dynamically allocate memory for boolean array
}

void generateStringVoltages() {
  for (int i = 0; i < arraySize; i++) {
    numericArray[i] = StringVoltageArray[i];  // Generates random numbers between 0 and 99
  }
}

int findLowestNumberIndex() {
  int minIndex = 0;
  for (int i = 1; i < arraySize; i++) {
    if (numericArray[i] < numericArray[minIndex]) {
      minIndex = i;
    }
  }
  return minIndex;
}

void updateBooleanArray(int lowestIndex) {
  int lowestValue = numericArray[lowestIndex];

  for (int i = 0; i < arraySize; i++) {
    if (abs(numericArray[i] - lowestValue) < 3) {
      booleanArray[i] = true;
    } else {
      booleanArray[i] = false;
    }
  }
}

void printArrays() {
  Serial.print("Numeric Array: ");
  for (int i = 0; i < arraySize; i++) {
    Serial.print(numericArray[i]);
    Serial.print(" ");
  }
  Serial.println();

  Serial.print("Boolean Array: ");
  for (int i = 0; i < arraySize; i++) {
    Serial.print(booleanArray[i]);
    Serial.print(" ");
  }
  Serial.println();
}


/////////////////////////

void setup() {






  //COMMENT IF YOU WANT DEBUG
  RXS = 3;
  TXS = 1;
  en_rs485 = 5;





  //  COMMENT IF YOU WANT MODBUS
  //  Serial.begin(115200);

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

  INVTempLim = preferences.getFloat("INVTempLim", 50);
  INVMaxCellLim = preferences.getFloat("INVMaxCellLim", 3.5);
  INVSOCLim = preferences.getFloat("INVSOCLim", 40);
  INVSOCLimMax = preferences.getFloat("INVSOCLimMax", 99);

  SOCOverlook = preferences.getFloat("SOCOverlook", 80);



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


  // User input for array size (you can set this value dynamically as needed)
  int userArraySize = StringSize;  // Example size, this can be set to any desired value
  initializeArrays(userArraySize);  // Initialize arrays with the user-defined size


}



void CANBUSTASK_INVERTER_CODE( void * pvParameters ) {
  Serial.println("CANBUSTASK_INVERTER TASK STARTED");



  can_start(250);
  for (;;) {

    if (SystemMaxCell * 0.01 < 4.7 && SystemMaxCell * 0.01 > 2.1) {
      String1MaxCellModbus = SystemMaxCell * 0.01;
    }

    if (SystemMinCell * 0.01 < 4.7 && SystemMinCell * 0.01 > 2.2) {
      String1MinCellModbus = SystemMinCell * 0.01;
    }


    //inverter algorithm starts here!!/////////////////////////////////////////////


    TemperatureRule = SystemTemp < INVTempLim;
    CellVoltageRule = String1MaxCellModbus < INVMaxCellLim;
    MaxSOCRule = SystemSOC < INVSOCLimMax;
    MinSOCRule = SystemSOC > INVSOCLim;
    SOCOverlookRule = SystemSOC > SOCOverlook;

    if (SOCOverlookRule) {

      if ((SystemVolt * 0.1 < MaxStringVolt * 0.980 )) {
        MaxSystemVoltageRule = 100;
      }
      else if (SystemVolt * 0.1 >= MaxStringVolt * 0.980 && SystemVolt * 0.1 < MaxStringVolt * 0.985) {
        MaxSystemVoltageRule = 50;
      }
      else if (SystemVolt * 0.1 >= MaxStringVolt * 0.985 && SystemVolt * 0.1 < MaxStringVolt * 0.990) {
        MaxSystemVoltageRule = 25;
      }
      else if (SystemVolt * 0.1 >= MaxStringVolt * 0.99 && SystemVolt * 0.1 < MaxStringVolt * 0.995) {
        MaxSystemVoltageRule = 10;
      }
      else if (SystemVolt * 0.1 >= MaxStringVolt * 0.995 && SystemVolt * 0.1 < MaxStringVolt * 1) {
        MaxSystemVoltageRule = 5;
      }
      else if (SystemVolt * 0.1 >= MaxStringVolt) {
        MaxSystemVoltageRule = 1;
      }
      else {
        MaxSystemVoltageRule = 0;
      }
    }
    else {
      MaxSystemVoltageRule = 100;
    }


    CalculatedChargeCurrent = MaxCurrent * ChargePercentage * MaxSystemVoltageRule * 0.01 * MaxSOCRule;
    CalculatedDischargeCurrent = MaxCurrent * DischargePercentage * 100 * 0.01 * MinSOCRule;


    if (String1MaxCellModbus < 4.4  && String1MinCellModbus < 4.7 && String1MaxCellModbus > 1.8  && String1MinCellModbus > 1.8) {
      set_maxvoltage(String1MaxCellModbus, String1MinCellModbus, SystemSOC, SystemSOC, 0, 0x180150F1);
      delay(10);
      set_totalvoltage(SystemVolt * 0.1 , SystemCurrent * 0.1 , CalculatedChargeCurrent, CalculatedDischargeCurrent, 0x180250F1);
      delay(10);
      set_groupnumber(1, 3, 4, 1, 3, 25, 0x180350F1);
      delay(100);
      set_groupnumbermin(2, 4, 6, 2, 3, 25,  0x180450F1);
      delay(100);
      set_warnings(4, 0, 0, 0, 0 , 0x180650F1);
      delay(100);
      set_warnings2(0, 0, 0, 0, 0, 0x180750F1);
      delay(100);
    }
    else { //ERROR CASE
      set_maxvoltage(3, 3, 3, 3, 0, 0x180150F1);
      delay(10);
      set_totalvoltage(333 , 333, 333, 333, 0x180250F1);
      delay(10);
      set_groupnumber(1, 3, 4, 1, 3, 25, 0x180350F1);
      delay(100);
      set_groupnumbermin(2, 4, 6, 2, 3, 25,  0x180450F1);
      delay(100);
      set_warnings(4, 0, 0, 0, 0 , 0x180650F1);
      delay(100);
      set_warnings2(0, 0, 0, 0, 0, 0x180750F1);
      delay(100);
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
        Serial.println("String no:" + String(x + 1));
        delay(50);

        for (int i = 0; i < (10 * ModuleSize) / 10; i++) {

          result = node.readHoldingRegisters(i * 10, 10);
          if (result == node.ku8MBSuccess)
          {
            for (int y = 0; y < 10; y++) {
              MasterArray[i * 10 + y + (10 * ModuleSize * x)] = node.getResponseBuffer(y);
              Serial.println("Value of register#" + String(i * 10 + y + (10 * ModuleSize * x)) + ": " + String(MasterArray[i * 10 + y + (10 * ModuleSize * x)]));
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

    for (int i = 0; i < StringSize; i++) {
      for (int x = 0; x < ModuleSize; x++) {
        StringChargeArray[i] = MasterArray[(ModuleSize * 6 + (ModuleSize * 10) * i) + x] && StringChargeArray[i];
      }

      for (int x = 0; x < ModuleSize; x++) {
        StringDischargeArray[i] = MasterArray[(ModuleSize * 7 + (ModuleSize * 10) * i) + x] && StringDischargeArray[i];
      }

      StringSOCArray[i] = MasterArray[ModuleSize * 5 + (ModuleSize * 10) * i];

      StringVolt = 0;
      for (int x = (ModuleSize * 10) * i; x < (ModuleSize * 1 + (ModuleSize * 10) * i); x++) {
        StringVolt = MasterArray[x] + StringVolt;
      }
      StringVoltageArray[i] = StringVolt;


      StringCurrent = 0;
      for (int x = (ModuleSize * 1 + (ModuleSize * 10) * i); x < (ModuleSize * 2 + (ModuleSize * 10) * i); x++) {
        StringCurrent = U32toI16(MasterArray[x]) + StringCurrent;
      }
      StringCurrentArray[i] = StringCurrent / ModuleSize;

      ////lllllllllllllllllll

      StringTemp = 0;
      for (int x = (ModuleSize * 2 + (ModuleSize * 10) * i); x < (ModuleSize * 3 + (ModuleSize * 10) * i); x++) {
        StringTemp = MasterArray[x] + StringTemp;
      }
      StringTempArray[i] = StringTemp / ModuleSize;


      StringMaxVolt = 0;
      for (int x = (ModuleSize * 3 + (ModuleSize * 10) * i); x < (ModuleSize * 4 + (ModuleSize * 10) * i); x++) {
        if (MasterArray[x] > StringMaxVolt) {
          // Serial.println(MasterArray[i]);
          StringMaxVolt = MasterArray[x];
        }
      }
      StringMaxVoltArray[i] = StringMaxVolt;


      StringMinVolt = 500;
      for (int x = (ModuleSize * 4 + (ModuleSize * 10) * i); x < (ModuleSize * 5 + (ModuleSize * 10) * i); x++) {
        if (MasterArray[x] < StringMinVolt) {
          StringMinVolt = MasterArray[x];
        }
      }
      StringMinVoltArray[i] = StringMinVolt;

    }





    ChargePercentage = getArrayAverage(StringChargeArray, StringSize);
    DischargePercentage = getArrayAverage(StringDischargeArray, StringSize);
    SystemSOC = getArrayAverage(StringSOCArray, StringSize) * 0.1;
    SystemTemp = getArrayAverage(StringTempArray, StringSize) * 0.1;
    SystemVolt = getArrayAverage(StringVoltageArray, StringSize);
    SystemCurrent = abs(sumOfArray(StringCurrentArray, StringSize));






    SystemMaxCell = findMaxValue(StringMaxVoltArray, StringSize);
    SystemMinCell = findMaxValue(StringMinVoltArray, StringSize);

    generateStringVoltages();
    // Find the lowest number
    int lowestIndex = findLowestNumberIndex();

    // Update boolean array: mark all indices where the difference with the lowest is < 3
    updateBooleanArray(lowestIndex);

    printArrays();
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

  int StringNo = 0;

  String BT_STATION = "ENMASTER " + FirmwareVer + "_" + String(SubID);

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

      //GET STATUS ////////////////////////////////
      param_start2 = message.indexOf("GETSTATUS");
      param_end2 = message.indexOf("#");
      if (param_start2 != -1 && param_end2 != -1) {
        SerialBT.println(InverterStatus1 + "/" + InverterStatus2 + "/" + InverterStatus3 + "/" + InverterStatus4 + "#");
        message = "";
      }

      //GET FIRMWARE ////////////////////////////////
      param_start2 = message.indexOf("GETFIRMWARE");
      param_end2 = message.indexOf("#");
      if (param_start2 != -1 && param_end2 != -1) {
        SerialBT.println(FirmwareVer);
        message = "";
      }



      //GETLIMITS ////////////////////////////////
      param_start2 = message.indexOf("GETLIMITS");
      param_end2 = message.indexOf("#");
      if (param_start2 != -1 && param_end2 != -1) {

        SerialBT.println("TemperatureRule:" + String(TemperatureRule) +
                         "/CellVoltageRule:" + String(CellVoltageRule) +
                         "/MaxSOCRule:" + String(MaxSOCRule) +
                         "/MinSOCRule:" + String(MinSOCRule) +
                         "/SOCOverlookRule:" + String(SOCOverlookRule) +
                         "/MaxSystemVoltageRule:" + String(MaxSystemVoltageRule) +
                         "/ChargePercentage:" + String(ChargePercentage) +
                         "/DischargePercentage:" + String(DischargePercentage) +
                         "/MaxCurrent:" + String(MaxCurrent) +
                         "/CalculatedChargeCurrent:" + String(CalculatedChargeCurrent) +
                         "/CalculatedDischargeCurrent:" + String(CalculatedDischargeCurrent)
                        );
        message = "";
      }

      param_start2 = message.indexOf("GETCURRENTS");
      param_end2 = message.indexOf("#");
      if (param_start2 != -1 && param_end2 != -1) {


        for (int i = 0; i < StringSize; i++) {
          SerialBT.println("String#" + String(i + 1) + ": " + String(StringCurrentArray[i] * 0.1) );
        }
        message = "";
      }

      //GET PERCENTAGE ////////////////////////////////
      param_start2 = message.indexOf("GETPERCENTAGE");
      param_end2 = message.indexOf("#");
      if (param_start2 != -1 && param_end2 != -1) {
        SerialBT.println("ChargePercentage:" + String(ChargePercentage) + "/" + "DischargePercentage:" + String(DischargePercentage) + "#");
        message = "";
      }

      //GET CHARGE CONTACTS ////////////////////////////////
      param_start2 = message.indexOf("GETCC");
      param_end2 = message.indexOf("#");
      if (param_start2 != -1 && param_end2 != -1) {

        SerialBT.println("ChargeContacts:");
        for (int i = 0; i < StringSize; i++) {
          SerialBT.print("Contact#" + String(i + 1) + ":" + String(StringChargeArray[i])) + "/";
        }
        SerialBT.println("#");
        message = "";
      }


      //GET DISCHARGE CONTACTS ////////////////////////////////
      param_start2 = message.indexOf("GETDC");
      param_end2 = message.indexOf("#");
      if (param_start2 != -1 && param_end2 != -1) {

        SerialBT.println("DischargeContacts:");
        for (int i = 0; i < StringSize; i++) {
          SerialBT.print("Contact#" + String(i + 1) + ":" + String(StringDischargeArray[i])) + "/";
        }
        SerialBT.println("#");
        message = "";
      }



      //GET MODULE CHARGE//////////

      param_start2 = message.indexOf("GETSTRINGCHARGE");
      param_end2 = message.indexOf("#");
      if (param_start2 != -1 && param_end2 != -1) {
        BTProcessor2 = message.substring(param_start2 + 15, param_end2);
        StringNo = BTProcessor2.toInt();
        SerialBT.println("String:" + String(StringNo));
        SerialBT.println("ChargeContacts:");

        for (int i = 0; i < ModuleSize; i++) {
          SerialBT.print("Module#" + String(i + 1) + ":" + String(MasterArray[(ModuleSize * 6 + (ModuleSize * 10) * StringNo) + i])) + "/";
        }
        SerialBT.println("#");
        message = "";
      }



      //GET MODULE DISCHARGE//////////

      param_start2 = message.indexOf("GETSTRINGDISCHARGE");
      param_end2 = message.indexOf("#");
      if (param_start2 != -1 && param_end2 != -1) {
        BTProcessor2 = message.substring(param_start2 + 18, param_end2);
        StringNo = BTProcessor2.toInt();
        SerialBT.println("String:" + String(StringNo));
        SerialBT.println("DischargeContacts:");

        for (int i = 0; i < ModuleSize; i++) {
          SerialBT.print("Module#" + String(i + 1) + ":" + String(MasterArray[(ModuleSize * 7 + (ModuleSize * 10) * StringNo) + i])) + "/";
        }
        SerialBT.println("#");
        message = "";
      }

      //mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm


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


      //SET INVTempLim//////////

      param_start2 = message.indexOf("SETINVTEMPLIM");
      param_end2 = message.indexOf("#");
      if (param_start2 != -1 && param_end2 != -1) {
        BTProcessor2 = message.substring(param_start2 + 13, param_end2);
        INVTempLim = BTProcessor2.toFloat();
        SerialBT.println("INVTempLim:" + String(INVTempLim));
        message = "";

        preferences.begin("my-app", false);
        preferences.putFloat("INVTempLim", INVTempLim);
        preferences.end();
      }



      param_start2 = message.indexOf("SETSOCOVERLOOK");
      param_end2 = message.indexOf("#");
      if (param_start2 != -1 && param_end2 != -1) {
        BTProcessor2 = message.substring(param_start2 + 14, param_end2);
        SOCOverlook = BTProcessor2.toFloat();
        SerialBT.println("SOC_Overlook:" + String(SOCOverlook));
        message = "";

        preferences.begin("my-app", false);
        preferences.putFloat("SOCOverlook", SOCOverlook);
        preferences.end();
      }

      //nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn
      //SOCOverlook


      //GET INVTempLim ////////////////////////////////
      param_start2 = message.indexOf("GETINVTEMPLIM");
      param_end2 = message.indexOf("#");
      if (param_start2 != -1 && param_end2 != -1) {
        SerialBT.println("INVTempLim:" + String(INVTempLim));
        message = "";
      }


      //SET INVMaxCellLim//////////

      param_start2 = message.indexOf("SETINVMAXCELL");
      param_end2 = message.indexOf("#");
      if (param_start2 != -1 && param_end2 != -1) {
        BTProcessor2 = message.substring(param_start2 + 13, param_end2);
        INVMaxCellLim = BTProcessor2.toFloat();
        SerialBT.println("INVMaxCellLim:" + String(INVMaxCellLim));
        message = "";

        preferences.begin("my-app", false);
        preferences.putFloat("INVMaxCellLim", INVMaxCellLim);
        preferences.end();
      }


      //GET INVMaxCellLim ////////////////////////////////
      param_start2 = message.indexOf("GETINVMAXCELL");
      param_end2 = message.indexOf("#");
      if (param_start2 != -1 && param_end2 != -1) {
        SerialBT.println("INVMaxCellLim:" + String(INVMaxCellLim));
        message = "";
      }

      //SET INVSOCLim//////////

      param_start2 = message.indexOf("SETINVSOCLIM");
      param_end2 = message.indexOf("#");
      if (param_start2 != -1 && param_end2 != -1) {
        BTProcessor2 = message.substring(param_start2 + 12, param_end2);
        INVSOCLim = BTProcessor2.toFloat();
        SerialBT.println("INVSOCLim:" + String(INVSOCLim));
        message = "";

        preferences.begin("my-app", false);
        preferences.putFloat("INVSOCLim", INVSOCLim);
        preferences.end();
      }

      param_start2 = message.indexOf("SETINVSOCMAX");
      param_end2 = message.indexOf("#");
      if (param_start2 != -1 && param_end2 != -1) {
        BTProcessor2 = message.substring(param_start2 + 12, param_end2);
        INVSOCLimMax = BTProcessor2.toFloat();
        SerialBT.println("INVSOCLimMax:" + String(INVSOCLimMax));
        message = "";

        preferences.begin("my-app", false);
        preferences.putFloat("INVSOCLimMax", INVSOCLimMax);
        preferences.end();
      }


      //GET INVSOCLim ////////////////////////////////
      param_start2 = message.indexOf("GETINVMAXCELL");
      param_end2 = message.indexOf("#");
      if (param_start2 != -1 && param_end2 != -1) {
        SerialBT.println("INVMaxCellLim:" + String(INVMaxCellLim));
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
    Serial.println("         MaxCurrent:" + String(MaxCurrent));

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

float getArrayAverage(float arr[], int length) {
  int sum = 0;
  for (int i = 0; i < length; i++) {
    sum += arr[i];
  }
  return (float)sum / length;
}


void sortArray(float arr[], int length) {
  for (int i = 0; i < length - 1; i++) {
    for (int j = 0; j < length - i - 1; j++) {
      if (arr[j] > arr[j + 1]) {
        // Swap elements if they are in the wrong order
        int temp = arr[j];
        arr[j] = arr[j + 1];
        arr[j + 1] = temp;
      }
    }
  }
}

void printArray(float arr[], int length) {
  for (int i = 0; i < length; i++) {
    Serial.print(arr[i]);
    Serial.print(" ");
  }
  Serial.println();
}

float sumOfArray(float * numericArray, int arraySize) {
  float sum = 0;
  for (int i = 0; i < arraySize; i++) {
    sum += numericArray[i];  // Add each element to the sum
  }
  return sum;  // Return the total sum
}

float findMaxValue(float * numericArray, int arraySize) {
  float maxValue = numericArray[0];  // Assume the first element is the largest initially
  for (int i = 1; i < arraySize; i++) {
    if (numericArray[i] > maxValue) {
      maxValue = numericArray[i];  // Update maxValue if current element is larger
    }
  }
  return maxValue;  // Return the maximum value
}


int16_t U32toI16(uint32_t u32_value) {
  // Extract the lower 16 bits from the 32-bit unsigned value
  uint16_t lower_16 = (uint16_t)(u32_value & 0xFFFF);

  // Check if the most significant bit (15th bit) is 1, indicating a negative number in I16 format
  if (lower_16 & 0x8000) {
    // If it's negative, convert it to a proper signed 16-bit integer
    return (int16_t)(lower_16 | 0xFFFF0000); // Sign extend to negative
  } else {
    // If positive, return the lower 16 bits as is
    return (int16_t)lower_16;
  }
}
