//----------------LIBRARY------------------//
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#include <StackArray.h>
#include <MFRC522.h>
#include <SPI.h>
#include "Button2.h"
#include "Wire.h"
#include "LiquidCrystal_I2C.h"

//----------------DEFINE PIN------------------//

#define REMOVE_PIN 3

const int pinRST = 0;
const int pinSDA = 2;

//----------------GLOBAL VARIABLE------------------//

LiquidCrystal_I2C lcd(0x27, 20, 4);
Button2 button_rm;
MFRC522 rfid(pinSDA, pinRST);

int statusM = 0;

FirebaseData fdbo;
FirebaseAuth auth;
FirebaseConfig config;

// String path = "/PurchaseHistory";

String user_id;
String m_path;
String u_path;
String n_path;
String username;
String item_name;

int cust_balance;
int item_price;
int count = 1;
int NoPurchase_db = 0;
int NoPurchase = 0;
int TotalPurchase = 0;
int countCartItem = 0;
int countCompareItem = 0;
int subBalance;

StackArray<String> CartItem;
StackArray<String> CompareItem;

FirebaseJson CartItemJson;
FirebaseJson updateNoPurchase;
FirebaseJson updateBalance;

//----------------FUNCTION DECLARATION------------------//

void pressed(Button2 &btn);
void click(Button2 &btn);
void longClickDetected(Button2 &btn);
void longClick(Button2 &btn);
void doubleClick(Button2 &btn);
void clearLine(int line);

//----------------WI-FI CREDENTIAL------------------//

#define WIFI_SSID "snowball"
#define WIFI_PASSWORD "snoowwyy"

//----------------FIREBASE CREDENTIAL------------------//

#define FIREBASE_HOST "smartshoppingcart-thesis1-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "8u9Yu5P5mAsqi2VMlRR7Xhh3Zto7mQmdp18DYXIV"

//----------------SETUP------------------//

void setup()
{
  //SERIAL
  Serial.begin(9600);

  //BUTTON
  button_rm.begin(REMOVE_PIN);
  button_rm.setClickHandler(click);
  button_rm.setLongClickDetectedHandler(longClickDetected);
  button_rm.setLongClickHandler(longClick);
  button_rm.setLongClickTime(1000);
  button_rm.setDoubleClickHandler(doubleClick);

  //MRFC522
  SPI.begin();
  rfid.PCD_Init();

  //LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Welcome!");
  lcd.setCursor(0, 1);
  lcd.print("Smart Cart by Puspa");

  //WI-FI
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lcd.setCursor(0, 2);
  lcd.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(300);
  }

  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("SmartCart is ready !");

  //FIREBASE
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
}

//----------------LOOP------------------//

void loop()
{

  String UID_card = "";
  if (count == 1)
  {
    lcd.setCursor(0, 1);
    lcd.print("Tap your ID Card !");
  }
  else
  {
    //read button
    button_rm.loop();
  }

  if (statusM == 0 || statusM == 1)
  {
    if (rfid.PICC_IsNewCardPresent())
    {
      if (rfid.PICC_ReadCardSerial())
      {
        for (byte i = 0; i < rfid.uid.size; ++i)
        {
          UID_card += String(rfid.uid.uidByte[i], HEX);
        }
        if (count == 1)
        {
          user_id = UID_card;

          if (Firebase.get(fdbo, "User/" + user_id + "/cust_name"))
          {
            username = fdbo.to<String>();
          }

          if (username != "null")
          {
            if (Firebase.get(fdbo, "User/" + user_id + "/cust_balance"))
            {
              cust_balance = fdbo.to<int>();
            }
            if (Firebase.get(fdbo, "User/" + user_id + "/noPurchase"))
            {
              NoPurchase_db = fdbo.to<int>();
            }
            NoPurchase = NoPurchase_db + 1;

            clearLine(0);
            lcd.setCursor(0, 0);
            lcd.print("Balance:");
            lcd.setCursor(8, 0);
            lcd.print(cust_balance);
            clearLine(1);
            lcd.setCursor(0, 1);
            lcd.print("Welcome ");
            lcd.setCursor(8, 1);
            lcd.print(username + "!");
            lcd.setCursor(0, 2);
            lcd.print("Waiting item...");
            lcd.setCursor(0, 3);
            lcd.print("Total:");
            lcd.setCursor(6, 3);
            lcd.print(TotalPurchase);
            count++;
          }
          else
          {
            clearLine(0);
            lcd.setCursor(0, 0);
            lcd.print("Unknown ID !");
            clearLine(1);
            lcd.setCursor(0, 1);
            lcd.print("Try another card");
          }
        }
        else
        {
          if (Firebase.get(fdbo, "ItemData/" + UID_card + "/item_name"))
          {
            item_name = fdbo.to<String>();
          }

          if (item_name != "null")
          {
            if (Firebase.get(fdbo, "ItemData/" + UID_card + "/item_price"))
            {
              item_price = fdbo.to<int>();
            }
            if (statusM == 0)
            {
              CartItem.push(UID_card);
              TotalPurchase += item_price;

              clearLine(1);
              lcd.setCursor(0, 1);
              lcd.print("1 item added !");
              clearLine(2);
              lcd.setCursor(0, 2);
              lcd.print(item_name);
              lcd.setCursor(0, 3);
              lcd.print("Total:");
              lcd.setCursor(6, 3);
              lcd.print(TotalPurchase);
              count++;
            }
            else if (statusM == 1)
            {
              clearLine(2);
              lcd.setCursor(0, 2);
              lcd.print("Updating cart...");

              countCartItem = CartItem.count();

              //Removing Item From Stack
              while (!CartItem.isEmpty())
              {
                String st = CartItem.pop();
                if (st != UID_card)
                {
                  CompareItem.push(st);
                }
                else
                {
                  break;
                }
              }

              countCompareItem = CompareItem.count();

              while (!CompareItem.isEmpty())
              {
                CartItem.push(CompareItem.pop());
              }

              if (countCompareItem != countCartItem)
              {
                TotalPurchase -= item_price;

                clearLine(1);
                lcd.setCursor(0, 1);
                lcd.print(item_name);

                clearLine(2);
                lcd.setCursor(0, 2);
                lcd.print(CartItem.count());

                clearLine(3);
                lcd.setCursor(0, 3);
                lcd.print("Total:");
                lcd.setCursor(6, 3);
                lcd.print(TotalPurchase);

                count--;
              }
              else
              {
                clearLine(1);
                clearLine(2);
                lcd.setCursor(0, 1);
                lcd.print("Item not in cart");
              }

              delay(1000);

              statusM = 0;

              clearLine(1);
              lcd.setCursor(0, 1);
              lcd.print("Let's back shopping");
              clearLine(2);
              lcd.setCursor(0, 2);
              lcd.print("Waiting Item...");
            }
          }
          else
          {
            clearLine(1);
            lcd.setCursor(0, 1);
            lcd.print("Item doesn't exist!");
            clearLine(2);
            lcd.setCursor(0, 2);
            lcd.print("Please ask our CS");
          }
        }
      }

      delay(1000);
    }
  }
  else if (statusM == 2)
  {
    //MAKE PAYMENT
    clearLine(1);
    clearLine(2);
    lcd.setCursor(0, 1);
    lcd.print("Processing payment..");
    lcd.setCursor(0, 2);
    lcd.print("Item in Cart:");
    lcd.setCursor(13, 2);
    lcd.print(CartItem.count());

    delay(1000);

    //UPDATE DATABASE

    //1. Check balance

    if (Firebase.get(fdbo, "User/" + user_id + "/cust_balance"))
    {
      cust_balance = fdbo.to<int>();
      clearLine(0);
      lcd.setCursor(0, 0);
      lcd.print("Balance:");
      lcd.setCursor(8, 0);
      lcd.print(cust_balance);
    }

    if (TotalPurchase <= cust_balance)
    {
      //2. Update Purchase History
      //a. set stack to json

      int noItem = 0;

      String noPurchase_path = "0" + String(NoPurchase);

      while (!CartItem.isEmpty())
      {
        noItem++;
        m_path = noPurchase_path + "/Items/Item" + String(noItem) + "/item_id";
        String item = CartItem.pop();
        CartItemJson.set(m_path, item);
      }

      if (noItem > 0)
      {
        //a. set Total Purchase to json
        u_path = noPurchase_path + "/TotalPurchase";
        CartItemJson.set(u_path, TotalPurchase);

        //b. set No Purchase to json
        n_path = noPurchase_path + "/NoPurchase";
        CartItemJson.set(n_path, noPurchase_path);

        //c. upload to DB
        if (Firebase.updateNode(fdbo, "/PurchaseHistory/" + user_id, CartItemJson))
        {
          //3. Update No of Purchase
          updateNoPurchase.set("noPurchase", NoPurchase);
          Firebase.updateNode(fdbo, "/User/" + user_id, updateNoPurchase);

          clearLine(1);
          lcd.setCursor(0, 1);
          lcd.print("Paying bill...");

          //4. Update Balance
          subBalance = cust_balance - TotalPurchase;
          updateBalance.set("cust_balance", subBalance);
          Firebase.updateNode(fdbo, "/User/" + user_id, updateBalance);

          delay(200);
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Transaction Success!");
          lcd.setCursor(0, 1);
          lcd.print("Thank You !");
          lcd.setCursor(0, 2);
          lcd.print("Remaining balance:");
          lcd.setCursor(0, 3);
          lcd.print(subBalance);

          statusM = 0;

          delay(2000);

          ESP.reset();
        }
        else
        {
          clearLine(1);
          lcd.setCursor(0, 1);
          lcd.print("ERROR");
          statusM = 0;
        }
      }
      else
      {
        statusM = 0;
        clearLine(1);
        clearLine(2);
        lcd.setCursor(0, 1);
        lcd.print("No item in cart!");
        delay(1000);
        clearLine(1);
        lcd.setCursor(0, 1);
        lcd.print("Waiting for item...");
      }
    }
    else
    {
      // lcd.setCursor(0, 0);
      // lcd.print("Transaction Failed!");
      clearLine(1);
      lcd.setCursor(0, 1);
      lcd.print("Insufficient Balance");
      clearLine(2);
      lcd.setCursor(0, 2);
      lcd.print("Please Top Up!");
      statusM = 0;
    }

    // statusM = 0;
  }

  //delay(500);
}

//----------------BUTTON FUNCTION------------------//

void click(Button2 &btn)
{
  statusM = 1;
  clearLine(1);
  lcd.setCursor(0, 1);
  lcd.print("Remove mode active");
}
void longClickDetected(Button2 &btn)
{
  clearLine(1);
  lcd.setCursor(0, 1);
  lcd.print("Pay mode active");
  clearLine(2);
  lcd.setCursor(0, 2);
  lcd.print("Release the button!");
}
void longClick(Button2 &btn)
{
  statusM = 2;
}

void doubleClick(Button2 &btn)
{
  statusM = 0;
  clearLine(1);
  lcd.setCursor(0, 1);
  lcd.print("Remove deactivate");
}

//----------------CLEAR LCD LINE------------------//
void clearLine(int line)
{
  lcd.setCursor(0, line);
  for (int n = 0; n < 20; n++)
  {
    lcd.print(" ");
  }
}
