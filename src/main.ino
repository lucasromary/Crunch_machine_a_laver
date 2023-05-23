#include <Arduino.h>
#include <M5Stack.h>
#include <Wire.h>
#include "GoPlus2.h"
#include <driver/rmt.h>
#include <math.h>

#define NOTE_D0 -1
#define NOTE_D1 294
#define NOTE_D2 330
#define NOTE_D3 350
#define NOTE_D4 393
#define NOTE_D5 441
#define NOTE_D6 495
#define NOTE_D7 556

#define NOTE_DL1 147
#define NOTE_DL2 165
#define NOTE_DL3 175
#define NOTE_DL4 196
#define NOTE_DL5 221
#define NOTE_DL6 248
#define NOTE_DL7 278

#define NOTE_DH1 589
#define NOTE_DH2 661
#define NOTE_DH3 700
#define NOTE_DH4 786
#define NOTE_DH5 882
#define NOTE_DH6 990
#define NOTE_DH7 112

GoPlus2 goPlus;
const int arraySizeProgramme = 4;
String programmeList[arraySizeProgramme] = {"Eco", "Delicat", "Rapide", "Intensif"};
int menu = 0;
int state = 0;
int auto_mode = 0;

unsigned char low_data[8] = {0};
unsigned char high_data[12] = {0};
int water_level = 0;
int water_level_init = 0;
int last_water_level_init = 0;
int last_water_level = 0;

int val_bouton_pause = 1;
int last_val_bouton_pause = 1;

int val_bouton_resume = 1;
int last_val_bouton_resume = 1;

#define NO_TOUCH 0xFE
#define THRESHOLD 100
#define ATTINY1_HIGH_ADDR 0x78
#define ATTINY2_LOW_ADDR 0x77

// M5.BtnC.wasPressed()
void header(const char *string, uint16_t color)
{
  M5.Lcd.fillScreen(color);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
  M5.Lcd.fillRect(0, 0, 320, 30, TFT_BLACK);
  M5.Lcd.setTextDatum(CC_DATUM);
  M5.Lcd.drawString(string, 160, TFT_WIDTH / 2, 4);
}

void motorControl(int speed) // 80 à 126
{
  goPlus.Motor_write_speed(MOTOR_NUM0, speed);
}

void pumpControl(int state)
{
  goPlus.hub1_wire_value(HUB1_W_ADDR, state);
}

void pumpVidangeControl(int state)
{
  goPlus.hub3_wire_value(HUB3_W_ADDR, state);
}

void electrovanneControl(int state)
{
  goPlus.hub2_wire_value(HUB2_W_ADDR, state);
}

void getHigh12SectionValue(void)
{
  memset(high_data, 0, sizeof(high_data));
  Wire.requestFrom(ATTINY1_HIGH_ADDR, 12);
  while (12 != Wire.available())
    ;

  for (int i = 0; i < 12; i++)
  {
    high_data[i] = Wire.read();
  }
  delay(10);
}

void getLow8SectionValue(void)
{
  memset(low_data, 0, sizeof(low_data));
  Wire.requestFrom(ATTINY2_LOW_ADDR, 8);
  while (8 != Wire.available())
    ;

  for (int i = 0; i < 8; i++)
  {
    low_data[i] = Wire.read(); // receive a byte as character
  }
  delay(10);
}

int getWaterLevel()
{
  int sensorvalue_min = 250;
  int sensorvalue_max = 255;
  int low_count = 0;
  int high_count = 0;
  uint32_t touch_val = 0;
  uint8_t trig_section = 0;
  low_count = 0;
  high_count = 0;

  getLow8SectionValue();
  getHigh12SectionValue();

  // Serial.println("low 8 sections value = ");
  for (int i = 0; i < 8; i++)
  {
    // Serial.print(low_data[i]);
    // Serial.print(".");
    if (low_data[i] >= sensorvalue_min && low_data[i] <= sensorvalue_max)
    {
      low_count++;
    }
    if (low_count == 8)
    {
      // Serial.print("      ");
      // Serial.print("PASS");
    }
  }
  // Serial.println("  ");
  // Serial.println("  ");
  // Serial.println("high 12 sections value = ");
  for (int i = 0; i < 12; i++)
  {
    // Serial.print(high_data[i]);
    // Serial.print(".");

    if (high_data[i] >= sensorvalue_min && high_data[i] <= sensorvalue_max)
    {
      high_count++;
    }
    if (high_count == 12)
    {
      // Serial.print("      ");
      // Serial.print("PASS");
    }
  }

  for (int i = 0; i < 8; i++)
  {
    if (low_data[i] > THRESHOLD)
    {
      touch_val |= 1 << i;
    }
  }
  for (int i = 0; i < 12; i++)
  {
    if (high_data[i] > THRESHOLD)
    {
      touch_val |= (uint32_t)1 << (8 + i);
    }
  }

  while (touch_val & 0x01)
  {
    trig_section++;
    touch_val >>= 1;
  }

  return trig_section * 5;
}

void cleanScreen()
{
  M5.Lcd.fillRect(0, 0, TFT_HEIGHT, TFT_WIDTH - 25, BLACK);
}

int deroulant(int array_size, int lengthX = 180, int lengthY = 30, int posX = 160, int posY = 80)
{
  M5.Lcd.setTextDatum(CC_DATUM);

  int startX = posX - lengthX / 2;
  int startY = posY - lengthY / 2;
  int choice = 0;
  int val_menu = 1;

  if (array_size > 4)
  {
    lengthX = 160;
    posX = 80;
    posY = 100;
    startX = posX - lengthX / 2;
    startY = posY - lengthY / 2;
  }

  M5.Lcd.drawRect(startX, startY, lengthX, lengthY, GREEN);

  while (!choice)
  {
    M5.update();

    if (M5.BtnB.wasReleased() || M5.BtnB.pressedFor(1000, 200))
    {
      choice = 1;
      state++;
    }
    if (M5.BtnA.pressedFor(3000, 100))
    {
      choice = 1;
      state = 10;
      auto_mode = 0;
    }

    if (M5.BtnC.wasReleased() || M5.BtnC.pressedFor(1000, 200))
    {
      M5.Lcd.drawRect(startX, startY, lengthX, lengthY, BLACK);
      if (val_menu < array_size)
      {
        startY += 30;
        val_menu++;
        if (startY > 200)
        {
          startX += 160;
          startY = posY - lengthY / 2;
        }
      }
      else
      {
        startX = posX - lengthX / 2;
        startY = posY - lengthY / 2;
        val_menu = 1;
      }
      M5.Lcd.drawRect(startX, startY, lengthX, lengthY, GREEN);
    }
  }

  return val_menu;
}

void creationMenu(String choice_tab[], int array_size, int two_colonnes_forced = 0, int startX = 160, int startY = 80, int espacementY = 30, int espacementX = 0)
{
  if (array_size > 4 && two_colonnes_forced == 0)
  {
    startX = 80;
    espacementX = 160;
  }

  int startY_legacy = startY;
  int startX_legacy = startX;

  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextDatum(CC_DATUM);

  for (int i = 0; i < array_size; i++)
  {
    M5.Lcd.drawString(choice_tab[i], startX, startY, 1);
    startY += espacementY;
    if (startY > 200)
    {
      startY = startY_legacy;
      startX = startX + espacementX;
    }
  }
}

int menuProgramme()
{
  cleanScreen();
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextDatum(CC_DATUM);
  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.drawString("Choix du", 160, 10, 1);
  M5.Lcd.drawString("Programme", 160, 40, 1);

  creationMenu(programmeList, arraySizeProgramme);
  // int res = deroulant(160, 110, 250, 30, arraySizeimprimantes);
  int res = deroulant(arraySizeProgramme);
  return res;
}

void menuResume()
{
  cleanScreen();
  M5.Lcd.fillRect(220, TFT_WIDTH - 40, 100, 40, BLACK);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextDatum(CC_DATUM);
  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.drawString("Votre choix :", 160, 80, 1);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.drawString(programmeList[menu - 1], 160, 120, 1);

  int choice = 0;

  while (!choice)
  {
    M5.update();
    if (M5.BtnB.wasReleased() || M5.BtnB.pressedFor(1000, 200))
    {
      choice = 1;
      state++;
    }
    if (M5.BtnA.wasReleased() || M5.BtnA.pressedFor(1000, 200))
    {
      choice = 1;
      state--;
    }
  }
}

void background()
{
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.setTextDatum(CC_DATUM);
  M5.Lcd.drawString("Retour", 50, TFT_WIDTH - 15, 1);
  M5.Lcd.drawString("Valider", TFT_HEIGHT / 2, TFT_WIDTH - 15, 1);
  M5.Lcd.drawString("Defiler", 270, TFT_WIDTH - 15, 1);
}

void remplissage(int pourcentage = 5)
{
  M5.Lcd.setTextSize(3);
  M5.Lcd.fillRect(0, 0, TFT_HEIGHT, TFT_WIDTH, TFT_BLACK);
  M5.Lcd.setTextDatum(CC_DATUM);
  M5.Lcd.drawString("Remplissage", 160, TFT_WIDTH / 2, 2);

  int water_level_goal = water_level_init - pourcentage;
  long timer = millis();
  int pass = 0;
  int pause_state = 0;
  long timer_tampon = 0;

  while (!pass)
  {
    if (water_level == water_level_goal && millis() - timer > 3000)
    {
      pass = 1;
      Serial.println("Assez d'eau");
    }

    readBouton();
    water_level = getWaterLevel();
    if (val_bouton_pause == 0 && last_val_bouton_pause == 1)
    {
      timer_tampon = timer_tampon + millis() - timer; // 3sec  1sec
      Serial.println("Pause remplissage");
      pumpControl(LOW);

      while (pause_state == 0)
      {
        readBouton();
        if (val_bouton_resume == 0 && last_val_bouton_resume == 1)
        {
          pause_state = 1;
          Serial.println("Resume remplissage");
          timer = millis();
        }
      }
      pause_state = 0;
    }

    pumpControl(HIGH);

    if (millis() - timer > 5000 - timer_tampon)
    {
      pass = 1;
    }
    delay(100);
  }
  pumpControl(LOW);
}

void lavage(int time = 20000)
{
  M5.Lcd.setTextSize(3);
  M5.Lcd.fillRect(0, 0, TFT_HEIGHT, TFT_WIDTH, TFT_BLACK);
  M5.Lcd.setTextDatum(CC_DATUM);
  M5.Lcd.drawString("Lavage", 160, TFT_WIDTH / 2, 2);

  motorControl(100);

  long timer = millis();
  long timer_tampon = 0;

  while (millis() - timer < time - timer_tampon)
  {
    int pause_state = 0;
    readBouton();

    if (val_bouton_pause == 0 && last_val_bouton_pause == 1)
    {
      timer_tampon = timer_tampon + millis() - timer; // 3sec
      Serial.println("Pause lavage");

      M5.Lcd.setTextColor(WHITE);
      M5.Lcd.drawString("(Pause)", 160, 170, 1);
      M5.Lcd.setTextColor(GREEN);

      electrovanneControl(LOW);
      motorControl(LOW);
      while (pause_state == 0)
      {
        readBouton();
        if (val_bouton_resume == 0 && last_val_bouton_resume == 1)
        {
          M5.Lcd.fillRect(80, 155, 180, 40, BLACK);
          electrovanneControl(HIGH);
          motorControl(100);
          pause_state = 1;
          Serial.println("Resume lavage");
          timer = millis();
        }
      }
    }
  }

  motorControl(0);
}

void vidange(int time = 30000)
{
  pumpVidangeControl(HIGH);
  M5.Lcd.setTextSize(3);
  M5.Lcd.fillRect(0, 0, TFT_HEIGHT, TFT_WIDTH, TFT_BLACK);
  M5.Lcd.setTextDatum(CC_DATUM);
  M5.Lcd.drawString("Vidange", 160, TFT_WIDTH / 2, 2);

  long timer = millis();
  int pass = 0;
  long timer_tampon = 0;

  while (!pass)
  {
    M5.update();
    getWaterLevel();

    if (M5.BtnA.pressedFor(1000, 200))
    {
      pass = 1;
      Serial.println("pass vidange");
    }

    if (water_level == water_level_init && millis() - timer > 30000)
    {
      pass = 1;
      Serial.println("Eau vidangé");
    }
    if (millis() - timer > time)
    {
      Serial.println("Temps écoulé vidange");
      pass = 1;
    }

    delay(100);
  }
  pumpVidangeControl(LOW);
}

void essorage(int time = 20000)
{
  M5.Lcd.setTextSize(3);
  M5.Lcd.fillRect(0, 0, TFT_HEIGHT, TFT_WIDTH, TFT_BLACK);
  M5.Lcd.setTextDatum(CC_DATUM);
  M5.Lcd.drawString("Essorage", 160, TFT_WIDTH / 2, 2);

  motorControl(100);

  long timer = millis();
  long timer_tampon = 0;

  while (millis() - timer < time - timer_tampon)
  {
    int pause_state = 0;
    readBouton();

    if (val_bouton_pause == 0 && last_val_bouton_pause == 1)
    {
      timer_tampon = timer_tampon + millis() - timer; // 3sec
      Serial.println("Pause essorage");

      M5.Lcd.setTextColor(WHITE);
      M5.Lcd.drawString("(Pause)", 160, 170, 1);
      M5.Lcd.setTextColor(GREEN);

      motorControl(0);
      while (pause_state == 0)
      {
        readBouton();
        if (val_bouton_resume == 0 && last_val_bouton_resume == 1)
        {
          M5.Lcd.fillRect(80, 155, 180, 40, BLACK);
          motorControl(100);
          pause_state = 1;
          Serial.println("Resume essorage");
          timer = millis();
        }
      }
    }
  }
  motorControl(0);
}

void readBouton()
{
  val_bouton_resume = goPlus.hub3_a_read_value(HUB3_R_ADDR); // read digital_input
  // Serial.print(val_bouton_resume);
  // Serial.print("  ");
  val_bouton_pause = goPlus.hub2_a_read_value(HUB2_R_ADDR); // read digital_input
  // Serial.println(val_bouton_pause);
}

void verifNiveauEau()
{
  background();
  cleanScreen();
  water_level_init = getWaterLevel();
  Serial.print("water level init = ");
  Serial.print(water_level_init);
  Serial.println("% ");

  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextDatum(CC_DATUM);
  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.drawString("Niveau d'eau :", 160, 25, 1);
  M5.Lcd.setTextColor(WHITE);

  int choice = 0;
  while (!choice)
  {
    M5.update();

    water_level_init = getWaterLevel();
    Serial.print("water level init = ");
    Serial.print(water_level_init);
    Serial.println("% ");
    if (last_water_level_init != water_level_init)
    {
      M5.Lcd.fillRect(120, 100, 80, 40, BLACK);
    }
    M5.Lcd.fillRect(0, 140, TFT_HEIGHT, 40, BLACK);
    M5.Lcd.drawString(String(water_level_init) + " %", 160, 120, 1);
    last_water_level_init = water_level_init;

    if (water_level_init == 0)
    {
      M5.Lcd.drawString("Nettoyez capteur", 160, 160, 1);
    }

    if (M5.BtnB.wasReleased() || M5.BtnB.pressedFor(1000, 200))
    {
      choice = 1;
      state++;
    }
    if (M5.BtnA.wasReleased() || M5.BtnA.pressedFor(1000, 200))
    {
      choice = 1;
      state--;
    }
    delay(100);
  }
}

void beepFinish()
{
  M5.Speaker.tone(NOTE_DH1, 200); // Set the speaker to ring at 661Hz for
  delay(200);
  M5.Speaker.tone(NOTE_DH3, 200); // Set the speaker to ring at 661Hz for
  delay(200);
  M5.Speaker.tone(NOTE_DH5, 200); // Set the speaker to ring at 661Hz for
  delay(400);
  M5.Speaker.tone(NOTE_DH3, 200); // Set the speaker to ring at 661Hz for
  delay(200);
  M5.Speaker.tone(NOTE_DH6, 400); // Set the speaker to ring at 661Hz for
  delay(400);
}

void modeManuel()
{
  cleanScreen();
  Serial.println("Mode Manuel");
  M5.Lcd.drawString("Mode manuel", 160, 90, 4);
  delay(1500);

  while (!auto_mode)
  {
    M5.update();
    readBouton();

    water_level = getWaterLevel();
    Serial.print("water level = ");
    Serial.print(water_level);
    Serial.print("% ");

    if (last_water_level != water_level)
    {
      M5.Lcd.fillRect(120, 120, 80, 60, BLACK);
    }
    last_water_level = water_level;

    M5.Lcd.drawString(String(water_level), 160, 150, 4);

    delay(100);

    if (val_bouton_resume > 1)
    {
      pumpControl(LOW);
      Serial.print("   pump LOW");
    }
    else
    {
      pumpControl(HIGH);
      Serial.print("   pump HIGH");
      M5.Speaker.tone(NOTE_DL7, 200); // Set the speaker to ring at 661Hz for
    }

    if (val_bouton_pause > 1)
    {
      electrovanneControl(LOW);
      Serial.print("   electro LOW");
    }
    else
    {
      electrovanneControl(HIGH);
      Serial.print("   electro HIGH");
    }

    if (M5.BtnC.isPressed())
    {
      pumpVidangeControl(HIGH);
      Serial.println("   Vidange HIGH");
    }
    else
    {
      pumpVidangeControl(LOW);
      Serial.println("   Vidange LOW");
    }

    if (M5.BtnA.pressedFor(3000, 100))
    {
      auto_mode = 1;
      Serial.println("Mode Automatic");
      state = 0;
    }
  }
  cleanScreen();
  delay(1500);
}

void setup()
{

  M5.begin(true, true, true, true);
  goPlus.begin();
  delay(100);

  Serial.begin(115200);

  // goPlus.hub2_set_io(HUB1_R_O_ADDR, 1); // set digital_output to digital_input
  // goPlus.hub3_set_io(HUB3_R_O_ADDR, 1); // set digital_output to digital_input

  header("Bonjour !", TFT_BLACK);
  M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
  delay(2000);
  cleanScreen();
  /*
    while (true)
    {
      M5.update();
      readBouton();

      water_level = getWaterLevel();
      Serial.print("water level = ");
      Serial.print(water_level);
      Serial.println("% ");
      header(String(water_level).c_str(), TFT_BLACK);
      delay(100);

      if (val_bouton_resume > 1)
      {
        pumpControl(LOW);
        Serial.print("pump LOW");
      }
      else
      {
        pumpControl(HIGH);
        Serial.print("pump HIGH");
      }

      if (val_bouton_pause > 1)
      {
        electrovanneControl(LOW);
        Serial.print("   electro LOW");
      }
      else
      {
        electrovanneControl(HIGH);
        Serial.print("   electro HIGH");
      }

      if (M5.BtnC.isPressed())
      {
        pumpVidangeControl(HIGH);
        Serial.println("   Vidange HIGH");
      }
      else
      {
        pumpVidangeControl(LOW);
        Serial.println("   Vidange LOW");
      }
    }
    */
}

void loop()
{

  switch (state)
  {
  case 0:
    background();
    menu = menuProgramme();
    Serial.println(menu);
    break;

  case 1:
    background();
    menuResume();
    break;

  case 2:
    verifNiveauEau();
    break;

  case 3:
    background();
    remplissage();
    electrovanneControl(HIGH);
    lavage();
    vidange();
    electrovanneControl(LOW);
    essorage();
    beepFinish();

    state = 0;
    break;

  case 10:

    modeManuel();
    break;

  default:
    break;
  }

  /*
  electrovanne_control(HIGH);
  pump_control(HIGH);
  delay(1000);
  electrovanne_control(LOW);
  pump_control(LOW);
  delay(1000);
  */

  /*
  motor_control(80);
  delay(2000);
  motor_control(125);
  delay(2000);
  motor_control(0);
  delay(2000);
  */
  /*
    water_level = getWaterLevel();

    Serial.print("water level = ");
    Serial.print(water_level);
    Serial.println("% ");
    header(String(water_level).c_str(), TFT_BLACK);
    delay(100);
  */
  /*
    // Lire limit_switch + ecrire sur relay PB1 electrovanne
    int val1 = goPlus.hub1_d_read_value(HUB1_R_O_ADDR);  //read digital_input
    Serial.println(val1);
    goPlus.hub1_wire_value(HUB1_W_ADDR,HIGH);
    delay(1000);
    goPlus.hub1_wire_value(HUB1_W_ADDR,LOW);
    delay(1000);
  */
  /*
  // Lire deux INPUT digital
  int val2 = goPlus.hub1_d_read_value(HUB1_R_O_ADDR); // read digital_input
  Serial.print(val2);
  Serial.print("  ");
  int val3 = goPlus.hub1_d_o_read_value(HUB1_R_O_ADDR); // read digital_input
  Serial.println(val3);
  delay(100);
  */
}