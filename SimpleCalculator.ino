#include "Arduino.h"
#include "Keypad.h"
#include <LiquidCrystal_I2C.h>

#define LCD_ROWS 2
#define LCD_COLS 16

// handles key press/release
// returns the key
char waitForKey(Keypad & keypad) 
{
  char key = NO_KEY;

  // wait for key press
  while (key == NO_KEY)
    key = keypad.getKey();

  // wait for the key to be released
  char tempKey = key;
  while (tempKey != NO_KEY)
    tempKey = keypad.getKey();
  
  return key;
}

// Quick helper to check if a key is one of + - * /
bool isOperator(char ch)
{
  return ch == '+' || ch == '-' || ch == '*' || ch == '/';
}

void printInitialState(LiquidCrystal_I2C & lcd)
{
  lcd.clear();
  lcd.setCursor(15, 0);
  lcd.print("0");
}

void clearRow(LiquidCrystal_I2C & lcd, uint8_t row) {
    lcd.setCursor(0, row);
    lcd.print("                ");
}

void showInput(LiquidCrystal_I2C & lcd, const char * text)
{
  uint8_t strLen = 0;
  while (text[strLen] != 0)
    strLen++;

  // clear leftover
  clearRow(lcd, 0);

  if (strLen == 0) {
    lcd.setCursor(LCD_COLS - 1, 0);
    lcd.print("0");
    return;
  }

  // limit to LCD width
  if (strLen > LCD_COLS)
    strLen = LCD_COLS;

  lcd.setCursor(LCD_COLS - strLen, 0);
  lcd.print(text);
}

// show result for integer values
void showResult(LiquidCrystal_I2C & lcd, long int result)
{
  char str[17];
  bool isNegative = false;
  uint8_t len = 0;

  // check if the number is negative
  if (result < 0) {
    isNegative = true;
    result = -result;
  }

  long int copy = result;
  if (copy == 0) {
    str[len++] = '0';
  }
  else {
    while (copy > 0 && len < 16) {
      str[len++] = '0' + (copy % 10);
      copy /= 10;
    }
    
    // reverse the string
    for (int8_t i = 0; i < len / 2; i++) {
      char temp = str[i];
      str[i] = str[(len - 1) - i];
      str[(len - 1) - i] = temp;
    }
  }

  // shift once to front
  // leave space for '-'
  if (isNegative && len < 16) {
    for (int i = len; i >= 1; i--)
      str[i] = str[i - 1];

    str[0] = '-';
    len++;
  }
  
  str[len] = 0; // end string

  // clear result leftover
  clearRow(lcd, 1);

  lcd.setCursor(0, 1);
  lcd.print("Result:");
  lcd.setCursor(LCD_COLS - len, 1);
  lcd.print(str);
}

// show result for float values
void showResult(LiquidCrystal_I2C & lcd, float result)
{
  char str[17];
  dtostrf(result, 0, 2, str); // convert float to string with 2 decimal places

  // clear previous result
  clearRow(lcd, 1);

  lcd.setCursor(0, 1);
  lcd.print("Result:");
  lcd.setCursor(LCD_COLS - strlen(str), 1);
  lcd.print(str);
}

// reads a number until an operator is inserted
// passes the result number by reference
// returns the operator
// handles negatives
// prints to lcd consistently
char readNumber(Keypad & keypad, LiquidCrystal_I2C & lcd, long int & numberOut)
{
  char buffer[17] = {0};
  uint8_t len = 0;
  
  while (true)
  {
    char key = waitForKey(keypad);

    if (key >= '0' && key <= '9') {
      if (len < 16) {
        buffer[len++] = key;
        buffer[len] = 0; // end the string
        showInput(lcd, buffer);
      }
    }
    else if (key == '-' && len == 0) {
      buffer[len++] = key;
      buffer[len] = 0; // end the string
      showInput(lcd, buffer);
    }
    else if (isOperator(key) || key == '=' || key == 'C') {
      numberOut = atol(buffer); // ascii to long int
      
      if (isOperator(key)) {
        char opStr[2] = {key, 0};
        showInput(lcd, opStr);
      }

      return key;
    }
  }
}

// Does the math, sets result
// Returns false on error (like divide-by-zero)
bool calculate(long int a, long int b, char op, long int & result)
{
  switch (op) {
    case '+': result = a + b; return true;
    case '-': result = a - b; return true;
    case '*': result = a * b; return true;
    case '/':
      if (b != 0) {
        result = a / b; // integer division
        return true;
      }
      else
        return false;
  }
  return false; // unknown operator
}

// Calculates division only
// sets result as floating point
bool calculate(long int a, long int b, float & result)
{
  if (b == 0) return false;
  result = 1.0 * a / b;
  return true;
}

int main()
{
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

  printInitialState(lcd);

  long int a, b;
  char mainOp, secondaryOp;
  while (true)
  {
    mainOp = readNumber(keypad, lcd, a);

    if (!isOperator(mainOp)) {
      if (mainOp == '=' || mainOp == 'C') {
        printInitialState(lcd);
        continue;
      }
    }
    
    secondaryOp = readNumber(keypad, lcd, b);

    if (isOperator(secondaryOp) || secondaryOp == '=') {
      long int result;
      float floatResult;
      bool isCorrect;
      if (mainOp != '/')
        isCorrect = calculate(a, b, mainOp, result);
      else
        isCorrect = calculate(a, b, floatResult);

      if (isCorrect) {
        if (mainOp == '/')
          showResult(lcd, floatResult);
        else
          showResult(lcd, result);
      }
    }
    else if (secondaryOp == 'C') {
      printInitialState(lcd);
      continue;
    }
  }

  return 0;
}