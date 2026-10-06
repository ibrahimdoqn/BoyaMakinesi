#include "BasincSensoru.h"

void BasincSensoru::begin(uint8_t pin) {
    _pin = pin;
    pinMode(_pin, INPUT);
    _ilk = true;
    _sonOrnek = millis();
    _hataDegisim = _sonOrnek;
}

void BasincSensoru::ayarla(uint16_t vMinMv, uint16_t vMaxMv, float maxPsi,
                           float ofsetPsi, uint8_t filtreN, uint16_t adcRefMv) {
    _vMin = vMinMv;
    _vMax = (vMaxMv > vMinMv) ? vMaxMv : vMinMv + 1;
    _maxPsi = maxPsi;
    _ofset = ofsetPsi;
    _ref = adcRefMv ? adcRefMv : 5000;
    if (filtreN < 1) filtreN = 1;
    _alfa = 2.0f / (filtreN + 1.0f);   // EMA: N örneklik ortalamaya eşdeğer
}

uint16_t BasincSensoru::milivolt() const {
    return (uint32_t)_adc * _ref / 1023UL;
}

float BasincSensoru::_adcPsi(uint16_t adc) const {
    float mv = (float)adc * _ref / 1023.0f;
    return (mv - _vMin) * _maxPsi / (float)(_vMax - _vMin);
}

int16_t BasincSensoru::psiOnda() const {
    float p = _psi * 10.0f;
    return (int16_t)(p >= 0 ? p + 0.5f : p - 0.5f);
}

void BasincSensoru::guncelle() {
    uint32_t simdi = millis();
    if (!_ilk && simdi - _sonOrnek < ORNEK_ARALIK_MS) return;
    _sonOrnek = simdi;

    _adc = analogRead(_pin);
    float p = _adcPsi(_adc) + _ofset;
    // Makul sınırlar içinde tut
    if (p < -_maxPsi * 0.1f) p = -_maxPsi * 0.1f;
    if (p > _maxPsi * 1.2f) p = _maxPsi * 1.2f;
    _ham = p;

    if (_ilk) {
        _psi = p;
        _ilk = false;
    } else {
        _psi += (p - _psi) * _alfa;
    }

    // Hata algılama: 0.5V tabanlı sensörde çıkış 0V'a yakınsa kablo kopuk,
    // besleme gerilimine yakınsa kısa devre kabul edilir.
    uint16_t mv = milivolt();
    bool hataVar = (_vMin >= 200 && mv < _vMin / 2) || mv > (uint16_t)(_ref - 30);
    if (hataVar != _hataAday) {
        _hataAday = hataVar;
        _hataDegisim = simdi;
    }
    if (_hataAday != _hata && simdi - _hataDegisim >= HATA_SURE_MS) {
        _hata = _hataAday;
    }
}
