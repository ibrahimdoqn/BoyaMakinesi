// =============================================
// Ekranlar - Ana ekran, menü ve servis ekranları
//
// Ekranlar bir yığın (stack) şeklinde açılır: Ana ekran -> Menü ->
// Kalibrasyon gibi. Bir ekran kapanınca bir öncekine dönülür.
// =============================================
#ifndef EKRANLAR_H
#define EKRANLAR_H

#include <Arduino.h>
#include <Gosterge.h>
#include <Butonlar.h>
#include <Menu.h>

class Ekran {
public:
    virtual void giris() {}
    virtual void cikis() {}
    virtual void tus(const TusBilgi& t) = 0;
    virtual void ciz(Gosterge& g) = 0;
    virtual void guncelle() {}
    // Uzun süre tuşa basılmazsa ana ekrana dönülsün mü?
    virtual bool zamanAsimiVar() const { return true; }
};

namespace Ekranlar {
    void begin();
    void guncelle();               // tuşları işler, ekranı çizer

    void ac(Ekran* ekran);
    void kapat();                  // en üstteki ekranı kapat
    void anaEkranaDon();

    // Kısa süreli bilgi mesajı
    void mesaj(FStr satir1, FStr satir2, uint16_t sureMs = 1500);

    // Hazır ekranlar
    void menuAc();
    void temizlikAc();
    void doldurAc(uint8_t pompaMaske);     // 1: boya, 2: sertleştirici, 3: karışım
    void kalibrasyonAc(uint8_t motor);
    void sifirlamaAc();
    void roleTestAc();
    void onayAc(FStr soru, void (*evet)());
}

// MenuTanim.cpp
Menu& menuAgaciniKur();

#endif
