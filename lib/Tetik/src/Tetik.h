// =============================================
// Tetik - Basınç düşüşünden tabanca tetiği algılama
//
// İki çalışma modu:
//
//  AKILLI (varsayılan) - Düşüş HIZINA bakar:
//    Kompresör dolup boşaldıkça basınç YAVAŞ değişir (saniyelerce),
//    tabanca tetiği çekilince basınç ANİDEN düşer (onlarca ms).
//    Son "pencere" süresi (ör. 300 ms) içindeki en yüksek basınç ile
//    şimdiki basınç karşılaştırılır:
//       tepe(pencere) - basınç >= fark   -> tetik çekildi
//    Yavaş dalgalanma 300 ms içinde sadece birkaç onda bir psi değiştirdiği
//    için tetik sanılmaz, mutlak basınç ne olursa olsun çalışır.
//
//    Tetik çekiliyken "akış basıncı" yavaşça takip edilir (kompresör
//    püskürtme sırasında devreye girse de uyum sağlar). Bırakma:
//       basınç - dip(pencere) >= fark*0.6   (ani yükseliş)  veya
//       basınç >= referans - histerezis     (statik basınca döndü)
//
//  MUTLAK - Basınç sabit bir eşiğin altına düşerse tetik çekilir,
//           eşik + histerezis üstüne çıkınca bırakılır.
//
// Ortak: çekme/bırakma onay gecikmesi, minimum çalışma basıncı
// (hava yokken tetik algılanmaz), sivri iğne (spike) reddi.
// =============================================
#ifndef TETIK_H
#define TETIK_H

#include <Arduino.h>

enum TetikModu : uint8_t {
    TETIK_AKILLI = 0,
    TETIK_MUTLAK = 1
};

class Tetik {
public:
    void ayarla(uint8_t mod, float farkPsi, float mutlakPsi, float histerezisPsi,
                uint16_t cekmeGecikmeMs, uint16_t birakmaGecikmeMs, float minBasincPsi,
                uint16_t pencereMs);

    // Her döngüde filtrelenmiş basınçla çağrılır
    void guncelle(float psi, uint32_t simdiMs);

    // Geçmişi mevcut basınçla doldurur, tetiği bırakılmış duruma getirir
    void sifirla(float psi);

    bool cekili() const { return _durum == CEKILI || _durum == BIRAKMA_BEKLE; }
    bool yeniCekildi();      // tetik az önce çekildiyse bir kez true döner
    bool yeniBirakildi();    // tetik az önce bırakıldıysa bir kez true döner

    bool basincYetersiz() const { return _yetersiz; }
    float referans() const { return _ref; }          // statik (tetik öncesi) basınç
    float cekmeEsigi() const;                         // bu basıncın altı = tetik
    float akisBasinci() const { return _akis; }      // tetik çekiliyken hat basıncı
    float anlikDusus() const { return _dusus; }      // pencere içindeki düşüş
    uint32_t cekiliSure(uint32_t simdiMs) const;

private:
    enum Durum : uint8_t { BOSTA, CEKME_BEKLE, CEKILI, BIRAKMA_BEKLE };

    void _ornekEkle(float psi);
    void _pencere(float& tepe, float& dip) const;
    void _guncelleAkilli(float psi, uint32_t simdi, float dt);
    void _guncelleMutlak(float psi, uint32_t simdi);
    void _cek(uint32_t simdi, float psi);
    void _birak();

    // Basınç geçmişi: 10 ms aralıkla, en fazla 640 ms
    static const uint8_t ORNEK_MS = 10;
    static const uint8_t GECMIS = 64;
    int16_t _gecmis[GECMIS];   // psi * 100
    uint8_t _yaz = 0;
    uint8_t _pencereOrnek = 30;
    uint32_t _sonOrnek = 0;

    uint8_t _mod = TETIK_AKILLI;
    float _fark = 5.0f, _mutlak = 45.0f, _hist = 2.0f, _min = 10.0f;
    uint16_t _cekmeGecikme = 50, _birakmaGecikme = 100;

    Durum _durum = BOSTA;
    bool _basladi = false;
    bool _yetersiz = true;
    bool _yeniCek = false, _yeniBirak = false;
    float _ref = 0.0f;       // statik basınç
    float _akis = 0.0f;      // akış basıncı (çekiliyken)
    float _dusus = 0.0f;
    uint32_t _sonGuncelleme = 0;
    uint32_t _durumZamani = 0;
    uint32_t _cekmeZamani = 0;
};

#endif
