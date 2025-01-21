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
        auto scanCode = getScanCode(key[0]);
        auto event = createKeypress(scanCode, true);
        sendKeypress(event);
    }

    void onKeyUp(std::string key) override {
        auto scanCode = getScanCode(key[0]);
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
        auto nativeWindow = (HWND) component.getTopLevelComponent()->getPeer()->getNativeHandle();
        SetFocus(nativeWindow);

        SendInput(1, &event, sizeof(INPUT));

        startTimer(30);
    }

    void timerCallback() override {
        SetFocus(resetWindow);
        stopTimer();
    }

private:
    choc::ui::WebView& webView;
    juce::Component& component;

    std::atomic<HWND> resetWindow;

    moodycamel::ReaderWriterQueue<INPUT> eventsToSend;
};

