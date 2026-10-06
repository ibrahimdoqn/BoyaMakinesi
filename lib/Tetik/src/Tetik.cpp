#include "Tetik.h"

// Tetik çekildikten sonra basıncın oturması için geçen süre. Bu sürede
// ilk sert düşüşün ardından gelen geri toparlanma (alt salınım) bırakma
// sayılmaz; sadece statik basınca dönüş bırakma sayılır.
static const uint16_t OTURMA_MS = 400;

// Akış basıncı takip zaman sabitleri (saniye)
static const float AKIS_OTURMA_TAU = 0.08f;   // oturma süresinde hızlı takip
static const float AKIS_DUSME_TAU = 0.3f;     // akış basıncı düşerken
static const float AKIS_YUKSELME_TAU = 0.4f;  // yükselme (kompresör dolumunu takip eder)

// Bırakma için gereken ani yükseliş = fark * bu oran
static const float BIRAKMA_YUKSELIS = 0.6f;

void Tetik::ayarla(uint8_t mod, float farkPsi, float mutlakPsi, float histerezisPsi,
                   uint16_t cekmeGecikmeMs, uint16_t birakmaGecikmeMs, float minBasincPsi,
                   uint16_t pencereMs) {
    _mod = mod;
    _fark = farkPsi > 0.2f ? farkPsi : 0.2f;
    _mutlak = mutlakPsi;
    _hist = histerezisPsi >= 0 ? histerezisPsi : 0;
    _cekmeGecikme = cekmeGecikmeMs;
    _birakmaGecikme = birakmaGecikmeMs;
    _min = minBasincPsi;
    uint16_t n = pencereMs / ORNEK_MS;
    if (n < 3) n = 3;
    if (n > GECMIS) n = GECMIS;
    _pencereOrnek = n;
}

void Tetik::sifirla(float psi) {
    int16_t v = (int16_t)(psi * 100.0f);
    for (uint8_t i = 0; i < GECMIS; i++) _gecmis[i] = v;
    _yaz = 0;
    _ref = _akis = psi;
    _dusus = 0;
    _durum = BOSTA;
    _basladi = true;
    _yeniCek = _yeniBirak = false;
    _sonGuncelleme = _sonOrnek = millis();
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

float Tetik::cekmeEsigi() const {
    return _mod == TETIK_MUTLAK ? _mutlak : _ref - _fark;
}

void Tetik::_ornekEkle(float psi) {
    float v = psi * 100.0f;
    if (v > 32000) v = 32000;
    if (v < -32000) v = -32000;
    _gecmis[_yaz] = (int16_t)v;
    _yaz = (_yaz + 1) % GECMIS;
}

// Son pencere içindeki en yüksek ve en düşük basınç
void Tetik::_pencere(float& tepe, float& dip) const {
    int16_t enB = -32768, enK = 32767;
    uint8_t i = _yaz;
    for (uint8_t n = 0; n < _pencereOrnek; n++) {
        i = (i == 0) ? GECMIS - 1 : i - 1;
        int16_t v = _gecmis[i];
        if (v > enB) enB = v;
        if (v < enK) enK = v;
    }
    tepe = enB / 100.0f;
    dip = enK / 100.0f;
}

void Tetik::_cek(uint32_t simdi, float psi) {
    _durum = CEKILI;
    _cekmeZamani = simdi;
    _akis = psi;
    _yeniCek = true;
}

void Tetik::_birak() {
    _durum = BOSTA;
    _yeniBirak = true;
}

void Tetik::guncelle(float psi, uint32_t simdi) {
    if (!_basladi) sifirla(psi);

    float dt = (simdi - _sonGuncelleme) / 1000.0f;
    _sonGuncelleme = simdi;
    if (dt > 0.5f) dt = 0.5f;

    // Geçmişe 10 ms'de bir örnek ekle (döngü yavaşlarsa aradaki örnekleri doldur)
    uint8_t eklenecek = 0;
    while (simdi - _sonOrnek >= ORNEK_MS && eklenecek < GECMIS) {
        _sonOrnek += ORNEK_MS;
        eklenecek++;
    }
    if (simdi - _sonOrnek >= ORNEK_MS) _sonOrnek = simdi;   // çok uzun duraklama
    while (eklenecek--) _ornekEkle(psi);

    _yetersiz = psi < _min;

    if (_mod == TETIK_MUTLAK) _guncelleMutlak(psi, simdi);
    else _guncelleAkilli(psi, simdi, dt);
}

void Tetik::_guncelleAkilli(float psi, uint32_t simdi, float dt) {
    float tepe, dip;
    _pencere(tepe, dip);
    _dusus = tepe - psi;
    if (_dusus < 0) _dusus = 0;

    switch (_durum) {
        case BOSTA:
            _ref = tepe;    // statik basınç = yakın geçmişteki tepe
            if (!_yetersiz && _dusus >= _fark) {
                _durum = CEKME_BEKLE;
                _durumZamani = simdi;
            }
            break;

        case CEKME_BEKLE:
            // _ref donduruldu (düşüş öncesi tepe)
            if (_yetersiz || psi > _ref - _fark * BIRAKMA_YUKSELIS) {
                _durum = BOSTA;                 // kısa iğne / hemen toparlandı
            } else if (simdi - _durumZamani >= _cekmeGecikme) {
                _cek(simdi, psi);
            }
            break;

        case CEKILI: {
            if (_yetersiz) {
                _birak();                       // hava kesildi: hemen bırak
                break;
            }
            bool oturuyor = simdi - _cekmeZamani < OTURMA_MS;
            float tau;
            if (oturuyor) tau = AKIS_OTURMA_TAU;
            else tau = psi < _akis ? AKIS_DUSME_TAU : AKIS_YUKSELME_TAU;
            float a = dt / tau;
            if (a > 1.0f) a = 1.0f;
            _akis += (psi - _akis) * a;

            bool statigeDondu = psi >= _ref - _hist;
            bool aniYukselis = !oturuyor && (psi - _akis) >= _fark * BIRAKMA_YUKSELIS;
            if (statigeDondu || aniYukselis) {
                _durum = BIRAKMA_BEKLE;
                _durumZamani = simdi;
            }
            break;
        }

        case BIRAKMA_BEKLE: {
            // Akış basıncı dondurulur; yükseliş buna göre ölçülür
            bool hala = psi >= _ref - _hist || (psi - _akis) >= _fark * BIRAKMA_YUKSELIS * 0.5f;
            if (_yetersiz || (hala && simdi - _durumZamani >= _birakmaGecikme)) {
                _birak();
            } else if (!hala) {
                _durum = CEKILI;
            }
            break;
        }
    }
}

void Tetik::_guncelleMutlak(float psi, uint32_t simdi) {
    float tepe, dip;
    _pencere(tepe, dip);
    _dusus = tepe > psi ? tepe - psi : 0;
    if (!cekili()) _ref = tepe;

    float birakmaEsik = _mutlak + _hist;
    switch (_durum) {
        case BOSTA:
            if (!_yetersiz && psi <= _mutlak) {
                _durum = CEKME_BEKLE;
                _durumZamani = simdi;
            }
            break;

        case CEKME_BEKLE:
            if (_yetersiz || psi > _mutlak) _durum = BOSTA;
            else if (simdi - _durumZamani >= _cekmeGecikme) _cek(simdi, psi);
            break;

        case CEKILI:
            _akis = psi;
            if (_yetersiz) _birak();
            else if (psi >= birakmaEsik) {
                _durum = BIRAKMA_BEKLE;
                _durumZamani = simdi;
            }
            break;

        case BIRAKMA_BEKLE:
            if (_yetersiz || simdi - _durumZamani >= _birakmaGecikme) _birak();
            else if (psi < birakmaEsik) _durum = CEKILI;
            break;
    }
}
