#pragma once
#define LGFX_USE_V1
#include <LovyanGFX.hpp>

class LGFX_SmartMosque : public lgfx::LGFX_Device {
    // Sesuaikan tipe panel (misal: Panel_RGB atau Panel_ILI9488)
    lgfx::Panel_ILI9488 _panel_instance; 
    lgfx::Bus_SPI       _bus_instance;   

public:
    LGFX_SmartMosque() {
        {
            auto cfg = _bus_instance.config();
            cfg.spi_host = SPI2_HOST;     
            cfg.pin_sclk = 12; // Ganti dengan pin SCLK Anda
            cfg.pin_mosi = 13; // Ganti dengan pin MOSI Anda
            cfg.pin_miso = 14; // Ganti dengan pin MISO Anda
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }
        {
            auto cfg = _panel_instance.config();
            cfg.panel_width  = 800; // Resolusi LCD 7"
            cfg.panel_height = 480;
            _panel_instance.config(cfg);
        }
        setPanel(&_panel_instance);
    }
};