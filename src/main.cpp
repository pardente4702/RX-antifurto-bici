#include <esp_now.h>
#include <WiFi.h>

#define LED_PIN 2  // LED sul ricevitore

typedef struct {
  uint8_t id;
  bool motion;
} msg_t;

// MAC del trasmettitore (solo se vuoi rispondere a uno specifico)
uint8_t transmitterMAC[6];

// Callback per la ricezione pacchetti
void onReceive(const uint8_t *mac_addr, const uint8_t *data, int len) {
  if (len != sizeof(msg_t)) return; // controllo lunghezza

  msg_t msg;
  memcpy(&msg, data, sizeof(msg_t));

  // Salvo MAC trasmettitore per ACK
  memcpy(transmitterMAC, mac_addr, 6);

  if (msg.motion) {
    Serial.println("ALLARME PIR RICEVUTO!");
    digitalWrite(LED_PIN, HIGH);  // accendi LED
    delay(2000);                  // mantieni acceso 2 secondi
    digitalWrite(LED_PIN, LOW);

    // Invia ACK software (pacchetto motion=false)
    msg_t ackMsg = {msg.id, false};
    esp_now_send(transmitterMAC, (uint8_t *)&ackMsg, sizeof(ackMsg));
    Serial.println("ACK inviato al trasmettitore");
  }
}

void setup() {
  Serial.begin(115200);

  // Pin LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Modalità Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.channel(1); // stesso canale del trasmettitore

  // Inizializza ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Errore inizializzazione ESP-NOW");
    return;
  }

  // Callback ricezione pacchetti
  esp_now_register_recv_cb(onReceive);
}

void loop() {
  // Nessuna logica bloccante, tutto gestito dai callback
}


