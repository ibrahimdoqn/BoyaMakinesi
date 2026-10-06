// =============================================
// Gosterge - 16x2 I2C LCD tamponlu sürücü
//
// - Tüm yazılar önce RAM'deki bir tampona yazılır, guncelle() sadece
//   değişen karakterleri LCD'ye gönderir (titreme olmaz, I2C yükü azalır).
// - UTF-8 Türkçe karakterleri (ç ğ ı ş ö ü Ç Ş İ Ü) özel karakterlerle
//   gösterir. Kaynak kodda metinler doğrudan Türkçe yazılabilir.
// =============================================
#ifndef GOSTERGE_H
#define GOSTERGE_H

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

#define GOSTERGE_SUTUN 16
#define GOSTERGE_SATIR 2

// HD44780 ROM'undaki ok işaretleri
#define OK_SAG "\x7E"
#define OK_SOL "\x7F"

class Gosterge {
public:
    Gosterge(uint8_t adres);

    void begin();

    // --- Tampon işlemleri ---
    void temizle();
    void satirTemizle(uint8_t satir);
    // (sutun, satir) konumundan itibaren yazar, satır sonunda keser.
    // Yazılan karakter sayısını döndürür.
    uint8_t yaz(uint8_t sutun, uint8_t satir, const char* metin);
    uint8_t yaz(uint8_t sutun, uint8_t satir, const __FlashStringHelper* metin);
    // Satırı temizleyip yazar
    void satir(uint8_t satir, const char* metin);
    void satir(uint8_t satir, const __FlashStringHelper* metin);
    // Satırın sağına yaslı yazar
    void sagaYaz(uint8_t satir, const char* metin);
    void sagaYaz(uint8_t satir, const __FlashStringHelper* metin);
    void karakter(uint8_t sutun, uint8_t satir, uint8_t kod);

    // Değişen karakterleri LCD'ye gönderir
    void guncelle();
    // Bir sonraki guncelle()'de tüm ekranı yeniden gönderir
    void tamYenile();

    void isik(bool acik);
    bool isik() const { return _isik; }

    // --- Yardımcılar ---
    // UTF-8 metnin ekranda kaplayacağı karakter sayısı
    static uint8_t uzunluk(const char* metin);
    static uint8_t uzunluk(const __FlashStringHelper* metin);
    // Tam sayıyı ondalıklı yazar: (123, 1) -> "12.3"
    static char* ondalik(char* tampon, int32_t deger, uint8_t basamak);

private:
    uint8_t _yaz(uint8_t sutun, uint8_t satir, const char* metin, bool flash);
    static uint8_t _uzunluk(const char* metin, bool flash);

    LiquidCrystal_I2C _lcd;
    uint8_t _tampon[GOSTERGE_SATIR][GOSTERGE_SUTUN];
    uint8_t _ekranda[GOSTERGE_SATIR][GOSTERGE_SUTUN];
    bool _isik;
};

#endif
