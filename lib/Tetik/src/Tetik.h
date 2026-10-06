// =============================================
// Tetik - Basınç düşüşünden tabanca tetiği algılama
//
// İki çalışma modu:
//  FARK   : Hat basıncı sürekli izlenir (referans). Basınç referansın
//           "fark" kadar altına düşerse tetik çekilmiş sayılır.
//           Örn: referans 50 psi, fark 5 psi -> 45 psi'da tetik.
//           Kompresör/regülatör ayarı değişse de kendini uyarlar.
//  MUTLAK : Basınç sabit bir eşiğin altına düşerse tetik çekilir.
//
// - Histerezis: tetik bırakma eşiği, çekme eşiğinden yüksektir
//   (titreşimli aç/kapa olmaz).
// - Çekme/bırakma gecikmesi: kısa basınç dalgalanmalarını yok sayar.
// - Minimum çalışma basıncı: hava yoksa (kompresör kapalı) tetik algılanmaz.
// =============================================
#ifndef TETIK_H
#define TETIK_H

#include <Arduino.h>

enum TetikModu : uint8_t {
    TETIK_FARK = 0,
    TETIK_MUTLAK = 1
};

class Tetik {
public:
    void ayarla(uint8_t mod, float farkPsi, float mutlakPsi, float histerezisPsi,
                uint16_t cekmeGecikmeMs, uint16_t birakmaGecikmeMs, float minBasincPsi);

    // Her döngüde filtrelenmiş basınçla çağrılır
    void guncelle(float psi, uint32_t simdiMs);

    // Referansı mevcut basınca eşitler, tetiği bırakılmış duruma getirir
    void sifirla(float psi);

    bool cekili() const { return _durum == CEKILI || _durum == BIRAKMA_BEKLE; }
    bool yeniCekildi();      // tetik az önce çekildiyse bir kez true döner
    bool yeniBirakildi();    // tetik az önce bırakıldıysa bir kez true döner

    bool basincYetersiz() const { return _yetersiz; }
    float referans() const { return _ref; }
    float cekmeEsigi() const { return _cekmeEsik; }
    float birakmaEsigi() const { return _birakmaEsik; }
    uint32_t cekiliSure(uint32_t simdiMs) const;

private:
    enum Durum : uint8_t { BOSTA, CEKME_BEKLE, CEKILI, BIRAKMA_BEKLE };

    void _esikleriHesapla(float psi);

    uint8_t _mod = TETIK_FARK;
    float _fark = 5.0f, _mutlak = 45.0f, _hist = 2.0f, _min = 10.0f;
    uint16_t _cekmeGecikme = 50, _birakmaGecikme = 80;

    Durum _durum = BOSTA;
    bool _basladi = false;
    bool _yetersiz = true;
    bool _yeniCek = false, _yeniBirak = false;
    float _ref = 0.0f;
    float _cekmeEsik = 0.0f, _birakmaEsik = 0.0f;
    uint32_t _sonGuncelleme = 0;
    uint32_t _durumZamani = 0;
    uint32_t _cekmeZamani = 0;
};

#endif
