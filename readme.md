# MOT-CycleControl v2.0

Firmware *spot welder* presisi berbasis siklus AC untuk Trafo MOT, ESP32, dan SSR Zero-Cross.

Proyek ini dirancang untuk memecahkan masalah umum kegagalan SSR Zero-Cross yang tidak menyala karena dipicu oleh logika *firmware* yang salah (pulsa pendek gaya TRIAC).

## ⚠️ PERINGATAN KESELAMATAN KRITIS ⚠️

1.  **TEGANGAN TINGGI (MAINS):** Proyek ini bekerja langsung dengan tegangan listrik 220V AC. Ini **MEMATIKAN**. Jangan pernah mengerjakan proyek ini saat terhubung ke listrik. Kesalahan perakitan dapat menyebabkan sengatan listrik, kebakaran, atau kematian. **Ini bukan proyek untuk pemula.**
2.  2.  **BAHAYA TRAFO MOT:** Trafo MOT menghasilkan rendah. Mereka tii (ratusan ampere) pada tegangan rendah. Mereka tidak dirancang untuk menyala terus-menerus dan akan **CEPAT PANAS (OVERHEAT)**.
    3.  3.  **RISIKO KEBAKARAN:** Menggunakan durasi las yang terlalu lama (misal Level 99 /ye2 dktik) dapat **MELELEHKAN** lilitan trafo, menyebabkan korsleting, dan **MENYEBABKAN KEBAKARAN.** Gunakan dengan sangat hati-hati.
      
        4.  **Anda bertanggung jawab penuh atas keselamatan Anda sendiri saat membangun atau menggunakan proyek ini.**
      
        5.  ---
      
        6.  ## Fitur Inti
      
        7.  * **Kontrol Berbasis Siklus AC:** Pengaturan presisi berbasis siklus 50Hz (1 Level = 1 Siklus = 20 milidetik).
            * * **Logika "Held HIGH":** Dirancang khusus untuk SSR Zero-Cross, menggunakan `delay()` yang disengaja untuk sinyal `HIGH` yang stabil dan handal.
              * * **Mode Single & Double Pulse:** Pilih antara satu lasan utama atau mode pre-weld + main-weld.
                * * **Antarmuka Ganda (Hybrid UI):** Kontrol penuh dari **UI Fisik** (OLED + Encoder) dan **Web UI** (via WiFi SoftAP).
                  * * **Sinkronisasi Penuh:** Pengaturan di OLED dan Web UI selalu sinkron.
                    * * **OTA Updates:** Perbarui firmware dengan mudah melalui browser di `/update`.
                      * * **Web Flasher:** Halaman *flasher* mandiri untuk instalasi awal tanpa perlu PlatformIO.
                        * * **Preset Material:** Rekomendasi pengaturan awal di dalam Web UI untuk membantu Anda memulai.
                         
                          * ## Logika Driver SSR & "Held HIGH"
                         
                          * * **Masalah Terpecahkan:** Banyak *firmware* gagal karena mengirim pulsa 1-2ms, yang diabaikan oleh SSR Zero-Cross.
                            * * **Solusi Kami:** Firmware ini mengirim sinyal `HIGH` yang **ditahan (held)** selama durasi siklus yang dipilih (misal 60ms). SSR Zero-Cross menerima sinyal ini dengan benar.
                              * * **Trade-off (Sengaja):** Kami menggunakan `delay()` yang *blocking*. Selama las (misal 60ms - 1980ms), UI fisik dan Web UI akan **freeze**. Ini adalah *trade-off* yang diterima untuk menjamin sinyal yang stabil dan kesederhanaan.
                                * * **Driver Hardware:** Sesuai spesifikasi, `GPIO26` digunakan untuk mengontrol jalur 5V eksternal ysh (via PlatformIO)
                                 
                                  * 1.  **CR.
                                   
                                    2.  ## Build & Flash (via PlatformIO)
                                   
                                    3.  1.  **Clone** repository ini.
                                        2.  2.  Buka proyek menggunakan **VS Code** dengan ekstensi **PlatformIO**.
                                            3.  3.  **Build:** Klik ikon centang (✓) PlatformIO di status bar atau jalankan `pio run`.
                                                4.  4.  **Upload:** Hubungkan ESP32, masuk ke mode "Bootloader" (tahan BOOT, tekan EN), lalu klik ikon panah (→) atau jalankan `pio run -t upload`.
                                                  
                                                    5.  ## Flasher Awal (Metode Paling Mudah via Web)
                                                  
                                                    6.  Jika Anda tidak punya PlatformIO, Anda bisa menggunakan Web Flasher untuk instalasi awal.
                                                  
                                                    7.  1.  Buka halaman **[LINK KE GITHUB PAGES ANDA DI SINI]** di browser berbasis Chrome/Edge.
                                                        2.  2.  Unduh file `firmware.bin` terbaru dari halaman **Releases** GitHub ini.
                                                            3.  3.  Tahan tombol "BOOT" di ESP32 Anda, lalu colokkan ke PC via USB.
                                                                4.  4.  Di halaman Web Flasher, klik "Connect", pilih port COM ESP32 Anda.
                                                                    5.  5.  Tambahkan file `firmware.bin` dan klik "Program".
                                                                      
                                                                        6.  ## Cara Penggunaan
                                                                      
                                                                        7.  1.  Nyalakan ESP32.
                                                                            2.  2.  Di HP atau PC Anda, cari jaringan WiFi bernama **`MOT-CycleControl`** (Password: **`motspotweld`**).
                                                                                3.  3.  Hubungkan ke jaringan tersebut.
                                                                                    4.  4.  Buka browser dan kunjungi alamat **`http://192.168.4.1`**.
                                                                                        5.  5.  Anda sekarang bisa mengontrol *spot welder* dari Web UI.
                                                                                            6.  6.  Anda juga bisa mengontrol langsung dari OLED dan Rotary Encoder.
                                                                                              
                                                                                                7.  ### Update Firmware (OTA)
                                                                                              
                                                                                                8.  Setelah firmware awal ter-flash, Anda tidak perlu kabel lagi.
                                                                                              
                                                                                                9.  1.  Hubungkan ke WiFi `MOT-CycleControl`.
                                                                                                    2.  2.  Buka browser dan kunjungi **`http://192.168.4.1/update`**.
                                                                                                        3.  3.  Upload file `firmware.bin` baru (yang didapat dari GitHub Actions).
                                                                                                          
                                                                                                            4.  ## Roadmap & Rencana Masa Depan
                                                                                                          
                                                                                                            5.  * Implementasi **Triple Pulse** (Preheat, Main Weld, Tailing).
                                                                                                                * * Refaktor ke **FSM (Finite State Machine)** non-blocking untuk UI yang tetap responsif saat mengelas.
                                                                                                                  * * **Start-Aligned ZC:** Menambahkan ZMPT101B untuk mendeteksi *zero cross* dan memulai `delay()` tepat di awal siklus (mengurangi jitter ±½ siklus).
                                                                                                                    * * **Penyimpanan NVS:** Menyimpan pengaturan terakhir ke flash internal ESP32.
