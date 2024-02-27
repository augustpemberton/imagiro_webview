//
// Created by pembe on 2/26/2024.
//

#pragma once

struct BufferedEvent {
    INPUT i;
    int lifetime {0};
};

class WinKeypressWorkaround : choc::ui::WebView::KeyListener, juce::Timer {
public:
    WinKeypressWorkaround(choc::ui::WebView& w, juce::Component& c) : webView(w), component(c) {
        startTimerHz(5);
        webView.addKeyListener(this);

        dawHandle = GetForegroundWindow();
    }

    ~WinKeypressWorkaround() override {
        webView.removeKeyListener(this);
    }

    void onKeyDown(std::string keyCode) override {
        queueKeypress(keyCode.c_str()[0], true);
    }

    void onKeyUp(std::string keyCode) override {
        queueKeypress(keyCode.c_str()[0], false);
    }

    void queueKeypress(unsigned char key, bool down) {
        // Convert the character to a virtual key code
        SHORT vkCode = VkKeyScan(key);
        if (vkCode == -1) return;

        // Extract the virtual key code
        UINT vk = LOBYTE(vkCode);

        // Convert the virtual key code to a scan code
        UINT scanCode = MapVirtualKey(vk, MAPVK_VK_TO_VSC);

        INPUT input = {};
        input.type = INPUT_KEYBOARD;
        input.ki.wVk = 0;
        input.ki.wScan = scanCode;
        input.ki.dwFlags = KEYEVENTF_SCANCODE | (down ? 0 : KEYEVENTF_KEYUP);
        input.ki.time = 0;
        input.ki.dwExtraInfo = GetMessageExtraInfo();

        eventsToSend.insert(std::make_unique<BufferedEvent>(input));
    }

    void timerCallback() override {

        auto nativeWindow = (HWND) component.getParentComponent()->getPeer()->getNativeHandle();
        SetFocus(nativeWindow);

        queueKeypress(' ', false);

//        for (auto it=eventsToSend.begin(); it != eventsToSend.end(); ) {
//            auto& event = (*it);
//            event->lifetime--;
//            if (event->lifetime <= 0) {
//                SetForegroundWindow((HWND)component.getTopLevelComponent()->getPeer()->getNativeHandle());
//                SetFocus((HWND)component.getTopLevelComponent()->getPeer()->getNativeHandle());
//                SendInput(1, &event->i, sizeof(INPUT));
//                SendInput(1, &event->i, sizeof(INPUT));
//                SendInput(1, &event->i, sizeof(INPUT));
//                SendInput(1, &event->i, sizeof(INPUT));
//                SendInput(1, &event->i, sizeof(INPUT));
//                it = eventsToSend.erase(it);
//            } else it++;
//        }
    }

private:
    choc::ui::WebView& webView;
    juce::Component& component;

    HWND dawHandle;

    std::unordered_set<std::unique_ptr<BufferedEvent>> eventsToSend;
};

