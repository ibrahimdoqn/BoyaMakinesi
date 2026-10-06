#include "PompaSurucu.h"
#include <util/atomic.h>

// ISR tarafından kullanılan kanal verisi
struct MotorKanal {
    volatile uint8_t* port;    // STEP pininin PORT register'ı
    uint8_t maske;             // STEP pininin bit maskesi
    volatile uint32_t faz;     // faz akümülatörü
    volatile uint32_t artis;   // her kesmede faza eklenen değer (hız)
    volatile uint32_t sayac;   // toplam adım
    volatile uint32_t limit;   // bu adıma ulaşınca dur
    volatile bool limitAktif;
    volatile bool limitBitti;
    volatile bool yuksek;      // STEP pini şu an HIGH mı
};

static MotorKanal kanal[PompaSurucu::MOTOR_SAYISI];

// 2^32 / KESME_FREKANSI : adım/sn -> faz artışı katsayısı
static const float FAZ_KATSAYI = 4294967296.0f / PompaSurucu::KESME_FREKANSI;

static inline void kanalIsle(MotorKanal& k) {
    // Önceki kesmede HIGH yapılan STEP pinini LOW yap (darbe genişliği 50 us)
    if (k.yuksek) {
        *k.port &= ~k.maske;
        k.yuksek = false;
    }
    uint32_t artis = k.artis;
    if (artis == 0) return;
    uint32_t eski = k.faz;
    uint32_t yeni = eski + artis;
    k.faz = yeni;
    // Taşma = bir adım zamanı geldi. artis < 2^31 olduğundan art arda
    // iki kesmede taşma olmaz, pin her zaman önce LOW'a döner.
    if (yeni < eski) {
        *k.port |= k.maske;
        k.yuksek = true;
        uint32_t s = k.sayac + 1;
        k.sayac = s;
        if (k.limitAktif && s >= k.limit) {
            k.artis = 0;
            k.limitAktif = false;
            k.limitBitti = true;
        }
    }
}

ISR(TIMER1_COMPA_vect) {
    kanalIsle(kanal[0]);
    kanalIsle(kanal[1]);
}

void PompaSurucu::begin(uint8_t step1, uint8_t dir1, uint8_t en1,
                        uint8_t step2, uint8_t dir2, uint8_t en2) {
    const uint8_t stepPin[MOTOR_SAYISI] = {step1, step2};
    _dirPin[0] = dir1;
    _dirPin[1] = dir2;
    _enPin[0] = en1;
    _enPin[1] = en2;

    for (uint8_t m = 0; m < MOTOR_SAYISI; m++) {
        pinMode(stepPin[m], OUTPUT);
        digitalWrite(stepPin[m], LOW);
        pinMode(_dirPin[m], OUTPUT);
        pinMode(_enPin[m], OUTPUT);

        MotorKanal& k = kanal[m];
        k.port = portOutputRegister(digitalPinToPort(stepPin[m]));
        k.maske = digitalPinToBitMask(stepPin[m]);
        k.faz = k.artis = k.sayac = k.limit = 0;
        k.limitAktif = k.limitBitti = k.yuksek = false;

        _hedef[m] = _hiz[m] = _rampaRef[m] = 0;
        _enAcik[m] = false;         // sürücü başlangıçta pasif
        _durmaZamani[m] = 0;
    }
    ayarla(_enAktifDusuk, false, false, _rampaMs);

    // Timer1: CTC modu, prescaler 8 -> 2 MHz, OCR1A = 2MHz / 20kHz - 1
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        TCCR1A = 0;
        TCCR1B = 0;
        TCNT1 = 0;
        OCR1A = (F_CPU / 8 / KESME_FREKANSI) - 1;
        TCCR1B = _BV(WGM12) | _BV(CS11);
        TIMSK1 |= _BV(OCIE1A);
    }
    _sonRampa = millis();
}

void PompaSurucu::ayarla(bool enAktifDusuk, bool ters1, bool ters2, uint16_t rampaMs) {
    _enAktifDusuk = enAktifDusuk;
    _rampaMs = rampaMs;
    digitalWrite(_dirPin[0], ters1 ? HIGH : LOW);
    digitalWrite(_dirPin[1], ters2 ? HIGH : LOW);
    for (uint8_t m = 0; m < MOTOR_SAYISI; m++) {
        bool seviye = _enAcik[m] ? !enAktifDusuk : enAktifDusuk;
        digitalWrite(_enPin[m], seviye ? HIGH : LOW);
    }
}

void PompaSurucu::_hizYaz(uint8_t m, float adimSn) {
    if (adimSn < 0) adimSn = 0;
    if (adimSn > MUTLAK_MAX_HIZ) adimSn = MUTLAK_MAX_HIZ;
    uint32_t artis = (uint32_t)(adimSn * FAZ_KATSAYI);
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        // Adım limiti dolduysa ISR motoru durdurmuştur, tekrar başlatma
        if (!kanal[m].limitBitti) kanal[m].artis = artis;
    }
}

void PompaSurucu::hizAyarla(uint8_t m, float adimSn) {
    if (m >= MOTOR_SAYISI) return;
    if (adimSn < 0) adimSn = 0;
    if (adimSn > MUTLAK_MAX_HIZ) adimSn = MUTLAK_MAX_HIZ;
    if (adimSn != _hedef[m]) {
        // Rampa eğimi hedef değiştiğinde sabitlenir: hız doğrusal değişir ve
        // hedefe rampa süresi içinde kesin olarak ulaşır.
        _rampaRef[m] = adimSn > _hiz[m] ? adimSn : _hiz[m];
    }
    _hedef[m] = adimSn;
    if (_rampaMs == 0) {
        _hiz[m] = adimSn;
        _hizYaz(m, adimSn);
    }
}

void PompaSurucu::adimlaCalistir(uint8_t m, float adimSn, uint32_t adim) {
    if (m >= MOTOR_SAYISI || adim == 0) return;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        kanal[m].limit = kanal[m].sayac + adim;
        kanal[m].limitAktif = true;
        kanal[m].limitBitti = false;
    }
    hizAyarla(m, adimSn);
}

void PompaSurucu::durdur() {
    for (uint8_t m = 0; m < MOTOR_SAYISI; m++) hizAyarla(m, 0);
}

void PompaSurucu::hemenDurdur() {
    for (uint8_t m = 0; m < MOTOR_SAYISI; m++) {
        _hedef[m] = _hiz[m] = 0;
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            kanal[m].artis = 0;
            kanal[m].limitAktif = false;
        }
    }
}

void PompaSurucu::guncelle() {
    uint32_t simdi = millis();
    uint32_t dt = simdi - _sonRampa;

    for (uint8_t m = 0; m < MOTOR_SAYISI; m++) {
        // ISR adım limitine ulaşıp motoru durdurduysa durumu eşitle
        bool bitti;
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            bitti = kanal[m].limitBitti;
            kanal[m].limitBitti = false;
        }
        if (bitti) _hedef[m] = _hiz[m] = 0;

        // Rampa: her iki motor kendi hedefinin aynı oranıyla ilerler,
        // böylece hızlanırken/yavaşlarken karışım oranı korunur.
        if (dt >= 2 && _hiz[m] != _hedef[m]) {
            if (_rampaMs == 0) {
                _hiz[m] = _hedef[m];
            } else {
                float ref = _rampaRef[m] > 1.0f ? _rampaRef[m] : 1.0f;
                float adim = ref * dt / _rampaMs;
                if (_hiz[m] < _hedef[m]) {
                    _hiz[m] += adim;
                    if (_hiz[m] > _hedef[m]) _hiz[m] = _hedef[m];
                } else {
                    _hiz[m] -= adim;
                    if (_hiz[m] < _hedef[m]) _hiz[m] = _hedef[m];
                }
            }
            _hizYaz(m, _hiz[m]);
        }

        // ENABLE yönetimi: çalışırken aç, durduktan 500 ms sonra kapat
        bool gerekli = _hiz[m] > 0 || _hedef[m] > 0;
        if (gerekli) {
            _durmaZamani[m] = simdi;
            if (!_enAcik[m]) {
                _enAcik[m] = true;
                digitalWrite(_enPin[m], _enAktifDusuk ? LOW : HIGH);
            }
        } else if (_enAcik[m] && simdi - _durmaZamani[m] > 500) {
            _enAcik[m] = false;
            digitalWrite(_enPin[m], _enAktifDusuk ? HIGH : LOW);
        }
    }
    if (dt >= 2) _sonRampa = simdi;
}

bool PompaSurucu::calisiyor() const {
    return calisiyor(0) || calisiyor(1);
}

bool PompaSurucu::calisiyor(uint8_t m) const {
    return m < MOTOR_SAYISI && (_hiz[m] > 0 || _hedef[m] > 0);
}

float PompaSurucu::anlikHiz(uint8_t m) const {
    return m < MOTOR_SAYISI ? _hiz[m] : 0;
}

float PompaSurucu::hedefHiz(uint8_t m) const {
    return m < MOTOR_SAYISI ? _hedef[m] : 0;
}

uint32_t PompaSurucu::adimSayisi(uint8_t m) const {
    if (m >= MOTOR_SAYISI) return 0;
    uint32_t s;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { s = kanal[m].sayac; }
    return s;
}
