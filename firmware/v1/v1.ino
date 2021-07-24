#include <ADS1115_WE.h>
#include<Wire.h>
#include <ArduinoQueue.h> // https://github.com/EinarArnason/ArduinoQueue
#include <SD.h>


#define I2C_ADDRESS 0x48
#define QUEUE_SIZE  64
#define TRIGGER_FRAME_COUNTER_INIT 25

ADS1115_WE adc = ADS1115_WE(I2C_ADDRESS);
ArduinoQueue<float> readingQueue(QUEUE_SIZE); // approx one second.
float trigger_voltage_mV = 30; // will save data if exceeded
int trigger_counter = TRIGGER_FRAME_COUNTER_INIT; //~0.1 sec.
bool trigger_activated;

/*
 *  Geophone docs: http://cdn.sparkfun.com/datasheets/Sensors/Accelerometers/SM-24%20Brochure.pdf
 * 
 */

unsigned long int frame_counter;

void setup() {
  
  Wire.begin();
  Serial.begin(9600);
  if(!adc.init()){
    Serial.println("ADS1115 not connected!");
  }
  else
  {
    Serial.println("ADS115 connected");
  }

  Serial.print("Initializing SD card...");
  if (!SD.begin(10)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");
  
  
  adc.setVoltageRange_mV(ADS1115_RANGE_6144); //comment line/change parameter to change range 
  //adc.setVoltageRange_mV(ADS1115_RANGE_4096); //comment line/change parameter to change range
  //adc.setVoltageRange_mV(ADS1115_RANGE_1024);
  //adc.setVoltageRange_mV(ADS1115_RANGE_0512);
  frame_counter = 0;
  trigger_activated = false;
}

void loop() {
  float voltage = 0.0;
  
 // Serial.print("0: ");
  voltage = readChannel(ADS1115_COMP_0_1);
  readingQueue.enqueue(voltage);
  if (fabs(voltage) > trigger_voltage_mV && trigger_activated == false)  
  {
    trigger_activated = true;
    //Serial.println("Trigger activated");
  }  

  if (trigger_activated) {
    if (trigger_counter > 0)
     trigger_counter--;   
     
    if (trigger_counter == 0 && readingQueue.isFull())
    {
        trigger_counter = TRIGGER_FRAME_COUNTER_INIT; 
        trigger_activated = false;
        saveData(); 
    }
  }
  //Serial.println(readingQueue.itemCount());
  
  //Serial.println(voltage);
  //delay(10);  
  
  frame_counter++;
  if (frame_counter % 1000 == 0)
  {
   // Serial.println((float) 1000* frame_counter/millis());
  }  
  
}

float readChannel(ADS1115_MUX channel) {
  float voltage = 0.0;
  adc.setCompareChannels(channel);
  adc.startSingleMeasurement();
  while(adc.isBusy()){}
  voltage = adc.getResult_mV(); // alternative: getResult_mV for Millivolt
  return voltage;
}

// save queue to SD card
void saveData() {
  char file_name[16];
  int idx = getAvailableFileNameIdx();

  sprintf(file_name, "log_%d.txt", idx);  
  Serial.println("Saving data to");
  Serial.println(file_name);
  
  File logFile = SD.open(file_name, FILE_WRITE);
  while (!readingQueue.isEmpty())
  {
    float x = readingQueue.dequeue();   
    logFile.println(x);
  }

  logFile.close();  
  
}

int getAvailableFileNameIdx()
{ 
 char file_name[16];
 int idx = 0;

 while (1)
 {
  sprintf(file_name, "log_%d.txt", idx);
  File myFile = SD.open(file_name);
  if (!myFile) {
    myFile.close();
    return idx;
  }
  myFile.close(); 
  idx++;
 } 
 
}
