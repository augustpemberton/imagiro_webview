//
// Created by August Pemberton on 11/05/2023.
//

#pragma once

#include <codecvt>
#include <regex>
#include <Ultralight/Ultralight.h>

#include "UltralightUtil.h"
#include "InspectorWindow.h"
#include "WebProcessor.h"

using namespace ultralight;

namespace imagiro {
    class UltralightViewComponent :
            public juce::Component,
            juce::Timer,
            juce::KeyListener,
            ViewListener {
    public:
        UltralightViewComponent(const RefPtr<View> &v, const RefPtr<Renderer> &r,
                                bool inspector = false)
                : renderer(r),
                  view(v),
                  allowInspector(inspector)
        {

            view->Focus();
            view->set_view_listener(this);

            // Listen to keyboard presses
            addKeyListener(this);
            // Allows this window to receive keyboard presses
            setWantsKeyboardFocus(true);

            setOpaque(true);
            startTimerHz(120);
        }

        void loadURL(const juce::String& url) {
            view->LoadURL(ultralight::String(url.toRawUTF8()));
        }

        void loadHTML(const juce::String& html) {
            view->LoadURL(ultralight::String(html.toRawUTF8()));
        }

    private:
        void timerCallback() override {
            repaint();
        }

        RefPtr<View> OnCreateInspectorView(View *caller, bool is_local, const String &inspected_url) override {
            auto v = UltralightUtil::CreateView(WebProcessor::RENDERER);

            inspector = std::make_unique<InspectorWindow>(v, WebProcessor::RENDERER, [&]() {
                inspector->setVisible(false);
            });

            return v;
        }

        void OnChangeCursor(ultralight::View *caller, ultralight::Cursor cursor) override {
            setMouseCursor(UltralightUtil::mapUltralightCursorToJUCE(cursor));
        }

        void paint(juce::Graphics &g) override {
            RenderFrame(g);
        }


        void resized() override {
            auto w = getWidth();
            auto h = getHeight();
            auto deviceScale = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay()->scale;
            view->set_device_scale(deviceScale);

            if (view.get() != nullptr){
                // Update Ultralight view size
                view->Resize(static_cast<uint32_t>(w * view->device_scale()),
                             static_cast<uint32_t>(h * view->device_scale()));
                view->Focus();
            }
        }

        void RenderFrame(juce::Graphics &g) {
            renderer->Update();
            renderer->Render();

            const auto surface = dynamic_cast<BitmapSurface *>(view->surface());

            if (!surface->dirty_bounds().IsEmpty()) {
                frame = RenderToImage(surface);
                surface->ClearDirtyBounds();
            }

            g.drawImage(frame,
                        0, 0, getWidth(), getHeight(),
                        0, 0,
                        static_cast<int>(getWidth() * view->device_scale()),
                        static_cast<int>(getHeight() * view->device_scale()));
        }

        static juce::Image RenderToImage(BitmapSurface *surface) {
            auto bitmap = surface->bitmap();
            auto pixels = bitmap->LockPixels();

            auto image = UltralightUtil::CopyPixelsToTexture(
                    pixels, bitmap->width(), bitmap->height(), bitmap->row_bytes()
            );

            bitmap->UnlockPixels();
            return image;
        }

        void mouseDown(const juce::MouseEvent &event) override {
            auto button = event.mods.isLeftButtonDown() ? MouseEvent::kButton_Left
                                                        : MouseEvent::kButton_Right;
            view->FireMouseEvent({
                                         MouseEvent::kType_MouseDown,
                                         event.x, event.y,
                                         button
                                 });

            repaint();
        }

        void mouseMove(const juce::MouseEvent &event) override {
            auto button = event.mods.isLeftButtonDown() ? MouseEvent::kButton_Left
                                                        : MouseEvent::kButton_Right;
            view->FireMouseEvent({
                                         MouseEvent::kType_MouseMoved,
                                         event.x, event.y,
                                         button
                                 });

            repaint();
        }

        void mouseUp(const juce::MouseEvent &event) override {
            auto button = event.mods.isLeftButtonDown() ? MouseEvent::kButton_Left
                                                        : MouseEvent::kButton_Right;
            view->FireMouseEvent({
                                         MouseEvent::kType_MouseUp,
                                         event.x, event.y,
                                         button
                                 });
            repaint();
        }

        void mouseWheelMove(const juce::MouseEvent &event, const juce::MouseWheelDetails &wheel) override {
            ScrollEvent evt {ScrollEvent::kType_ScrollByPixel,
                             static_cast<int>(wheel.deltaX * 100),
                             static_cast<int>(wheel.deltaY * 100)};
            view->FireScrollEvent(evt);
            repaint();
        }

        bool keyPressed(const juce::KeyPress &key, Component *originatingComponent) override {

            if (allowInspector &&
                tolower(key.getKeyCode()) == 'i'
                && key.getModifiers().isCommandDown()
                && key.getModifiers().isShiftDown()) {
                if (!inspector) view->CreateLocalInspectorView();
                if (inspector) {
                    inspector->setVisible(true);
                    return true;
                }
            }

            // Check if key character contains only single letter or number
            std::regex regexPattern(R"([A-Za-z0-9`\-=[\\\];',./~!@#$%^&*()_+{}|:"<>?\u0020])");

            auto str = juce::String::charToString(key.getTextCharacter()).toStdString();

            ultralight::KeyEvent evt;
            if (std::regex_match(str, regexPattern)) {
                // Single character
                evt.type = ultralight::KeyEvent::kType_Char;
                ultralight::String keyStr(str.c_str());
                evt.text = keyStr;
                evt.native_key_code = 0;
                evt.modifiers = 0;
                evt.unmodified_text = evt.text; // If not available, set to same as evt.text
            } else {
                // Special keys
                evt.type = ultralight::KeyEvent::kType_RawKeyDown;
                evt.virtual_key_code = UltralightUtil::mapJUCEKeyCodeToUltralight(key.getKeyCode());
                evt.native_key_code = 0;
                evt.modifiers = 0;
                GetKeyIdentifierFromVirtualKeyCode(evt.virtual_key_code, evt.key_identifier);
            }

            view->FireKeyEvent(evt);

            return true; // Indicate that the key press is consumed
        }

    private:
        RefPtr<Renderer> renderer;
        RefPtr<View> view;

        juce::Image frame;
        std::unique_ptr<InspectorWindow> inspector;
        bool allowInspector;
    };
}
