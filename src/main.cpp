#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <Credentials.h>

#define LED_PIN 2 // LED sul ricevitore
#define CHANNEL 6

volatile bool alarmPending = false;
unsigned long alarmTime = 0;

typedef struct
{
  uint8_t id;
  bool motion;
} msg_t;

// MAC del trasmettitore (solo se vuoi rispondere a uno specifico)
uint8_t transmitterMAC[6];

WiFiClientSecure client;
UniversalTelegramBot bot(botToken, client);

bool telegramSent = false;

uint8_t ch;
wifi_second_chan_t sec;

// Callback per la ricezione pacchetti
void onReceive(const uint8_t *mac_addr, const uint8_t *data, int len)
{
  if (len != sizeof(msg_t))
    return; // controllo lunghezza

  msg_t msg;
  memcpy(&msg, data, sizeof(msg_t));

  // Salvo MAC trasmettitore per ACK
  memcpy(transmitterMAC, mac_addr, 6);

  Serial.printf(
      "Transmitter MAC salvato: %02X:%02X:%02X:%02X:%02X:%02X\n",
      transmitterMAC[0], transmitterMAC[1], transmitterMAC[2],
      transmitterMAC[3], transmitterMAC[4], transmitterMAC[5]);

  if (msg.motion)
  {
    Serial.println("ALLARME PIR RICEVUTO!");

    // Invia ACK software (pacchetto motion=false)
    msg_t ackMsg = {msg.id, false};

    delay(10); // breve pausa prima di inviare ACK

    /*
    if (!esp_now_is_peer_exist(transmitterMAC))
    {
      esp_now_peer_info_t peer = {};
      memcpy(peer.peer_addr, transmitterMAC, 6);
      peer.channel = CHANNEL;
      peer.encrypt = false;
      esp_now_add_peer(&peer);
    }
      */

    esp_now_send(transmitterMAC, (uint8_t *)&ackMsg, sizeof(ackMsg));
    Serial.println("ACK inviato al trasmettitore");

    // Segnala allarme al loop()
    alarmPending = true;
    telegramSent = false;
    alarmTime = millis();
  }
}

void setup()
{
  Serial.begin(115200);

  // Pin LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Modalità Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  Serial.println("WiFi connesso");
  esp_wifi_get_channel(&ch, &sec);
  Serial.printf("Canale WiFi 2.4GHz reale: %d\n", ch);  // stampa canale reale

  client.setInsecure(); // necessario per HTTPS Telegram

  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  esp_wifi_set_max_tx_power(78); // 20.5 dBm

  // Inizializza ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Errore inizializzazione ESP-NOW");
    return;
  }

  // Callback ricezione pacchetti
  esp_now_register_recv_cb(onReceive);
}

void loop()
{
  if (alarmPending)
  {
    digitalWrite(LED_PIN, HIGH);

    if (millis() - alarmTime >= 2000)
    {
      digitalWrite(LED_PIN, LOW);
      alarmPending = false;
    }
  }
  if (!telegramSent)
  {
    bot.sendMessage(chatID, "ALLARME PIR RICEVUTO!", "");
    telegramSent = true;
  }
}
