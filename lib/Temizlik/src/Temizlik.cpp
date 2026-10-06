#include "Temizlik.h"

static const uint16_t DARBE_ACIK_MS = 3000;
static const uint16_t DARBE_KAPALI_MS = 1000;

void Temizlik::begin(Vanalar& vanalar, PompaSurucu& pompa,
                     uint8_t boyaVana, uint8_t sertVana, uint8_t temizVana) {
    _v = &vanalar;
    _p = &pompa;
    _boyaVana = boyaVana;
    _sertVana = sertVana;
    _temizVana = temizVana;
    _durum = TMZ_KAPALI;
}

void Temizlik::ayarla(uint16_t hizAdimSn, uint8_t pompaMaske, uint16_t maxSureSn,
                      uint16_t vanaGecikmeMs, bool darbeli) {
    _hiz = hizAdimSn;
    _maske = pompaMaske ? pompaMaske : TEMIZLIK_POMPA2;
    _maxSn = maxSureSn;
    _vanaGecikme = vanaGecikmeMs;
    _darbeli = darbeli;
    if (_durum == TMZ_POMPALIYOR && _darbeAcik) _pompalariAyarla(true);  // hız değişti
}

void Temizlik::_pompalariAyarla(bool calis) {
    for (uint8_t m = 0; m < PompaSurucu::MOTOR_SAYISI; m++) {
        bool secili = _maske & (1 << m);
        _p->hizAyarla(m, (calis && secili) ? _hiz : 0);
    }
}

void Temizlik::baslat() {
    if (!_v || !_p) return;
    _p->durdur();
    _v->kapat(_boyaVana);
    _v->kapat(_sertVana);
    _v->ac(_temizVana);
    _durum = TMZ_HAZIRLIK;
    _sureDoldu = false;
    _baslangic = millis();
    for (uint8_t m = 0; m < PompaSurucu::MOTOR_SAYISI; m++)
        _adimBaslangic[m] = _p->adimSayisi(m);
}

void Temizlik::durdur() {
    if (!calisiyor()) return;
    _p->durdur();
    _v->kapat(_temizVana);
    _durum = TMZ_BITTI;
    _bitis = millis();
    _adimBitis = pompalananAdim();
}

void Temizlik::guncelle() {
    if (!calisiyor()) return;
    uint32_t simdi = millis();

    if (_durum == TMZ_HAZIRLIK) {
        // Boya pompaları tamamen durup vana açıldıktan sonra başla
        if (simdi - _baslangic >= _vanaGecikme && !_p->calisiyor()) {
            _durum = TMZ_POMPALIYOR;
            _darbeAcik = true;
            _darbeZamani = simdi;
            _pompalariAyarla(true);
        }
        return;
    }

    if (_maxSn && simdi - _baslangic >= (uint32_t)_maxSn * 1000UL) {
        _sureDoldu = true;
        durdur();
        return;
    }

    if (_darbeli) {
        uint16_t aralik = _darbeAcik ? DARBE_ACIK_MS : DARBE_KAPALI_MS;
        if (simdi - _darbeZamani >= aralik) {
            _darbeAcik = !_darbeAcik;
            _darbeZamani = simdi;
            _pompalariAyarla(_darbeAcik);
        }
    }
}

uint32_t Temizlik::gecenSureMs() const {
    if (_durum == TMZ_KAPALI) return 0;
    return (calisiyor() ? millis() : _bitis) - _baslangic;
}

uint32_t Temizlik::pompalananAdim() const {
    if (_durum == TMZ_KAPALI) return 0;
    if (_durum == TMZ_BITTI) return _adimBitis;
    uint32_t toplam = 0;
    for (uint8_t m = 0; m < PompaSurucu::MOTOR_SAYISI; m++)
        if (_maske & (1 << m)) toplam += _p->adimSayisi(m) - _adimBaslangic[m];
    return toplam;
}
