#include "Sistem.h"
#include "PinConfig.h"

// --- Donanım nesneleri ---
Gosterge gosterge(LCD_ADRES);
Butonlar butonlar;
BasincSensoru sensor;
Tetik tetik;
PompaSurucu pompa;
Vanalar vanalar;
Temizlik temizlik;

namespace {
    SistemModu _mod = MOD_BEKLEME;

    bool _puskurtme = false;
    bool _maxAsildi = false;
    uint32_t _puskurtmeBas = 0;
    uint32_t _pompaBaslat = 0;

    // Pot ömrü (karışımın hortumda bekleme süresi) takibi
    bool _karisimVar = false;
    uint32_t _sonPuskurtme = 0;

    // Tüketim sayaçları
    uint32_t _sonAdim[PompaSurucu::MOTOR_SAYISI];
    float _oturumMl[PompaSurucu::MOTOR_SAYISI];
    bool _sayacKirli = false;
    uint32_t _sayacKayitZamani = 0;
    uint32_t _boyaDakikaZamani = 0;

    // Ertelenmiş ayar kaydı
    bool _kaydetBekliyor = false;
    uint32_t _kaydetZamani = 0;

    // Açılışta boya moduna otomatik geçiş (sensör otursun diye gecikmeli)
    bool _acilisBoyaBekliyor = false;
    uint32_t _acilisZamani = 0;

    const uint32_t KAYIT_GECIKME_MS = 2000;
    const uint32_t SAYAC_KAYIT_ARALIK_MS = 10UL * 60UL * 1000UL;
    const uint32_t TEMIZLIK_GECERLI_MS = 5000;   // bu süreden uzun temizlik pot ömrünü sıfırlar

    void boyaVanalariniAyarla() {
        if (_mod != MOD_BOYA) return;
        bool ac = ayar.vanaModu == VANA_MOD_BOYUNCA || _puskurtme;
        vanalar.ayarla(VANA_BOYA, ac && ayar.oranBoya > 0);
        vanalar.ayarla(VANA_SERT, ac && ayar.oranSert > 0);
    }

    void role4Guncelle() {
        if (_mod == MOD_SERVIS) return;   // röle testi serbestçe sürebilsin
        bool ac = false;
        switch (ayar.role4Gorev) {
            case ROLE4_BOYA_MODU: ac = _mod == MOD_BOYA; break;
            case ROLE4_PUSKURTME: ac = _puskurtme; break;
            case ROLE4_TEMIZLIK:  ac = temizlik.calisiyor(); break;
            default: break;       // görevsiz: kapalı
        }
        vanalar.ayarla(VANA_YEDEK, ac);
    }

    void boyaGuncelle(uint32_t simdi) {
        bool istek = tetik.cekili() && !sensor.hata();
        if (!tetik.cekili()) _maxAsildi = false;     // tetik bırakılınca kilit kalkar
        if (_puskurtme && ayar.maxPuskurtmeSn &&
            simdi - _puskurtmeBas >= (uint32_t)ayar.maxPuskurtmeSn * 1000UL)
            _maxAsildi = true;
        if (_maxAsildi) istek = false;

        if (istek && !_puskurtme) {
            _puskurtme = true;
            _puskurtmeBas = simdi;
            sayac.puskurtmeSayisi++;
            _sayacKirli = true;
            boyaVanalariniAyarla();
            _pompaBaslat = simdi + (ayar.vanaModu == VANA_MOD_PUSKURTMEDE ? ayar.vanaGecikme : 0);
        }

        if (!_puskurtme) return;

        if (!istek) {
            _puskurtme = false;
            pompa.durdur();
            _sonPuskurtme = simdi;
            boyaVanalariniAyarla();
            return;
        }

        if ((int32_t)(simdi - _pompaBaslat) >= 0) {
            // Her döngüde yeniden hesapla: menüden oran/hız değişirse anında uygulanır
            float h1, h2;
            Sistem::oranHizlari(h1, h2);
            pompa.hizAyarla(0, h1);
            pompa.hizAyarla(1, h2);
        }
        if (ayar.oranSert > 0) _karisimVar = true;
        _sonPuskurtme = simdi;
    }

    void sayaclariGuncelle(uint32_t simdi) {
        for (uint8_t m = 0; m < PompaSurucu::MOTOR_SAYISI; m++) {
            uint32_t a = pompa.adimSayisi(m);
            uint32_t fark = a - _sonAdim[m];
            if (fark == 0) continue;
            _sonAdim[m] = a;
            float ml = fark / Sistem::adimMl(m);
            if (_mod == MOD_TEMIZLIK) {
                sayac.temizMl += ml;
            } else {
                if (m == 0) sayac.boyaMl += ml;
                else sayac.sertMl += ml;
                _oturumMl[m] += ml;
            }
            _sayacKirli = true;
        }

        if (_mod == MOD_BOYA && simdi - _boyaDakikaZamani >= 60000UL) {
            _boyaDakikaZamani += 60000UL;
            sayac.calismaDk++;
            _sayacKirli = true;
        }

        if (_sayacKirli && !_puskurtme && simdi - _sayacKayitZamani >= SAYAC_KAYIT_ARALIK_MS) {
            sayacKaydet();
            _sayacKirli = false;
            _sayacKayitZamani = simdi;
        }
    }

    void sayaclariKaydet() {
        if (!_sayacKirli) return;
        sayacKaydet();
        _sayacKirli = false;
        _sayacKayitZamani = millis();
    }
}

namespace Sistem {

void begin() {
    ayarYukle();
    sayacYukle();

    const uint8_t roleler[Vanalar::SAYI] = {ROLE1_PIN, ROLE2_PIN, ROLE3_PIN, ROLE4_PIN};
    vanalar.begin(roleler, !ayar.roleAktifYuksek);

    pompa.begin(MOTOR1_STEP_PIN, MOTOR1_DIR_PIN, MOTOR1_EN_PIN,
                MOTOR2_STEP_PIN, MOTOR2_DIR_PIN, MOTOR2_EN_PIN);

    const uint8_t tuslar[TUS_SAYISI] = {BUTON_YUKARI_PIN, BUTON_ASAGI_PIN,
                                        BUTON_SOL_PIN, BUTON_SAG_PIN, BUTON_ORTA_PIN};
    butonlar.begin(tuslar, true);

    sensor.begin(BASINC_SENSOR_PIN);
    temizlik.begin(vanalar, pompa, VANA_BOYA, VANA_SERT, VANA_TEMIZ);

    ayarlariUygula();

    for (uint8_t m = 0; m < PompaSurucu::MOTOR_SAYISI; m++) {
        _sonAdim[m] = pompa.adimSayisi(m);
        _oturumMl[m] = 0;
    }
    _mod = MOD_BEKLEME;
    _acilisBoyaBekliyor = ayar.acilisModu == ACILIS_BOYA;
    _acilisZamani = millis();
}

void ayarlariUygula() {
    sensor.ayarla(ayar.sensVmin, ayar.sensVmax, ayar.sensMaxPsi,
                  ayar.sifirOfset_x10 / 10.0f, ayar.filtre, ayar.adcRef);
    tetik.ayarla(ayar.tetikModu, ayar.tetikFark_x10 / 10.0f, ayar.tetikMutlak_x10 / 10.0f,
                 ayar.histerezis_x10 / 10.0f, ayar.cekmeGecikme, ayar.birakmaGecikme,
                 ayar.minBasinc_x10 / 10.0f);
    pompa.ayarla(!ayar.enAktifYuksek, ayar.yon1Ters, ayar.yon2Ters, ayar.rampaMs);
    vanalar.aktifDusukAyarla(!ayar.roleAktifYuksek);
    temizlik.ayarla(ayar.temizHiz, ayar.temizPompa + 1, ayar.temizMaxSn,
                    ayar.vanaGecikme, ayar.temizDarbeli);
    gosterge.isik(ayar.lcdIsik);
    boyaVanalariniAyarla();
}

void kaydetIste() {
    _kaydetBekliyor = true;
    _kaydetZamani = millis();
}

void guncelle() {
    uint32_t simdi = millis();

    pompa.guncelle();
    tetik.guncelle(sensor.psi(), simdi);   // referans her modda takip edilir

    if (_acilisBoyaBekliyor && simdi - _acilisZamani >= 1500) {
        _acilisBoyaBekliyor = false;
        if (_mod == MOD_BEKLEME) boyaModuBaslat();
    }

    switch (_mod) {
        case MOD_BOYA:
            boyaGuncelle(simdi);
            break;
        case MOD_TEMIZLIK:
            temizlik.guncelle();
            if (temizlik.gecenSureMs() >= TEMIZLIK_GECERLI_MS) _karisimVar = false;
            break;
        default:
            break;
    }

    role4Guncelle();
    sayaclariGuncelle(simdi);

    if (_kaydetBekliyor && simdi - _kaydetZamani >= KAYIT_GECIKME_MS) {
        _kaydetBekliyor = false;
        ayarKaydet();
    }
}

SistemModu mod() { return _mod; }

void bekleme() {
    if (temizlik.calisiyor()) temizlik.durdur();
    pompa.durdur();
    if (_puskurtme) _sonPuskurtme = millis();
    _puskurtme = false;
    _maxAsildi = false;
    vanalar.hepsiniKapat();
    if (_mod == MOD_BOYA || _mod == MOD_TEMIZLIK) sayaclariKaydet();
    _mod = MOD_BEKLEME;
}

void boyaModuBaslat() {
    bekleme();
    tetik.sifirla(sensor.psi());
    _mod = MOD_BOYA;
    _boyaDakikaZamani = millis();
    boyaVanalariniAyarla();
}

void boyaModuDegistir() {
    if (_mod == MOD_BOYA) bekleme();
    else boyaModuBaslat();
}

void temizlikModu() {
    bekleme();
    _mod = MOD_TEMIZLIK;
}

void servisModu() {
    bekleme();
    _mod = MOD_SERVIS;
}

float adimMl(uint8_t motor) {
    uint32_t k = motor == 0 ? ayar.kal1_x10 : ayar.kal2_x10;
    if (k < 10) k = 10;
    return k / 10.0f;
}

void oranHizlari(float& h1, float& h2) {
    // Hacimsel oran -> adım oranı (her pompanın kendi kalibrasyonu ile)
    float s1 = ayar.oranBoya * adimMl(0);
    float s2 = ayar.oranSert * adimMl(1);
    float enBuyuk = s1 > s2 ? s1 : s2;
    if (enBuyuk <= 0) {
        h1 = h2 = 0;
        return;
    }
    // En hızlı pompa "Motor Hızı" ayarında döner, diğeri orana göre
    float k = ayar.motorHizi / enBuyuk;
    h1 = s1 * k;
    h2 = s2 * k;
}

float akisMlDk() {
    float h1, h2;
    oranHizlari(h1, h2);
    return (h1 / adimMl(0) + h2 / adimMl(1)) * 60.0f;
}

bool puskurtuyor() { return _puskurtme; }
uint32_t puskurtmeSuresiMs() { return _puskurtme ? millis() - _puskurtmeBas : 0; }
bool maxSureAsildi() { return _maxAsildi; }

int32_t potKalanSn() {
    if (!_karisimVar || ayar.potOmruDk == 0) return -1;
    uint32_t omur = (uint32_t)ayar.potOmruDk * 60UL;
    if (_puskurtme) return omur;
    uint32_t gecen = (millis() - _sonPuskurtme) / 1000UL;
    return gecen >= omur ? 0 : (int32_t)(omur - gecen);
}

bool potOmruDoldu() { return potKalanSn() == 0; }

void potUyarisiSustur() {
    // Süreyi yeniden başlatır; temizlik yapılmadıysa bir pot ömrü sonra tekrar uyarır
    _sonPuskurtme = millis();
}

float oturumBoyaMl() { return _oturumMl[0]; }
float oturumSertMl() { return _oturumMl[1]; }

void oturumSifirla() {
    _oturumMl[0] = _oturumMl[1] = 0;
}

}  // namespace Sistem
