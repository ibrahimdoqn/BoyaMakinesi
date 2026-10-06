#ifndef PINCONFIG_H
#define PINCONFIG_H

// =============================================
// 2K Boya Sistemi - Pin Konfigürasyonu
// Arduino Mega 2560
// =============================================

// --- Röle Pinleri (Selenoid Vanalar) ---
#define ROLE1_PIN 47    // Boya vanası
#define ROLE2_PIN 46    // Sertleştirici vanası
#define ROLE3_PIN 45    // Temizlik sıvısı vanası
#define ROLE4_PIN 44    // Yedek röle (görevi menüden seçilir)

// Vanalar kütüphanesindeki röle sıraları
#define VANA_BOYA    0
#define VANA_SERT    1
#define VANA_TEMIZ   2
#define VANA_YEDEK   3

// --- Step Motor 1 (Boya Pompası) ---
#define MOTOR1_STEP_PIN 23
#define MOTOR1_DIR_PIN  24
#define MOTOR1_EN_PIN   25

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
#define MOTOR_MAX_HIZ       3200    // adım/sn
#define MOTOR_MIN_HIZ       200     // adım/sn
#define MOTOR_HIZ_ADIM      100     // menüde artış miktarı
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
