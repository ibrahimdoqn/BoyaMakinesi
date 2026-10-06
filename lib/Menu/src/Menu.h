// =============================================
// Menu - 16x2 LCD için modüler menü sistemi
//
// Ekran düzeni:
//   Satır 1: Öğe adı                  (sağda sıra no: 3/8)
//   Satır 2: Öğenin değeri / açıklaması
//
// Tuşlar (gezinme):
//   YUKARI / AŞAĞI : önceki / sonraki öğe
//   OK / SAĞ       : gir, düzenle, çalıştır
//   SOL            : geri (ana menüde: menüden çık)
// Tuşlar (değer düzenlerken, değer [köşeli parantez] içinde görünür):
//   YUKARI / AŞAĞI : artır / azalt (basılı tutunca hızlanır)
//   OK / SAĞ       : kaydet
//   SOL            : iptal
//
// Öğe tipleri:
//   SayiOge<T>  : sayısal ayar (min, max, adım, ondalık, birim)
//   SecimOge    : seçenek listesi ("Kapalı|Açık" gibi)
//   OranOge     : iki parçalı oran (Boya : Sertleştirici)
//   AltMenuOge  : alt menüye geçiş
//   AksiyonOge  : bir fonksiyon çalıştırır
//   BilgiOge    : canlı değer gösterir (salt okunur)
//   GeriOge     : bir üst menüye döner
// Yeni bir tip için MenuOge'den türetmek yeterlidir.
// =============================================
#ifndef MENU_H
#define MENU_H

#include <Arduino.h>
#include <Gosterge.h>
#include <Butonlar.h>

#define MENU_TAMPON 40          // UTF-8 için 16 karakterden büyük tampon
#define MENU_MAX_DERINLIK 5

typedef const __FlashStringHelper* FStr;

class MenuSistemi;

// Flash'taki metni tampona ekler
void menuEkle(char* tampon, uint8_t boyut, FStr metin);
void menuEkle(char* tampon, uint8_t boyut, const char* metin);

// ---------------------------------------------------------------
class MenuOge {
public:
    explicit MenuOge(FStr etiket) : _etiket(etiket) {}
    FStr etiket() const { return _etiket; }

    // İkinci satırda gösterilecek metin
    virtual void deger(char* tampon, uint8_t boyut, bool duzenleniyor) const;
    virtual bool duzenlenebilir() const { return false; }
    virtual void duzenlemeBasla() {}
    virtual void degistir(int8_t yon, uint8_t tekrar) { (void)yon; (void)tekrar; }
    // true: düzenleme bitti ve değer kaydedildi. false: bir sonraki alana geç
    virtual bool onayla() { return true; }
    virtual void iptal() {}
    // Düzenlenemeyen öğede OK'a basıldığında
    virtual void sec(MenuSistemi& menu) { (void)menu; }

protected:
    FStr _etiket;
};

// ---------------------------------------------------------------
class Menu {
public:
    Menu(FStr baslik, MenuOge* const* ogeler, uint8_t sayi)
        : baslik(baslik), ogeler(ogeler), sayi(sayi) {}
    FStr baslik;
    MenuOge* const* ogeler;
    uint8_t sayi;
};

// ---------------------------------------------------------------
class MenuSistemi {
public:
    typedef void (*DegisimFn)(MenuOge* oge);

    void begin(Gosterge& gosterge, Menu& kok);
    void degisimBildir(DegisimFn fn) { _degisim = fn; }

    void ac();                    // ana menüyü aç
    void kapat();
    bool acik() const { return _acik; }
    bool duzenleniyor() const { return _duzenle; }

    void tus(const TusBilgi& t);
    void ciz();

    void altMenuAc(Menu* menu);
    void geri();

private:
    MenuOge* _secili() const;

    Gosterge* _g = nullptr;
    Menu* _yigin[MENU_MAX_DERINLIK];
    uint8_t _secim[MENU_MAX_DERINLIK];
    uint8_t _derinlik = 0;
    bool _acik = false;
    bool _duzenle = false;
    DegisimFn _degisim = nullptr;
};

// ===============================================================
// Hazır öğe tipleri
// ===============================================================

// Basılı tutma süresine göre artış çarpanı
uint8_t menuCarpan(uint8_t tekrar);

template <typename T>
class SayiOge : public MenuOge {
public:
    // deger     : ayarın tutulduğu değişken (ondalıklı ise x10, x100... saklanır)
    // ondalik   : gösterimde virgülden sonraki basamak sayısı
    // birim     : "psi", "ms" ... (nullptr olabilir)
    // sifirYazi : değer 0 iken gösterilecek metin, örn "Kapalı"
    SayiOge(FStr etiket, T* deger, int32_t min, int32_t max, int32_t adim,
            uint8_t ondalik = 0, FStr birim = nullptr, FStr sifirYazi = nullptr)
        : MenuOge(etiket), _p(deger), _min(min), _max(max), _adim(adim),
          _ondalik(ondalik), _birim(birim), _sifir(sifirYazi) {}

    void deger(char* t, uint8_t boyut, bool duz) const override {
        int32_t v = duz ? _gecici : (int32_t)*_p;
        t[0] = 0;
        menuEkle(t, boyut, duz ? "[" : " ");
        if (v == 0 && _sifir) {
            menuEkle(t, boyut, _sifir);
        } else {
            char s[14];
            Gosterge::ondalik(s, v, _ondalik);
            menuEkle(t, boyut, s);
        }
        menuEkle(t, boyut, duz ? "]" : " ");
        if (_birim && !(v == 0 && _sifir)) menuEkle(t, boyut, _birim);
    }
    bool duzenlenebilir() const override { return true; }
    void duzenlemeBasla() override { _gecici = (int32_t)*_p; }
    void degistir(int8_t yon, uint8_t tekrar) override {
        int32_t carpan = menuCarpan(tekrar);
        // Çarpan, aralığın onda birini geçmesin
        while (carpan > 1 && _adim * carpan * 10 > (_max - _min)) carpan /= 10;
        int32_t v = _gecici + yon * _adim * carpan;
        if (carpan > 1) v = (v / (_adim * carpan)) * (_adim * carpan);  // yuvarla
        if (v < _min) v = _min;
        if (v > _max) v = _max;
        _gecici = v;
    }
    bool onayla() override {
        *_p = (T)_gecici;
        return true;
    }

private:
    T* _p;
    int32_t _min, _max, _adim;
    uint8_t _ondalik;
    FStr _birim;
    FStr _sifir;
    int32_t _gecici = 0;
};

class SecimOge : public MenuOge {
public:
    // secenekler: "Kapalı|Açık" biçiminde, '|' ile ayrılmış
    SecimOge(FStr etiket, uint8_t* deger, FStr secenekler);
    void deger(char* t, uint8_t boyut, bool duz) const override;
    bool duzenlenebilir() const override { return true; }
    void duzenlemeBasla() override { _gecici = *_p < _sayi ? *_p : 0; }
    void degistir(int8_t yon, uint8_t tekrar) override;
    bool onayla() override { *_p = _gecici; return true; }

private:
    void _secenek(uint8_t i, char* t, uint8_t boyut) const;
    uint8_t* _p;
    FStr _sec;
    uint8_t _sayi;
    uint8_t _gecici = 0;
};

class OranOge : public MenuOge {
public:
    OranOge(FStr etiket, uint8_t* a, uint8_t* b, uint8_t min, uint8_t max);
    void deger(char* t, uint8_t boyut, bool duz) const override;
    bool duzenlenebilir() const override { return true; }
    void duzenlemeBasla() override;
    void degistir(int8_t yon, uint8_t tekrar) override;
    bool onayla() override;

private:
    uint8_t *_a, *_b;
    uint8_t _min, _max;
    uint8_t _ga = 0, _gb = 0;
    uint8_t _alan = 0;
};

class AltMenuOge : public MenuOge {
public:
    AltMenuOge(FStr etiket, Menu* alt) : MenuOge(etiket), _alt(alt) {}
    void deger(char* t, uint8_t boyut, bool duz) const override;
    void sec(MenuSistemi& m) override { m.altMenuAc(_alt); }

private:
    Menu* _alt;
};

class AksiyonOge : public MenuOge {
public:
    typedef void (*Fn)();
    typedef void (*YaziFn)(char* tampon, uint8_t boyut);
    // aciklama veya yazi: ikinci satırda gösterilecek metin
    AksiyonOge(FStr etiket, Fn fn, FStr aciklama = nullptr, YaziFn yazi = nullptr)
        : MenuOge(etiket), _fn(fn), _aciklama(aciklama), _yazi(yazi) {}
    void deger(char* t, uint8_t boyut, bool duz) const override;
    void sec(MenuSistemi& m) override { (void)m; if (_fn) _fn(); }

private:
    Fn _fn;
    FStr _aciklama;
    YaziFn _yazi;
};

class BilgiOge : public MenuOge {
public:
    typedef void (*YaziFn)(char* tampon, uint8_t boyut);
    BilgiOge(FStr etiket, YaziFn yazi) : MenuOge(etiket), _yazi(yazi) {}
    void deger(char* t, uint8_t boyut, bool duz) const override {
        (void)duz;
        t[0] = 0;
        if (_yazi) _yazi(t, boyut);
    }

private:
    YaziFn _yazi;
};

class GeriOge : public MenuOge {
public:
    explicit GeriOge(FStr etiket) : MenuOge(etiket) {}
    void deger(char* t, uint8_t boyut, bool duz) const override;
    void sec(MenuSistemi& m) override { m.geri(); }
};

#endif
