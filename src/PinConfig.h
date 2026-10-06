#ifndef PINCONFIG_H
#define PINCONFIG_H

// =============================================
// 2K Boya Sistemi - Pin Konfigürasyonu
// Arduino Mega 2560
// =============================================

// Not: Vanalar manuel (elle) kumanda edilir, röle kullanılmaz.
// Eski röle pinleri (44-47) boştadır.

// --- Step Motor 1 (Boya Pompası) ---
#define MOTOR1_STEP_PIN 23
#define MOTOR1_DIR_PIN  24
#define MOTOR1_EN_PIN   25

// --- Motor sürücü: TB6600, 16 mikro adım ---
#define MOTOR_ADIM_TUR      3200    // varsayılan darbe/tur (menüden değişir)
// Donanımın temel yönü. Menüdeki "Normal" yön bu yönü kullanır.
// (1 = DIR pini HIGH iken ileri)
#define MOTOR1_YON_TERS     1
#define MOTOR2_YON_TERS     1

// --- Step Motor 2 (Sertleştirici Pompası) ---
#define MOTOR2_STEP_PIN 50
#define MOTOR2_DIR_PIN  49
#define MOTOR2_EN_PIN   48

// --- Basınç Sensörü (0-200 PSI) ---
#define BASINC_SENSOR_PIN A0

// --- Buton Pinleri (Pull-down, aktif HIGH) ---
#define BUTON_YUKARI_PIN 6
#define BUTON_ASAGI_PIN  9
#define BUTON_SOL_PIN    5
#define BUTON_SAG_PIN    10
#define BUTON_ORTA_PIN   7

// --- I2C LCD (16x2) ---
#define LCD_ADRES 0x27
#define LCD_SUTUN 16
#define LCD_SATIR 2

// --- Sistem Sabitleri ---
#define MOTOR_MAX_RPM       180     // 180 dev/dk x 3200 = 9600 adım/sn (sürücü sınırı 10000)
#define MOTOR_MIN_RPM       5
#define MOTOR_RPM_ADIM      5       // menüde artış miktarı

// --- Peristaltik hortum: 17# (İç Çap 6.4 mm, Dış Çap 9.6 mm) ---
// 17# hortum standart pompa kafasında yaklaşık 2.8 ml/tur basar.
// 3200 adım / 2.8 ml = ~1143 adım/ml. Kesin değer için kalibrasyon yapın.
#define HORTUM_ML_TUR_X10   28

#define MAX_KARISIM_ORANI   10
#define MIN_KARISIM_ORANI   0
#define BASINC_MAX_PSI      200.0f
#define ADC_MAX             1023.0f

// --- Sensör varsayılanları (0.5V - 4.5V çıkışlı sensör) ---
#define SENSOR_VMIN_MV      500
#define SENSOR_VMAX_MV      4500
#define ADC_REF_MV          5000

#define YAZILIM_SURUMU      "v1.0"

#endif
