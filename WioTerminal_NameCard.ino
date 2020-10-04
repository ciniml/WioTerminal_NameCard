#include <LovyanGFX.hpp>

static LGFX lcd;

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

void setup()
{
    lcd.init();
    lcd.clear(0xffff);
    pinMode(RTL8720D_CHIP_PU, OUTPUT);
    digitalWrite(RTL8720D_CHIP_PU, 0);
}

static std::uint16_t animation_counter = 0;

void loop()
{
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
}