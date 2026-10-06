#include "Ekranlar.h"
#include "Sistem.h"
#include "PinConfig.h"

static const uint32_t CIZIM_ARALIK_MS = 100;
static const uint32_t ZAMAN_ASIMI_MS = 90000UL;   // menüde 90 sn işlem yoksa ana ekran
static const uint8_t YIGIN_MAX = 5;

static MenuSistemi menu;

// ---------------------------------------------------------------
// Yardımcılar
// ---------------------------------------------------------------
static void psiYaz(char* t, int16_t psi_x10) {
    Gosterge::ondalik(t, psi_x10, 1);
}

static int16_t onda(float deger) {
    return (int16_t)(deger >= 0 ? deger * 10.0f + 0.5f : deger * 10.0f - 0.5f);
}

static void sureYaz(char* t, uint32_t sn) {
    uint16_t dk = sn / 60;
    uint8_t s = sn % 60;
    if (dk > 99) dk = 99;
    t[0] = '0' + dk / 10;
    t[1] = '0' + dk % 10;
    t[2] = ':';
    t[3] = '0' + s / 10;
    t[4] = '0' + s % 10;
    t[5] = 0;
}

static void mlYaz(char* t, float ml) {
    Gosterge::ondalik(t, (int32_t)(ml * 10.0f + 0.5f), 1);
}

static uint8_t pompaSecimMaske() {
    return ayar.temizPompa + 1;
}

// ---------------------------------------------------------------
// Ana ekran
//   YUKARI : Boya modu başlat / durdur
//   AŞAĞI  : Temizlik ekranı
//   SOL/SAĞ: bilgi sayfaları
//   OK     : Menü (pot ömrü uyarısı varsa önce uyarıyı susturur)
// ---------------------------------------------------------------
class AnaEkran : public Ekran {
public:
    void tus(const TusBilgi& t) override {
        if (t.olay != OLAY_BASILDI) return;
        switch (t.tus) {
            case TUS_OK:
                if (Sistem::potOmruDoldu()) Sistem::potUyarisiSustur();
                else Ekranlar::menuAc();
                break;
            case TUS_YUKARI: Sistem::boyaModuDegistir(); _sayfa = 0; break;
            case TUS_ASAGI:  Ekranlar::temizlikAc(); break;
            case TUS_SOL:    _sayfa = (_sayfa + SAYFA - 1) % SAYFA; break;
            case TUS_SAG:    _sayfa = (_sayfa + 1) % SAYFA; break;
        }
    }

    void ciz(Gosterge& g) override {
        char t[24];
        bool yanip = (millis() / 500) % 2;
        bool potDoldu = Sistem::potOmruDoldu();

        // Uyarı varsa arka ışık yanıp söner
        g.isik(potDoldu ? yanip : ayar.lcdIsik);

        // Satır 1: basınç ve karışım oranı
        g.temizle();
        if (sensor.hata()) {
            g.satir(0, F("SENSÖR HATASI!"));
        } else {
            psiYaz(t, sensor.psiOnda());
            strcat(t, " psi");
            g.yaz(0, 0, t);
        }
        char oran[8];
        itoa(ayar.oranBoya, oran, 10);
        strcat(oran, ":");
        itoa(ayar.oranSert, oran + strlen(oran), 10);
        if (!sensor.hata()) g.sagaYaz(0, oran);

        // Satır 2: seçili bilgi sayfası
        switch (_sayfa) {
            case 0: _durumSatiri(g, yanip, potDoldu); break;
            case 1: {
                char e[8];
                t[0] = 0;
                if (ayar.tetikModu == TETIK_FARK) {
                    strcat(t, "R:");
                    psiYaz(t + strlen(t), onda(tetik.referans()));
                    strcat(t, " ");
                }
                strcat(t, "E:");
                psiYaz(e, onda(tetik.cekmeEsigi()));
                strcat(t, e);
                if (tetik.cekili()) strcat(t, " *");
                g.satir(1, t);
                break;
            }
            case 2:
                g.satir(1, F("Akış:"));
                mlYaz(t, Sistem::akisMlDk());
                strcat(t, "ml/dk");
                g.sagaYaz(1, t);
                break;
            case 3: {
                char s[10];
                strcpy(t, "B:");
                mlYaz(s, Sistem::oturumBoyaMl());
                strcat(t, s);
                strcat(t, " S:");
                mlYaz(s, Sistem::oturumSertMl());
                strcat(t, s);
                g.satir(1, t);
                break;
            }
            case 4: {
                int32_t kalan = Sistem::potKalanSn();
                g.satir(1, F("Pot ömrü:"));
                if (kalan < 0) {
                    g.sagaYaz(1, F("--"));
                } else {
                    sureYaz(t, kalan);
                    g.sagaYaz(1, t);
                }
                break;
            }
        }
    }

private:
    void _durumSatiri(Gosterge& g, bool yanip, bool potDoldu) {
        char t[24];
        if (potDoldu && yanip) {
            g.satir(1, F("POT ÖMRÜ DOLDU!"));
            return;
        }
        if (potDoldu) {
            g.satir(1, F("Temizlik yapın"));
            return;
        }
        if (Sistem::mod() != MOD_BOYA) {
            g.satir(1, F("Bekleme  OK=Menü"));
            return;
        }
        if (sensor.hata()) {
            g.satir(1, F("Pompa durduruldu"));
        } else if (Sistem::maxSureAsildi()) {
            g.satir(1, yanip ? F("MAKS SÜRE!") : F("Tetiği bırakın"));
        } else if (Sistem::puskurtuyor()) {
            g.satir(1, F(OK_SAG "PÜSKÜRTME"));
            sureYaz(t, Sistem::puskurtmeSuresiMs() / 1000);
            g.sagaYaz(1, t);
        } else if (tetik.basincYetersiz()) {
            g.satir(1, F("Düşük basınç!"));
        } else {
            g.satir(1, F("BOYA HAZIR"));
            t[0] = 'E';
            psiYaz(t + 1, onda(tetik.cekmeEsigi()));
            g.sagaYaz(1, t);
        }
    }

    static const uint8_t SAYFA = 5;
    uint8_t _sayfa = 0;
};

// ---------------------------------------------------------------
// Menü ekranı (Menu kütüphanesini sarmalar)
// ---------------------------------------------------------------
class MenuEkrani : public Ekran {
public:
    void giris() override { menu.ac(); }
    void cikis() override { menu.kapat(); }
    void tus(const TusBilgi& t) override {
        menu.tus(t);
        if (!menu.acik()) Ekranlar::kapat();
    }
    void ciz(Gosterge& g) override {
        g.isik(ayar.lcdIsik);
        menu.ciz();
    }
};

// ---------------------------------------------------------------
// Temizlik ekranı
//   OK: başlat, çalışırken herhangi bir tuş: DURDUR, SOL: çıkış
// ---------------------------------------------------------------
class TemizlikEkrani : public Ekran {
public:
    void giris() override { Sistem::temizlikModu(); }
    void cikis() override { Sistem::bekleme(); }
    bool zamanAsimiVar() const override { return false; }

    void tus(const TusBilgi& t) override {
        if (t.olay != OLAY_BASILDI) return;
        if (temizlik.calisiyor()) {
            temizlik.durdur();             // herhangi bir tuş durdurur
            return;
        }
        if (t.tus == TUS_OK) temizlik.baslat();
        else if (t.tus == TUS_SOL) Ekranlar::kapat();
    }

    void ciz(Gosterge& g) override {
        char t[16];
        g.isik(ayar.lcdIsik);
        g.temizle();
        g.yaz(0, 0, F("TEMİZLİK"));
        sureYaz(t, temizlik.gecenSureMs() / 1000);
        g.sagaYaz(0, t);

        switch (temizlik.durum()) {
            case TMZ_KAPALI:
                // Vanalar manuel: başlamadan önce hatırlat
                if ((millis() / 1500) % 2) g.satir(1, F("Vanaları çevirin"));
                else g.satir(1, F("OK:Başla " OK_SOL "Çıkış"));
                break;
            case TMZ_HAZIRLIK:
                g.satir(1, F("Hazırlanıyor.."));
                break;
            case TMZ_POMPALIYOR: {
                uint8_t m = pompaSecimMaske() == 1 ? 0 : 1;
                mlYaz(t, temizlik.pompalananAdim() / Sistem::adimMl(m));
                strcat(t, "ml");
                g.yaz(0, 1, t);
                g.sagaYaz(1, F("Tuş:DUR"));
                break;
            }
            case TMZ_BITTI:
                g.satir(1, temizlik.sureDoldu() ? F("Süre doldu OK:Tk") : F("Bitti  OK:Tekrar"));
                break;
        }
    }
};

// ---------------------------------------------------------------
// Doldurma (hortum doldurma / manuel pompa) ekranı
//   OK basılı tutulduğu sürece pompalar,
//   SAĞ: sürekli çalıştır / durdur, SOL: çıkış
// ---------------------------------------------------------------
class DoldurEkrani : public Ekran {
public:
    uint8_t maske = 3;

    void giris() override {
        Sistem::servisModu();
        _surekli = false;
        _bas[0] = pompa.adimSayisi(0);
        _bas[1] = pompa.adimSayisi(1);
    }
    void cikis() override { Sistem::bekleme(); }
    bool zamanAsimiVar() const override { return false; }

    void tus(const TusBilgi& t) override {
        if (t.olay != OLAY_BASILDI) return;
        if (t.tus == TUS_SAG) _surekli = !_surekli;
        else if (t.tus == TUS_SOL) {
            if (_surekli) _surekli = false;
            else Ekranlar::kapat();
        } else if (t.tus != TUS_OK) {
            _surekli = false;
        }
    }

    void guncelle() override {
        bool calis = _surekli || butonlar.basili(TUS_OK);
        if (calis) {
            float h1 = 0, h2 = 0;
            if (maske == 3) Sistem::oranHizlari(h1, h2);
            else if (maske == 1) h1 = Sistem::rpmAdim(ayar.motorHizi);
            else h2 = Sistem::rpmAdim(ayar.motorHizi);
            pompa.hizAyarla(0, h1);
            pompa.hizAyarla(1, h2);
        } else {
            pompa.durdur();
        }
    }

    void ciz(Gosterge& g) override {
        char t[16];
        g.isik(ayar.lcdIsik);
        g.temizle();
        if (maske == 1) g.yaz(0, 0, F("Doldur: Boya"));
        else if (maske == 2) g.yaz(0, 0, F("Doldur: Sertleş."));
        else g.yaz(0, 0, F("Doldur: Karışım"));

        if (pompa.calisiyor()) {
            float ml = (pompa.adimSayisi(0) - _bas[0]) / Sistem::adimMl(0) +
                       (pompa.adimSayisi(1) - _bas[1]) / Sistem::adimMl(1);
            mlYaz(t, ml);
            strcat(t, " ml");
            g.yaz(0, 1, t);
            g.sagaYaz(1, _surekli ? F("SÜREKLİ") : F("ÇALIŞIYOR"));
        } else {
            g.satir(1, F("OK:Tut SAĞ:Sürek"));
        }
    }

private:
    bool _surekli = false;
    uint32_t _bas[2];
};

// ---------------------------------------------------------------
// Pompa kalibrasyon ekranı
//   1. OK ile "Kalib. Adım" kadar adım basılır (bir kaba alın)
//   2. Ölçülen ml girilir (YUKARI/AŞAĞI), OK ile kaydedilir
//   3. adım/ml = basılan adım / ölçülen ml
// ---------------------------------------------------------------
class KalibrasyonEkrani : public Ekran {
public:
    uint8_t motor = 0;

    void giris() override {
        Sistem::servisModu();
        _durum = HAZIR;
    }
    void cikis() override {
        pompa.hemenDurdur();
        Sistem::bekleme();
    }
    bool zamanAsimiVar() const override { return false; }

    void tus(const TusBilgi& t) override {
        bool bas = t.olay == OLAY_BASILDI;
        bool tekrar = bas || t.olay == OLAY_TEKRAR;
        switch (_durum) {
            case HAZIR:
                if (bas && t.tus == TUS_OK) {
                    _bas = pompa.adimSayisi(motor);
                    pompa.adimlaCalistir(motor, Sistem::rpmAdim(ayar.motorHizi), ayar.kalibAdim);
                    _durum = POMPA;
                } else if (bas && t.tus == TUS_SOL) {
                    Ekranlar::kapat();
                }
                break;
            case POMPA:
                if (bas && t.tus == TUS_SOL) {
                    pompa.hemenDurdur();
                    _durum = HAZIR;
                }
                break;
            case OLCUM:
                if (tekrar && (t.tus == TUS_YUKARI || t.tus == TUS_ASAGI)) {
                    int32_t d = menuCarpan(t.tekrar);
                    int32_t v = (int32_t)_ml_x10 + (t.tus == TUS_YUKARI ? d : -d);
                    if (v < 1) v = 1;
                    if (v > 99999) v = 99999;
                    _ml_x10 = v;
                } else if (bas && t.tus == TUS_OK) {
                    uint32_t k = (_adim * 100UL + _ml_x10 / 2) / _ml_x10;   // (adım/ml) * 10
                    if (k < 10) k = 10;
                    if (motor == 0) ayar.kal1_x10 = k;
                    else ayar.kal2_x10 = k;
                    Sistem::ayarlariUygula();
                    Sistem::kaydetIste();
                    _durum = SONUC;
                } else if (bas && t.tus == TUS_SOL) {
                    _durum = HAZIR;
                }
                break;
            case SONUC:
                if (bas) Ekranlar::kapat();
                break;
        }
    }

    void guncelle() override {
        if (_durum == POMPA && !pompa.calisiyor(motor)) {
            _adim = pompa.adimSayisi(motor) - _bas;
            if (_adim == 0) {
                _durum = HAZIR;
                return;
            }
            // Mevcut kalibrasyona göre beklenen miktar başlangıç değeri olsun
            uint32_t kal = motor == 0 ? ayar.kal1_x10 : ayar.kal2_x10;
            _ml_x10 = (_adim * 100UL) / (kal ? kal : 1);
            if (_ml_x10 < 1) _ml_x10 = 1;
            _durum = OLCUM;
        }
    }

    void ciz(Gosterge& g) override {
        char t[20];
        g.isik(ayar.lcdIsik);
        g.temizle();
        switch (_durum) {
            case HAZIR:
                g.yaz(0, 0, motor == 0 ? F("P1 Boya Kalib.") : F("P2 Sert. Kalib."));
                strcpy(t, "OK:");
                ultoa(ayar.kalibAdim, t + 3, 10);
                strcat(t, " adım");
                g.yaz(0, 1, t);
                break;
            case POMPA:
                g.yaz(0, 0, F("Pompalanıyor.."));
                ultoa(pompa.adimSayisi(motor) - _bas, t, 10);
                g.yaz(0, 1, t);
                g.sagaYaz(1, F(OK_SOL "İptal"));
                break;
            case OLCUM:
                g.yaz(0, 0, F("Ölçülen miktar:"));
                strcpy(t, "[");
                Gosterge::ondalik(t + 1, _ml_x10, 1);
                strcat(t, "] ml");
                g.yaz(0, 1, t);
                g.sagaYaz(1, F("OK"));
                break;
            case SONUC:
                g.yaz(0, 0, F("Kaydedildi:"));
                Gosterge::ondalik(t, motor == 0 ? ayar.kal1_x10 : ayar.kal2_x10, 1);
                strcat(t, " adım/ml");
                g.yaz(0, 1, t);
                break;
        }
    }

private:
    enum { HAZIR, POMPA, OLCUM, SONUC };
    uint8_t _durum = HAZIR;
    uint32_t _bas = 0, _adim = 0, _ml_x10 = 0;
};

// ---------------------------------------------------------------
// Sensör sıfır kalibrasyonu: hat havası boşaltılmışken okunan değeri 0 yapar
// ---------------------------------------------------------------
class SifirlamaEkrani : public Ekran {
public:
    void tus(const TusBilgi& t) override {
        if (t.olay != OLAY_BASILDI) return;
        if (t.tus == TUS_SOL) {
            Ekranlar::kapat();
        } else if (t.tus == TUS_OK) {
            float ofset = sensor.sifirOfsetiHesapla();
            int16_t o = onda(ofset);
            if (o < -200 || o > 200 || sensor.hata()) {
                Ekranlar::mesaj(F("Sıfırlanamadı!"), F("Hat havasını boş"));
                return;
            }
            ayar.sifirOfset_x10 = o;
            Sistem::ayarlariUygula();
            Sistem::kaydetIste();
            Ekranlar::kapat();
            Ekranlar::mesaj(F("Sensör sıfırlandı"), F("Kaydedildi"));
        }
    }
    void ciz(Gosterge& g) override {
        char t[20];
        g.temizle();
        g.yaz(0, 0, F("Basınç:"));
        psiYaz(t, sensor.psiOnda());
        strcat(t, "psi");
        g.sagaYaz(0, t);
        g.satir(1, F("OK:Sıfırla " OK_SOL "İptal"));
    }
};

// ---------------------------------------------------------------
// Onay ekranı
// ---------------------------------------------------------------
class OnayEkrani : public Ekran {
public:
    FStr soru = nullptr;
    void (*evet)() = nullptr;

    void tus(const TusBilgi& t) override {
        if (t.olay != OLAY_BASILDI) return;
        if (t.tus == TUS_OK) {
            Ekranlar::kapat();
            if (evet) evet();
        } else if (t.tus == TUS_SOL) {
            Ekranlar::kapat();
        }
    }
    void ciz(Gosterge& g) override {
        g.temizle();
        g.yaz(0, 0, soru);
        g.satir(1, F("OK:Evet  " OK_SOL "İptal"));
    }
};

// ---------------------------------------------------------------
// Ekran yığını
// ---------------------------------------------------------------
static AnaEkran anaEkran;
static MenuEkrani menuEkrani;
static TemizlikEkrani temizlikEkrani;
static DoldurEkrani doldurEkrani;
static KalibrasyonEkrani kalibrasyonEkrani;
static SifirlamaEkrani sifirlamaEkrani;
static OnayEkrani onayEkrani;

static Ekran* yigin[YIGIN_MAX];
static uint8_t derinlik = 0;
static uint32_t sonCizim = 0;

static FStr mesaj1 = nullptr, mesaj2 = nullptr;
static uint32_t mesajBitis = 0;

static void menuDegisti(MenuOge* oge) {
    (void)oge;
    Sistem::ayarlariUygula();
    Sistem::kaydetIste();
}

namespace Ekranlar {

void begin() {
    menu.begin(gosterge, menuAgaciniKur());
    menu.degisimBildir(menuDegisti);
    derinlik = 0;
    ac(&anaEkran);
}

void ac(Ekran* e) {
    if (!e || derinlik >= YIGIN_MAX) return;
    // Aynı ekran zaten yığındaysa ona kadar kapat (iki kez açılmasın)
    for (uint8_t i = 0; i < derinlik; i++) {
        if (yigin[i] == e) {
            while (derinlik > i + 1) kapat();
            return;
        }
    }
    yigin[derinlik++] = e;
    e->giris();
    sonCizim = 0;
}

void kapat() {
    if (derinlik <= 1) return;     // ana ekran kapanmaz
    Ekran* e = yigin[--derinlik];
    e->cikis();
    sonCizim = 0;
}

void anaEkranaDon() {
    while (derinlik > 1) kapat();
}

void mesaj(FStr s1, FStr s2, uint16_t sureMs) {
    mesaj1 = s1;
    mesaj2 = s2;
    mesajBitis = millis() + sureMs;
    sonCizim = 0;
}

void guncelle() {
    uint32_t simdi = millis();
    Ekran* aktif = yigin[derinlik - 1];

    while (butonlar.olayVar()) {
        TusBilgi t = butonlar.oku();
        if (mesaj1 && (int32_t)(mesajBitis - simdi) > 0) {
            if (t.olay == OLAY_BASILDI) mesajBitis = simdi;   // tuş mesajı kapatır
            continue;
        }
        aktif->tus(t);
        aktif = yigin[derinlik - 1];   // ekran değişmiş olabilir
        sonCizim = 0;                  // tuşa basınca hemen yeniden çiz
    }

    aktif->guncelle();

    if (aktif->zamanAsimiVar() && derinlik > 1 &&
        simdi - butonlar.sonEtkinlik() > ZAMAN_ASIMI_MS) {
        anaEkranaDon();
        aktif = yigin[0];
    }

    if (sonCizim != 0 && simdi - sonCizim < CIZIM_ARALIK_MS) return;
    sonCizim = simdi ? simdi : 1;

    if (mesaj1 && (int32_t)(mesajBitis - simdi) > 0) {
        gosterge.temizle();
        gosterge.satir(0, mesaj1);
        gosterge.satir(1, mesaj2);
    } else {
        mesaj1 = nullptr;
        aktif->ciz(gosterge);
    }
    gosterge.guncelle();
}

void menuAc() { ac(&menuEkrani); }
void temizlikAc() { ac(&temizlikEkrani); }
void sifirlamaAc() { ac(&sifirlamaEkrani); }

void doldurAc(uint8_t maske) {
    doldurEkrani.maske = maske;
    ac(&doldurEkrani);
}

void kalibrasyonAc(uint8_t motor) {
    kalibrasyonEkrani.motor = motor;
    ac(&kalibrasyonEkrani);
}

void onayAc(FStr soru, void (*evet)()) {
    onayEkrani.soru = soru;
    onayEkrani.evet = evet;
    ac(&onayEkrani);
}

}  // namespace Ekranlar
