#include <Bluepad32.h>

// Definición de pines para los motores
// Motor A (derecho)
#define ENA 25
#define IN1 26
#define IN2 27

// Motor B (izquierdo)
#define ENB 14
#define IN3 12
#define IN4 13

// Velocidades
int velocidadMaxima = 255;
int velocidadActual = 200;

ControllerPtr myControllers[BP32_MAX_GAMEPADS];

// Variables para control del robot
bool robotHabilitado = true;

// Callback cuando se conecta un control
void onConnectedController(ControllerPtr ctl) {
    bool foundEmptySlot = false;
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == nullptr) {
            Serial.printf("Control conectado, indice=%d\n", i);
            ControllerProperties properties = ctl->getProperties();
            Serial.printf("Modelo: %s, VID=0x%04x, PID=0x%04x\n", 
                         ctl->getModelName().c_str(), 
                         properties.vendor_id,
                         properties.product_id);
            myControllers[i] = ctl;
            foundEmptySlot = true;
            break;
        }
    }
    if (!foundEmptySlot) {
        Serial.println("No hay espacio para más controles");
    }
}

// Callback cuando se desconecta un control
void onDisconnectedController(ControllerPtr ctl) {
    bool foundController = false;
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++) {
        if (myControllers[i] == ctl) {
            Serial.printf("Control desconectado, indice=%d\n", i);
            myControllers[i] = nullptr;
            foundController = true;
            break;
        }
    }
    if (!foundController) {
        Serial.println("Control desconectado no encontrado");
    }
    
    // Detener el robot cuando se desconecta el control
    detenerRobot();
}

// Funciones de control de motores
void motorDerecho(int velocidad) {
    if (velocidad > 0) {
        // Adelante
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);
        analogWrite(ENA, velocidad);
    } else if (velocidad < 0) {
        // Atrás
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, HIGH);
        analogWrite(ENA, -velocidad);
    } else {
        // Parar
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, LOW);
        analogWrite(ENA, 0);
    }
}

void motorIzquierdo(int velocidad) {
    if (velocidad > 0) {
        // Adelante
        digitalWrite(IN3, HIGH);
        digitalWrite(IN4, LOW);
        analogWrite(ENB, velocidad);
    } else if (velocidad < 0) {
        // Atrás
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, HIGH);
        analogWrite(ENB, -velocidad);
    } else {
        // Parar
        digitalWrite(IN3, LOW);
        digitalWrite(IN4, LOW);
        analogWrite(ENB, 0);
    }
}

void adelante(int vel) {
    motorIzquierdo(vel);
    motorDerecho(vel);
}

void atras(int vel) {
    motorIzquierdo(-vel);
    motorDerecho(-vel);
}

void derecha(int vel) {
    motorIzquierdo(vel);
    motorDerecho(-vel);
}

void izquierda(int vel) {
    motorIzquierdo(-vel);
    motorDerecho(vel);
}

void detenerRobot() {
    motorIzquierdo(0);
    motorDerecho(0);
}

void aumentarVelocidad() {
    if (velocidadActual + 25 <= velocidadMaxima) {
        velocidadActual += 25;
        Serial.printf("Velocidad aumentada a: %d\n", velocidadActual);
    }
}

void disminuirVelocidad() {
    if (velocidadActual - 25 >= 0) {
        velocidadActual -= 25;
        Serial.printf("Velocidad disminuida a: %d\n", velocidadActual);
    }
}

// Procesar el control tipo gamepad
void procesarGamepad(ControllerPtr ctl) {
    if (!robotHabilitado) return;
    
    // Obtener valores de los ejes
    int ejeX = ctl->axisX();     // -511 a 512 (izquierda/derecha)
    int ejeY = ctl->axisY();     // -511 a 512 (adelante/atrás)
    
    // Mapear valores a rango de velocidad (-255 a 255)
    int velocidadY = map(ejeY, -511, 512, -velocidadActual, velocidadActual);
    int velocidadX = map(ejeX, -511, 512, -velocidadActual, velocidadActual);
    
    // Control diferencial (tank drive)
    int velIzquierda = velocidadY + velocidadX;
    int velDerecha = velocidadY - velocidadX;
    
    // Limitar velocidades
    velIzquierda = constrain(velIzquierda, -velocidadActual, velocidadActual);
    velDerecha = constrain(velDerecha, -velocidadActual, velocidadActual);
    
    // Aplicar movimiento
    if (abs(velocidadY) < 20 && abs(velocidadX) < 20) {
        // Movimiento muy pequeño, detener (zona muerta)
        detenerRobot();
    } else {
        motorIzquierdo(velIzquierda);
        motorDerecho(velDerecha);
    }
    
    // Botones para funciones especiales
    if (ctl->a()) {
        // Disparo/pateo (puedes agregar un servomotor o solenoide aquí)
        Serial.println("¡Disparo!");
        // Aquí puedes agregar código para activar un mecanismo de pateo
    }
    
    if (ctl->b()) {
        // Dribbler (activar motor para mantener el balón)
        Serial.println("Dribbler activado");
        // Aquí puedes activar un motor para el dribbler
    }
    
    if (ctl->x()) {
        // Turbo (velocidad máxima temporal)
        motorIzquierdo(velocidadMaxima);
        motorDerecho(velocidadMaxima);
        delay(500);
        procesarGamepad(ctl); // Restaurar control normal
    }
    
    if (ctl->y()) {
        // Habilitar/deshabilitar robot
        robotHabilitado = !robotHabilitado;
        Serial.printf("Robot %s\n", robotHabilitado ? "habilitado" : "deshabilitado");
        delay(200);
    }
    
    // Botones del pad direccional para control de velocidad
    if (ctl->dpad() == 0x01) { // Arriba
        aumentarVelocidad();
        delay(200);
    }
    
    if (ctl->dpad() == 0x02) { // Abajo
        disminuirVelocidad();
        delay(200);
    }
    
    // Botones laterales
    if (ctl->l1()) {
        // Giro rápido a la izquierda
        izquierda(velocidadActual);
    }
    
    if (ctl->r1()) {
        // Giro rápido a la derecha
        derecha(velocidadActual);
    }
    
    if (ctl->l2()) {
        // Retroceder lentamente
        atras(velocidadActual / 2);
    }
    
    if (ctl->r2()) {
        // Avanzar lentamente
        adelante(velocidadActual / 2);
    }
}

// Procesar todos los controles conectados
void procesarControles() {
    for (auto myController : myControllers) {
        if (myController && myController->isConnected() && myController->hasData()) {
            if (myController->isGamepad()) {
                procesarGamepad(myController);
            }
        }
    }
}

void setup() {
    Serial.begin(115200);
    
    // Configurar pines de motores
    pinMode(ENA, OUTPUT);
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(ENB, OUTPUT);
    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);
    
    // Inicializar motores detenidos
    detenerRobot();
    
    Serial.printf("Firmware Bluepad32: %s\n", BP32.firmwareVersion());
    const uint8_t* addr = BP32.localBdAddress();
    Serial.printf("Dirección Bluetooth: %2X:%2X:%2X:%2X:%2X:%2X\n", 
                  addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
    
    // Configurar Bluepad32
    BP32.setup(&onConnectedController, &onDisconnectedController);
    BP32.forgetBluetoothKeys();
    BP32.enableVirtualDevice(false);
    
    Serial.println("Robot listo para controlar");
    Serial.println("Usa el joystick izquierdo para moverte");
    Serial.println("A: Disparo, B: Dribbler, X: Turbo, Y: Habilitar/Deshabilitar");
    Serial.println("D-Pad Arriba/Abajo: Velocidad, L1/R1: Giros rápidos");
}

void loop() {
    bool dataUpdated = BP32.update();
    if (dataUpdated) {
        procesarControles();
    }
    
    delay(10); // Pequeña pausa para el watchdog
}
