//
// Created by August Pemberton on 20/01/2024.
//

#pragma once
#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

using namespace ultralight;
namespace UltralightUtil {
    /// \brief Copies the raw pixels from the Ultralight rendered BitmapSurface to a JUCE Image
    /// \param pixels The raw pixels from the Ultralight BitmapSurface
    /// \param width The width of the image
    /// \param height The height of the image
    /// \param stride The stride of the image (approximately number of bytes per row, but there are some edge case - see below)
    static juce::Image CopyPixelsToTexture(
            void *pixels,
            uint32_t width,
            uint32_t height,
            uint32_t stride) {
        // Create a JUCE Image with the same dimensions as the Ultralight BitmapSurface
        juce::Image image(juce::Image::ARGB, static_cast<int>(width), static_cast<int>(height), false);
        // Create a BitmapData object to access the raw pixels of the JUCE Image
        juce::Image::BitmapData bitmapData(image, 0, 0, static_cast<int>(width), static_cast<int>(height),
            juce::Image::BitmapData::writeOnly);
        // Set the pixel format to ARGB (same as Ultralight)
        bitmapData.pixelFormat = juce::Image::ARGB;

        // Normal case: the stride is the same as the width * 4 (4 bytes per pixel)
        // In this case, we can just memcpy the whole image
        if (width * 4 == stride) {
            std::memcpy(bitmapData.data, pixels, stride * height);
        }
        // Special case: the stride is different from the width * 4
        // In this case, we need to copy the image line by line
        // The reason for this is that in some cases, the stride is not the same as the width * 4,
        // for example when the JUCE window width is uneven (e.g. 1001px)
        else {
            for (uint32_t y = 0; y < height; ++y)
                std::memcpy(bitmapData.getLinePointer(static_cast<int>(y)), static_cast<uint8_t*>(pixels) + y * stride, width * 4);
        }

        return image;
    }

    static int mapJUCEKeyCodeToUltralight(int juceKeyCode)
    {
        if (juceKeyCode == juce::KeyPress::backspaceKey) return ultralight::KeyCodes::GK_BACK;
        if (juceKeyCode == juce::KeyPress::tabKey) return ultralight::KeyCodes::GK_TAB;
        if (juceKeyCode == juce::KeyPress::returnKey) return ultralight::KeyCodes::GK_RETURN;
         if (juceKeyCode == juce::KeyPress::escapeKey) return ultralight::KeyCodes::GK_ESCAPE;
         if (juceKeyCode == juce::KeyPress::spaceKey) return ultralight::KeyCodes::GK_SPACE;
         if (juceKeyCode == juce::KeyPress::deleteKey) return ultralight::KeyCodes::GK_DELETE;
         if (juceKeyCode == juce::KeyPress::homeKey) return ultralight::KeyCodes::GK_HOME;
         if (juceKeyCode == juce::KeyPress::endKey) return ultralight::KeyCodes::GK_END;
         if (juceKeyCode == juce::KeyPress::pageUpKey) return ultralight::KeyCodes::GK_PRIOR;
         if (juceKeyCode == juce::KeyPress::pageDownKey) return ultralight::KeyCodes::GK_NEXT;
         if (juceKeyCode == juce::KeyPress::leftKey) return ultralight::KeyCodes::GK_LEFT;
         if (juceKeyCode == juce::KeyPress::rightKey) return ultralight::KeyCodes::GK_RIGHT;
         if (juceKeyCode == juce::KeyPress::upKey) return ultralight::KeyCodes::GK_UP;
         if (juceKeyCode == juce::KeyPress::downKey) return ultralight::KeyCodes::GK_DOWN;
         if (juceKeyCode == juce::KeyPress::F1Key) return ultralight::KeyCodes::GK_F1;
         if (juceKeyCode == juce::KeyPress::F2Key) return ultralight::KeyCodes::GK_F2;
         if (juceKeyCode == juce::KeyPress::F3Key) return ultralight::KeyCodes::GK_F3;
         if (juceKeyCode == juce::KeyPress::F4Key) return ultralight::KeyCodes::GK_F4;
         if (juceKeyCode == juce::KeyPress::F5Key) return ultralight::KeyCodes::GK_F5;
         if (juceKeyCode == juce::KeyPress::F6Key) return ultralight::KeyCodes::GK_F6;
         if (juceKeyCode == juce::KeyPress::F7Key) return ultralight::KeyCodes::GK_F7;
         if (juceKeyCode == juce::KeyPress::F8Key) return ultralight::KeyCodes::GK_F8;
         if (juceKeyCode == juce::KeyPress::F9Key) return ultralight::KeyCodes::GK_F9;
         if (juceKeyCode == juce::KeyPress::F10Key) return ultralight::KeyCodes::GK_F10;
         if (juceKeyCode == juce::KeyPress::F11Key) return ultralight::KeyCodes::GK_F11;
         if (juceKeyCode == juce::KeyPress::F12Key) return ultralight::KeyCodes::GK_F12;
            // Add more if- conditions for other JUCE keycodes as needed
         return -1;
        // Return -1 if there's no matching Ultralight keycode
    }

    static RefPtr<View> CreateView(const RefPtr<Renderer> &renderer, int width = 0, int height = 0,
                                   float dpiScale = 1) {
        ViewConfig view_config;
        view_config.is_accelerated = false; // use CPU
        view_config.is_transparent = false;

        auto view = renderer->CreateView(width, height, view_config, nullptr);

        view->set_device_scale(dpiScale);

        return view;
    }

    static juce::MouseCursor mapUltralightCursorToJUCE(ultralight::Cursor ultralightCursor) {
        switch (ultralightCursor) {
            case ultralight::kCursor_Hand:
                return juce::MouseCursor::PointingHandCursor;
            case ultralight::kCursor_Pointer:
                return juce::MouseCursor::NormalCursor;
            case ultralight::kCursor_IBeam:
                return juce::MouseCursor::IBeamCursor;
            case ultralight::kCursor_Cross:
                return juce::MouseCursor::CrosshairCursor;
            case ultralight::kCursor_Wait:
                return juce::MouseCursor::WaitCursor;
            case ultralight::kCursor_Help:
                return juce::MouseCursor::PointingHandCursor;
            case ultralight::kCursor_EastResize:
                return juce::MouseCursor::RightEdgeResizeCursor;
            case ultralight::kCursor_NorthResize:
                return juce::MouseCursor::TopEdgeResizeCursor;
            case ultralight::kCursor_NorthEastResize:
                return juce::MouseCursor::TopRightCornerResizeCursor;
            case ultralight::kCursor_NorthWestResize:
                return juce::MouseCursor::TopLeftCornerResizeCursor;
            case ultralight::kCursor_SouthResize:
                return juce::MouseCursor::BottomEdgeResizeCursor;
            case ultralight::kCursor_SouthEastResize:
                return juce::MouseCursor::BottomRightCornerResizeCursor;
            case ultralight::kCursor_SouthWestResize:
                return juce::MouseCursor::BottomLeftCornerResizeCursor;
            case ultralight::kCursor_WestResize:
                return juce::MouseCursor::LeftEdgeResizeCursor;
            case ultralight::kCursor_NorthSouthResize:
                return juce::MouseCursor::UpDownResizeCursor;
            case ultralight::kCursor_EastWestResize:
                return juce::MouseCursor::LeftRightResizeCursor;
            case ultralight::kCursor_NorthEastSouthWestResize:
                return juce::MouseCursor::UpDownLeftRightResizeCursor;
            case ultralight::kCursor_NorthWestSouthEastResize:
                return juce::MouseCursor::UpDownLeftRightResizeCursor;
            case ultralight::kCursor_ColumnResize:
                return juce::MouseCursor::UpDownResizeCursor;
            case ultralight::kCursor_RowResize:
                return juce::MouseCursor::LeftRightResizeCursor;
            case ultralight::kCursor_MiddlePanning:
                return juce::MouseCursor::DraggingHandCursor;
            case ultralight::kCursor_EastPanning:
                return juce::MouseCursor::DraggingHandCursor;
            case ultralight::kCursor_NorthPanning:
                return juce::MouseCursor::DraggingHandCursor;
            case ultralight::kCursor_NorthEastPanning:
                return juce::MouseCursor::DraggingHandCursor;
            case ultralight::kCursor_NorthWestPanning:
                return juce::MouseCursor::DraggingHandCursor;
            case ultralight::kCursor_SouthPanning:
                return juce::MouseCursor::DraggingHandCursor;
            case ultralight::kCursor_SouthEastPanning:
                return juce::MouseCursor::DraggingHandCursor;
            case ultralight::kCursor_SouthWestPanning:
                return juce::MouseCursor::DraggingHandCursor;
            case ultralight::kCursor_WestPanning:
                return juce::MouseCursor::DraggingHandCursor;
            default:
                return juce::MouseCursor::NormalCursor;
        }
    }

    static std::string toStdString(ultralight::String s) {
        return s.utf8().data();
    }

    static juce::String toJUCEString(ultralight::String s) {
        return s.utf8().data();
    }

    static std::string getStdString(const JSValue& v) {
        return toStdString(v.ToString());
    }

}
