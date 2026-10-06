#include "Ayarlar.h"
#include <EEPROM.h>
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
    ayar.motorHizi = 1600;
    ayar.rampaMs = 150;
    ayar.kal1_x10 = 10000;        // 1000.0 adım/ml (kalibre edilmeli)
    ayar.kal2_x10 = 10000;
    ayar.kalibAdim = 5000;
    ayar.yon1Ters = 0;
    ayar.yon2Ters = 0;
    ayar.enAktifYuksek = 0;

    ayar.tetikModu = 0;           // Fark
    ayar.tetikFark_x10 = 50;      // 5.0 psi (50 -> 45 psi)
    ayar.tetikMutlak_x10 = 450;   // 45.0 psi
    ayar.histerezis_x10 = 20;     // 2.0 psi
    ayar.cekmeGecikme = 60;
    ayar.birakmaGecikme = 100;
    ayar.minBasinc_x10 = 100;     // 10.0 psi
    ayar.maxPuskurtmeSn = 120;

    ayar.vanaModu = VANA_MOD_BOYUNCA;
    ayar.vanaGecikme = 100;
    ayar.roleAktifYuksek = 0;
    ayar.role4Gorev = ROLE4_YOK;

    ayar.temizHiz = 2000;
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
}

bool ayarYukle() {
    EEPROM.get(AYAR_ADRES, ayar);
    if (ayar.imza != AYAR_IMZA || ayar.surum != AYAR_SURUM || ayar.crc != yapiCrc(ayar)) {
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
