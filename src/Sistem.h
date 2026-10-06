// =============================================
// Sistem - Ana durum makinesi
//
// Modlar:
//  BEKLEME  : pompalar durur, basınç izlenir
//  BOYA     : tetik çekilince pompalar karışım oranında boya basar
//             (vanalar manueldir, elle açılır)
//  TEMIZLIK : temizlik döngüsü (Temizlik kütüphanesi)
//  SERVIS   : doldurma, kalibrasyon (ekranlar pompaları doğrudan sürer)
// =============================================
#ifndef SISTEM_H
#define SISTEM_H

#include <Arduino.h>
#include <Gosterge.h>
#include <Butonlar.h>
#include <BasincSensoru.h>
#include <Tetik.h>
#include <PompaSurucu.h>
#include <Temizlik.h>
#include "Ayarlar.h"

enum SistemModu : uint8_t {
    MOD_BEKLEME = 0,
    MOD_BOYA,
    MOD_TEMIZLIK,
    MOD_SERVIS
};

extern Gosterge gosterge;
extern Butonlar butonlar;
extern BasincSensoru sensor;
extern Tetik tetik;
extern PompaSurucu pompa;
extern Temizlik temizlik;

namespace Sistem {
    void begin();
    void guncelle();

    SistemModu mod();
    void bekleme();               // her şeyi durdur, BEKLEME moduna geç
    void boyaModuBaslat();
    void boyaModuDegistir();      // BOYA <-> BEKLEME
    void temizlikModu();          // TEMIZLIK moduna geç (başlatmaz)
    void servisModu();            // SERVIS moduna geç

    void ayarlariUygula();        // ayar değişince modülleri güncelle
    void kaydetIste();            // ayarları kısa süre sonra EEPROM'a yaz

    float rpmAdim(float rpm);     // dev/dk -> adım/sn
    // Karışım oranına göre pompa hızları (adım/sn)
    void oranHizlari(float& h1, float& h2);
    float akisMlDk();             // hesaplanan toplam akış
    float adimMl(uint8_t motor);  // kalibrasyon: adım/ml

    // Durum bilgileri
    bool puskurtuyor();
    uint32_t puskurtmeSuresiMs();
    bool maxSureAsildi();
    bool potOmruDoldu();
    int32_t potKalanSn();         // -1: takip yok
    void potUyarisiSustur();
    float oturumBoyaMl();
    float oturumSertMl();
    void oturumSifirla();
}

#endif
