//
// Created by pembe on 2/26/2024.
//

#pragma once

class WinKeypressWorkaround : choc::ui::WebView::KeyListener, juce::Timer {
public:
    WinKeypressWorkaround(choc::ui::WebView& w, juce::Component& c) : webView(w), component(c) {
        webView.addKeyListener(this);
    }

    ~WinKeypressWorkaround() override {
        webView.removeKeyListener(this);
    }

    void onKeyDown(std::string key) override {
        if (key.empty()) return;
        auto scanCode = getScanCode(key[0]);
        if (scanCode == 0) return;
        auto event = createKeypress(scanCode, true);
        sendKeypress(event);
    }

    void onKeyUp(std::string key) override {
        if (key.empty()) return;
        auto scanCode = getScanCode(key[0]);
        if (scanCode == 0) return;
        auto event = createKeypress(scanCode, false);
        sendKeypress(event);
    }

    static unsigned int getScanCode(char key) {
        SHORT vkCode = VkKeyScan(key);
        if (vkCode == -1) return 0;

        // Extract the virtual key code
        UINT vk = LOBYTE(vkCode);

        // Convert the virtual key code to a scan code
        UINT scanCode = MapVirtualKey(vk, MAPVK_VK_TO_VSC);
        return scanCode;
    }

    static INPUT createKeypress(unsigned int scanCode, bool down) {
        INPUT input = {};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = 0;
        input.ki.wScan = scanCode;
        input.ki.dwFlags = KEYEVENTF_SCANCODE | (down ? 0 : KEYEVENTF_KEYUP);
        input.ki.time = 0;
        input.ki.dwExtraInfo = GetMessageExtraInfo();

        return input;
    }

    void sendKeypress(INPUT event) {
        resetWindow = GetFocus();
        auto* topLevel = component.getTopLevelComponent();
        if (topLevel == nullptr) return;

        auto* peer = topLevel->getPeer();
        if (peer == nullptr) return;

        auto nativeWindow = (HWND) peer->getNativeHandle();
        if (nativeWindow == nullptr) return;

        SetFocus(nativeWindow);

        SendInput(1, &event, sizeof(INPUT));

        startTimer(30);
    }

    void timerCallback() override {
        if (auto* window = resetWindow.load())
            SetFocus(window);

        resetWindow = nullptr;
        stopTimer();
    }

private:
    choc::ui::WebView& webView;
    juce::Component& component;

    std::atomic<HWND> resetWindow;

    moodycamel::ReaderWriterQueue<INPUT> eventsToSend;
};
