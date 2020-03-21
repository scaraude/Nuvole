//--------------------------------BIBLIOTHEQUE------------------
#include <SPI.h>
#include <SD.h>
#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal.h>

//---------------------------CONF TAB--------------------------
#define NOTEREC 60

//--------------------------------LED COLORS------------------
#define BLEU pixels.Color(0,0,255)
#define ROUGE pixels.Color(255,0,0)
#define VERT pixels.Color(0,255,0)
#define TURQUOISE pixels.Color(0,150,150)
#define ORANGE pixels.Color(150,50,0)

//--------------------------------CONFIG LED------------------
#define PIN        22 // On Trinket or Gemma, suggest changing this to 1
#define BRIGHTNESS 100
#define NUMPIXELS 144 // Popular NeoPixel ring size

//--------------------------------DECL. LED------------------
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

//-------------------------------CONFIG BUTTONS LCD-----------
#define btnRIGHT  1
#define btnUP     2
#define btnDOWN   3
#define btnLEFT   4
#define btnSELECT 5
#define btnNONE   0

#define DELAY     500

//--------------------------------DECL. LCD-------------------
// Select the pin used on LCD
LiquidCrystal lcd(8, 9, 30, 5, 6, 7);

//--------------------------------DECL. FILE------------------
File myFile;

//--------------------------------STRUCTURE NOTE--------------
typedef struct {
  uint8_t led;
  uint8_t tdebut;
  uint8_t duree;
  uint8_t main;
} note;

//---------------------------CONF TAB--------------------------
#define NOTEREC 60

//--------------------------------VARIABLE GLOB.------------------
note stockGauche[NOTEREC], stockDroit[NOTEREC];
uint8_t compteurGauche, compteurDroit;

note *stockP, *stockR;
uint8_t *compteurP, *compteurR;

bool JouerDansDroit;
uint8_t nextMesure;
int tempo = 80;
uint8_t curseurP;
static unsigned long tRec;


void setup() {
  // put your setup code here, to run once:


  //--------------------INIT SERIAL-----------------
  Serial.begin(9600);
  while (!Serial) {
  }

  //--------------------INIT SD-----------------
  Serial.print("Initializing SD card...");

  if (!SD.begin(3)) {
    Serial.println("initialization failed!");
    while (1);
  }
  Serial.println("initialization done.");

  //--------------------INIT LED-----------------
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.setBrightness(BRIGHTNESS);
  pixels.show();   // Send the updated pixel colors to the hardware.

  //--------------------INIT LCD--------------------
  lcd.begin(16, 2);

  //------------------------FILE-----------------
  myFile = ChooseFile();
}

void loop() {

  // put your main code here, to run repeatedly:
  bool recFini = false;
  static bool fileChange = false;
  int debut = 1;
  int fin = 1000;
  char *fileName = myFile.name();

  if (fileChange)
  {
    myFile = ChooseFile();
    fileChange = false;
    fileName = myFile.name();
  }

  if (!myFile)
    myFile = SD.open(fileName);
  //--------------------INIT VARIABLE-----------------
  InitVariables();

  // -------------------PREMIER REMPLISSAGE---------
  Rec();

  while (nextMesure < debut + 1)
  {
    while (!recFini)
    {
      recFini = Rec();
    }
    Aiguillage();

    recFini = false;
  }
  //-------------------------------------------------
  tRec = micros();

  while (stockP[0].led != 0 && myFile)  // Musique fini (rien dans le stock play après l'aiguillage
  {
    while (stockP[0].led != 0 && myFile) // Lecture de la mesure fini (plus rien dans le stock play)
    {
      while (!recFini)
      {
        Lecture();
        recFini = Rec();
      }
      if (read_LCD_buttons() == btnSELECT)
        myFile.close();
      else if (read_LCD_buttons())
      {
        fileChange = true;
        myFile.close();
      }
      Lecture();
    }
    if (nextMesure >= fin)
      myFile.close();
    Aiguillage();                           //inverse le stock joué et le stock enregistré

    recFini = false;
  }
  if (myFile)
    myFile.close();
}

void Lecture()
{
  float sensorValue = 0.006466 * analogRead(A8) - 2.549569;
  float utemps = 60000000 / (tempo * 4) * sensorValue;
  static bool offPassed = false, firstOnPassed = false;

  if (micros() >= tRec + utemps * 0.7 && !offPassed) // check si il faut eteindre
  {
    curseurP++;
    LedOff();
    //Serial.println("into LEDOFF");
    offPassed = true;
  }

  if (micros() >= tRec + utemps || !firstOnPassed)
  {
    LedOn();
    tRec = micros();
    offPassed = false;
    firstOnPassed = true;
    //Serial.println("into LEDON");
  }
}

void LedOff()
{
  for (int i = 0; i <= *compteurP; i++)
  {
    if (stockP[i].led == 253)
    {
      tempo = stockP[i].tdebut;

      stockP[*compteurP].led = 0;
      stockP[*compteurP].tdebut = 0;
      Decalage(i);

      if (*compteurP > 0)
      {
        (*compteurP)--;
        i--;
      }
    }
    else if (curseurP >= (stockP[i].tdebut + stockP[i].duree))
    {
      //Serial.println("dans Erase");
      pixels.setPixelColor(stockP[i].led, 0);
      pixels.show();

      stockP[i].led = 0;
      stockP[i].tdebut = 0;
      stockP[i].duree = 0;
      stockP[i].main = 0;
      Decalage(i);

      if (*compteurP > 0)
      {
        (*compteurP)--;
        i--;
      }
    }

  }
}

void Decalage(int i)
{
  for (int t = 1; stockP[i + t].led != 0; t++)
  {
    stockP[i + t - 1].led = stockP[i + t].led;
    stockP[i + t].led = 0;
    stockP[i + t - 1].tdebut = stockP[i + t].tdebut;
    stockP[i + t].tdebut = 0;
    stockP[i + t - 1].duree = stockP[i + t].duree;
    stockP[i + t].duree = 0;
    stockP[i + t - 1].main = stockP[i + t].main;
    stockP[i + t].main = 0;
  }
}

void LedOn()
{
  for (int i = 0; i <= *compteurP; i++)
  {
    if (stockP[i].tdebut == curseurP)
    {
      switch (stockP[i].main)
      {
        case 1 : pixels.setPixelColor(stockP[i].led, BLEU); break;
        case 2 : pixels.setPixelColor(stockP[i].led, ROUGE); break;
        case 3 : pixels.setPixelColor(stockP[i].led, TURQUOISE); break;
        case 4 : pixels.setPixelColor(stockP[i].led, ORANGE); break;
      }
      pixels.show();
    }
  }
}


void InitVariables()
{
  //------------------------INIT STOCK
  memset(&stockGauche, 0, sizeof(stockGauche));
  memset(&stockDroit, 0, sizeof(stockDroit));

  stockP = stockDroit;
  stockR = stockGauche;

  //------------------------INIT DRAP
  JouerDansDroit = true;

  //------------------------INIT COMPTEUR
  compteurGauche = 0;
  compteurDroit = 0;

  //---------------------------INIT POINTEUR
  compteurP = &compteurDroit;
  compteurR = &compteurGauche;

  nextMesure = 0;
  curseurP = 0;

  tRec = 0;
}

bool Rec()
{
  uint8_t b;

  //Serial.println("dans Rec");

  if (myFile.available())
    b = myFile.read();
  else
    return true;

  if (b == 254)
  {
    nextMesure = myFile.read();
    return true;
  }
  else if (b == 253 || b == 255)             // détecte les tempo, erreur
  {
    stockR[*compteurR].led = b;                  //enregistre la spé
    stockR[*compteurR].tdebut = myFile.read();   //puis la valeur
    (*compteurR)++;
  }
  else if (b == 251)                    // détecte les silences
  {
    for (int i = 0; i < 3; i++)         // ignore le silence (avance dans le fichier)
      myFile.read();
  }
  else                                  //enregistre toutes les infos de la note
  {
    stockR[*compteurR].led = b;
    stockR[*compteurR].tdebut = myFile.read();
    stockR[*compteurR].duree = myFile.read();
    stockR[*compteurR].main = myFile.read();
    (*compteurR)++;
  }
  return false;                         // baisse drapeau silence
}

void Aiguillage()                       //INVERSE LES TABLEAUX JOUER ET ENREGISTRER (inverse les pointeurs)
{
  //Serial.println("dans Aiguillage");
  if (JouerDansDroit)
  {
    memset(&stockDroit, 0, sizeof(stockDroit));
    compteurDroit = 0;

    stockP = stockGauche;
    compteurP = &compteurGauche;

    stockR = stockDroit;
    compteurR = &compteurDroit;

    JouerDansDroit = false;
  }
  else
  {
    memset(&stockGauche, 0, sizeof(stockGauche));
    compteurGauche = 0;

    stockP = stockDroit;
    compteurP = &compteurDroit;

    stockR = stockGauche;
    compteurR = &compteurGauche;

    JouerDansDroit = true;
  }

  curseurP = 0;
  //Affiche();

  pixels.clear();
  pixels.show();

  lcd.clear();
  lcd.setCursor(0, 0);           // The cursor moves to the beginning of the second line.
  lcd.print(myFile.name());
  lcd.setCursor(7, 1);           // The cursor moves to the beginning of the second line.
  lcd.print(nextMesure);
}

File ChooseFile()
{

  //f.close() le fichier en cour (myFile)
  File file;
  File copy;

  int compteurFichier = 0;
  int key = btnNONE;

  lcd.clear();
  lcd.setCursor(0, 0);           // The cursor moves to the beginning of the second line.

  file = SD.open("/");
  copy = file.openNextFile();
  compteurFichier++;
  copy = file.openNextFile();
  compteurFichier++;
  copy = file.openNextFile();
  compteurFichier++;
  //test = copy;

  while (key != btnSELECT)
  {
    key = read_LCD_buttons();  // read key

    lcd.setCursor(0, 0);           // The cursor moves to the beginning of the second line.
    lcd.print(copy.name());

    switch (key)               // display key
    {
      case btnDOWN:
        {
          lcd.clear();

          copy = file.openNextFile();
          compteurFichier++;

          if (!copy) {
            compteurFichier = 0;

            file.rewindDirectory();
            copy = file.openNextFile();
            compteurFichier++;
            copy = file.openNextFile();
            compteurFichier++;
            copy = file.openNextFile();
            compteurFichier++;
          }
          delay(DELAY);
          break;
        }
      case btnUP:
        {
          lcd.clear();

          int j;
          file.rewindDirectory();
          for (j = 0; j < compteurFichier - 1; j++)
            copy = file.openNextFile();
          compteurFichier = j;
          delay(DELAY);
          break;
        }
      case btnSELECT:
        {
          lcd.clear();
          delay(DELAY);
          break;
        }
    }
  }

  lcd.clear();
  delay(DELAY);
  return copy;
}

int read_LCD_buttons()
{

  int adc_key_in  = analogRead(0);          // read analog A0 value

  // when read the 5 key values in the vicinity of the following：0,144,329,504,741
  // By setting different threshold, you can read the one button
  if (adc_key_in > 1000) return btnNONE;
  if (adc_key_in < 50)   return btnRIGHT;
  if (adc_key_in < 250)  return btnUP;
  if (adc_key_in < 450)  return btnDOWN;
  if (adc_key_in < 650)  return btnLEFT;
  if (adc_key_in < 850)  return btnSELECT;

  return btnNONE;
}

void Affiche()
{
  //Serial.println("dans Affichage");
  for (int i = 0; i < *compteurP; i++)
  {
    Serial.print(stockP[i].led);
    Serial.print("\t");
    Serial.print(stockP[i].tdebut);
    Serial.print("\t");
    Serial.print(stockP[i].duree);
    Serial.print("\t");
    Serial.println(stockP[i].main);
  }
  Serial.println(nextMesure);
}
