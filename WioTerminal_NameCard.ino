#include <Wire.h>
#include <LovyanGFX.hpp>

static LGFX lcd;

#include "fuga.h"

static constexpr std::uint16_t color888(uint8_t r, uint8_t g, uint8_t b) {
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

static constexpr const std::uint16_t FaceColor = color888(0, 150, 200);

static constexpr const std::uint16_t ScreenWidth = 320;
static constexpr const std::uint16_t ScreenHeight = 240;

static constexpr const std::uint16_t EyeOffsetX = 60;
static constexpr const std::uint16_t EyeOffsetY = 70;
static constexpr const std::uint16_t EyeRadius = 25;
static constexpr const std::uint16_t MouthRadius = 35;
static constexpr const std::uint16_t MouthOffset = 70;

static void showTwitterIcon()
{
    lcd.fillScreen(0xffff);
    lcd.drawPng(fuga_png, fuga_png_len, 40, 20);
    lcd.setTextColor(0);
    lcd.setBaseColor(0xffff);
    lcd.setTextDatum(textdatum_t::top_center);
    lcd.setFont(&fonts::FreeMonoBold24pt7b);
    lcd.drawString("Kenta IDA", 160, 0);
    lcd.setFont(&fonts::FreeMonoBold12pt7b);
    lcd.drawString("Twitter: @ciniml", 160, 40);
}

struct WioTerminalChan
{
    LGFX_Sprite eye_sprite_0;
    LGFX_Sprite eye_sprite_1;
    LGFX_Sprite eye_sprite_2;
    LGFX_Sprite eye_sprite_3;
    LGFX_Sprite mouth_sprite;
    LGFX& lcd;

    WioTerminalChan(LGFX& lcd)
        : eye_sprite_0(&lcd)
        , eye_sprite_1(&lcd)
        , eye_sprite_2(&lcd)
        , eye_sprite_3(&lcd)
        , mouth_sprite(&lcd)
        , lcd(lcd)
    {
        this->eye_sprite_0.createSprite(EyeRadius*2 + 1, EyeRadius*2 + 1);
        this->eye_sprite_1.createSprite(EyeRadius*2 + 1, EyeRadius*2 + 1);
        this->eye_sprite_2.createSprite(EyeRadius*2 + 1, EyeRadius*2 + 1);
        this->eye_sprite_3.createSprite(EyeRadius*2 + 1, EyeRadius*2 + 1);
        this->mouth_sprite.createSprite(MouthRadius*2 + 1, MouthRadius*2 + 1);

        this->updateEyeSprite(FaceColor, 0xffff);
        this->updateMouthSprite(FaceColor, 0xffff);
    }

    void updateEyeSprite(std::uint16_t eye_color, std::uint16_t background_color)
    {
        this->eye_sprite_0.clear(background_color);
        this->eye_sprite_0.setBaseColor(eye_color);
        this->eye_sprite_0.setColor(eye_color);
        this->eye_sprite_0.fillCircle(EyeRadius, EyeRadius, EyeRadius);

        this->eye_sprite_1.clear(background_color);
        this->eye_sprite_1.setBaseColor(eye_color);
        this->eye_sprite_1.setColor(eye_color);
        this->eye_sprite_1.fillArc(EyeRadius, EyeRadius, EyeRadius, 0, 0, 180);
        
        this->eye_sprite_2.clear(background_color);
        this->eye_sprite_2.setBaseColor(eye_color);
        this->eye_sprite_2.setColor(eye_color);
        this->eye_sprite_2.fillArc(EyeRadius, EyeRadius, EyeRadius, 0, 0, 180);
        this->eye_sprite_2.fillArc(EyeRadius, EyeRadius*3/4, EyeRadius, 0, 0, 180, background_color);

        this->eye_sprite_3.clear(background_color);
        this->eye_sprite_3.setBaseColor(eye_color);
        this->eye_sprite_3.setColor(eye_color);
        this->eye_sprite_3.drawArc(EyeRadius, EyeRadius, EyeRadius, EyeRadius-1, 0, 180);
    }

    void updateMouthSprite(std::uint16_t mouth_color, std::uint16_t background_color)
    {
        this->mouth_sprite.clear(background_color);
        this->mouth_sprite.fillCircle(MouthRadius, MouthRadius, MouthRadius, FaceColor);
        this->mouth_sprite.fillRect(0, 0, MouthRadius*2 + 1, MouthRadius, FaceColor);
    }
    
    void drawEye(std::uint16_t offset_x, std::uint16_t offset_y, int eye_state)
    {
        switch(eye_state) {
            case 0: this->eye_sprite_0.pushSprite(offset_x - EyeRadius, offset_y - EyeRadius); break;
            case 1: this->eye_sprite_1.pushSprite(offset_x - EyeRadius, offset_y - EyeRadius); break;
            case 2: this->eye_sprite_2.pushSprite(offset_x - EyeRadius, offset_y - EyeRadius); break;
            case 3: this->eye_sprite_3.pushSprite(offset_x - EyeRadius, offset_y - EyeRadius); break;
        }
    }
    void drawMouth(std::uint16_t offset_x, std::uint16_t offset_y)
    {
        this->mouth_sprite.pushSprite(offset_x - MouthRadius, offset_y - MouthRadius); 
    }
    
    void drawFace(int left_eye_state, int right_eye_state)
    {
        this->drawEye(EyeOffsetX, EyeOffsetY, left_eye_state);
        this->drawEye(ScreenWidth - EyeOffsetX, EyeOffsetY, left_eye_state);
        this->drawMouth(ScreenWidth / 2, ScreenHeight - MouthOffset);
    }
};

static WioTerminalChan wio_terminal_chan(lcd);


static constexpr std::uint8_t INA219Address = 0b1000101;
static constexpr double INA219MaxCurrent = 2.0;
static constexpr double INA219ShuntResistance = 4.0e-2;

class INA219
{
private:
    static constexpr std::uint8_t RegConfiguration  = 0x00;
    static constexpr std::uint8_t RegShuntVoltage   = 0x01;
    static constexpr std::uint8_t RegBusVoltage     = 0x02;
    static constexpr std::uint8_t RegPower          = 0x03;
    static constexpr std::uint8_t RegCurrent        = 0x04;
    static constexpr std::uint8_t RegCalibration    = 0x05;

    TwoWire& wire;
    std::uint8_t address;

    std::uint8_t writeRegister(std::uint8_t reg_address, std::uint16_t value) 
    {
        std::uint8_t buffer[3] = {
            reg_address,
            value >> 8,
            value & 0xff,
        };
        this->wire.beginTransmission(this->address);
        this->wire.write(buffer, sizeof(buffer));
        this->wire.endTransmission(true);
    }
    std::uint8_t readRegister(std::uint8_t reg_address, std::uint16_t& value) 
    {
        this->wire.beginTransmission(this->address);
        this->wire.write(reg_address);
        this->wire.endTransmission(true);

        auto result = this->wire.requestFrom(this->address, 2);
        if( result != 2 ) {
            return result;
        }
        
        std::uint8_t buffer[2];
        this->wire.readBytes(buffer, 2);
        value = (buffer[0] << 8) | buffer[1];
        return 0;
    }

public:
    static constexpr double calculateCurrentLSB(double maxCurrent)
    {
        return maxCurrent/32768.0;
    }
    static constexpr std::uint16_t calculateCalibration(double maxCurrent, double shuntResistance)
    {
        return static_cast<std::uint16_t>(0.04096/(calculateCurrentLSB(maxCurrent)*shuntResistance));
    }

    INA219(TwoWire& wire, std::uint8_t address) : wire(wire), address(address) {}

    void reset()
    {
        this->writeRegister(RegConfiguration, 0x8000);    // Reset
    }

    void setCalibration(std::uint16_t value)
    {
        this->writeRegister(RegCalibration, value);
    }
    void setCalibration(double maxCurrent, double shuntResistance)
    {
        this->setCalibration(calculateCalibration(maxCurrent, shuntResistance));
    }

    std::int32_t readShuntVoltage()
    {
        std::uint16_t value = 0;
        this->readRegister(RegShuntVoltage, value);
        return static_cast<std::int16_t>(value) * 10;
    }

    std::uint32_t readBusVoltage(bool waitConversionReady=true)
    {
        std::uint16_t value = 0;
        while(waitConversionReady && (value & 2) == 0) {
            this->readRegister(RegBusVoltage, value);
        }
        value >>= 3;
        return value * 4;
    }

    std::int32_t readCurrent(double maxCurrent)
    {
        std::uint16_t value = 0;
        this->readRegister(RegCurrent, value);
        return static_cast<std::int16_t>(value) * static_cast<std::uint32_t>(calculateCurrentLSB(maxCurrent)*1000000);
    }

    std::int32_t readPower(double maxCurrent)
    {
        std::uint16_t value = 0;
        this->readRegister(RegPower, value);
        return static_cast<std::int16_t>(value) * static_cast<std::uint32_t>(calculateCurrentLSB(maxCurrent)*1000*20);
    }
};

static INA219 ina219(Wire1, INA219Address);

static void showBatteryStatus()
{
    auto voltage = 0;
    char buf[80];
    snprintf(buf, 80, "%0.2f[V]", voltage);
    lcd.setTextColor(0x07e0);
    lcd.setColor(0);
    lcd.setTextDatum(textdatum_t::top_right);
    lcd.drawString(buf, 320, 0, 4);
}

void setup()
{
    lcd.init();
    lcd.clear(0xffff);
    pinMode(SWITCH_Z, INPUT_PULLUP);
    pinMode(RTL8720D_CHIP_PU, OUTPUT);
    digitalWrite(RTL8720D_CHIP_PU, 0);

    Wire1.begin();
    Wire1.setClock(100000); // 100kHz

    ina219.reset();
    ina219.setCalibration(INA219::calculateCalibration(INA219MaxCurrent, INA219ShuntResistance));
}

enum class State
{
    Face,
    Twitter,
    Battery,    
};

static State state = State::Face;
static State prev_state = State::Face;
static std::uint16_t animation_counter = 0;
static std::uint16_t button_filter = 0;

void loop()
{
    bool is_button_pushed = button_filter == 0 && digitalRead(SWITCH_Z) == 0;
    bool state_changed = state != prev_state;
    prev_state = state;

    if( is_button_pushed ) {
        button_filter = 10;
    }
    else if(button_filter > 0) {
        button_filter--;
    }

    switch(state)
    {
        case State::Face: {
            if( is_button_pushed ) {
                state = State::Twitter;
                break;
            }
            if( state_changed ) {
                lcd.clear(0xffff);
                animation_counter = 0;
            }
            if( animation_counter < 60 ) {
                wio_terminal_chan.drawFace(0, 0);
                animation_counter++;
            }
            else if( animation_counter <= 62 ) {
                auto eye = animation_counter - 60 + 1;
                wio_terminal_chan.drawFace(eye, eye);
                animation_counter++;
            }
            else if( animation_counter <= 64 ) {
                auto eye = 2 - (animation_counter - 63);
                wio_terminal_chan.drawFace(eye, eye);
                animation_counter++;
            }
            else {
                wio_terminal_chan.drawFace(0, 0);
                animation_counter = 0;
            }
            delay(50);
            break;
        }
        case State::Twitter: {
            if( is_button_pushed ) {
                state = State::Battery;
                break;
            }
            if( state_changed ) {
                showTwitterIcon();
            }
            delay(50);
            break;
        }
        case State::Battery: {
            if( is_button_pushed ) {
                state = State::Face;
                break;
            }
            if( state_changed ) {
                animation_counter = 0;
            }
            if( animation_counter == 0 ) {
                auto busVoltage = ina219.readBusVoltage();
                auto shuntVoltage = ina219.readShuntVoltage();
                auto current = ina219.readCurrent(INA219MaxCurrent);
                auto power = ina219.readPower(INA219MaxCurrent);

                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.setTextDatum(textdatum_t::top_left);
                lcd.setTextColor(0xffff);
                lcd.setBaseColor(0x0000);
                lcd.setFont(&fonts::FreeMono9pt7b);
                lcd.printf("Bus Voltage  : %d [mV]\n", busVoltage);
                lcd.printf("Shunt Voltage: %d [uV]\n", shuntVoltage);
                lcd.printf("Current      : %d [uA]\n", current);
                lcd.printf("Power        : %d [mW]\n", power);
            }
            animation_counter = animation_counter < 10 ? animation_counter + 1 : 0;
            delay(100);
            break;
        }
    }
}