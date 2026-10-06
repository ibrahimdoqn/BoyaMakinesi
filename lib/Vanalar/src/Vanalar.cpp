#include "Vanalar.h"

void Vanalar::begin(const uint8_t pinler[SAYI], bool aktifDusuk) {
    _aktifDusuk = aktifDusuk;
    for (uint8_t i = 0; i < SAYI; i++) {
        _pin[i] = pinler[i];
        _acik[i] = false;
        _yaz(i);                 // önce pasif seviye
        pinMode(_pin[i], OUTPUT);
    }
}

void Vanalar::aktifDusukAyarla(bool aktifDusuk) {
    _aktifDusuk = aktifDusuk;
    for (uint8_t i = 0; i < SAYI; i++) _yaz(i);
}

void Vanalar::_yaz(uint8_t i) {
    bool yuksek = _acik[i] ? !_aktifDusuk : _aktifDusuk;
    digitalWrite(_pin[i], yuksek ? HIGH : LOW);
}

void Vanalar::ayarla(uint8_t vana, bool acik) {
    if (vana >= SAYI || _acik[vana] == acik) return;
    _acik[vana] = acik;
    _yaz(vana);
}

void Vanalar::hepsiniKapat() {
    for (uint8_t i = 0; i < SAYI; i++) ayarla(i, false);
}

bool Vanalar::acik(uint8_t vana) const {
    return vana < SAYI && _acik[vana];
}
