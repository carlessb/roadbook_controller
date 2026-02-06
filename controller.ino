// --- Creado por Carles Sallés ---

#include <Bounce2.h>
#include <BleKeyboard.h>
#include <Preferences.h>

// Define el nombre del dispositivo Bluetooth
BleKeyboard bleKeyboard("Roadbook mando");
Preferences preferences;

// --- Pines GPIO para los botones ---
const int Flecha_Arriba = 27;
const int Flecha_Abajo = 25;
const int Flecha_Dcha = 32;
const int Flecha_Izda = 12;
const int LED_PIN = 2; // Pin del LED integrado en el ESP32 D1 Mini

// --- Tiempos de espera para las pulsaciones ---
const int pulsacion = 300; // Delay para pulsación normal
const int pulsacion_rapida = 50; // Delay para repetición rápida en modo Navegación
const unsigned long longPressDuration = 1000; // 1 segundo para activar la repetición rápida

#define NUM_BUTTONS 4 
const uint8_t BUTTON_PINS[NUM_BUTTONS] = {Flecha_Arriba, Flecha_Abajo, Flecha_Dcha, Flecha_Izda}; 

#define ID_ARRIBA 0
#define ID_ABAJO 1
#define ID_DCHA 2
#define ID_IZDA 3

// Se crean los objetos Bounce para el antirrebote de los botones
Bounce buttons[NUM_BUTTONS];

// --- Lógica para el cambio de modo ---
int currentMode = 0; // 0: Navegación, 1: Multimedia

// --- Variables para la pulsación larga de los botones de flecha ---
unsigned long longPressStartDcha = 0;
unsigned long longPressStartIzda = 0;

// Variables para detectar la pulsación larga de la combinación de botones
unsigned long comboStartTime = 0;
const unsigned long comboHoldDuration = 2000; // 2 segundos en milisegundos

// Función para actualizar el estado del LED según el modo
void updateLED() {
  if (currentMode == 0) {
    digitalWrite(LED_PIN, HIGH); // ON para Navegación
  } else {
    digitalWrite(LED_PIN, LOW);  // OFF para Multimedia
  }
}

// Función para dar feedback visual al cambiar de modo
void blinkFeedback() {
  for(int i = 0; i < 3; i++) {
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    delay(100);
    digitalWrite(LED_PIN, !digitalRead(LED_PIN));
    delay(100);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando mando Roadbook con cambio de modo...");

  // Configura el pin del LED
  pinMode(LED_PIN, OUTPUT);

  preferences.begin("roadbook", false);
  currentMode = preferences.getInt("mode", 0);
  
  // Aplicar el estado inicial del LED
  updateLED();

  // Inicializa el teclado Bluetooth
  bleKeyboard.begin();

  // Configura los botones con Bounce2 para el antirrebote (debounce)
  for (int i = 0; i < NUM_BUTTONS; i++) {
    buttons[i].attach(BUTTON_PINS[i], INPUT_PULLUP);
    buttons[i].interval(30); 
  }

  Serial.println("Modo actual: NAVEGACION (Teclas de flecha)");
}

// Función para comprobar si se debe cambiar de modo
void checkModeSwitch() {
  bool dchaPressed = (buttons[ID_DCHA].read() == LOW);
  bool izdaPressed = (buttons[ID_IZDA].read() == LOW);

  if (dchaPressed && izdaPressed) {
    if (comboStartTime == 0) {
      comboStartTime = millis();
    }
    else if (millis() - comboStartTime >= comboHoldDuration) {
      currentMode = 1 - currentMode; 
      
      preferences.putInt("mode", currentMode);
      
      if (currentMode == 0) {
        Serial.println("Modo cambiado a: NAVEGACION (Teclas de flecha)");
      } else {
        Serial.println("Modo cambiado a: MULTIMEDIA (Control de medios)");
      }

      // Feedback visual y actualización del LED
      blinkFeedback();
      updateLED();

      while (buttons[ID_DCHA].read() == LOW || buttons[ID_IZDA].read() == LOW) {
        for(int i=0; i<NUM_BUTTONS; i++) buttons[i].update(); 
        delay(10); 
      }
      
      comboStartTime = 0;
    }
  }
  else {
    comboStartTime = 0;
  }
}

void loop() {
  // 1. Actualiza el estado de todos los botones
  for (int i = 0; i < NUM_BUTTONS; i++) {
    buttons[i].update();
  }
  
  // 2. Comprueba si el usuario quiere cambiar de modo
  checkModeSwitch();

  unsigned long currentMillis = millis();

  // 3. Gestiona las pulsaciones de cada botón según el modo actual

  // --- ARRIBA ---
  if (buttons[ID_ARRIBA].fell()) {
    if (currentMode == 0) {
      bleKeyboard.write(KEY_UP_ARROW);
    } else {
      bleKeyboard.write(KEY_MEDIA_PREVIOUS_TRACK);
    }
  }
  // --- ABAJO ---
  else if (buttons[ID_ABAJO].fell()) {
    if (currentMode == 0) {
      bleKeyboard.write(KEY_DOWN_ARROW);
    } else {
      bleKeyboard.write(KEY_MEDIA_NEXT_TRACK);
    }
  }
  // --- Lógica para Flecha Derecha ---
  else if (buttons[ID_DCHA].read() == LOW) {
    if (longPressStartDcha == 0) {
      longPressStartDcha = currentMillis;
      if (currentMode == 0) {
        bleKeyboard.write(KEY_RIGHT_ARROW);
      } else {
        bleKeyboard.write(KEY_MEDIA_VOLUME_UP);
      }
      delay(pulsacion); 
    } 
    else if (currentMode == 0 && (currentMillis - longPressStartDcha > longPressDuration)) {
      bleKeyboard.write(KEY_RIGHT_ARROW);
      delay(pulsacion_rapida);
    }
  }
  // --- Lógica para Flecha Izquierda ---
  else if (buttons[ID_IZDA].read() == LOW) {
    if (longPressStartIzda == 0) {
      longPressStartIzda = currentMillis;
      if (currentMode == 0) {
        bleKeyboard.write(KEY_LEFT_ARROW);
      } else {
        bleKeyboard.write(KEY_MEDIA_VOLUME_DOWN);
      }
      delay(pulsacion);
    } 
    else if (currentMode == 0 && (currentMillis - longPressStartIzda > longPressDuration)) {
      bleKeyboard.write(KEY_LEFT_ARROW);
      delay(pulsacion_rapida);
    }
  }
  else {
    longPressStartDcha = 0;
    longPressStartIzda = 0;
  }
}
