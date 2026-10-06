#include "Gosterge.h"

// Özel karakterler 8..15 kodlarından yazılır (0..7 ile aynı CGRAM, ama
// 0 kodu C dizilerinde sonlandırıcı olduğu için 8..15 kullanılır).
enum {
    KR_c = 8,   // ç
    KR_g,       // ğ
    KR_i,       // ı
    KR_s,       // ş
    KR_C,       // Ç
    KR_S,       // Ş
    KR_I,       // İ
    KR_U        // Ü
};

static const uint8_t OZEL_KARAKTERLER[8][8] PROGMEM = {
    {0x00, 0x00, 0x0E, 0x10, 0x10, 0x0E, 0x04, 0x0C},  // ç
    {0x0E, 0x00, 0x0F, 0x11, 0x11, 0x0F, 0x01, 0x0E},  // ğ
    {0x00, 0x00, 0x0C, 0x04, 0x04, 0x04, 0x0E, 0x00},  // ı
    {0x00, 0x00, 0x0E, 0x10, 0x0E, 0x01, 0x1E, 0x04},  // ş
    {0x0E, 0x11, 0x10, 0x10, 0x11, 0x0E, 0x04, 0x0C},  // Ç
    {0x0F, 0x10, 0x0E, 0x01, 0x01, 0x1E, 0x04, 0x0C},  // Ş
    {0x04, 0x00, 0x0E, 0x04, 0x04, 0x04, 0x0E, 0x00},  // İ
    {0x0A, 0x00, 0x11, 0x11, 0x11, 0x11, 0x0E, 0x00},  // Ü
};

static inline uint8_t oku(const char* p, bool flash) {
    return flash ? pgm_read_byte(p) : (uint8_t)*p;
}

// UTF-8 metinden bir karakter çözer, LCD kodunu döndürür. 0 = metin sonu.
static uint8_t karakterCoz(const char*& p, bool flash) {
    uint8_t c = oku(p, flash);
    if (c == 0) return 0;
    p++;
    if (c < 0x80) return c;

    uint8_t c2 = oku(p, flash);
    if ((c2 & 0xC0) != 0x80) return '?';
    p++;
    if (c == 0xC3) {
        switch (c2) {
            case 0xA7: return KR_c;   // ç
            case 0x87: return KR_C;   // Ç
            case 0xB6: return 0xEF;   // ö (ROM)
            case 0x96: return 'O';    // Ö
            case 0xBC: return 0xF5;   // ü (ROM)
            case 0x9C: return KR_U;   // Ü
        }
    } else if (c == 0xC4) {
        switch (c2) {
            case 0x9F: return KR_g;   // ğ
            case 0x9E: return 'G';    // Ğ
            case 0xB1: return KR_i;   // ı
            case 0xB0: return KR_I;   // İ
        }
    } else if (c == 0xC5) {
        switch (c2) {
            case 0x9F: return KR_s;   // ş
            case 0x9E: return KR_S;   // Ş
        }
    } else if (c == 0xC2 && c2 == 0xB0) {
        return 0xDF;                  // ° (ROM)
    }
    // Desteklenmeyen çok baytlı karakter: kalan devam baytlarını atla
    while ((oku(p, flash) & 0xC0) == 0x80) p++;
    return '?';
}

Gosterge::Gosterge(uint8_t adres)
    : _lcd(adres, GOSTERGE_SUTUN, GOSTERGE_SATIR), _isik(true) {}

void Gosterge::begin() {
    _lcd.init();
    _lcd.backlight();
    uint8_t tampon[8];
    for (uint8_t i = 0; i < 8; i++) {
        memcpy_P(tampon, OZEL_KARAKTERLER[i], 8);
        _lcd.createChar(i, tampon);
    }
    _lcd.clear();
    temizle();
    tamYenile();
}

void Gosterge::temizle() {
    memset(_tampon, ' ', sizeof(_tampon));
}

void Gosterge::satirTemizle(uint8_t satir) {
    if (satir < GOSTERGE_SATIR) memset(_tampon[satir], ' ', GOSTERGE_SUTUN);
}

uint8_t Gosterge::_yaz(uint8_t sutun, uint8_t satir, const char* metin, bool flash) {
    if (satir >= GOSTERGE_SATIR || metin == nullptr) return 0;
    uint8_t n = 0;
    const char* p = metin;
    while (sutun < GOSTERGE_SUTUN) {
        uint8_t k = karakterCoz(p, flash);
        if (k == 0) break;
        _tampon[satir][sutun++] = k;
        n++;
    }
    return n;
}

uint8_t Gosterge::yaz(uint8_t sutun, uint8_t satir, const char* metin) {
    return _yaz(sutun, satir, metin, false);
}

uint8_t Gosterge::yaz(uint8_t sutun, uint8_t satir, const __FlashStringHelper* metin) {
    return _yaz(sutun, satir, (const char*)metin, true);
}

void Gosterge::satir(uint8_t satir, const char* metin) {
    satirTemizle(satir);
    _yaz(0, satir, metin, false);
}

void Gosterge::satir(uint8_t satir, const __FlashStringHelper* metin) {
    satirTemizle(satir);
    _yaz(0, satir, (const char*)metin, true);
}

void Gosterge::sagaYaz(uint8_t satir, const char* metin) {
    uint8_t u = uzunluk(metin);
    _yaz(u >= GOSTERGE_SUTUN ? 0 : GOSTERGE_SUTUN - u, satir, metin, false);
}

void Gosterge::sagaYaz(uint8_t satir, const __FlashStringHelper* metin) {
    uint8_t u = uzunluk(metin);
    _yaz(u >= GOSTERGE_SUTUN ? 0 : GOSTERGE_SUTUN - u, satir, (const char*)metin, true);
}

void Gosterge::karakter(uint8_t sutun, uint8_t satir, uint8_t kod) {
    if (satir < GOSTERGE_SATIR && sutun < GOSTERGE_SUTUN) _tampon[satir][sutun] = kod;
}

void Gosterge::guncelle() {
    for (uint8_t r = 0; r < GOSTERGE_SATIR; r++) {
        int8_t imlec = -1;  // LCD imlecinin bilinen konumu
        for (uint8_t c = 0; c < GOSTERGE_SUTUN; c++) {
            if (_tampon[r][c] == _ekranda[r][c]) continue;
            if (imlec != (int8_t)c) _lcd.setCursor(c, r);
            _lcd.write(_tampon[r][c]);
            _ekranda[r][c] = _tampon[r][c];
            imlec = c + 1;
        }
    }
}

void Gosterge::tamYenile() {
    // Ekrandaki kopyayı geçersiz kıl, her karakter yeniden gönderilsin
    memset(_ekranda, 0xFF, sizeof(_ekranda));
}

void Gosterge::isik(bool acik) {
    if (acik == _isik) return;
    _isik = acik;
    if (acik) _lcd.backlight();
    else _lcd.noBacklight();
}

uint8_t Gosterge::_uzunluk(const char* metin, bool flash) {
    if (metin == nullptr) return 0;
    uint8_t n = 0;
    const char* p = metin;
    while (karakterCoz(p, flash) != 0) n++;
    return n;
}

uint8_t Gosterge::uzunluk(const char* metin) { return _uzunluk(metin, false); }
uint8_t Gosterge::uzunluk(const __FlashStringHelper* metin) {
    return _uzunluk((const char*)metin, true);
}

char* Gosterge::ondalik(char* tampon, int32_t deger, uint8_t basamak) {
    if (basamak == 0) {
        ltoa(deger, tampon, 10);
        return tampon;
    }
    int32_t bolen = 1;
    for (uint8_t i = 0; i < basamak; i++) bolen *= 10;
    bool negatif = deger < 0;
    uint32_t mutlak = negatif ? (uint32_t)(-deger) : (uint32_t)deger;
    char* p = tampon;
    if (negatif) *p++ = '-';
    ultoa(mutlak / bolen, p, 10);
    p += strlen(p);
    *p++ = '.';
    uint32_t kesir = mutlak % bolen;
    for (int8_t i = basamak - 1; i >= 0; i--) {
        p[i] = '0' + (kesir % 10);
        kesir /= 10;
    }
    p[basamak] = 0;
    return tampon;
}
