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
int tempo = 40;
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

  //--------------------INIT VARIABLE-----------------
  InitVariables();

  //------------------------FILE-----------------
  myFile = SD.open("Nuvole.txt");

  // -------------------PREMIER REMPLISSAGE---------
  bool recFini = false;

  Rec();

  while (!recFini)
  {
    //Serial.println("Dans 3 eme boucle");
    recFini = Rec();
  }
  Aiguillage();
}

void loop() {
  // put your main code here, to run repeatedly:
  bool recFini = false, musiqueFini = false;

  while (!musiqueFini)
  {
    //Serial.println(*compteurP);
    //Serial.println("Dans 1 eme boucle");
    while (*compteurP != 0)
    {
      //Serial.println("Dans 2 eme boucle");
      while (!recFini)
      {
        //Serial.println("Dans 3 eme boucle");
        Lecture();
        recFini = Rec();
      }
      Lecture();
    }
    musiqueFini = Aiguillage();                           //inverse le stock joué et le stock enregistré
    //Affiche();

    recFini = false;
    tRec = micros();
  }

  pixels.show();
  tRec = micros();
}

void Lecture()
{
  float utemps = 60000000 / (tempo *4);
  static bool offPassed = false;

  if (micros() >= tRec + utemps * 0.5 && !offPassed) // check si il faut eteindre
  {
    curseurP++;
    LedOff();
    //Serial.println("into LEDOFF");
    offPassed = true;
  }

  if (micros() >= tRec + utemps)
  {
    LedOn();
    tRec = micros();
    offPassed = false;
    //Serial.println("into LEDON");
  }
}

void LedOff()
{
  for (int i = 0; i <= *compteurP; i++)
  {
    if (curseurP >= (stockP[i].tdebut + stockP[i].duree))
    {
      pixels.setPixelColor(stockP[i].led, 0);
      pixels.show();

      stockP[*compteurP].led = 0;
      stockP[*compteurP].tdebut = 0;
      stockP[*compteurP].duree = 0;
      stockP[*compteurP].main = 0;

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

      if (*compteurP > 0)
      {
        (*compteurP)--;
        i--;
      }
    }
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

bool Aiguillage()                       //INVERSE LES TABLEAUX JOUER ET ENREGISTRER (inverse les pointeurs)
{
  //Serial.println("dans Aiguillage");
  if (JouerDansDroit)
  {
    memset(&stockDroit, 0, sizeof(stockDroit)); //rénitialise le tableau pour l'enregistrement
    compteurDroit = 0;

    stockP = stockGauche;
    compteurP = &compteurGauche;

    stockR = stockDroit;
    compteurR = &compteurDroit;

    JouerDansDroit = false;
  }
  else
  {
    memset(&stockGauche, 0, sizeof(stockGauche)); //rénitialise le tableau pour l'enregistrement
    compteurGauche = 0;

    stockP = stockDroit;
    compteurP = &compteurDroit;

    stockR = stockGauche;
    compteurR = &compteurGauche;

    JouerDansDroit = true;
  }

  curseurP = 0;
  Affiche();

  if (!myFile.available())
    return true;

  return false;
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
