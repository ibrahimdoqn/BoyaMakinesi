# 2K Akrilik Boya Karışım Makinesi

Arduino Mega 2560 + PlatformIO. Hava hattındaki basınç düşüşünden tabanca tetiğini algılar,
iki peristaltik step motor pompayla boya ve sertleştiriciyi ayarlanan oranda karıştırarak basar.
Bittiğinde temizlik sıvısıyla hortumu yıkar. Tüm ayarlar menüden yapılır ve EEPROM'da kalıcı saklanır.
Vanalar manueldir (elle çevrilir); yazılım röle kullanmaz.

## Donanım

- Motor sürücü: **TB6600**, 16 mikro adım → **3200 darbe/tur** (menü: Pompa Ayarları → Adım/Tur)
- Peristaltik hortum: **17#** (İç Çap 6.4 mm, Dış Çap 9.6 mm) → yaklaşık **2.8 ml/tur**,
  varsayılan kalibrasyon 3200 / 2.8 ≈ **1142.9 adım/ml**. Kesin değer için kalibrasyon yapın.
- Motor hızı **dev/dk** olarak ayarlanır (5–180). 60 dev/dk ≈ 168 ml/dk (tek pompa).
- Motor yönü: `PinConfig.h` içindeki `MOTOR1_YON_TERS` / `MOTOR2_YON_TERS` donanımın temel yönüdür;
  menüdeki "Normal/Ters" bunun üzerine uygulanır.

## Proje yapısı

```
src/
  PinConfig.h     Pin tanımları ve sabitler
  main.cpp        setup()/loop() – orkestratör
  Ayarlar.*       Ayar yapısı + EEPROM (imza, sürüm, CRC)
  Sistem.*        Mod durum makinesi: BEKLEME / BOYA / TEMİZLİK / SERVİS
  Ekranlar.*      Ana ekran, temizlik, doldurma, kalibrasyon, sensör sıfırlama
  MenuTanim.cpp   Menü ağacı (yeni ayar eklemek için tek yer)
lib/
  BasincSensoru/  Basınç okuma, EMA filtre, sıfır kalibrasyonu, kopuk kablo algılama
  Tetik/          Basınç düşüşünden tetik algılama (fark / mutlak mod, histerezis)
  PompaSurucu/    Timer1 kesmeli 2 step motor sürücü, oran korumalı rampa, adım sayacı
  Temizlik/       Temizlik döngüsü
  Butonlar/       5 tuş, debounce, basılı tutma hızlanması
  Gosterge/       16x2 I2C LCD tamponu, Türkçe karakter desteği
  Menu/           Genel amaçlı modüler menü motoru
```

Kütüphaneler birbirinden ve `PinConfig.h`'den bağımsızdır; pinler ve ayarlar `begin()` / `ayarla()` ile verilir.

## Tuşlar

**Ana ekran**

| Tuş    | İşlev |
|--------|-------|
| YUKARI | Boya modunu başlat / durdur |
| AŞAĞI  | Temizlik ekranı |
| SOL/SAĞ| Bilgi sayfaları: durum, tetik referansı/eşiği, akış, oturum tüketimi, pot ömrü |
| OK     | Menü (pot ömrü uyarısı varsa önce uyarıyı susturur) |

**Menüde:** YUKARI/AŞAĞI gezin, OK/SAĞ gir-düzenle, SOL geri.
**Değer düzenlerken** değer `[köşeli parantez]` içinde görünür: YUKARI/AŞAĞI değiştir
(basılı tutunca x10, x100 hızlanır), OK kaydet, SOL iptal.
Kaydedilen ayar hemen uygulanır ve 2 sn sonra EEPROM'a yazılır.

## Çalışma

1. **Boya modu:** Boya ve sertleştirici vanalarını elle açın.
   Basınç referansın *Düşüş Farkı* kadar altına düşerse (ör. 50 → 45 psi) tetik çekilmiş sayılır,
   pompalar oranla çalışır. Basınç geri yükselince durur.
2. **Karışım oranı** hacimseldir. Her pompanın kalibrasyonu (adım/ml) hesaba katılır;
   en çok basan pompa *Motor Hızı*'nda döner, diğeri orana göre.
3. **Temizlik:** Vanaları elle temizlik sıvısına çevirin, OK ile başlatın. Seçilen pompa
   (varsayılan Pompa 2) herhangi bir tuşa basılana veya *Maks Süre* dolana kadar basar.

## Menü haritası

- **Boya Modu** – başlat/durdur
- **Temizlik** – temizlik ekranı
- **Karışım Oranı** – Boya : Sert (0–10 : 0–10, ör. 4:1, 2:1, 1:1, 1:0)
- **Motor Hızı** – 5–180 dev/dk
- **Tetik Ayarları** – Canlı basınç, Mod (Fark/Mutlak), Düşüş Farkı, Mutlak Eşik, Histerezis,
  Çekme/Bırakma gecikmesi, Min basınç, Maks püskürtme süresi
- **Pompa Ayarları** – Akış (hesap), Rampa, Boya/Sert./Karışım doldur, P1/P2 kalibrasyon,
  adım/ml değerleri, kalibrasyon adımı, adım/tur, motor yönleri, sürücü EN mantığı
- **Temizlik Ayar.** – Hız (dev/dk), pompa seçimi, maks süre, darbeli mod, pot ömrü uyarısı
- **Sensör Ayarı** – Canlı okuma, sıfır kalibrasyonu, ofset, sensör min/max V, aralık, ADC referansı, filtre
- **Sistem** – Açılış modu, LCD ışığı, İstatistik (toplam boya/sert./temizlik ml, püskürtme sayısı,
  çalışma süresi, sıfırla), Şimdi kaydet, Fabrika ayarı, Yazılım sürümü

## İlk kurulum

1. **Boya Doldur / Sert. Doldur** ile hortumları doldurun (OK basılı tut, SAĞ sürekli çalıştır).
   Pompa ters dönüyorsa *P1/P2 Yönü*.
2. **P1 / P2 Kalibrasyon**: çıkışa ölçü kabı koyun, OK → pompa *Kalib. Adım* kadar basar
   (varsayılan 32000 adım = 10 tur ≈ 28 ml), ölçülen ml'yi girin. Oranın doğru olması için
   iki pompa da kalibre edilmelidir. *Adım/Tur* değiştirilirse kalibrasyonu tekrarlayın.
3. Hava hattı boşken **Sıfır Kalibre**.
4. **Tetik Ayarları → Canlı Basınç** ekranında tabancayı tetikleyerek düşüşü gözleyin,
   *Düşüş Farkı*'nı buna göre ayarlayın.

## Güvenlik özellikleri

- Sensör kablosu kopuk/kısa devre → pompalar durur, "SENSÖR HATASI"
- Hat basıncı *Min Basınç* altındaysa (kompresör kapalı) tetik algılanmaz
- *Maks Püskürtme* süresi aşılırsa (takılı tetik, kaçak) pompalar durur; tetik bırakılınca devam
- *Pot ömrü*: son püskürtmeden bu yana süre dolarsa ekran yanıp söner, temizlik ister
- Açılışta motor sürücüleri pasif başlar

## Notlar

- `PompaSurucu` Timer1'i kullanır (Servo kütüphanesiyle birlikte kullanılamaz). Step darbeleri
  kesmeyle üretildiği için LCD/menü işlemleri motorları etkilemez. Bu nedenle AccelStepper kaldırıldı.
- LCD'de Türkçe karakterler (ç ğ ı ş ö ü Ç Ş İ Ü) özel karakterlerle gösterilir; Ğ ve Ö, G ve O görünür.
- Ayar yapısını değiştirirseniz `Ayarlar.h` içindeki `AYAR_SURUM`'u artırın; cihaz varsayılanlara döner.
