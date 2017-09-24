#include "usb_datachannel.h"

void setup() {
  // put your setup code here, to run once:
  delay(100);
  Serial.begin(115200);
  delay(100);
  Serial.println("setup");

  USBDataChannel usbdevice;
  usbdevice.begin();
}

void loop() {
  // put your main code here, to run repeatedly:

}
