#include "Butonlar.h"

void Butonlar::begin(const uint8_t pinler[TUS_SAYISI], bool aktifYuksek) {
    _aktifYuksek = aktifYuksek;
    uint32_t simdi = millis();
    for (uint8_t i = 0; i < TUS_SAYISI; i++) {
        _pin[i] = pinler[i];
        // Aktif HIGH: harici pull-down var. Aktif LOW: dahili pull-up kullan.
        pinMode(_pin[i], aktifYuksek ? INPUT : INPUT_PULLUP);
        _ham[i] = _durum[i] = false;
        _degisim[i] = _basma[i] = _sonTekrar[i] = simdi;
        _tekrarSay[i] = 0;
        _uzunGitti[i] = false;
    }
    _bas = _son = 0;
}

void Butonlar::ayarla(uint16_t tekrarGecikmeMs, uint16_t tekrarAralikMs, uint16_t uzunSureMs) {
    _tekrarGecikme = tekrarGecikmeMs;
    _tekrarAralik = tekrarAralikMs;
    _uzunSure = uzunSureMs;
}

void Butonlar::_ekle(uint8_t tus, uint8_t olay, uint8_t tekrar) {
    uint8_t sonraki = (_son + 1) % KUYRUK;
    if (sonraki == _bas) return;  // kuyruk dolu, olayı at
    _kuyruk[_son].tus = tus;
    _kuyruk[_son].olay = olay;
    _kuyruk[_son].tekrar = tekrar;
    _son = sonraki;
}

TusBilgi Butonlar::oku() {
    TusBilgi t = {TUS_YOK, OLAY_YOK, 0};
    if (_bas != _son) {
        t = _kuyruk[_bas];
        _bas = (_bas + 1) % KUYRUK;
    }
    return t;
}

bool Butonlar::basili(uint8_t tus) const {
    return tus < TUS_SAYISI && _durum[tus];
}

void Butonlar::guncelle() {
    uint32_t simdi = millis();
    for (uint8_t i = 0; i < TUS_SAYISI; i++) {
        bool okunan = digitalRead(_pin[i]) == (_aktifYuksek ? HIGH : LOW);
        if (okunan != _ham[i]) {
            _ham[i] = okunan;
            _degisim[i] = simdi;
        }

        // Debounce: ham değer yeterince uzun süre sabit kaldıysa kabul et
        if (_ham[i] != _durum[i] && simdi - _degisim[i] >= DEBOUNCE_MS) {
            _durum[i] = _ham[i];
            _sonEtkinlik = simdi;
            if (_durum[i]) {
                _basma[i] = _sonTekrar[i] = simdi;
                _tekrarSay[i] = 0;
                _uzunGitti[i] = false;
                _ekle(i, OLAY_BASILDI, 0);
            } else {
                _ekle(i, OLAY_BIRAKILDI, 0);
            }
            continue;
        }

        if (!_durum[i]) continue;

        uint32_t basiliSure = simdi - _basma[i];
        _sonEtkinlik = simdi;

        if (!_uzunGitti[i] && basiliSure >= _uzunSure) {
            _uzunGitti[i] = true;
            _ekle(i, OLAY_UZUN, 0);
        }

        if (basiliSure >= _tekrarGecikme) {
            // Uzun basıldıkça tekrar aralığı kısalır
            uint16_t aralik = _tekrarAralik;
            if (_tekrarSay[i] > 30) aralik /= 3;
            else if (_tekrarSay[i] > 10) aralik /= 2;
            if (simdi - _sonTekrar[i] >= aralik) {
                _sonTekrar[i] = simdi;
                if (_tekrarSay[i] < 255) _tekrarSay[i]++;
                _ekle(i, OLAY_TEKRAR, _tekrarSay[i]);
            }
        }
    }
}
