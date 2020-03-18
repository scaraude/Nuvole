//--------------------------------BIBLIOTHEQUE------------------
#include <SPI.h>
#include <SD.h>
#include <Adafruit_NeoPixel.h>
#include <LiquidCrystal.h>

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
uint8_t nextMesure = 0;



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

  //--------------------INIT VARIABLE-----------------
  InitVariables();
}

void loop() {
  // put your main code here, to run repeatedly:
  myFile = SD.open("Nuvole.txt");
  bool recFini = false, musiqueFini = false;
  
  while (!musiqueFini)
  {
    while (!recFini)
    {
      recFini = Rec();
    }
    musiqueFini = Aiguillage();                           //inverse le stock joué et le stock enregistré
    Affiche();
    recFini = false;
  }

  /*while (*compteurP != 0)
    {
    LedOff();
    LedOn();

    LedOff();
    LedOn();
    }*/


  myFile.close();

  Serial.println("an other time ?");

  while (!Serial.available()) {
    ; // Wait for enter key;
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
