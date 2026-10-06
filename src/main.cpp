// =============================================
// 2K Akrilik Boya Karışım ve Püskürtme Sistemi
// Arduino Mega 2560 + PlatformIO
//
// Ana Orkestratör - Tüm alt sistemleri koordine eder
//
// Modüller (lib/):
//   BasincSensoru : basınç okuma, filtre, kalibrasyon, hata algılama
//   Tetik         : basınç düşüşünden tetik algılama
//   PompaSurucu   : iki step motor, kesme tabanlı, oran korumalı
//   Vanalar       : röle / selenoid vana kontrolü
//   Temizlik      : temizlik döngüsü
//   Butonlar      : 5 tuş, debounce, basılı tutma
//   Gosterge      : 16x2 LCD tamponu, Türkçe karakter
//   Menu          : modüler menü sistemi
// Uygulama (src/):
//   Ayarlar       : ayarlar + EEPROM
//   Sistem        : mod durum makinesi (bekleme/boya/temizlik/servis)
//   Ekranlar      : ana ekran ve servis ekranları
//   MenuTanim     : menü ağacı
// =============================================

#include <Arduino.h>
#include <Wire.h>

#include "PinConfig.h"
#include "Sistem.h"
#include "Ekranlar.h"

// =============================================
// SETUP
// =============================================
void setup() {
    Serial.begin(115200);

    gosterge.begin();         // LCD (Wire.begin dahil)
    Wire.setClock(400000);    // LCD güncellemesi hızlı olsun

    Sistem::begin();          // ayarları EEPROM'dan yükler, donanımı başlatır

    gosterge.satir(0, F("2K Boya Sistemi"));
    gosterge.satir(1, F(YAZILIM_SURUMU));
    gosterge.guncelle();

    // Sensör filtresi otursun diye kısa bekleme (pompalar bu sırada kapalı)
    uint32_t bas = millis();
    while (millis() - bas < 1200) {
        sensor.guncelle();
        pompa.guncelle();
    }
    tetik.sifirla(sensor.psi());

    Ekranlar::begin();
}

// =============================================
// LOOP - hiçbir yerde bekleme (delay) yok
// =============================================
void loop() {
    butonlar.guncelle();
    sensor.guncelle();
    Sistem::guncelle();
    Ekranlar::guncelle();
}
