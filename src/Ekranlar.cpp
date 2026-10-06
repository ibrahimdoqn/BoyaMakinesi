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
//
// Satır 1: basınç + durum (BEKLE / HAZIR / PÜSK / DÜŞÜK ...)
// Satır 2: BEKLEME'de tuş ipucu, BOYA modunda hızlı ayar sayfaları
//
// BEKLEME : YUKARI = boya modunu başlat, AŞAĞI = temizlik, OK = menü
// BOYA    : SOL/SAĞ = sayfa seç, YUKARI/AŞAĞI = değeri değiştir (anında
//           uygulanır, otomatik kaydedilir), OK = menü,
//           OK basılı tut (1 sn) = boya modunu durdur
// Hızlı ayar sayfaları: Motor hızı, Karışım oranı, Tetik farkı/eşiği
// Bilgi sayfaları     : Tetik canlı, Oturum tüketimi, Pot ömrü
// ---------------------------------------------------------------

// Hızlı seçim için yaygın karışım oranları (boya oranı büyükten küçüğe)
static const uint8_t ORAN_LISTESI[][2] PROGMEM = {
    {1, 0}, {10, 1}, {5, 1}, {4, 1}, {3, 1}, {2, 1}, {3, 2}, {1, 1}, {1, 2}, {1, 4},
};
static const uint8_t ORAN_SAYISI = sizeof(ORAN_LISTESI) / sizeof(ORAN_LISTESI[0]);

// Boya oranı / toplam: oranları sıralamak için
static float boyaPayi(uint8_t a, uint8_t b) {
    return (a + b) ? (float)a / (a + b) : 1.0f;
}

class AnaEkran : public Ekran {
public:
    void tus(const TusBilgi& t) override {
        bool boya = Sistem::mod() == MOD_BOYA;

        // OK: kısa bas-bırak = menü, uzun bas = boya modunu durdur
        if (t.tus == TUS_OK) {
            if (t.olay == OLAY_BASILDI) {
                _okBasili = true;
                _okUzun = false;
            } else if (t.olay == OLAY_UZUN && _okBasili) {
                _okUzun = true;
                if (boya) {
                    Sistem::bekleme();
                    Ekranlar::mesaj(F("Boya modu"), F("durduruldu"));
                }
            } else if (t.olay == OLAY_BIRAKILDI && _okBasili) {
                _okBasili = false;
                if (_okUzun) return;
                if (Sistem::potOmruDoldu()) Sistem::potUyarisiSustur();
                else Ekranlar::menuAc();
            }
            return;
        }

        bool bas = t.olay == OLAY_BASILDI;
        bool tekrar = bas || t.olay == OLAY_TEKRAR;

        if (!boya) {
            if (!bas) return;
            if (t.tus == TUS_YUKARI) {
                Sistem::boyaModuBaslat();
                _sayfa = 0;
            } else if (t.tus == TUS_ASAGI) {
                Ekranlar::temizlikAc();
            }
            return;
        }

        switch (t.tus) {
            case TUS_SOL: if (bas) _sayfa = (_sayfa + SAYFA - 1) % SAYFA; break;
            case TUS_SAG: if (bas) _sayfa = (_sayfa + 1) % SAYFA; break;
            case TUS_YUKARI: if (tekrar) _degistir(+1, t.tekrar); break;
            case TUS_ASAGI:  if (tekrar) _degistir(-1, t.tekrar); break;
        }
    }

    void ciz(Gosterge& g) override {
        char t[24];
        bool yanip = (millis() / 500) % 2;
        bool potDoldu = Sistem::potOmruDoldu();
        bool boya = Sistem::mod() == MOD_BOYA;

        // Uyarı varsa arka ışık yanıp söner
        g.isik(potDoldu ? yanip : ayar.lcdIsik);
        g.temizle();

        // --- Satır 1: basınç + durum ---
        if (sensor.hata()) {
            g.satir(0, F("SENSÖR HATASI!"));
        } else {
            psiYaz(t, sensor.psiOnda());
            strcat(t, "psi");
            g.yaz(0, 0, t);
            if (potDoldu && yanip)               g.sagaYaz(0, F("POT!"));
            else if (!boya)                      g.sagaYaz(0, F("BEKLE"));
            else if (Sistem::maxSureAsildi())    g.sagaYaz(0, yanip ? F("MAKS!") : F("BIRAK"));
            else if (Sistem::puskurtuyor())      g.sagaYaz(0, F(OK_SAG "PÜSK"));
            else if (tetik.basincYetersiz())     g.sagaYaz(0, F("DÜŞÜK"));
            else                                 g.sagaYaz(0, F("HAZIR"));
        }

        // --- Satır 2 ---
        if (potDoldu && !yanip) {
            g.satir(1, F("Temizlik yapın!"));
            return;
        }
        if (!boya) {
            g.satir(1, F("^Boya  vTemizlik"));
            return;
        }

        switch (_sayfa) {
            case 0: {   // Motor hızı + hesaplanan akış
                g.yaz(0, 1, F("Hız"));
                itoa(ayar.motorHizi, t, 10);
                g.yaz(4, 1, t);
                itoa((int)(Sistem::akisMlDk() + 0.5f), t, 10);
                strcat(t, "ml/dk");
                g.sagaYaz(1, t);
                break;
            }
            case 1: {   // Karışım oranı
                g.yaz(0, 1, F("Oran"));
                itoa(ayar.oranBoya, t, 10);
                strcat(t, " : ");
                itoa(ayar.oranSert, t + strlen(t), 10);
                g.sagaYaz(1, t);
                break;
            }
            case 2: {   // Tetik hassasiyeti
                if (ayar.tetikModu == TETIK_AKILLI) {
                    g.yaz(0, 1, F("Tetik Fark"));
                    Gosterge::ondalik(t, ayar.tetikFark_x10, 1);
                } else {
                    g.yaz(0, 1, F("Tetik Eşik"));
                    Gosterge::ondalik(t, ayar.tetikMutlak_x10, 1);
                }
                g.sagaYaz(1, t);
                break;
            }
            case 3: {   // Tetik canlı (salt okunur)
                char e[8];
                t[0] = 0;
                if (tetik.cekili()) {
                    strcat(t, "Akış:");
                    psiYaz(e, onda(tetik.akisBasinci()));
                    strcat(t, e);
                } else if (ayar.tetikModu == TETIK_AKILLI) {
                    strcat(t, "R:");
                    psiYaz(e, onda(tetik.referans()));
                    strcat(t, e);
                    strcat(t, " D:");
                    psiYaz(e, onda(tetik.anlikDusus()));
                    strcat(t, e);
                } else {
                    strcat(t, "Eşik:");
                    psiYaz(e, onda(tetik.cekmeEsigi()));
                    strcat(t, e);
                }
                g.satir(1, t);
                break;
            }
            case 4: {   // Oturum tüketimi
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
            case 5: {   // Pot ömrü
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
    void _degistir(int8_t yon, uint8_t tekrar) {
        switch (_sayfa) {
            case 0: {
                int16_t v = ayar.motorHizi + yon * MOTOR_RPM_ADIM * (tekrar > 15 ? 2 : 1);
                if (v < MOTOR_MIN_RPM) v = MOTOR_MIN_RPM;
                if (v > MOTOR_MAX_RPM) v = MOTOR_MAX_RPM;
                ayar.motorHizi = v;
                break;
            }
            case 1: {
                if (t_tekrarKilit(tekrar)) return;
                float simdiki = boyaPayi(ayar.oranBoya, ayar.oranSert);
                // YUKARI: daha çok boya (listede yukarı), AŞAĞI: daha çok sertleştirici
                int8_t bulunan = -1;
                if (yon > 0) {
                    for (int8_t i = ORAN_SAYISI - 1; i >= 0; i--) {
                        uint8_t a = pgm_read_byte(&ORAN_LISTESI[i][0]);
                        uint8_t b = pgm_read_byte(&ORAN_LISTESI[i][1]);
                        if (boyaPayi(a, b) > simdiki + 0.0001f) { bulunan = i; break; }
                    }
                } else {
                    for (uint8_t i = 0; i < ORAN_SAYISI; i++) {
                        uint8_t a = pgm_read_byte(&ORAN_LISTESI[i][0]);
                        uint8_t b = pgm_read_byte(&ORAN_LISTESI[i][1]);
                        if (boyaPayi(a, b) < simdiki - 0.0001f) { bulunan = i; break; }
                    }
                }
                if (bulunan < 0) return;
                ayar.oranBoya = pgm_read_byte(&ORAN_LISTESI[bulunan][0]);
                ayar.oranSert = pgm_read_byte(&ORAN_LISTESI[bulunan][1]);
                break;
            }
            case 2: {
                uint16_t& p = ayar.tetikModu == TETIK_AKILLI ? ayar.tetikFark_x10
                                                             : ayar.tetikMutlak_x10;
                int16_t adim = tekrar > 15 ? 10 : 5;         // 0.5 / 1.0 psi
                int16_t v = (int16_t)p + yon * adim;
                int16_t enAz = ayar.tetikModu == TETIK_AKILLI ? 5 : 10;
                int16_t enCok = ayar.tetikModu == TETIK_AKILLI ? 500 : 1900;
                if (v < enAz) v = enAz;
                if (v > enCok) v = enCok;
                p = v;
                break;
            }
            default:
                return;   // bilgi sayfaları değiştirilemez
        }
        Sistem::ayarlariUygula();
        Sistem::kaydetIste();
    }

    // Oran listesinde basılı tutunca çok hızlı atlamasın: 3 tekrarda bir ilerle
    static bool t_tekrarKilit(uint8_t tekrar) {
        return tekrar > 0 && (tekrar % 3) != 0;
    }

    static const uint8_t SAYFA = 6;
    uint8_t _sayfa = 0;
    bool _okBasili = false;
    bool _okUzun = false;
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
// Tetik öğrenme sihirbazı
//   1. Tetik bırakılıyken 2 sn statik basınç ve dalgalanma ölçülür
//   2. Kullanıcı tetiği çeker: düşüş anı otomatik yakalanır, düşüşün
//      ne kadar sürede gerçekleştiği ve akış basıncı ölçülür
//   3. Önerilen "Düşüş Farkı" ve "Algı Penceresi" gösterilir, OK ile kaydedilir
// ---------------------------------------------------------------
class TetikOgrenEkrani : public Ekran {
public:
    void giris() override {
        Sistem::servisModu();      // öğrenirken pompalar çalışmasın
        _durum = STATIK_HAZIR;
    }
    void cikis() override { Sistem::bekleme(); }
    bool zamanAsimiVar() const override { return false; }

    void tus(const TusBilgi& t) override {
        if (t.olay != OLAY_BASILDI) return;
        if (t.tus == TUS_SOL) {
            Ekranlar::kapat();
            return;
        }
        if (t.tus != TUS_OK) return;
        switch (_durum) {
            case STATIK_HAZIR:
                _olcumBasla();
                _durum = STATIK_OLC;
                break;
            case SONUC:
                ayar.tetikModu = TETIK_AKILLI;
                ayar.tetikFark_x10 = _oneriFark;
                ayar.tetikPencereMs = _oneriPencere;
                ayar.histerezis_x10 = _oneriHist;
                Sistem::ayarlariUygula();
                Sistem::kaydetIste();
                Ekranlar::kapat();
                Ekranlar::mesaj(F("Tetik ayarları"), F("kaydedildi"));
                break;
            case HATA:
                _durum = STATIK_HAZIR;
                break;
            default:
                break;
        }
    }

    void guncelle() override {
        uint32_t simdi = millis();
        float p = sensor.psi();
        switch (_durum) {
            case STATIK_OLC:
                _olcumEkle(p);
                if (simdi - _bas >= 2000) {
                    _statik = _toplam / _sayi;
                    _dalga = _enB - _enK;          // gürültü + kompresör dalgası
                    _durum = CEKME_BEKLE;
                    _bas = simdi;
                }
                break;

            case CEKME_BEKLE: {
                // Statik aralığın belirgin altına inince düşüş başladı say
                float esik = _enK - (_dalga > 1.0f ? _dalga * 0.5f : 0.5f);
                if (p < esik) {
                    _durum = CEKILI_OLC;
                    _bas = simdi;
                    _izSayi = 0;
                    _sonIz = simdi;
                    _olcumBasla();
                    _bas = simdi;
                } else if (simdi - _bas > 15000) {
                    _hata = F("Tetik algılanmadı");
                    _durum = HATA;
                }
                break;
            }

            case CEKILI_OLC:
                // İlk 1 sn'nin izini 10 ms aralıkla tut (düşüş hızı için)
                if (_izSayi < IZ && simdi - _sonIz >= 10) {
                    _sonIz = simdi;
                    _iz[_izSayi++] = p;
                }
                if (simdi - _bas >= 1000) _olcumEkle(p);   // 1..3 sn: akış basıncı
                if (simdi - _bas >= 3000) _hesapla();
                break;

            default:
                break;
        }
    }

    void ciz(Gosterge& g) override {
        char t[20];
        g.isik(ayar.lcdIsik);
        g.temizle();
        switch (_durum) {
            case STATIK_HAZIR:
                g.yaz(0, 0, F("Tetiği BIRAKIN"));
                g.satir(1, F("OK:Ölç " OK_SOL "İptal"));
                break;
            case STATIK_OLC:
                g.yaz(0, 0, F("Ölçülüyor..."));
                psiYaz(t, sensor.psiOnda());
                strcat(t, "psi");
                g.sagaYaz(1, t);
                break;
            case CEKME_BEKLE:
                g.yaz(0, 0, F("Tetiği ÇEKİN"));
                g.satir(1, F("ve 3 sn tutun"));
                break;
            case CEKILI_OLC:
                g.yaz(0, 0, F("Tutun..."));
                psiYaz(t, sensor.psiOnda());
                strcat(t, "psi");
                g.sagaYaz(1, t);
                break;
            case SONUC:
                g.yaz(0, 0, F("Düşüş"));
                psiYaz(t, onda(_dusus));
                strcat(t, "psi");
                g.sagaYaz(0, t);
                strcpy(t, "F");
                Gosterge::ondalik(t + 1, _oneriFark, 1);
                strcat(t, " ");
                itoa(_oneriPencere, t + strlen(t), 10);
                strcat(t, "ms");
                g.yaz(0, 1, t);
                g.sagaYaz(1, F("OK"));
                break;
            case HATA:
                g.satir(0, _hata);
                g.satir(1, F("OK:Tekrar " OK_SOL "Çık"));
                break;
        }
    }

private:
    enum { STATIK_HAZIR, STATIK_OLC, CEKME_BEKLE, CEKILI_OLC, SONUC, HATA };
    static const uint8_t IZ = 100;

    void _olcumBasla() {
        _bas = millis();
        _toplam = 0;
        _sayi = 0;
        _enB = -1000;
        _enK = 1000;
    }
    void _olcumEkle(float p) {
        _toplam += p;
        _sayi++;
        if (p > _enB) _enB = p;
        if (p < _enK) _enK = p;
    }

    void _hesapla() {
        float akis = _sayi ? _toplam / _sayi : _statik;
        _dusus = _statik - akis;
        // Düşüşün %90'ına ulaşma süresi
        uint16_t t90 = IZ * 10;
        for (uint8_t i = 0; i < _izSayi; i++) {
            if (_statik - _iz[i] >= _dusus * 0.9f) {
                t90 = (uint16_t)i * 10;
                break;
            }
        }
        // Fark: düşüşün yarısı, ama dalgalanmanın rahatça üstünde olsun
        float fark = _dusus * 0.5f;
        if (fark < _dalga * 1.5f) fark = _dalga * 1.5f;
        if (fark < 1.0f) fark = 1.0f;
        if (_dusus < 1.5f || fark > _dusus * 0.8f) {
            _hata = F("Düşüş yetersiz!");
            _durum = HATA;
            return;
        }
        _oneriFark = ((uint16_t)(fark * 2.0f + 0.5f)) * 5;      // 0.5 psi'ye yuvarla (x10)
        float hist = _dusus * 0.2f;
        if (hist < 0.5f) hist = 0.5f;
        if (hist > 5.0f) hist = 5.0f;
        _oneriHist = (uint16_t)(hist * 10.0f + 0.5f);
        // Pencere: düşüş süresinin 2 katı + pay, 150..600 ms
        uint16_t pencere = (t90 * 2 + 100) / 10 * 10;
        if (pencere < 150) pencere = 150;
        if (pencere > 600) pencere = 600;
        _oneriPencere = pencere;
        _durum = SONUC;
    }

    uint8_t _durum = STATIK_HAZIR;
    uint32_t _bas = 0, _sonIz = 0;
    float _toplam = 0, _enB = 0, _enK = 0;
    uint16_t _sayi = 0;
    float _statik = 0, _dalga = 0, _dusus = 0;
    float _iz[IZ];
    uint8_t _izSayi = 0;
    uint16_t _oneriFark = 50, _oneriHist = 20, _oneriPencere = 300;
    FStr _hata = nullptr;
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
static TetikOgrenEkrani tetikOgrenEkrani;

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

void tetikOgrenAc() { ac(&tetikOgrenEkrani); }

void onayAc(FStr soru, void (*evet)()) {
    onayEkrani.soru = soru;
    onayEkrani.evet = evet;
    ac(&onayEkrani);
}

}  // namespace Ekranlar
