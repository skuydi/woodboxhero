/*
 * Like all Arduino code - copied from somewhere else :)
 * So don't claim it as your own
 * 
 * skuydi - 25/03
 * 
 * inspired by codes found on -->
 *  https://www.instructables.com/PAD-HERO-Guitar-Hero-Using-Arduino --> https://www.youtube.com/watch?v=j1HRlRGxcZA
 *  https://gist.githubusercontent.com/hvandermillen --> https://www.youtube.com/watch?v=ESsHV3H9lGw
 *  
 
    V   Ti    L  H  MiF
 *  225 30000 55 72 80
*/

#include <Adafruit_NeoPixel.h>
#include <TM1637Display.h>
#include <TM1637TinyDisplay.h>
#include <MIDIUSB.h>
#include "animations.h"

// ===========================
// Définition des constantes
// ===========================

#define LOG 1

#define LED_PIN     10        // NeoPixel
#define LED_PIN_B1  26
#define LED_PIN_B2  27
#define LED_PIN_B3  28
#define LED_PIN_B4  29
#define LED_PIN_B5  30

#define brightnessButton 9    // Bouton mode luminosité (jour/nuit)
#define smallModeButton 12    // Bouton mode 4/5 colonnes
#define chordsModeButton 13   // (non utilisé ici) --> Bouton mode accords
#define soloButton 14         // (non utilisé ici)
#define timerButton 15        // (non utilisé ici)

#define CLK1 4   // TM1637 #1
#define DIO1 5
#define CLK2 6   // TM1637 #2
#define DIO2 7

#define NUMPIXELS   50        // Nombre total de NeoPixels

const int buttonPin[5] = {2, 3, 20, 21, 18}; // Pins boutons de jeu 1 à 5

// ===========================
// Structures et variables globales
// ===========================

// --- Buffer MIDI circulaire (ring buffer)
struct MidiNote {
  byte pitch;
  byte velocity;
  unsigned long timestamp;
  unsigned long duration;
  bool active;
  bool played;
};

const int BUFFER_SIZE = 8;
MidiNote noteBuffer[BUFFER_SIZE];
int writeIndex = 0;
int readIndex = 0;
int playIndex = 0;
unsigned long timeDiff = 0;

// --- Gestion LEDs et affichages
Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_RGB + NEO_KHZ800);
TM1637Display display1_1(CLK1, DIO1);
TM1637TinyDisplay display1_2(CLK1, DIO1);
TM1637Display display2_1(CLK2, DIO2);
TM1637TinyDisplay display2_2(CLK2, DIO2);

const int Matrix5x10[5][10] = {
  {0, 1, 2, 3, 4, 5, 6, 7, 8, 9},
  {19, 18, 17, 16, 15, 14, 13, 12, 11, 10},
  {20, 21, 22, 23, 24, 25, 26, 27, 28, 29},
  {39, 38, 37, 36, 35, 34, 33, 32, 31, 30},
  {40, 41, 42, 43, 44, 45, 46, 47, 48, 49}
};
int ledPos[5][11] = {0}; // toutes LED éteintes

// --- Etats du jeu
const int INITIALISATION = 0;
const int PREPARATION0 = 1;
const int PREPARATION1 = 2;
const int PREPARATION2 = 3;
const int PREPARATION3 = 4;
const int WAIT_FOR_FIRST_NOTE = 7;
const int GAME = 5;
const int FINAL = 6;

int state = INITIALISATION;
int cycleTimer = 0;
int choiceButton = 0;
float speedIncrement = 0;

// --- Variables boutons et anti-rebond (debounce)
volatile bool buttonPressed[5] = {false, false, false, false, false};
unsigned long lastDebounce[5] = {0, 0, 0, 0, 0};
const unsigned long debounceDelay = 100;

// --- Autres variables de jeu
float noteDuration = 250;
float currentCycle = noteDuration;
unsigned long lastCycle = 0;
unsigned long lastMillis = 0;

unsigned long timeToPlay = 30000;
unsigned long remainingTime = 0;

int total_notes = 0;
int total_score = 0;
int high_score = 0;

int animationCounter = 0;
bool animation = HIGH;
int timerState = true;

uint32_t aux_color = 0;

int LcdBrightness = 5;
int LcdBrightnessDay = 5;
int LcdBrightnessNight = 1;
int MatrixBrightness = 50;
int MatrixBrightnessDay = 15;
int MatrixBrightnessNight = 10;

byte midiOutputChannel = 0;

int pitchLow = 61;
int pitchHigh = 78;

uint32_t columnColors[5] = {0, 0, 0, 0, 0};

bool hasViewed = false; 
bool hasDisplayed = false;

int missed_notes = 0;
float accuracy = 0;    
float high_accuracy = 0;    

unsigned long windowMs = 30;
 
// ===========================
// Prototypes des fonctions
// ===========================

void resetBuffer();
void clearMidiBuffer();
void updateLEDS();
void addMelodyLED();
void playNote();
void show_score();
String getNoteName(int noteNumber);
void chiffre_1();
void chiffre_2();
void chiffre_3();
void endGame();
void checkButtons();
void handleMidiInput();

bool hasAnimated = false;

void setup() {
  Serial.begin(9600);
  Serial.println("Starting...");
  pixels.begin();
  pixels.setBrightness(MatrixBrightness);
  pixels.clear();
  for (int i = 0; i < BUFFER_SIZE; i++) noteBuffer[i].active = false;
  pinMode(brightnessButton, INPUT_PULLUP);
  pinMode(timerButton, INPUT_PULLUP);
  pinMode(smallModeButton, INPUT_PULLUP);
  pinMode(chordsModeButton, INPUT_PULLUP);
  pinMode(soloButton, INPUT_PULLUP);
  for (int i = 0; i < 5; i++) {
    pinMode(buttonPin[i], INPUT_PULLUP);
  }
  attachInterrupt(digitalPinToInterrupt(buttonPin[0]), Button1_ISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(buttonPin[1]), Button2_ISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(buttonPin[2]), Button3_ISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(buttonPin[3]), Button4_ISR, FALLING);
  attachInterrupt(digitalPinToInterrupt(buttonPin[4]), Button5_ISR, FALLING);
  pinMode(LED_PIN_B1, OUTPUT);
  pinMode(LED_PIN_B2, OUTPUT);
  pinMode(LED_PIN_B3, OUTPUT);
  pinMode(LED_PIN_B4, OUTPUT);
  pinMode(LED_PIN_B5, OUTPUT);

  display1_1.setBrightness(LcdBrightness);
  display1_1.showNumberDecEx(0, 0b01000000, true, 4, 4);
  display1_2.setBrightness(LcdBrightness);
  display1_2.showAnimation_P(ANIMATION1, FRAMES(ANIMATION1), TIME_MS(50));
  display2_1.setBrightness(LcdBrightness);
  display2_1.showNumberDecEx(0, 0b01000000, true, 4, 4);
  display2_2.setBrightness(LcdBrightness);
  display2_2.showAnimation_P(ANIMATION1, FRAMES(ANIMATION1), TIME_MS(50));
  columnColors[0] = pixels.Color(0, 255, 0);
  columnColors[1] = pixels.Color(255, 0, 0);
  columnColors[2] = pixels.Color(0, 0, 255);
  columnColors[3] = pixels.Color(255, 165, 0);
  columnColors[4] = pixels.Color(0, 255, 255);
  aux_color = pixels.Color(random(255), random(255), random(255));
  Serial.println("System ready!!!");
}

void Button1_ISR() { buttonPressed[0] = true; }
void Button2_ISR() { buttonPressed[1] = true; }
void Button3_ISR() { buttonPressed[2] = true; }
void Button4_ISR() { buttonPressed[3] = true; }
void Button5_ISR() { buttonPressed[4] = true; }

void checkButtons() {
  for (int i = 0; i < 5; i++) {
    if (buttonPressed[i]) {
      unsigned long now = millis();
      if (now - lastDebounce[i] > debounceDelay) {
        choiceButton = i + 1;
        lastDebounce[i] = now;
      }
      buttonPressed[i] = false;
    }
  }
}



void handleMidiInput() {
  static unsigned long lastNoteTimestamp = 0;
  midiEventPacket_t rx = MidiUSB.read();
  if (rx.header != 0) {
    if (rx.byte1 == 0x90 && rx.byte3 > 0) {
      unsigned long now = millis();
      if (now - lastNoteTimestamp > windowMs) {
        if ((writeIndex + 1) % BUFFER_SIZE != readIndex) {
          noteBuffer[writeIndex].pitch = rx.byte2;
          noteBuffer[writeIndex].velocity = rx.byte3;
          noteBuffer[writeIndex].timestamp = now;
          noteBuffer[writeIndex].active = true;
          noteBuffer[writeIndex].played = false;
          if (writeIndex > 0) {
            int prevIndex = (writeIndex - 1 + BUFFER_SIZE) % BUFFER_SIZE;
            timeDiff = noteBuffer[writeIndex].timestamp - noteBuffer[prevIndex].timestamp;
            noteBuffer[writeIndex].duration = timeDiff;
          }
          writeIndex = (writeIndex + 1) % BUFFER_SIZE;
        }
        lastNoteTimestamp = now;
      }
    }
  }
}


/*
// Function to read MIDI events
void handleMidiInput() {
  midiEventPacket_t rx = MidiUSB.read();
  if (rx.header != 0) {
    // Note ON
    if (rx.byte1 == 0x90) {
      byte pitch = rx.byte2;
      byte velocity = rx.byte3;
      if (BUFFER_SIZE < BUFFER_SIZE+1) {
        if (velocity > 0) {
          noteBuffer[writeIndex].pitch = pitch;
          noteBuffer[writeIndex].velocity = velocity;
          noteBuffer[writeIndex].timestamp = millis();
          noteBuffer[writeIndex].active = true;
          noteBuffer[writeIndex].played = false;          
          // Compare the time difference between the current note and the previous one
          if (writeIndex > 0) {
            timeDiff = noteBuffer[writeIndex].timestamp - noteBuffer[writeIndex - 1].timestamp;
            noteBuffer[writeIndex].duration = timeDiff;   // Add the note duration to the buffer
            if (timeDiff < 10) {
              if (digitalRead(chordsModeButton) == HIGH) {
                noteBuffer[writeIndex].active = false;    // Simple mode. only the root note of the chord is played.
              } else {
                noteBuffer[writeIndex].active = true;     // Hard mode. All notes of the chord are played.
              }
            } else {
              if (digitalRead(chordsModeButton) == HIGH) {
                if (timeDiff > 100) {
                //currentCycle = noteBuffer[writeIndex].duration;
                currentCycle = noteDuration;
                }
              } else {
                //currentCycle = noteBuffer[writeIndex].duration;
                currentCycle = noteDuration;
              }
            }
          }
        }
      }
    writeIndex = (writeIndex + 1) % BUFFER_SIZE;
    }
  }
}
*/

void resetBuffer() {
  if (writeIndex > BUFFER_SIZE) {
    for (int i = 0; i < readIndex; i++) noteBuffer[i].active = false;
    for (int i = 0; i < playIndex; i++) noteBuffer[i].played = false;
    writeIndex = 0;
  }
  if (readIndex >= BUFFER_SIZE) readIndex = 0;
  if (playIndex >= BUFFER_SIZE) playIndex = 0;
}

void clearMidiBuffer() {
  for (int i = 0; i < BUFFER_SIZE; i++) {
    noteBuffer[i].active = false;
    noteBuffer[i].played = false;
  }
  writeIndex = 0;
  readIndex = 0;
  playIndex = 0;
  Serial.println("MIDI buffer cleared.");
}

void handleSerialInput(String input) {
  input.trim(); // Nettoie la ligne

  // Mode: tout d'un coup (ex: "150 60000 60 80 100")
  int values[5];
  int found = 0;
  int lastSpace = -1;

  // On découpe la ligne à chaque espace
  for (int i = 0; i < 5; i++) {
    int spaceIndex = input.indexOf(' ', lastSpace + 1);
    String part;
    if (spaceIndex == -1) {
      part = input.substring(lastSpace + 1);
    } else {
      part = input.substring(lastSpace + 1, spaceIndex);
    }
    part.trim();
    values[i] = part.toInt();
    lastSpace = spaceIndex;
    found++;
    if (spaceIndex == -1) break;
  }

  // Vérifie qu'il y a bien 5 valeurs
  if (found == 5) {
    noteDuration = constrain(values[0], 50, 600);
    timeToPlay   = constrain(values[1], 5000, 300000);
    pitchLow     = constrain(values[2], 0, 127);
    pitchHigh    = constrain(values[3], 0, 127);
    windowMs     = constrain(values[4], 10, 1000);

    Serial.println("Paramètres modifiés :");
    Serial.print("noteDuration = "); Serial.println(noteDuration);
    Serial.print("timeToPlay   = "); Serial.println(timeToPlay);
    Serial.print("pitchLow     = "); Serial.println(pitchLow);
    Serial.print("pitchHigh    = "); Serial.println(pitchHigh);
    Serial.print("windowMs     = "); Serial.println(windowMs);
  } else {
    Serial.println("Erreur : Veuillez entrer 5 valeurs séparées par des espaces !");
    Serial.println("Exemple : 150 60000 60 80 100");
  }
}


void loop() {
  handleMidiInput();
  resetBuffer();
  checkButtons();

  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    handleSerialInput(input);
  }

  if (digitalRead(brightnessButton) == LOW) {
    LcdBrightness = LcdBrightnessNight;
    MatrixBrightness = MatrixBrightnessNight;
  } else {
    LcdBrightness = LcdBrightnessDay;
    MatrixBrightness = MatrixBrightnessDay;
  }
  display1_1.setBrightness(LcdBrightness);
  display1_2.setBrightness(LcdBrightness);
  display2_1.setBrightness(LcdBrightness);
  display2_2.setBrightness(LcdBrightness);
  pixels.setBrightness(MatrixBrightness);

  if (digitalRead(timerButton) == LOW) timerState = true;
  else timerState = true;

  if (millis() - lastCycle > currentCycle) cycleTimer = 1;

  switch (state) {
    case INITIALISATION:
      Serial.println("Initializing...");
      display1_1.showNumberDecEx(accuracy, 0b00000000, true, 4, 4);
      display2_1.showNumberDecEx(high_accuracy, 0b00000000, true, 4, 4);
      choiceButton = 0;
      total_score = 0;
      total_notes = 0;
      for (int i = 0; i < 5; i++)
        for (int j = 0; j < 10; j++)
          ledPos[i][j] = 0;
      clearMidiBuffer();

      if (digitalRead(brightnessButton) == LOW)
        Serial.println("* Night mode");
      else
        Serial.println("* Day mode");

      if (digitalRead(timerButton) == LOW)
        Serial.println("* No error mode");
      else
        Serial.println("* Timer mode");

      if (digitalRead(smallModeButton) == LOW)
        Serial.println("* Buttons [5] mode");
      else
        Serial.println("* Buttons [4] mode");

      if (digitalRead(chordsModeButton) == LOW)
        Serial.println("* Solo mode");
      else
        Serial.println("* Chord mode");

      currentCycle = noteDuration;
      hasViewed = false;
      delay(1000);
      state = PREPARATION0;
      break;

    case PREPARATION0:
      if (!hasViewed) {
        choiceButton = 0;
        Serial.println("Preparation 0 - Animation...");
        hasViewed = true;
      }
      if (cycleTimer) {
        pixels.setPixelColor(Matrix5x10[random(5)][random(10)], pixels.Color(random(255), random(255), random(255)));
        pixels.show();
        lastCycle = millis();
        cycleTimer = 0;
      }
      if (choiceButton == 1) {
        high_score = 0;
        choiceButton = 0;
        hasViewed = false;
      }
      if (choiceButton == 3) {
        choiceButton = 0;
        pixels.clear();
        pixels.show();
        hasViewed = false;
        hasDisplayed = false;
        delay(1000);
        state = PREPARATION1;
      }
      break;

    case PREPARATION1:
      chiffre_3();
      if (!hasViewed) {
        choiceButton = 0;
        Serial.print("Preparation 1 - Velocity: ");
        Serial.println(noteDuration);
        display1_1.showNumberDecEx(noteDuration, 0b00000000, true, 4, 4);
        display2_1.showNumberDecEx(noteDuration, 0b00000000, true, 4, 4);
        hasViewed = true;
      }
      if (cycleTimer) {
        lastCycle = millis();
        cycleTimer = 0;
      }
      if (choiceButton == 2) {
        if (noteDuration > 50) noteDuration -= 5;
        hasViewed = false;
        choiceButton = 0;
      }
      else if (choiceButton == 1) {
        if (noteDuration < 600) noteDuration += 5;
        hasViewed = false;
        choiceButton = 0;
      }
      if (choiceButton == 5) {
        if (noteDuration > 50) noteDuration -= 10;
        hasViewed = false;
        choiceButton = 0;
      }
      else if (choiceButton == 4) {
        if (noteDuration < 600) noteDuration += 10;
        hasViewed = false;
        choiceButton = 0;
      }
      if (choiceButton == 3) {
        choiceButton = 0;
        pixels.clear();
        pixels.show();
        hasViewed = false;
        hasDisplayed = false;
        delay(1000);
        state = PREPARATION2;
      }
      break;

    case PREPARATION2:
      chiffre_2();
      if (!hasViewed) {
        choiceButton = 0;
        Serial.print("Preparation 2 - Pitch Low: ");
        Serial.print(pitchLow);
        Serial.print(" - Pitch High: ");
        Serial.print(pitchHigh);
        Serial.print(" - Velocity: ");
        Serial.println(noteDuration);
        display1_1.showNumberDecEx(pitchLow, 0b00000000, true, 4, 4);
        display2_1.showNumberDecEx(pitchHigh, 0b00000000, true, 4, 4);
        hasViewed = true;
      }
      if (cycleTimer) {
        lastCycle = millis();
        cycleTimer = 0;
      }
      if (choiceButton == 5) {
        if (pitchLow > 0) pitchLow--;
        hasViewed = false;
        choiceButton = 0;
      }
      else if (choiceButton == 4) {
        if (pitchLow < 128) pitchLow++;
        hasViewed = false;
        choiceButton = 0;
      }
      if (choiceButton == 2) {
        if (pitchHigh > 0) pitchHigh--;
        hasViewed = false;
        choiceButton = 0;
      }
      else if (choiceButton == 1) {
        if (pitchHigh < 128) pitchHigh++;
        hasViewed = false;
        choiceButton = 0;
      }
      if (choiceButton == 3) {
        choiceButton = 0;
        pixels.clear();
        pixels.show();
        hasViewed = false;
        hasDisplayed = false;
        delay(1000);
        state = PREPARATION3;
      }
      break;

    case PREPARATION3:
      chiffre_1();
      if (!hasViewed) {
        choiceButton = 0;
        Serial.print("Preparation 3 - Time to play: ");
        Serial.print(timeToPlay / 1000);
        Serial.print(" [s.] - MIDI filter: ");
        Serial.println(speedIncrement);
        display1_1.showNumberDecEx(timeToPlay / 1000, 0b00000000, true, 4, 4);
        //display2_1.showNumberDecEx(speedIncrement * 10, 0b00000000, true, 4, 4);
        display2_1.showNumberDecEx(windowMs, 0b00000000, true, 4, 4);
        hasViewed = true;
      }
      if (choiceButton == 5) {
        if (timeToPlay > 0) timeToPlay -= 5000;
        hasViewed = false;
        choiceButton = 0;
      }
      else if (choiceButton == 4) {
        if (timeToPlay < 300000) timeToPlay += 5000;
        hasViewed = false;
        choiceButton = 0;
      }
      if (choiceButton == 2) {
        //if (speedIncrement > 0.1) speedIncrement -= 0.1;
        if (windowMs > 30 && digitalRead(chordsModeButton) == LOW) {
          windowMs -= 20;
        } else if (windowMs > 130 && digitalRead(chordsModeButton) != LOW){
          windowMs -= 100;
        }
         hasViewed = false;
        choiceButton = 0;
      }
      else if (choiceButton == 1) {
        //if (speedIncrement < 5) speedIncrement += 0.1;       
        if (windowMs < 1000 && digitalRead(chordsModeButton) == LOW) {
          windowMs += 20;
        } else if (windowMs < 1000 && digitalRead(chordsModeButton) != LOW) {
         windowMs += 100;
        }
        hasViewed = false;
        choiceButton = 0;
      }
      if (choiceButton == 3) {
        choiceButton = 0;
        pixels.clear();
        pixels.show();
        hasViewed = false;
        hasDisplayed = false;
        lastCycle = millis();
        delay(1000);
        state = WAIT_FOR_FIRST_NOTE;
      }
      break;

    case WAIT_FOR_FIRST_NOTE:
      // On attend la première note MIDI avant de démarrer la partie
      display1_1.showNumberDecEx(0, 0b00000000, true, 4, 4);
      display2_1.showNumberDecEx(0, 0b00000000, true, 4, 4);
      if (writeIndex != readIndex) {
        // Première note reçue : démarrage du jeu synchronisé !
        lastMillis = millis();
        state = GAME;
        hasViewed = false;
      }
      break;

    case GAME:
      if (!hasViewed) {
        choiceButton = 0;
        Serial.println("Game started...");
        hasViewed = true;
      }
  
      if (cycleTimer) {
        // Éteindre toutes les LEDs classiques d'un coup
        for (int i = 0; i < 5; i++) {
          digitalWrite(LED_PIN_B1 + i, LOW);
        }
        updateLEDS();
        addMelodyLED();
        lastCycle = millis();
        cycleTimer = 0;
      }

      // Gestion des LEDs/matrice/boutons
      for (int i = 0; i < 5; i++) {
        if (ledPos[i][9] == 1) {
          ledPos[i][9] = 0;
          if (noteDuration > 200) noteDuration -= speedIncrement;
            pixels.setPixelColor(Matrix5x10[i][9], pixels.Color(0, 0, 0));
            pixels.show();
            total_notes++;
            if (choiceButton == (i + 1)) {
              choiceButton = 0;
              digitalWrite(LED_PIN_B1 + i, HIGH); // Idem, si les pins se suivent
              total_score++;
              playNote();
            }
          }
        }
  
      // Fin de partie
      if (timerState && ((millis() - lastMillis) > timeToPlay)) endGame();
      else if (!timerState && (choiceButton || ledPos[0][9] == 1 || ledPos[1][9] == 1 || ledPos[2][9] == 1 || ledPos[3][9] == 1 || ledPos[4][9] == 1))
      endGame();
      break;
case FINAL:
  if (!hasAnimated) {
    Serial.print("Accuracy = ");
    Serial.println(accuracy);
    
    int ledsToLight = map(accuracy, 0, 100, 0, NUMPIXELS);  // 0 à 50 LEDs allumées
    pixels.clear();

    for (int i = 0; i < ledsToLight; i++) {
      float ratio = float(i) / NUMPIXELS;
      uint8_t red = uint8_t(255 * (1.0 - ratio));
      uint8_t green = uint8_t(255 * ratio);
      uint8_t blue = 0;

      pixels.setPixelColor(i, pixels.Color(red, green, blue));
      pixels.show();
      delay(40);
    }

    hasAnimated = true;  // ✅ animation effectuée une seule fois
    choiceButton = 0;
  }

  // Ensuite on attend un appui pour redémarrer
  if (choiceButton) {
    Serial.println("Restarting...");
    choiceButton = 0;
    animationCounter = 0;
    hasAnimated = false;
    delay(2000);
    state = INITIALISATION;
  }
  break;


  }
}

void updateLEDS() {
  remainingTime = (timeToPlay-(millis() - lastMillis)) / 1000;
  display2_1.showNumberDecEx(remainingTime, 0b00000000, true, 4, 4);
  display1_1.showNumberDecEx(total_score, 0b00000000, true, 4, 4);

  for (int i = 0; i < 5; i++) {
    if (ledPos[i][9] == 1) {
      missed_notes++;
    }
  }

  for (int i = 0; i < 5; i++)
    for (int j = 10; j >= 0; j--)
      ledPos[i][j] = (j == 0) ? 0 : ledPos[i][j - 1];

  for (int i = 0; i < 5; i++)
    for (int j = 0; j < 10; j++)
      pixels.setPixelColor(Matrix5x10[i][j], ledPos[i][j] ? columnColors[i] : pixels.Color(0, 0, 0));
  pixels.show();
}

void addMelodyLED() {
  if (readIndex != writeIndex) {
    MidiNote &currentNote = noteBuffer[readIndex];
    if (currentNote.active) {
      int column;
      if (digitalRead(smallModeButton) == LOW) {
        if (currentNote.pitch > pitchHigh) pitchHigh = currentNote.pitch;
        if (currentNote.pitch < pitchLow) pitchLow = currentNote.pitch;
        column = map(currentNote.pitch, pitchLow, pitchHigh, 4, 0);
      } else {
        if (currentNote.pitch > pitchHigh) pitchHigh = currentNote.pitch;
        if (currentNote.pitch < pitchLow) pitchLow = currentNote.pitch;
        column = map(currentNote.pitch, pitchLow, pitchHigh, 3, 0);
      }
      ledPos[column][0] = 1;
      pixels.setPixelColor(Matrix5x10[column][0], columnColors[column]);
      pixels.show();
      currentNote.active = false;
    }
    readIndex = (readIndex + 1) % BUFFER_SIZE;
  }
}

void playNote() {
  if (playIndex != writeIndex) {
    MidiNote &currentNote = noteBuffer[playIndex];
    if (!currentNote.played) {
      String noteName = getNoteName(currentNote.pitch);
      noteOn(midiOutputChannel, currentNote.pitch, 100);
      delay(150);
      noteOff(midiOutputChannel, currentNote.pitch, 0);
      currentNote.played = true;
     #if LOG == 1 
      Serial.print("Game speed: ");
      Serial.print(currentCycle);
      Serial.print(" - Timestamp: ");
      Serial.print(noteBuffer[writeIndex].timestamp);
      Serial.print(" - TimeDiff: ");
      Serial.print(timeDiff);
      Serial.print(" - Buffer W: ");
      Serial.print(writeIndex);
      Serial.print(" - Buffer R: ");
      Serial.print(readIndex);
      Serial.print(" - Buffer P: ");
      Serial.print(playIndex);
      Serial.print(" - MIDI: ");
      Serial.print(currentNote.pitch);
      Serial.print(" - Note: ");
      Serial.print(noteName);
      Serial.print(" - Velocity: ");
      Serial.print(currentNote.velocity);
      Serial.print(" - Total notes: ");
      Serial.print(total_notes);
      Serial.print(" - Total score: ");
      Serial.print(total_score);
      Serial.print(" - Remaining time: ");
      Serial.print(remainingTime);
      Serial.println(" s.");     
     #endif
    }
    playIndex = (playIndex + 1) % BUFFER_SIZE;
  }
}

void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x91 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
  MidiUSB.flush();
}
void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x81 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
  MidiUSB.flush();
}

void endGame() {
  digitalWrite(LED_PIN_B1, LOW);
  digitalWrite(LED_PIN_B2, LOW);
  digitalWrite(LED_PIN_B3, LOW);
  digitalWrite(LED_PIN_B4, LOW);
  digitalWrite(LED_PIN_B5, LOW);
  pixels.clear();
  pixels.show();
  choiceButton = 0;
  for (unsigned i = 0; i <= 49; i++) {
    pixels.setPixelColor(i, pixels.Color(random(255), random(255), random(255)));
  }
  pixels.show();
  delay(1500);
  pixels.clear();
  pixels.show();
  show_score();
  state = FINAL;
}

void show_score() {
  accuracy = 0.0;
  if (total_notes > 0) {
    accuracy = 100.0 * float(total_score) / float(total_notes);
  }
  if (total_score > high_score) {
    high_score = total_score;
  }
  if (accuracy > high_accuracy) {
    high_accuracy = accuracy;
  }
  Serial.println("------------------------------------------------------------------------------------------------------------------------------------------------------------");
  Serial.print(" *** GAME OVER *** ");
  Serial.print(" - TOTAL NOTES: ");
  Serial.print(total_notes);
  Serial.print(" - PLAYED NOTES: ");
  Serial.print(total_score);
  Serial.print(" - ACCURACY: ");
  Serial.print(accuracy, 1);
  Serial.print("%");
  Serial.print(" - PLAYED NOTES [HIGH]: ");
  Serial.print(high_score);
  Serial.print(" - ACCURACY [HIGH]: ");
  Serial.print(high_accuracy, 1);
  Serial.println("%");
  Serial.println("------------------------------------------------------------------------------------------------------------------------------------------------------------");

  display1_1.showNumberDecEx(total_score, 0b00000000, true, 4, 4);
  display2_1.showNumberDecEx(accuracy, 0b00000000, true, 4, 4);
}

String getNoteName(int noteNumber) {
  const char* noteNames[] = {
    "C-1", "C#-1", "D-1", "D#-1", "E-1", "F-1", "F#-1", "G-1", "G#-1", "A-1", "A#-1", "B-1",
    "C0", "C#0", "D0", "D#0", "E0", "F0", "F#0", "G0", "G#0", "A0", "A#0", "B0",
    "C1", "C#1", "D1", "D#1", "E1", "F1", "F#1", "G1", "G#1", "A1", "A#1", "B1",
    "C2", "C#2", "D2", "D#2", "E2", "F2", "F#2", "G2", "G#2", "A2", "A#2", "B2",
    "C3", "C#3", "D3", "D#3", "E3", "F3", "F#3", "G3", "G#3", "A3", "A#3", "B3",
    "C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4", "A4", "A#4", "B4",
    "C5", "C#5", "D5", "D#5", "E5", "F5", "F#5", "G5", "G#5", "A5", "A#5", "B5",
    "C6", "C#6", "D6", "D#6", "E6", "F6", "F#6", "G6", "G#6", "A6", "A#6", "B6",
    "C7", "C#7", "D7", "D#7", "E7", "F7", "F#7", "G7", "G#7", "A7", "A#7", "B7",
    "C8", "C#8", "D8", "D#8", "E8", "F8", "F#8", "G8", "G#8", "A8", "A#8", "B8",
    "C9", "C#9", "D9", "D#9", "E9", "F9", "F#9", "G9"
  };
  if (noteNumber >= 0 && noteNumber <= 127) {
    return noteNames[noteNumber];
  } else {
    return "Numéro de note invalide";
  }
}

void chiffre_1() { for (int i = 20; i < 30; i++) pixels.setPixelColor(i, pixels.Color(0, 255, 0)); pixels.show(); }
void chiffre_2() { for (int i = 10; i < 20; i++) pixels.setPixelColor(i, pixels.Color(255, 50, 0)); for (int i = 30; i < 40; i++) pixels.setPixelColor(i, pixels.Color(255, 50, 0)); pixels.show(); }
void chiffre_3() { for (int i = 10; i < 40; i++) pixels.setPixelColor(i, pixels.Color(255, 0, 0)); pixels.show(); }
