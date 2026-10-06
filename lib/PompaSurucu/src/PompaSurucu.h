// =============================================
// PompaSurucu - İki peristaltik step motor pompa sürücüsü
//
// STEP darbeleri Timer1 kesmesiyle (20 kHz) üretilir. Böylece:
//  - LCD/I2C, menü vb. işlemler motorları yavaşlatmaz, titretmez
//  - İki motor aynı zaman tabanından sürüldüğü için karışım oranı
//    uzun sürede bile kaymaz (DDA / faz akümülatörü yöntemi)
//  - Her motorun attığı adım sayılır (tüketim ml hesabı)
//
// Hız rampası (yumuşak kalkış/duruş) her iki motorda orantılı uygulanır,
// rampa sırasında da karışım oranı korunur.
// Not: Timer1 bu kütüphane tarafından kullanılır (Servo kütüphanesi ile çakışır).
// =============================================
#ifndef POMPA_SURUCU_H
#define POMPA_SURUCU_H

#include <Arduino.h>

class PompaSurucu {
public:
    static const uint8_t MOTOR_SAYISI = 2;
    static const uint16_t KESME_FREKANSI = 20000;           // Hz
    static const uint16_t MUTLAK_MAX_HIZ = KESME_FREKANSI / 2;

    void begin(uint8_t step1, uint8_t dir1, uint8_t en1,
               uint8_t step2, uint8_t dir2, uint8_t en2);

    // enAktifDusuk : sürücü ENABLE girişi LOW'da aktifse true (A4988, TB6600...)
    // ters1/ters2  : motor dönüş yönünü ters çevir
    // rampaMs      : 0'dan hedef hıza çıkış süresi (0 = rampasız)
    void ayarla(bool enAktifDusuk, bool ters1, bool ters2, uint16_t rampaMs);

    // Hedef hız (adım/sn). 0 = dur. Rampa ile bu hıza ulaşılır.
    void hizAyarla(uint8_t motor, float adimSn);
    // Motor 'adim' kadar adım attıktan sonra kendiliğinden durur
    void adimlaCalistir(uint8_t motor, float adimSn, uint32_t adim);

    void durdur();                 // rampa ile durdur
    void hemenDurdur();            // rampasız, anında durdur

    void guncelle();               // loop() içinde sürekli çağrılmalı (rampa, enable)

    bool calisiyor() const;
    bool calisiyor(uint8_t motor) const;
    float anlikHiz(uint8_t motor) const;
    float hedefHiz(uint8_t motor) const;
    uint32_t adimSayisi(uint8_t motor) const;   // açılıştan beri toplam adım

private:
    void _hizYaz(uint8_t motor, float adimSn);

    uint8_t _enPin[MOTOR_SAYISI];
    uint8_t _dirPin[MOTOR_SAYISI];
    bool _enAktifDusuk = true;
    bool _enAcik[MOTOR_SAYISI];
    uint16_t _rampaMs = 150;
    float _hedef[MOTOR_SAYISI];
    float _hiz[MOTOR_SAYISI];
    uint32_t _durmaZamani[MOTOR_SAYISI];
    uint32_t _sonRampa = 0;
};

#endif
