// Jacek Fedorynski <jfedor@jfedor.org>
// http://www.jfedor.org/

#include "HID.h"

// I used an Arduino Pro Micro board, but it should work with any ATmega32U4-based board, just set the right pins below.
// If you want L3/R3, the PS button or analog sticks, you'll need to add them.

#define PIN_UP        4
#define PIN_DOWN      14
#define PIN_LEFT      5
#define PIN_RIGHT     9
#define PIN_CROSS     2
#define PIN_CIRCLE    8
#define PIN_TRIANGLE  3
#define PIN_SQUARE    A0
#define PIN_L1        10
#define PIN_L2        A2
#define PIN_R1        6
#define PIN_R2        A1
#define PIN_SELECT    15
#define PIN_START     16

// HID report descriptor taken from a HORI Fighting Stick V3
// works out of the box with PC and PS3
// as dumped by usbhid-dump and parsed by https://eleccelerator.com/usbdescreqparser/
static const uint8_t hidReportDescriptor[] PROGMEM = {
  0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
  0x09, 0x05,        // Usage (Game Pad)
  0xA1, 0x01,        // Collection (Application)
  0x85, 0x01,        // Report ID, not in the original descriptor, but the Arduino HID library can't seem to live without it
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x01,        //   Logical Maximum (1)
  0x35, 0x00,        //   Physical Minimum (0)
  0x45, 0x01,        //   Physical Maximum (1)
  0x75, 0x01,        //   Report Size (1)
  0x95, 0x0D,        //   Report Count (13)
  0x05, 0x09,        //   Usage Page (Button)
  0x19, 0x01,        //   Usage Minimum (0x01)
  0x29, 0x0D,        //   Usage Maximum (0x0D)
  0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0x95, 0x03,        //   Report Count (3)
  0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
  0x25, 0x07,        //   Logical Maximum (7)
  0x46, 0x3B, 0x01,  //   Physical Maximum (315)
  0x75, 0x04,        //   Report Size (4)
  0x95, 0x01,        //   Report Count (1)
  0x65, 0x14,        //   Unit (System: English Rotation, Length: Centimeter)
  0x09, 0x39,        //   Usage (Hat switch)
  0x81, 0x42,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,Null State)
  0x65, 0x00,        //   Unit (None)
  0x95, 0x01,        //   Report Count (1)
  0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0x26, 0xFF, 0x00,  //   Logical Maximum (255)
  0x46, 0xFF, 0x00,  //   Physical Maximum (255)
  0x09, 0x30,        //   Usage (X)
  0x09, 0x31,        //   Usage (Y)
  0x09, 0x32,        //   Usage (Z)
  0x09, 0x35,        //   Usage (Rz)
  0x75, 0x08,        //   Report Size (8)
  0x95, 0x04,        //   Report Count (4)
  0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0x06, 0x00, 0xFF,  //   Usage Page (Vendor Defined 0xFF00)
  0x09, 0x20,        //   Usage (0x20)
  0x09, 0x21,        //   Usage (0x21)
  0x09, 0x22,        //   Usage (0x22)
  0x09, 0x23,        //   Usage (0x23)
  0x09, 0x24,        //   Usage (0x24)
  0x09, 0x25,        //   Usage (0x25)
  0x09, 0x26,        //   Usage (0x26)
  0x09, 0x27,        //   Usage (0x27)
  0x09, 0x28,        //   Usage (0x28)
  0x09, 0x29,        //   Usage (0x29)
  0x09, 0x2A,        //   Usage (0x2A)
  0x09, 0x2B,        //   Usage (0x2B)
  0x95, 0x0C,        //   Report Count (12)
  0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0x0A, 0x21, 0x26,  //   Usage (0x2621)
  0x95, 0x08,        //   Report Count (8)
  0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  0x0A, 0x21, 0x26,  //   Usage (0x2621)
  0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  0x26, 0xFF, 0x03,  //   Logical Maximum (1023)
  0x46, 0xFF, 0x03,  //   Physical Maximum (1023)
  0x09, 0x2C,        //   Usage (0x2C)
  0x09, 0x2D,        //   Usage (0x2D)
  0x09, 0x2E,        //   Usage (0x2E)
  0x09, 0x2F,        //   Usage (0x2F)
  0x75, 0x10,        //   Report Size (16)
  0x95, 0x04,        //   Report Count (4)
  0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  0xC0,              // End Collection
};

bool wasLEFT = false;
bool wasRIGHT = false;
bool wasUP = false;
bool wasDOWN = false;

typedef struct {
  uint16_t buttons; // bits: 0 = square, 1 = cross, 2 = circle, 3 = triangle,
                    // 4 = L1, 5 = R1, 6 = L2, 7 = R2, 8 = select, 9 = start, 12 = PS
                    // 10 = L3?, 11 = R3?
  uint8_t dpadHat; // 0 = up, 1 = up/right, 2 = right etc., 0x0f = neutral
  uint8_t leftStickXAxis;
  uint8_t leftStickYAxis;
  uint8_t rightStickXAxis;
  uint8_t rightStickYAxis;
  uint8_t dpadRightAxis;
  uint8_t dpadLeftAxis;
  uint8_t dpadUpAxis;
  uint8_t dpadDownAxis;
  uint8_t triangleAxis;
  uint8_t circleAxis;
  uint8_t crossAxis;
  uint8_t squareAxis;
  uint8_t L1Axis;
  uint8_t L2Axis;
  uint8_t R1Axis;
  uint8_t R2Axis;
  uint16_t accelerometerXAxis; // 10 bits (these are guesses BTW)
  uint16_t accelerometerYAxis; // 10 bits
  uint16_t accelerometerZAxis; // 10 bits
  uint16_t gyroscopeAxis; // 10 bits
} hid_report_t;

hid_report_t report;
hid_report_t prevReport;

void dpad(bool up, bool down, bool left, bool right) {
  report.leftStickXAxis = fTwoIP(left, right, wasLEFT, wasRIGHT);
  report.leftStickYAxis = fTwoIP(up, down, wasUP, wasDOWN);
}

void sendReport() {
  if (memcmp(&prevReport, &report, sizeof(report))) {
    HID().SendReport(1, &report, sizeof(report));
    memcpy(&prevReport, &report, sizeof(report));
  }
}

void setup() {
  static HIDSubDescriptor node(hidReportDescriptor, sizeof(hidReportDescriptor));
  HID().AppendDescriptor(&node);

  pinMode(PIN_UP, INPUT_PULLUP);
  pinMode(PIN_DOWN, INPUT_PULLUP);
  pinMode(PIN_LEFT, INPUT_PULLUP);
  pinMode(PIN_RIGHT, INPUT_PULLUP);
  pinMode(PIN_CROSS, INPUT_PULLUP);
  pinMode(PIN_CIRCLE, INPUT_PULLUP);
  pinMode(PIN_TRIANGLE, INPUT_PULLUP);
  pinMode(PIN_SQUARE, INPUT_PULLUP);
  pinMode(PIN_L1, INPUT_PULLUP);
  pinMode(PIN_L2, INPUT_PULLUP);
  pinMode(PIN_R1, INPUT_PULLUP);
  pinMode(PIN_R2, INPUT_PULLUP);
  pinMode(PIN_SELECT, INPUT_PULLUP);
  pinMode(PIN_START, INPUT_PULLUP);

  memset(&report, 0, sizeof(report));
  report.dpadHat = 0x0f;
  report.accelerometerXAxis = 0x02; // not sure why 0x02, but that's what HORI Fighting Stick V3 sends
  report.accelerometerYAxis = 0x02;
  report.accelerometerZAxis = 0x02;
  report.gyroscopeAxis = 0x02;
  report.leftStickXAxis = 0x80;
  report.leftStickYAxis = 0x80;
  report.rightStickXAxis = 0x80;
  report.rightStickYAxis = 0x80;
  memcpy(&prevReport, &report, sizeof(report));
}

void loop() {
  report.buttons = 0x00;
  report.triangleAxis = 0x00;
  report.circleAxis = 0x00;
  report.crossAxis = 0x00;
  report.squareAxis = 0x00;
  report.L1Axis = 0x00;
  report.L2Axis = 0x00;
  report.R1Axis = 0x00;
  report.R2Axis = 0x00;

  dpad(digitalRead(PIN_UP) == LOW,
       digitalRead(PIN_DOWN) == LOW,
       digitalRead(PIN_LEFT) == LOW,
       digitalRead(PIN_RIGHT) == LOW);

  if (digitalRead(PIN_SQUARE) == LOW) {
    report.buttons |= 1 << 0;
    report.squareAxis = 0xff;
  }
  if (digitalRead(PIN_CROSS) == LOW) {
    report.buttons |= 1 << 1;
    report.crossAxis = 0xff;
  }
  if (digitalRead(PIN_CIRCLE) == LOW) {
    report.buttons |= 1 << 2;
    report.circleAxis = 0xff;
  }
  if (digitalRead(PIN_TRIANGLE) == LOW) {
    report.buttons |= 1 << 3;
    report.triangleAxis = 0xff;
  }
  if (digitalRead(PIN_L1) == LOW) {
    report.buttons |= 1 << 4;
    report.L1Axis = 0xff;
  }
  if (digitalRead(PIN_R1) == LOW) {
    report.buttons |= 1 << 5;
    report.R1Axis = 0xff;
  }
  if (digitalRead(PIN_L2) == LOW) {
    report.buttons |= 1 << 6;
    report.L2Axis = 0xff;
  }
  if (digitalRead(PIN_R2) == LOW) {
    report.buttons |= 1 << 7;
    report.R2Axis = 0xff;
  }
  if (digitalRead(PIN_SELECT) == LOW) {
    report.buttons |= 1 << 8;
  }
  if (digitalRead(PIN_START) == LOW) {
    report.buttons |= 1 << 9;
  }

  sendReport();
}

uint8_t fTwoIP(bool isLOW, bool isHIGH, bool& wasLOW, bool& wasHIGH) {
  uint8_t control = 128;
  if (isLOW && wasHIGH)
    control = 0x00;
  if (isHIGH && wasLOW)
    control = 0xFF;
  if (!isLOW && isHIGH) {
    control = 0xFF;
    wasHIGH = true;
    wasLOW = false;
  }
  if (isLOW && !isHIGH) {
    control = 0x00;
    wasLOW = true;
    wasHIGH = false;
  }
  return control;
}
