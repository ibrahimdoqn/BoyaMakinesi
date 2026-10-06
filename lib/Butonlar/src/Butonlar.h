// =============================================
// Butonlar - 5 tuşlu menü klavyesi
//
// - Yazılımsal debounce (titreşim önleme)
// - Basılı tutunca otomatik tekrar (giderek hızlanır)
// - Uzun basma olayı
// - Olay kuyruğu: hiçbir tuş basışı kaybolmaz
// =============================================
#ifndef BUTONLAR_H
#define BUTONLAR_H

#include <Arduino.h>

enum Tus : uint8_t {
    TUS_YUKARI = 0,
    TUS_ASAGI,
    TUS_SOL,
    TUS_SAG,
    TUS_OK,
    TUS_SAYISI,
    TUS_YOK = 0xFF
};

enum TusOlay : uint8_t {
    OLAY_YOK = 0,
    OLAY_BASILDI,     // tuşa ilk basıldığında
    OLAY_TEKRAR,      // basılı tutulurken periyodik
    OLAY_UZUN,        // uzun basıldığında (bir kez)
    OLAY_BIRAKILDI    // tuş bırakıldığında
};

struct TusBilgi {
    uint8_t tus;
    uint8_t olay;
    uint8_t tekrar;   // kaçıncı tekrar (hızlandırma için)
};

class Butonlar {
public:
    // pinler: TUS_YUKARI, TUS_ASAGI, TUS_SOL, TUS_SAG, TUS_OK sırasıyla
    void begin(const uint8_t pinler[TUS_SAYISI], bool aktifYuksek = true);
    void ayarla(uint16_t tekrarGecikmeMs, uint16_t tekrarAralikMs, uint16_t uzunSureMs);

    void guncelle();               // loop() içinde sürekli çağrılmalı

    bool olayVar() const { return _bas != _son; }
    TusBilgi oku();                // kuyruktan bir olay alır
    void kuyrukTemizle() { _bas = _son; }

    bool basili(uint8_t tus) const;
    uint32_t sonEtkinlik() const { return _sonEtkinlik; }

private:
    void _ekle(uint8_t tus, uint8_t olay, uint8_t tekrar);

    static const uint8_t KUYRUK = 8;
    static const uint8_t DEBOUNCE_MS = 20;

    uint8_t _pin[TUS_SAYISI];
    bool _aktifYuksek = true;
    bool _ham[TUS_SAYISI];
    bool _durum[TUS_SAYISI];
    uint32_t _degisim[TUS_SAYISI];
    uint32_t _basma[TUS_SAYISI];
    uint32_t _sonTekrar[TUS_SAYISI];
    uint8_t _tekrarSay[TUS_SAYISI];
    bool _uzunGitti[TUS_SAYISI];

    uint16_t _tekrarGecikme = 450;
    uint16_t _tekrarAralik = 130;
    uint16_t _uzunSure = 1000;

    TusBilgi _kuyruk[KUYRUK];
    volatile uint8_t _bas = 0, _son = 0;
    uint32_t _sonEtkinlik = 0;
};

#endif
