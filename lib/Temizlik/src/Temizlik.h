// =============================================
// Temizlik - Hortum ve karıştırıcı temizlik döngüsü
//
// Sıra:
//  1. Pompalar durdurulur, boya ve sertleştirici vanaları kapatılır
//  2. Temizlik sıvısı vanası açılır, vananın açılması beklenir
//  3. Seçilen pompa(lar) temizlik sıvısı basar
//     - durdur() çağrılana kadar (DURDUR tuşu), veya
//     - ayarlanan maksimum süre dolana kadar
//  4. Pompalar durur, temizlik vanası kapanır
//
// Darbeli mod: 3 sn basma / 1 sn bekleme şeklinde çalışır; hortumdaki
// kalıntının çalkalanarak sökülmesine yardım eder.
// =============================================
#ifndef TEMIZLIK_H
#define TEMIZLIK_H

#include <Arduino.h>
#include <Vanalar.h>
#include <PompaSurucu.h>

enum TemizlikDurum : uint8_t {
    TMZ_KAPALI = 0,     // hiç başlatılmadı
    TMZ_HAZIRLIK,       // vana açılıyor
    TMZ_POMPALIYOR,
    TMZ_BITTI           // kullanıcı durdurdu veya süre doldu
};

// Temizlikte çalışacak pompalar (bit maskesi)
#define TEMIZLIK_POMPA1 0x01
#define TEMIZLIK_POMPA2 0x02

class Temizlik {
public:
    void begin(Vanalar& vanalar, PompaSurucu& pompa,
               uint8_t boyaVana, uint8_t sertVana, uint8_t temizVana);

    // hizAdimSn   : temizlik pompa hızı
    // pompaMaske  : TEMIZLIK_POMPA1 | TEMIZLIK_POMPA2
    // maxSureSn   : en uzun temizlik süresi (0 = sınırsız)
    // vanaGecikme : vana açıldıktan sonra pompa başlamadan önce bekleme
    // darbeli     : 3 sn bas / 1 sn bekle
    void ayarla(uint16_t hizAdimSn, uint8_t pompaMaske, uint16_t maxSureSn,
                uint16_t vanaGecikmeMs, bool darbeli);

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

    Vanalar* _v = nullptr;
    PompaSurucu* _p = nullptr;
    uint8_t _boyaVana = 0, _sertVana = 1, _temizVana = 2;

    uint16_t _hiz = 1600;
    uint8_t _maske = TEMIZLIK_POMPA2;
    uint16_t _maxSn = 0;
    uint16_t _vanaGecikme = 100;
    bool _darbeli = false;

    uint8_t _durum = TMZ_KAPALI;
    bool _sureDoldu = false;
    bool _darbeAcik = true;
    uint32_t _baslangic = 0, _bitis = 0, _darbeZamani = 0;
    uint32_t _adimBaslangic[PompaSurucu::MOTOR_SAYISI];
    uint32_t _adimBitis = 0;
};

#endif
