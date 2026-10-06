// =============================================
// MenuTanim - Menü ağacı
//
// Yeni bir ayar eklemek için:
//   1. Ayarlar.h'deki yapıya alanı ekleyin, AYAR_SURUM'u artırın
//   2. ayarVarsayilan() içinde varsayılan değeri verin
//   3. Aşağıda uygun alt menüye bir öğe ekleyin
//   4. Gerekirse Sistem::ayarlariUygula() içinde ilgili modüle aktarın
// =============================================
#include "Ekranlar.h"
#include "Sistem.h"
#include "PinConfig.h"

// ---------------------------------------------------------------
// Bilgi / yazı fonksiyonları
// ---------------------------------------------------------------
static void mlEkle(char* t, uint8_t n, float ml) {
    char s[14];
    if (ml >= 10000.0f) {
        Gosterge::ondalik(s, (int32_t)(ml / 10.0f + 0.5f), 2);   // litre
        menuEkle(t, n, s);
        menuEkle(t, n, " L");
    } else {
        Gosterge::ondalik(s, (int32_t)(ml * 10.0f + 0.5f), 1);
        menuEkle(t, n, s);
        menuEkle(t, n, " ml");
    }
}

static void psiEkle(char* t, uint8_t n, float psi) {
    char s[10];
    Gosterge::ondalik(s, (int32_t)(psi * 10.0f + (psi >= 0 ? 0.5f : -0.5f)), 1);
    menuEkle(t, n, s);
}

static void yaziBoyaModu(char* t, uint8_t n) {
    menuEkle(t, n, Sistem::mod() == MOD_BOYA ? F(" OK: Durdur") : F(" OK: Başlat"));
}

static void yaziCanliTetik(char* t, uint8_t n) {
    // "48.5 D:0.3"  D = son pencerede düşüş,  "* " = tetik çekili
    if (tetik.cekili()) menuEkle(t, n, "* ");
    psiEkle(t, n, sensor.psi());
    if (ayar.tetikModu == TETIK_AKILLI) {
        menuEkle(t, n, " D:");
        psiEkle(t, n, tetik.anlikDusus());
    } else {
        menuEkle(t, n, " E:");
        psiEkle(t, n, tetik.cekmeEsigi());
    }
}

static void yaziAkis(char* t, uint8_t n) {
    char s[12];
    Gosterge::ondalik(s, (int32_t)(Sistem::akisMlDk() * 10.0f + 0.5f), 1);
    menuEkle(t, n, s);
    menuEkle(t, n, " ml/dk");
}

static void yaziSensor(char* t, uint8_t n) {
    char s[10];
    psiEkle(t, n, sensor.psi());
    menuEkle(t, n, "psi ");
    Gosterge::ondalik(s, sensor.milivolt() / 10, 2);
    menuEkle(t, n, s);
    menuEkle(t, n, "V");
}

static void yaziToplamBoya(char* t, uint8_t n)  { mlEkle(t, n, sayac.boyaMl); }
static void yaziToplamSert(char* t, uint8_t n)  { mlEkle(t, n, sayac.sertMl); }
static void yaziToplamTemiz(char* t, uint8_t n) { mlEkle(t, n, sayac.temizMl); }

static void yaziPuskurtme(char* t, uint8_t n) {
    char s[12];
    ultoa(sayac.puskurtmeSayisi, s, 10);
    menuEkle(t, n, s);
    menuEkle(t, n, " kez");
}

static void yaziCalisma(char* t, uint8_t n) {
    char s[12];
    ultoa(sayac.calismaDk / 60, s, 10);
    menuEkle(t, n, s);
    menuEkle(t, n, " sa ");
    ultoa(sayac.calismaDk % 60, s, 10);
    menuEkle(t, n, s);
    menuEkle(t, n, " dk");
}

static void yaziSurum(char* t, uint8_t n) {
    menuEkle(t, n, F(YAZILIM_SURUMU " 2K Boya"));
}

// ---------------------------------------------------------------
// Aksiyonlar
// ---------------------------------------------------------------
static void akBoyaModu() {
    Sistem::boyaModuDegistir();
    Ekranlar::anaEkranaDon();
}
static void akTemizlik()     { Ekranlar::temizlikAc(); }
static void akKalib1()       { Ekranlar::kalibrasyonAc(0); }
static void akKalib2()       { Ekranlar::kalibrasyonAc(1); }
static void akDoldurBoya()   { Ekranlar::doldurAc(1); }
static void akDoldurSert()   { Ekranlar::doldurAc(2); }
static void akDoldurKarisim(){ Ekranlar::doldurAc(3); }
static void akSifirla()      { Ekranlar::sifirlamaAc(); }
static void akTetikOgren()   { Ekranlar::tetikOgrenAc(); }

static void sayacSifirlaEvet() {
    sayacSifirla();
    sayacKaydet();
    Sistem::oturumSifirla();
    Ekranlar::mesaj(F("Sayaçlar"), F("sıfırlandı"));
}
static void akSayacSifirla() { Ekranlar::onayAc(F("Sayaç sıfırla?"), sayacSifirlaEvet); }

static void fabrikaEvet() {
    Sistem::bekleme();
    ayarVarsayilan();
    ayarKaydet();
    Sistem::ayarlariUygula();
    Ekranlar::mesaj(F("Fabrika ayarları"), F("yüklendi"));
}
static void akFabrika() { Ekranlar::onayAc(F("Fabrika ayarı?"), fabrikaEvet); }

static void akKaydet() {
    ayarKaydet();
    Ekranlar::mesaj(F("Ayarlar"), F("kaydedildi"));
}

// ---------------------------------------------------------------
// Menü ağacı
// ---------------------------------------------------------------
#define DIZI_BOYUT(d) (sizeof(d) / sizeof((d)[0]))

Menu& menuAgaciniKur() {
    // --- Tetik ayarları ---
    static AksiyonOge tOgren(F("Tetik Öğren"), akTetikOgren, F(" OK: Başlat"));
    static SecimOge tMod(F("Tetik Modu"), &ayar.tetikModu, F("Akıllı (hız)|Mutlak eşik"));
    static SayiOge<uint16_t> tPencere(F("Algı Penceresi"), &ayar.tetikPencereMs, 100, 600, 10, 0, F("ms"));
    static SayiOge<uint16_t> tFark(F("Düşüş Farkı"), &ayar.tetikFark_x10, 5, 500, 5, 1, F("psi"));
    static SayiOge<uint16_t> tMutlak(F("Mutlak Eşik"), &ayar.tetikMutlak_x10, 10, 1900, 5, 1, F("psi"));
    static SayiOge<uint16_t> tHist(F("Histerezis"), &ayar.histerezis_x10, 0, 200, 5, 1, F("psi"));
    static SayiOge<uint16_t> tCekme(F("Çekme Gecik."), &ayar.cekmeGecikme, 0, 1000, 10, 0, F("ms"));
    static SayiOge<uint16_t> tBirak(F("Bırakma Gecik."), &ayar.birakmaGecikme, 0, 2000, 10, 0, F("ms"));
    static SayiOge<uint16_t> tMin(F("Min Basınç"), &ayar.minBasinc_x10, 0, 1000, 10, 1, F("psi"));
    static SayiOge<uint16_t> tMax(F("Maks Püskürt."), &ayar.maxPuskurtmeSn, 0, 600, 5, 0, F("sn"), F("Kapalı"));
    static BilgiOge tCanli(F("Canlı Basınç"), yaziCanliTetik);
    static GeriOge tGeri(F("Geri"));
    static MenuOge* const tetikOgeler[] = {&tCanli, &tOgren, &tMod, &tFark, &tPencere, &tMutlak, &tHist,
                                           &tCekme, &tBirak, &tMin, &tMax, &tGeri};
    static Menu tetikMenu(F("Tetik"), tetikOgeler, DIZI_BOYUT(tetikOgeler));

    // --- Pompa ayarları ---
    static BilgiOge pAkis(F("Akış (hesap)"), yaziAkis);
    static SayiOge<uint16_t> pRampa(F("Rampa Süresi"), &ayar.rampaMs, 0, 2000, 10, 0, F("ms"), F("Kapalı"));
    static AksiyonOge pDoldurB(F("Boya Doldur"), akDoldurBoya, F(" OK: Aç"));
    static AksiyonOge pDoldurS(F("Sert. Doldur"), akDoldurSert, F(" OK: Aç"));
    static AksiyonOge pDoldurK(F("Karışım Doldur"), akDoldurKarisim, F(" OK: Aç"));
    static AksiyonOge pKal1(F("P1 Kalibrasyon"), akKalib1, F(" OK: Başlat"));
    static AksiyonOge pKal2(F("P2 Kalibrasyon"), akKalib2, F(" OK: Başlat"));
    static SayiOge<uint32_t> pK1(F("P1 Adım/ml"), &ayar.kal1_x10, 10, 999999, 1, 1);
    static SayiOge<uint32_t> pK2(F("P2 Adım/ml"), &ayar.kal2_x10, 10, 999999, 1, 1);
    static SayiOge<uint16_t> pKAdim(F("Kalib. Adım"), &ayar.kalibAdim, 800, 64000, 800, 0, F("adım"));
    static SayiOge<uint16_t> pAdimTur(F("Adım/Tur"), &ayar.adimTur, 200, 25600, 200, 0, F("darbe"));
    static SecimOge pYon1(F("P1 Yönü"), &ayar.yon1Ters, F("Normal|Ters"));
    static SecimOge pYon2(F("P2 Yönü"), &ayar.yon2Ters, F("Normal|Ters"));
    static SecimOge pEn(F("Sürücü EN"), &ayar.enAktifYuksek, F("Aktif LOW|Aktif HIGH"));
    static GeriOge pGeri(F("Geri"));
    static MenuOge* const pompaOgeler[] = {&pAkis, &pRampa, &pDoldurB, &pDoldurS, &pDoldurK,
                                           &pKal1, &pKal2, &pK1, &pK2, &pKAdim, &pAdimTur,
                                           &pYon1, &pYon2, &pEn, &pGeri};
    static Menu pompaMenu(F("Pompa"), pompaOgeler, DIZI_BOYUT(pompaOgeler));

    // --- Temizlik ayarları ---
    static SayiOge<uint16_t> cHiz(F("Temizlik Hızı"), &ayar.temizHiz, MOTOR_MIN_RPM, MOTOR_MAX_RPM,
                                  MOTOR_RPM_ADIM, 0, F("dev/dk"));
    static SecimOge cPompa(F("Temiz. Pompa"), &ayar.temizPompa, F("Pompa 1|Pompa 2|İkisi"));
    static SayiOge<uint16_t> cMax(F("Maks Süre"), &ayar.temizMaxSn, 0, 1800, 10, 0, F("sn"), F("Sınırsız"));
    static SecimOge cDarbe(F("Darbeli Mod"), &ayar.temizDarbeli, F("Kapalı|Açık (3s/1s)"));
    static SayiOge<uint16_t> cPot(F("Pot Ömrü"), &ayar.potOmruDk, 0, 480, 5, 0, F("dk"), F("Kapalı"));
    static GeriOge cGeri(F("Geri"));
    static MenuOge* const temizOgeler[] = {&cHiz, &cPompa, &cMax, &cDarbe, &cPot, &cGeri};
    static Menu temizMenu(F("Temizlik"), temizOgeler, DIZI_BOYUT(temizOgeler));

    // --- Sensör ayarları ---
    static BilgiOge sCanli(F("Canlı Okuma"), yaziSensor);
    static AksiyonOge sSifir(F("Sıfır Kalibre"), akSifirla, F(" OK: Aç"));
    static SayiOge<int16_t> sOfset(F("Sıfır Ofset"), &ayar.sifirOfset_x10, -200, 200, 1, 1, F("psi"));
    static SayiOge<uint16_t> sVmin(F("Sensör Min V"), &ayar.sensVmin, 0, 2500, 10, 3, F("V"));
    static SayiOge<uint16_t> sVmax(F("Sensör Max V"), &ayar.sensVmax, 2500, 5000, 10, 3, F("V"));
    static SayiOge<uint16_t> sPsi(F("Sensör Aralığı"), &ayar.sensMaxPsi, 10, 1000, 10, 0, F("psi"));
    static SayiOge<uint16_t> sRef(F("ADC Referans"), &ayar.adcRef, 4000, 5500, 10, 3, F("V"));
    static SayiOge<uint8_t> sFiltre(F("Filtre"), &ayar.filtre, 1, 50, 1, 0, F("örnek"));
    static GeriOge sGeri(F("Geri"));
    static MenuOge* const sensorOgeler[] = {&sCanli, &sSifir, &sOfset, &sVmin, &sVmax,
                                            &sPsi, &sRef, &sFiltre, &sGeri};
    static Menu sensorMenu(F("Sensör"), sensorOgeler, DIZI_BOYUT(sensorOgeler));

    // --- İstatistik ---
    static BilgiOge iBoya(F("Toplam Boya"), yaziToplamBoya);
    static BilgiOge iSert(F("Toplam Sert."), yaziToplamSert);
    static BilgiOge iTemiz(F("Toplam Temiz."), yaziToplamTemiz);
    static BilgiOge iPusk(F("Püskürtme"), yaziPuskurtme);
    static BilgiOge iCalisma(F("Boya Süresi"), yaziCalisma);
    static AksiyonOge iSifirla(F("Sayaç Sıfırla"), akSayacSifirla);
    static GeriOge iGeri(F("Geri"));
    static MenuOge* const istOgeler[] = {&iBoya, &iSert, &iTemiz, &iPusk, &iCalisma, &iSifirla, &iGeri};
    static Menu istMenu(F("İstatistik"), istOgeler, DIZI_BOYUT(istOgeler));

    // --- Sistem ---
    static SecimOge yAcilis(F("Açılış Modu"), &ayar.acilisModu, F("Bekleme|Boya modu"));
    static SecimOge yIsik(F("LCD Işığı"), &ayar.lcdIsik, F("Kapalı|Açık"));
    static AltMenuOge yIst(F("İstatistik"), &istMenu);
    static AksiyonOge yKaydet(F("Şimdi Kaydet"), akKaydet);
    static AksiyonOge yFabrika(F("Fabrika Ayarı"), akFabrika);
    static BilgiOge ySurum(F("Yazılım"), yaziSurum);
    static GeriOge yGeri(F("Geri"));
    static MenuOge* const sistemOgeler[] = {&yAcilis, &yIsik, &yIst, &yKaydet, &yFabrika, &ySurum, &yGeri};
    static Menu sistemMenu(F("Sistem"), sistemOgeler, DIZI_BOYUT(sistemOgeler));

    // --- Ana menü ---
    static AksiyonOge aBoya(F("Boya Modu"), akBoyaModu, nullptr, yaziBoyaModu);
    static AksiyonOge aTemiz(F("Temizlik"), akTemizlik, F(" OK: Aç"));
    static OranOge aOran(F("Karışım Oranı"), &ayar.oranBoya, &ayar.oranSert,
                         MIN_KARISIM_ORANI, MAX_KARISIM_ORANI);
    static SayiOge<uint16_t> aHiz(F("Motor Hızı"), &ayar.motorHizi, MOTOR_MIN_RPM, MOTOR_MAX_RPM,
                                  MOTOR_RPM_ADIM, 0, F("dev/dk"));
    static AltMenuOge aTetik(F("Tetik Ayarları"), &tetikMenu);
    static AltMenuOge aPompa(F("Pompa Ayarları"), &pompaMenu);
    static AltMenuOge aTemizA(F("Temizlik Ayar."), &temizMenu);
    static AltMenuOge aSensor(F("Sensör Ayarı"), &sensorMenu);
    static AltMenuOge aSistem(F("Sistem"), &sistemMenu);
    static GeriOge aCikis(F("Çıkış"));
    static MenuOge* const anaOgeler[] = {&aBoya, &aTemiz, &aOran, &aHiz, &aTetik, &aPompa,
                                         &aTemizA, &aSensor, &aSistem, &aCikis};
    static Menu anaMenu(F("Ana Menü"), anaOgeler, DIZI_BOYUT(anaOgeler));

    return anaMenu;
}
