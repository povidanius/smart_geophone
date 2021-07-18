#include <ADS1115_WE.h>
#include<Wire.h>
#define I2C_ADDRESS 0x48

ADS1115_WE adc = ADS1115_WE(I2C_ADDRESS);

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
  adc.setVoltageRange_mV(ADS1115_RANGE_6144); //comment line/change parameter to change range
}

void loop() {
  float voltage = 0.0;
  Serial.print("0: ");
  voltage = readChannel(ADS1115_COMP_0_1);
  Serial.println(voltage);
  //delay(10);
}

float readChannel(ADS1115_MUX channel) {
  float voltage = 0.0;
  adc.setCompareChannels(channel);
  adc.startSingleMeasurement();
  while(adc.isBusy()){}
  voltage = adc.getResult_mV(); // alternative: getResult_mV for Millivolt
  return voltage;
}