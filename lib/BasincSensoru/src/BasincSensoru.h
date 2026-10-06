// =============================================
// BasincSensoru - Analog basınç sensörü kütüphanesi
//
// Varsayılan: 0.5V - 4.5V çıkışlı, 0 - 200 PSI sensör.
// - Engellemeyen (non-blocking) örnekleme, her 2 ms'de bir okuma
// - Üstel hareketli ortalama (EMA) filtresi
// - Sıfır ofset kalibrasyonu
// - Kablo kopuk / kısa devre (sensör hatası) algılama
// =============================================
#ifndef BASINC_SENSORU_H
#define BASINC_SENSORU_H

#include <Arduino.h>

class BasincSensoru {
public:
    void begin(uint8_t pin);

    // vMinMv   : 0 basınçtaki sensör çıkışı (mV)
    // vMaxMv   : maksimum basınçtaki sensör çıkışı (mV)
    // maxPsi   : sensörün ölçüm aralığı (PSI)
    // ofsetPsi : sıfır kalibrasyon ofseti (okunan değere eklenir)
    // filtreN  : ortalama örnek sayısı (1 = filtre yok)
    // adcRefMv : Arduino'nun 5V pinindeki gerçek gerilim (mV)
    void ayarla(uint16_t vMinMv, uint16_t vMaxMv, float maxPsi,
                float ofsetPsi, uint8_t filtreN, uint16_t adcRefMv = 5000);

    void guncelle();                       // loop() içinde sürekli çağrılmalı

    float psi() const { return _psi; }     // filtrelenmiş basınç
    int16_t psiOnda() const;               // psi * 10 (yuvarlanmış)
    float hamPsi() const { return _ham; }  // filtresiz son okuma
    uint16_t adc() const { return _adc; }
    uint16_t milivolt() const;
    bool hata() const { return _hata; }    // sensör bağlantı hatası

    // Şu anki basıncı 0 yapacak yeni ofset değeri (PSI)
    float sifirOfsetiHesapla() const { return _ofset - _psi; }

private:
    float _adcPsi(uint16_t adc) const;     // ofsetsiz basınç

    static const uint8_t ORNEK_ARALIK_MS = 2;
    static const uint16_t HATA_SURE_MS = 250;

    uint8_t _pin = A0;
    uint16_t _vMin = 500, _vMax = 4500, _ref = 5000;
    float _maxPsi = 200.0f;
    float _ofset = 0.0f;
    float _alfa = 0.2f;

    uint16_t _adc = 0;
    float _ham = 0.0f;
    float _psi = 0.0f;
    bool _ilk = true;
    bool _hata = false;
    bool _hataAday = false;
    uint32_t _hataDegisim = 0;
    uint32_t _sonOrnek = 0;
};

#endif
