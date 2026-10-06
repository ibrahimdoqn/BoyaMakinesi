// =============================================
// Temizlik - Hortum ve karıştırıcı temizlik döngüsü
//
// Vanalar manueldir: kullanıcı temizlik sıvısı vanasını elle açar.
// Sıra:
//  1. Boya pompaları durdurulur (tamamen durması beklenir)
//  2. Seçilen pompa(lar) temizlik sıvısı basar
//     - durdur() çağrılana kadar (DURDUR tuşu), veya
//     - ayarlanan maksimum süre dolana kadar
//  3. Pompalar durur
//
// Darbeli mod: 3 sn basma / 1 sn bekleme şeklinde çalışır; hortumdaki
// kalıntının çalkalanarak sökülmesine yardım eder.
// =============================================
#ifndef TEMIZLIK_H
#define TEMIZLIK_H

#include <Arduino.h>
#include <PompaSurucu.h>

enum TemizlikDurum : uint8_t {
    TMZ_KAPALI = 0,     // hiç başlatılmadı
    TMZ_HAZIRLIK,       // önceki pompalama duruyor
    TMZ_POMPALIYOR,
    TMZ_BITTI           // kullanıcı durdurdu veya süre doldu
};

// Temizlikte çalışacak pompalar (bit maskesi)
#define TEMIZLIK_POMPA1 0x01
#define TEMIZLIK_POMPA2 0x02

class Temizlik {
public:
    void begin(PompaSurucu& pompa);

    // hizAdimSn   : temizlik pompa hızı
    // pompaMaske  : TEMIZLIK_POMPA1 | TEMIZLIK_POMPA2
    // maxSureSn   : en uzun temizlik süresi (0 = sınırsız)
    // darbeli     : 3 sn bas / 1 sn bekle
    void ayarla(float hizAdimSn, uint8_t pompaMaske, uint16_t maxSureSn, bool darbeli);

    void baslat();
    void durdur();
    void guncelle();               // loop() içinde sürekli çağrılmalı

    uint8_t durum() const { return _durum; }
    bool calisiyor() const { return _durum == TMZ_HAZIRLIK || _durum == TMZ_POMPALIYOR; }
    bool sureDoldu() const { return _sureDoldu; }
    uint32_t gecenSureMs() const;  // son temizliğin süresi
    uint32_t pompalananAdim() const;

private:
    void _pompalariAyarla(bool calis);

    PompaSurucu* _p = nullptr;

    float _hiz = 5000;
    uint8_t _maske = TEMIZLIK_POMPA2;
    uint16_t _maxSn = 0;
    bool _darbeli = false;

    uint8_t _durum = TMZ_KAPALI;
    bool _sureDoldu = false;
    bool _darbeAcik = true;
    uint32_t _baslangic = 0, _bitis = 0, _darbeZamani = 0;
    uint32_t _adimBaslangic[PompaSurucu::MOTOR_SAYISI];
    uint32_t _adimBitis = 0;
};

#endif
