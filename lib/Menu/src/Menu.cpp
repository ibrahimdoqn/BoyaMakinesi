#include "Menu.h"

void menuEkle(char* t, uint8_t boyut, FStr metin) {
    if (metin) strlcat_P(t, (const char*)metin, boyut);
}

void menuEkle(char* t, uint8_t boyut, const char* metin) {
    if (metin) strlcat(t, metin, boyut);
}

uint8_t menuCarpan(uint8_t tekrar) {
    if (tekrar > 40) return 100;
    if (tekrar > 15) return 10;
    return 1;
}

void MenuOge::deger(char* t, uint8_t boyut, bool duz) const {
    (void)boyut;
    (void)duz;
    t[0] = 0;
}

// ---------------------------------------------------------------
// MenuSistemi
// ---------------------------------------------------------------
void MenuSistemi::begin(Gosterge& gosterge, Menu& kok) {
    _g = &gosterge;
    _yigin[0] = &kok;
    _secim[0] = 0;
    _derinlik = 1;
    _acik = false;
    _duzenle = false;
}

void MenuSistemi::ac() {
    _derinlik = 1;
    _secim[0] = 0;
    _duzenle = false;
    _acik = true;
}

void MenuSistemi::kapat() {
    if (_duzenle) {
        MenuOge* o = _secili();
        if (o) o->iptal();
    }
    _duzenle = false;
    _acik = false;
}

MenuOge* MenuSistemi::_secili() const {
    if (_derinlik == 0) return nullptr;
    Menu* m = _yigin[_derinlik - 1];
    if (m->sayi == 0) return nullptr;
    return m->ogeler[_secim[_derinlik - 1]];
}

void MenuSistemi::altMenuAc(Menu* menu) {
    if (!menu || _derinlik >= MENU_MAX_DERINLIK) return;
    _yigin[_derinlik] = menu;
    _secim[_derinlik] = 0;
    _derinlik++;
}

void MenuSistemi::geri() {
    if (_derinlik > 1) _derinlik--;
    else kapat();
}

void MenuSistemi::tus(const TusBilgi& t) {
    if (!_acik || t.olay == OLAY_YOK) return;
    MenuOge* o = _secili();
    bool basildi = t.olay == OLAY_BASILDI;
    bool tekrarli = basildi || t.olay == OLAY_TEKRAR;

    if (_duzenle && o) {
        switch (t.tus) {
            case TUS_YUKARI: if (tekrarli) o->degistir(+1, t.tekrar); break;
            case TUS_ASAGI:  if (tekrarli) o->degistir(-1, t.tekrar); break;
            case TUS_OK:
            case TUS_SAG:
                if (basildi && o->onayla()) {
                    _duzenle = false;
                    if (_degisim) _degisim(o);
                }
                break;
            case TUS_SOL:
                if (basildi) {
                    o->iptal();
                    _duzenle = false;
                }
                break;
        }
        return;
    }

    Menu* m = _yigin[_derinlik - 1];
    uint8_t& s = _secim[_derinlik - 1];
    switch (t.tus) {
        case TUS_YUKARI:
            if (tekrarli && m->sayi) s = (s == 0) ? m->sayi - 1 : s - 1;
            break;
        case TUS_ASAGI:
            if (tekrarli && m->sayi) s = (s + 1 >= m->sayi) ? 0 : s + 1;
            break;
        case TUS_OK:
        case TUS_SAG:
            if (basildi && o) {
                if (o->duzenlenebilir()) {
                    o->duzenlemeBasla();
                    _duzenle = true;
                } else {
                    o->sec(*this);
                }
            }
            break;
        case TUS_SOL:
            if (basildi) geri();
            break;
    }
}

void MenuSistemi::ciz() {
    if (!_acik || !_g) return;
    Menu* m = _yigin[_derinlik - 1];
    MenuOge* o = _secili();
    _g->temizle();
    if (!o) {
        _g->satir(0, m->baslik);
        _g->satir(1, F("(bos)"));
        return;
    }

    // Satır 1: öğe adı + sıra numarası
    uint8_t yazilan = _g->yaz(0, 0, o->etiket());
    char sira[8];
    uint8_t no = _secim[_derinlik - 1] + 1;
    itoa(no, sira, 10);
    strcat(sira, "/");
    itoa(m->sayi, sira + strlen(sira), 10);
    uint8_t u = strlen(sira);
    if (yazilan + u + 1 <= GOSTERGE_SUTUN) _g->sagaYaz(0, sira);

    // Satır 2: değer
    char t[MENU_TAMPON];
    o->deger(t, sizeof(t), _duzenle);
    _g->yaz(0, 1, t);
}

// ---------------------------------------------------------------
// SecimOge
// ---------------------------------------------------------------
SecimOge::SecimOge(FStr etiket, uint8_t* deger, FStr secenekler)
    : MenuOge(etiket), _p(deger), _sec(secenekler), _sayi(1) {
    const char* p = (const char*)secenekler;
    char c;
    while ((c = pgm_read_byte(p++)) != 0)
        if (c == '|') _sayi++;
}

void SecimOge::_secenek(uint8_t i, char* t, uint8_t boyut) const {
    const char* p = (const char*)_sec;
    uint8_t n = 0;
    char c;
    // i. seçeneğin başına git
    while (n < i && (c = pgm_read_byte(p)) != 0) {
        if (c == '|') n++;
        p++;
    }
    uint8_t u = strlen(t);
    while ((c = pgm_read_byte(p++)) != 0 && c != '|' && u + 1 < boyut) t[u++] = c;
    t[u] = 0;
}

void SecimOge::deger(char* t, uint8_t boyut, bool duz) const {
    uint8_t i = duz ? _gecici : *_p;
    if (i >= _sayi) i = 0;
    t[0] = 0;
    menuEkle(t, boyut, duz ? "[" : " ");
    _secenek(i, t, boyut);
    if (duz) menuEkle(t, boyut, "]");
}

void SecimOge::degistir(int8_t yon, uint8_t tekrar) {
    (void)tekrar;
    if (yon > 0) _gecici = (_gecici + 1 >= _sayi) ? 0 : _gecici + 1;
    else _gecici = (_gecici == 0) ? _sayi - 1 : _gecici - 1;
}

// ---------------------------------------------------------------
// OranOge
// ---------------------------------------------------------------
OranOge::OranOge(FStr etiket, uint8_t* a, uint8_t* b, uint8_t min, uint8_t max)
    : MenuOge(etiket), _a(a), _b(b), _min(min), _max(max) {}

void OranOge::deger(char* t, uint8_t boyut, bool duz) const {
    uint8_t a = duz ? _ga : *_a;
    uint8_t b = duz ? _gb : *_b;
    char s[5];
    t[0] = 0;
    menuEkle(t, boyut, (duz && _alan == 0) ? "[" : " ");
    menuEkle(t, boyut, itoa(a, s, 10));
    menuEkle(t, boyut, (duz && _alan == 0) ? "]:" : " :");
    menuEkle(t, boyut, (duz && _alan == 1) ? "[" : " ");
    menuEkle(t, boyut, itoa(b, s, 10));
    menuEkle(t, boyut, (duz && _alan == 1) ? "]" : " ");
    menuEkle(t, boyut, F(" Boya:Sert"));
}

void OranOge::duzenlemeBasla() {
    _ga = *_a;
    _gb = *_b;
    _alan = 0;
}

void OranOge::degistir(int8_t yon, uint8_t tekrar) {
    (void)tekrar;
    uint8_t& v = _alan == 0 ? _ga : _gb;
    if (yon > 0 && v < _max) v++;
    if (yon < 0 && v > _min) v--;
}

bool OranOge::onayla() {
    if (_alan == 0) {
        _alan = 1;     // ikinci sayıya geç
        return false;
    }
    if (_ga == 0 && _gb == 0) _ga = 1;   // 0:0 geçersiz
    *_a = _ga;
    *_b = _gb;
    return true;
}

// ---------------------------------------------------------------
void AltMenuOge::deger(char* t, uint8_t boyut, bool duz) const {
    (void)duz;
    t[0] = 0;
    menuEkle(t, boyut, F("        Gir " OK_SAG));
}

void AksiyonOge::deger(char* t, uint8_t boyut, bool duz) const {
    (void)duz;
    t[0] = 0;
    if (_yazi) _yazi(t, boyut);
    else if (_aciklama) menuEkle(t, boyut, _aciklama);
    else menuEkle(t, boyut, F(" OK: Çalıştır"));
}

void GeriOge::deger(char* t, uint8_t boyut, bool duz) const {
    (void)duz;
    t[0] = 0;
    menuEkle(t, boyut, F(" " OK_SOL " Üst menü"));
}
