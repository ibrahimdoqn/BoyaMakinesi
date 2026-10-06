// =============================================
// Vanalar - Röle ile sürülen selenoid vanalar
//
// Açılışta röleler kapalı konumda başlatılır (pin önce pasif seviyeye
// çekilir, sonra çıkış yapılır; böylece açılışta vana "tık" yapmaz).
// =============================================
#ifndef VANALAR_H
#define VANALAR_H

#include <Arduino.h>

class Vanalar {
public:
    static const uint8_t SAYI = 4;

    // aktifDusuk: röle modülü LOW sinyalde çekiyorsa true (çoğu modül böyledir)
    void begin(const uint8_t pinler[SAYI], bool aktifDusuk = true);
    void aktifDusukAyarla(bool aktifDusuk);

    void ayarla(uint8_t vana, bool acik);
    void ac(uint8_t vana) { ayarla(vana, true); }
    void kapat(uint8_t vana) { ayarla(vana, false); }
    void hepsiniKapat();

    bool acik(uint8_t vana) const;

private:
    void _yaz(uint8_t vana);

    uint8_t _pin[SAYI];
    bool _acik[SAYI];
    bool _aktifDusuk = true;
};

#endif
