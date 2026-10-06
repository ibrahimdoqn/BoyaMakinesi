// =============================================
// Ayarlar - Tüm kullanıcı ayarları ve EEPROM'da kalıcı saklama
//
// - Ayarlar EEPROM adres 0'dan, sayaçlar SAYAC_ADRES'ten itibaren saklanır
// - İmza + sürüm + CRC8 ile doğrulanır; bozuksa varsayılanlar yüklenir
// - EEPROM.put sadece değişen baytları yazar (EEPROM ömrü korunur)
// - Menüden yapılan değişiklikler 2 sn sonra otomatik kaydedilir
//
// Ondalıklı değerler tam sayı olarak saklanır: "_x10" = değer * 10
// =============================================
#ifndef AYARLAR_H
#define AYARLAR_H

#include <Arduino.h>

#define AYAR_IMZA   0xB0A2
#define AYAR_SURUM  1        // yapı değişirse artırın -> varsayılanlara döner
#define SAYAC_IMZA  0x5A7C
#define AYAR_ADRES  0
#define SAYAC_ADRES 256

// Vana modu
enum { VANA_MOD_BOYUNCA = 0, VANA_MOD_PUSKURTMEDE = 1 };
// Röle 4 görevi
enum { ROLE4_YOK = 0, ROLE4_BOYA_MODU, ROLE4_PUSKURTME, ROLE4_TEMIZLIK };
// Açılış modu
enum { ACILIS_BEKLEME = 0, ACILIS_BOYA = 1 };

struct Ayarlar {
    uint16_t imza;
    uint8_t  surum;

    // --- Karışım / Pompa ---
    uint8_t  oranBoya;          // karışım oranı boya kısmı
    uint8_t  oranSert;          // karışım oranı sertleştirici kısmı
    uint16_t motorHizi;         // en hızlı pompanın hızı (adım/sn)
    uint16_t rampaMs;           // yumuşak kalkış/duruş süresi
    uint32_t kal1_x10;          // pompa 1 kalibrasyonu (adım/ml * 10)
    uint32_t kal2_x10;          // pompa 2 kalibrasyonu (adım/ml * 10)
    uint16_t kalibAdim;         // kalibrasyonda basılacak adım sayısı
    uint8_t  yon1Ters;
    uint8_t  yon2Ters;
    uint8_t  enAktifYuksek;     // 0: EN aktif LOW (A4988/TB6600), 1: aktif HIGH

    // --- Tetik ---
    uint8_t  tetikModu;         // 0: Fark, 1: Mutlak
    uint16_t tetikFark_x10;     // psi * 10
    uint16_t tetikMutlak_x10;   // psi * 10
    uint16_t histerezis_x10;    // psi * 10
    uint16_t cekmeGecikme;      // ms
    uint16_t birakmaGecikme;    // ms
    uint16_t minBasinc_x10;     // psi * 10
    uint16_t maxPuskurtmeSn;    // 0 = kapalı

    // --- Vanalar ---
    uint8_t  vanaModu;
    uint16_t vanaGecikme;       // ms
    uint8_t  roleAktifYuksek;   // 0: röle LOW'da çeker, 1: HIGH'da çeker
    uint8_t  role4Gorev;

    // --- Temizlik ---
    uint16_t temizHiz;          // adım/sn
    uint8_t  temizPompa;        // 0: P1, 1: P2, 2: ikisi
    uint16_t temizMaxSn;        // 0 = sınırsız
    uint8_t  temizDarbeli;
    uint16_t potOmruDk;         // karışım ömrü uyarısı, 0 = kapalı

    // --- Sensör ---
    uint16_t sensVmin;          // mV
    uint16_t sensVmax;          // mV
    uint16_t sensMaxPsi;
    uint16_t adcRef;            // mV
    int16_t  sifirOfset_x10;    // psi * 10
    uint8_t  filtre;            // örnek sayısı

    // --- Sistem ---
    uint8_t  acilisModu;
    uint8_t  lcdIsik;

    uint8_t  crc;
};

struct Sayaclar {
    uint16_t imza;
    float    boyaMl;            // toplam basılan boya
    float    sertMl;            // toplam basılan sertleştirici
    float    temizMl;           // toplam temizlik sıvısı
    uint32_t puskurtmeSayisi;
    uint32_t calismaDk;         // boya modunda geçen süre
    uint8_t  crc;
};

extern Ayarlar ayar;
extern Sayaclar sayac;

void ayarVarsayilan();
bool ayarYukle();               // false: geçersizdi, varsayılan yüklendi
void ayarKaydet();
void sayacSifirla();
bool sayacYukle();
void sayacKaydet();

#endif
