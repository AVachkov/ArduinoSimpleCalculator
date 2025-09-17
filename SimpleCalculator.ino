#include "Arduino.h"
#include "Keypad.h"
#include <LiquidCrystal_I2C.h>

#define LCD_ROWS 2
#define LCD_COLS 16

void printToLcd(LiquidCrystal_I2C & lcd, char key, bool reset = false, bool isNegative = false)
{
  static char buffer[17] = {0}; // leave space for null terminator
  static uint8_t strStartIdx = 15;

  if (reset) {
    for (uint8_t i = 0; i < LCD_COLS; i++)
      buffer[i] = 0;
    strStartIdx = 15;
    lcd.clear();
    return;
  }

  // shift left once
  for (uint8_t i = 1; i <= LCD_COLS - 1; i++)
    buffer[i - 1] = buffer[i];

  buffer[LCD_COLS - 1] = key;

  // gather str to print
  char str[17];
  uint8_t strLen = 0;

  for (uint8_t i = strStartIdx; i < LCD_COLS; i++) {
    str[strLen] = buffer[i];
    strLen++;
  }

  str[strLen] = 0; // string ends here

  lcd.setCursor(LCD_COLS - strLen, 0);
  lcd.print(str);

  if (strStartIdx > 0)
      strStartIdx--;
}

void collectNumberAndGetNextOperator(Keypad & keypad, LiquidCrystal_I2C & lcd, long int & numberOut, char & operatorOut, bool & resDisplayed)
{
  bool isNegative = false;
  bool numberStarted = false;
  bool negativeSignPressed = false;
  char key;
  numberOut = 0;

  // collecting first number
  while (true)
  {
    key = NO_KEY;

    // wait for key press
    while (key == NO_KEY)
      key = keypad.getKey();

    if (resDisplayed && key >= '0' && key <= '9') {
      printToLcd(lcd, 0, true);
      resDisplayed = false;
    }

    // wait for the key to be released
    char tempKey = key;
    while (tempKey != NO_KEY)
      tempKey = keypad.getKey();

    if (key >= '0' && key <= '9') {
      if (!numberStarted)
        numberStarted = true;

      uint8_t digit = key - '0';
      numberOut = (numberOut * 10) + digit;
      printToLcd(lcd, key);
    }
    else if (key == '+' || key == '-' || key == '*' || key == '/') {
      if (!numberStarted) {
        if (key == '-' && !negativeSignPressed) {
          isNegative = true;
          negativeSignPressed = true;
          printToLcd(lcd, key);
        }
      }
      else {
        printToLcd(lcd, key);
        break;
      }
    }
    else if (key == '=') {
      break;
    }
    else if (key == 'C') {
      numberOut = 0;
      break;
    }
  }

  if (isNegative)
    numberOut = -numberOut;

  operatorOut = key;
}

long int doCalculation(const long int first, const long int second, const char op) {
  switch (op) {
    case '+': return first + second;
    case '-': return first - second;
    case '*': return first * second;
    case '/':
      if (second != 0) return first / second;
      else return -1;
  }
  return 0;
}

void printNumberToLcd(LiquidCrystal_I2C & lcd, long int number)
{
  // the number as string
  char str[17];

  // number length without the '-'
  uint8_t len = 0;

  if (number == 0) {
    lcd.setCursor(0, 1);
    lcd.print("Result:");
    lcd.setCursor((LCD_COLS - 1) - len, 1);
    lcd.print("0");
    return;
  }

  // check if the number is negative
  int8_t stringBoundary = 0;
  if (number < 0) {
    str[0] = '-';
    stringBoundary = 1;
    number *= -1;
  }

  // count the number length
  {
    long int copy = number;
    while (copy > 0) {
      copy /= 10;
      len++;
    }
  }

  // enter the number inside the buffer
  int8_t idx = (len - 1) + stringBoundary;
  while (idx >= stringBoundary) {
    str[idx] = '0' + (number % 10);
    number /= 10;
    idx--;
  }
  str[len + stringBoundary] = 0; // end the string

  lcd.setCursor(0, 1);
  lcd.print("Result:");
  lcd.setCursor(LCD_COLS - (len + stringBoundary), 1);
  lcd.print(str);
}

int main() {
  init();
  Serial.begin(9600);

  const byte ROWS = 4;
  const byte COLS = 4;

  char keys[ROWS][COLS] = {
    { '1', '2', '3', '+' },
    { '4', '5', '6', '-' },
    { '7', '8', '9', '*' },
    { 'C', '0', '=', '/' }
  };

  byte rowPins[ROWS] = { 13, 12, 11, 10 };
  byte colPins[COLS] = { 9, 8, 7, 6 };

  Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

  LiquidCrystal_I2C lcd(0x27, LCD_COLS, LCD_ROWS);
  lcd.init();
  lcd.backlight();
  
  long int firstNumber, secondNumber;  
  char mainOperator;

  bool resDisplayed = false;
  while (true)
  {
    collectNumberAndGetNextOperator(keypad, lcd, firstNumber, mainOperator, resDisplayed);
    Serial.println(firstNumber);
    Serial.println(mainOperator);

    if (mainOperator == 'C') {
      printToLcd(lcd, mainOperator, true);
      continue;
    }
    else if (mainOperator == '=')
      continue;

    char secondOp;
    collectNumberAndGetNextOperator(keypad, lcd, secondNumber, secondOp, resDisplayed);
    Serial.println(secondNumber);
    Serial.println(secondOp);
    Serial.println(doCalculation(firstNumber, secondNumber, mainOperator));

    if (secondOp == 'C') {
      printToLcd(lcd, secondOp, true);
      continue;
    }
    else if (secondOp == '=') {
      printNumberToLcd(lcd, doCalculation(firstNumber, secondNumber, mainOperator));
      resDisplayed = true;
    }
  }

  return 0;
}