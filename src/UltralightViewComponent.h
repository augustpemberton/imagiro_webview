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
            LoadListener,
            ViewListener {
    public:
        UltralightViewComponent(LoadListenerWrappedView &v, const RefPtr<Renderer> &r,
                                bool inspector = false)
                : renderer(r),
                  loadWrappedView(v),
                  view(loadWrappedView.getView()),
                  allowInspector(inspector)
        {
            view->Focus();
            view->set_view_listener(this);

            loadWrappedView.addLoadListener(this);

            // Listen to keyboard presses
            addKeyListener(this);
            // Allows this window to receive keyboard presses
            setWantsKeyboardFocus(true);

            setOpaque(true);
            startTimerHz(120);
        }

        ~UltralightViewComponent() {
            loadWrappedView.removeLoadListener(this);
        }

        void loadURL(const juce::String& url) {
            view->LoadURL(ultralight::String(url.toRawUTF8()));
        }

        void loadHTML(const juce::String& html) {
            view->LoadURL(ultralight::String(html.toRawUTF8()));
        }
    private:
        void timerCallback() override {
            updateDisplayScaleIfNeeded();
            repaint();
        }

        RefPtr<View> OnCreateInspectorView(View *caller, bool is_local, const String &inspected_url) override {
            auto v = UltralightUtil::CreateView(WebProcessor::RENDERER);

            inspector = std::make_unique<InspectorWindow>(v, WebProcessor::RENDERER, [&]() {
                inspector->setVisible(false);
            });

            return v;
        }

        void OnDOMReady(ultralight::View *caller, uint64_t frame_id, bool is_main_frame, const ultralight::String &url) override {
            domReady = true;
            repaint();
        }

        void OnChangeCursor(ultralight::View *caller, ultralight::Cursor cursor) override {
            setMouseCursor(UltralightUtil::mapUltralightCursorToJUCE(cursor));
        }

        void paint(juce::Graphics &g) override {
            g.fillAll(juce::Colour(234, 229, 219));
            RenderFrame();
            if (domReady) PaintFrame(g);
        }


        void resized() override {
            updateDisplayScaleIfNeeded();
            updateViewSize();
        }

        void updateViewSize() {
            auto w = getWidth();
            auto h = getHeight();

            if (view.get() != nullptr){
                // Update Ultralight view size
                view->Resize(static_cast<uint32_t>(w * this->dpiScale),
                             static_cast<uint32_t>(h * this->dpiScale));
                view->Focus();
            }
        }

        void updateDisplayScaleIfNeeded() {
            auto currentDisplay = juce::Desktop::getInstance().getDisplays().getDisplayForRect(
                    getTopLevelComponent()->getScreenBounds());

            auto deviceScale = currentDisplay->scale;
            if (almostEqual(deviceScale, this->dpiScale)) return;

            this->dpiScale = deviceScale;
            view->set_device_scale(deviceScale);
            updateViewSize();
        }

        void RenderFrame() {
            renderer->Update();
            renderer->Render();
//            renderer->RefreshDisplay(view->display_id()); // ultralight v1.4
        }

        void PaintFrame(juce::Graphics &g) {

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

        void mouseDrag(const juce::MouseEvent &event) override {
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

        bool keyStateChanged(bool isKeyDown) override {
            for (auto it = keysDown.begin(); it != keysDown.end();) {
                auto keyPress = juce::KeyPress(*it);
                if (!keyPress.isCurrentlyDown()) {
                    sendKeyUpEvent(keyPress);
                    it = keysDown.erase(it);
                } else ++it;
            }
        }

        void sendKeyUpEvent(juce::KeyPress key) {
            DBG("sending key up " << key.getTextDescription());
            ultralight::KeyEvent evt;

            // Special keys
            evt.type = ultralight::KeyEvent::kType_KeyUp;
            evt.virtual_key_code = UltralightUtil::mapJUCEKeyCodeToUltralight(key.getKeyCode());
            evt.native_key_code = 0;
            evt.modifiers = 0;
            GetKeyIdentifierFromVirtualKeyCode(evt.virtual_key_code, evt.key_identifier);
            view->FireKeyEvent(evt);
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

            keysDown.insert(key.getKeyCode());

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

        LoadListenerWrappedView& loadWrappedView;
        RefPtr<View> view;

        juce::Image frame;
        std::unique_ptr<InspectorWindow> inspector;

        std::unordered_set<int> keysDown;

        bool domReady {false};

        double dpiScale;
        bool allowInspector;
    };
}
