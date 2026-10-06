#include "Tetik.h"

// Referans takip zaman sabitleri (saniye)
static const float REF_YUKSELME_TAU = 0.5f;   // basınç yükselirken hızlı takip
static const float REF_DUSME_TAU = 4.0f;      // yavaş düşüşleri (kaçak, ayar) takip

void Tetik::ayarla(uint8_t mod, float farkPsi, float mutlakPsi, float histerezisPsi,
                   uint16_t cekmeGecikmeMs, uint16_t birakmaGecikmeMs, float minBasincPsi) {
    _mod = mod;
    _fark = farkPsi > 0.1f ? farkPsi : 0.1f;
    _mutlak = mutlakPsi;
    _hist = histerezisPsi >= 0 ? histerezisPsi : 0;
    _cekmeGecikme = cekmeGecikmeMs;
    _birakmaGecikme = birakmaGecikmeMs;
    _min = minBasincPsi;
}

void Tetik::sifirla(float psi) {
    _ref = psi;
    _durum = BOSTA;
    _basladi = true;
    _yeniCek = _yeniBirak = false;
    _sonGuncelleme = millis();
}

bool Tetik::yeniCekildi() {
    bool r = _yeniCek;
    _yeniCek = false;
    return r;
}

bool Tetik::yeniBirakildi() {
    bool r = _yeniBirak;
    _yeniBirak = false;
    return r;
}

uint32_t Tetik::cekiliSure(uint32_t simdiMs) const {
    return cekili() ? simdiMs - _cekmeZamani : 0;
}

void Tetik::_esikleriHesapla(float psi) {
    if (_mod == TETIK_FARK) {
        _cekmeEsik = _ref - _fark;
        _birakmaEsik = _cekmeEsik + _hist;
        // Bırakma eşiği referansın altında kalmalı, yoksa tetik hiç bırakılmaz
        float ust = _ref - _fark * 0.2f;
        if (_birakmaEsik > ust) _birakmaEsik = ust;
        _yetersiz = (_ref < _min) || (psi < _min);
    } else {
        _cekmeEsik = _mutlak;
        _birakmaEsik = _mutlak + _hist;
        _yetersiz = psi < _min;
    }
}

void Tetik::guncelle(float psi, uint32_t simdi) {
    if (!_basladi) sifirla(psi);

    float dt = (simdi - _sonGuncelleme) / 1000.0f;
    _sonGuncelleme = simdi;
    if (dt > 0.5f) dt = 0.5f;

    // Referans takibi: sadece tetik boştayken ve basınç tetik yönünde
    // hızla düşmüyorken. Düşüş başladığında referans dondurulur.
    if (_durum == BOSTA) {
        float a = 0.0f;
        if (psi >= _ref) a = dt / REF_YUKSELME_TAU;
        else if (psi > _ref - _fark * 0.5f) a = dt / REF_DUSME_TAU;
        if (a > 1.0f) a = 1.0f;
        _ref += (psi - _ref) * a;
    }

    _esikleriHesapla(psi);

    switch (_durum) {
        case BOSTA:
            if (!_yetersiz && psi <= _cekmeEsik) {
                _durum = CEKME_BEKLE;
                _durumZamani = simdi;
            }
            break;

        case CEKME_BEKLE:
            if (_yetersiz || psi > _cekmeEsik) {
                _durum = BOSTA;
            } else if (simdi - _durumZamani >= _cekmeGecikme) {
                _durum = CEKILI;
                _cekmeZamani = simdi;
                _yeniCek = true;
            }
            break;

        case CEKILI:
            if (_yetersiz) {
                _durum = BOSTA;          // hava kesildi: hemen bırak
                _yeniBirak = true;
            } else if (psi >= _birakmaEsik) {
                _durum = BIRAKMA_BEKLE;
                _durumZamani = simdi;
            }
            break;

        case BIRAKMA_BEKLE:
            if (_yetersiz || simdi - _durumZamani >= _birakmaGecikme) {
                _durum = BOSTA;
                _yeniBirak = true;
            } else if (psi < _birakmaEsik) {
                _durum = CEKILI;
            }
            break;
    }
}
