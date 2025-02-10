#pragma once

#include "src/attachment/AuthAttachment.h"
#include "src/attachment/util/BufferUtil.h"
#include "src/AssetServer/BinaryDataAssetServer.h"

#if JUCE_WINDOWS or JUCE_MAC
#include "src/connection/web/WebUIPluginEditor.h"
#include "src/connection/web/WebProcessor.h"
#include "src/connection/web/ChocBrowserComponent.h"
#include "src/connection/web/WebUIConnection.h"
#endif

#include "src/connection/SocketUIConnection.h"
#include "src/connection/DummyUIConnection.h"
#include "src/ConnectedProcessor.h"
