#ifndef DISPLAYOBJECT_H
#define DISPLAYOBJECT_H

#include <Adafruit_GFX.h>

class DisplayObject
{
protected:
    int16_t x, y;
    int16_t width, height;
    GFXcanvas16 *canvas = nullptr;
    bool isDirty = false;
    long lastDrawTime = 0;

private:
    bool isFirstDraw = true;

public:
    DisplayObject(int16_t x, int16_t y, int16_t width, int16_t height)
        : x(x), y(y), width(width), height(height)
    {
        canvas = new GFXcanvas16(width, height);
    }

    virtual ~DisplayObject()
    {
        if (canvas != nullptr)
        {
            delete canvas;
            canvas = nullptr;
        }
    }

    virtual bool shouldDraw() { return millis() - lastDrawTime > 500; }
    virtual void draw() = 0;

    void tick()
    {

        isDirty |= shouldDraw();
        isDirty |= isFirstDraw;

        if (!isDirty)
        {
            return;
        }
        draw();

        isFirstDraw = false;
        isDirty = false;
    }

    // Getters
    int16_t getX() const { return x; }
    int16_t getY() const { return y; }
    int16_t getWidth() const { return width; }
    int16_t getHeight() const { return height; }
    GFXcanvas16 *getCanvas() const { return canvas; }
};

#endif