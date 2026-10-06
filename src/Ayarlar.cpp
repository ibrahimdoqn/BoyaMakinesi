#include "Ayarlar.h"
#include <EEPROM.h>
#include <stddef.h>
#include "PinConfig.h"

Ayarlar ayar;
Sayaclar sayac;

static uint8_t crc8(const uint8_t* veri, uint16_t uzunluk) {
    uint8_t crc = 0;
    while (uzunluk--) {
        uint8_t b = *veri++;
        for (uint8_t i = 0; i < 8; i++) {
            uint8_t karis = (crc ^ b) & 0x01;
            crc >>= 1;
            if (karis) crc ^= 0x8C;
            b >>= 1;
        }
    }
    return crc;
}

template <typename T>
static uint8_t yapiCrc(const T& yapi) {
    return crc8((const uint8_t*)&yapi, sizeof(T) - 1);   // son bayt crc
}

void ayarVarsayilan() {
    memset(&ayar, 0, sizeof(ayar));
    ayar.imza = AYAR_IMZA;
    ayar.surum = AYAR_SURUM;

    ayar.oranBoya = 4;
    ayar.oranSert = 1;
    ayar.motorHizi = 60;          // dev/dk (17# hortumla ~168 ml/dk)
    ayar.adimTur = MOTOR_ADIM_TUR;
    ayar.rampaMs = 150;
    // 17# hortum ~2.8 ml/tur -> 3200 / 2.8 = 1142.9 adım/ml (kalibre edilmeli)
    ayar.kal1_x10 = (uint32_t)MOTOR_ADIM_TUR * 100UL / HORTUM_ML_TUR_X10;
    ayar.kal2_x10 = ayar.kal1_x10;
    ayar.kalibAdim = MOTOR_ADIM_TUR * 10;   // 10 tur (~28 ml)
    ayar.yon1Ters = 0;
    ayar.yon2Ters = 0;
    ayar.enAktifYuksek = 0;

    ayar.tetikModu = 0;           // Akıllı
    ayar.tetikFark_x10 = 50;      // 5.0 psi (50 -> 45 psi)
    ayar.tetikMutlak_x10 = 450;   // 45.0 psi
    ayar.histerezis_x10 = 20;     // 2.0 psi
    ayar.cekmeGecikme = 60;
    ayar.birakmaGecikme = 100;
    ayar.minBasinc_x10 = 100;     // 10.0 psi
    ayar.maxPuskurtmeSn = 120;

    ayar.temizHiz = 90;           // dev/dk
    ayar.temizPompa = 1;          // Pompa 2
    ayar.temizMaxSn = 300;
    ayar.temizDarbeli = 0;
    ayar.potOmruDk = 30;

    ayar.sensVmin = SENSOR_VMIN_MV;
    ayar.sensVmax = SENSOR_VMAX_MV;
    ayar.sensMaxPsi = (uint16_t)BASINC_MAX_PSI;
    ayar.adcRef = ADC_REF_MV;
    ayar.sifirOfset_x10 = 0;
    ayar.filtre = 8;

    ayar.acilisModu = ACILIS_BEKLEME;
    ayar.lcdIsik = 1;

    ayar.tetikPencereMs = 300;
}

// Sürüm 2 ayarlarını koruyarak sürüm 3'e geçir.
// v2 yapısı, v3'ün tetikPencereMs alanına kadar olan kısmı + crc'dir.
static bool surum2denGecir() {
    const uint16_t v2Boyut = offsetof(Ayarlar, tetikPencereMs);
    uint8_t* ham = (uint8_t*)&ayar;
    for (uint16_t i = 0; i < v2Boyut; i++) ham[i] = EEPROM.read(AYAR_ADRES + i);
    uint8_t crc = EEPROM.read(AYAR_ADRES + v2Boyut);
    if (ayar.imza != AYAR_IMZA || ayar.surum != 2 || crc != crc8(ham, v2Boyut)) return false;
    // Yeni alanlar varsayılan değerle başlar. v2'nin "Fark" modu (0)
    // aynı numarayla yeni "Akıllı" moda geçer, fark değeri korunur.
    ayar.tetikPencereMs = 300;
    ayarKaydet();
    return true;
}

bool ayarYukle() {
    EEPROM.get(AYAR_ADRES, ayar);
    if (ayar.imza != AYAR_IMZA || ayar.surum != AYAR_SURUM || ayar.crc != yapiCrc(ayar)) {
        if (surum2denGecir()) return true;
        ayarVarsayilan();
        ayarKaydet();
        return false;
    }
    return true;
}

void ayarKaydet() {
    ayar.imza = AYAR_IMZA;
    ayar.surum = AYAR_SURUM;
    ayar.crc = yapiCrc(ayar);
    EEPROM.put(AYAR_ADRES, ayar);    // sadece değişen baytları yazar
}

void sayacSifirla() {
    memset(&sayac, 0, sizeof(sayac));
    sayac.imza = SAYAC_IMZA;
}

bool sayacYukle() {
    EEPROM.get(SAYAC_ADRES, sayac);
    if (sayac.imza != SAYAC_IMZA || sayac.crc != yapiCrc(sayac)) {
        sayacSifirla();
        sayacKaydet();
        return false;
    }
    return true;
}

void sayacKaydet() {
    sayac.imza = SAYAC_IMZA;
    sayac.crc = yapiCrc(sayac);
    EEPROM.put(SAYAC_ADRES, sayac);
}

static_assert(sizeof(Ayarlar) <= SAYAC_ADRES, "Ayarlar yapisi sayac alanina tasiyor");
static_assert(SAYAC_ADRES + sizeof(Sayaclar) <= 4096, "EEPROM boyutu asildi");
