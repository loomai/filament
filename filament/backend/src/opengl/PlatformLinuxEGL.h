/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef TNT_FILAMENT_DRIVER_OPENGL_PLATFORM_LINUX_EGL_H
#define TNT_FILAMENT_DRIVER_OPENGL_PLATFORM_LINUX_EGL_H

#include <cstdint>

#include <bluegl/BlueGL.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <backend/DriverEnums.h>

#include "private/backend/OpenGLPlatform.h"

namespace filament {

class PlatformLinuxEGL final : public backend::OpenGLPlatform {
public:

    PlatformLinuxEGL() noexcept;

    backend::Driver* createDriver(void* sharedContext) noexcept override;
    void terminate() noexcept override;

    SwapChain* createSwapChain(void* nativewindow, uint64_t& flags) noexcept final;
    SwapChain* createSwapChain(uint32_t width, uint32_t height, uint64_t& flags) noexcept final;
    void destroySwapChain(SwapChain* swapChain) noexcept final;
    void makeCurrent(SwapChain* drawSwapChain, SwapChain* readSwapChain) noexcept final;
    void commit(SwapChain* swapChain) noexcept final;

    bool canCreateFence() noexcept final { return true; }
    Fence* createFence() noexcept final;
    void destroyFence(Fence* fence) noexcept final;
    backend::FenceStatus waitFence(Fence* fence, uint64_t timeout) noexcept final;

    void setPresentationTime(int64_t time) noexcept final override {}

    Stream* createStream(void* nativeStream) noexcept final override { return nullptr; }
    void destroyStream(Stream* stream) noexcept final override {}
    void attach(Stream* stream, intptr_t tname) noexcept final override {}
    void detach(Stream* stream) noexcept final override {}
    void updateTexImage(Stream* stream, int64_t* timestamp) noexcept final override {}

    ExternalTexture* createExternalTextureStorage() noexcept final override { return nullptr; }
    void reallocateExternalStorage(ExternalTexture* ets,
            uint32_t w, uint32_t h, backend::TextureFormat format) noexcept final override { }
    void destroyExternalTextureStorage(ExternalTexture* ets) noexcept final override { }

    int getOSVersion() const noexcept final override { return 0; }

private:
    EGLBoolean makeCurrent(EGLSurface drawSurface, EGLSurface readSurface) noexcept;

    EGLDisplay mEGLDisplay = EGL_NO_DISPLAY;
    EGLContext mEGLContext = EGL_NO_CONTEXT;
    EGLSurface mCurrentDrawSurface = EGL_NO_SURFACE;
    EGLSurface mCurrentReadSurface = EGL_NO_SURFACE;
    EGLSurface mEGLDummySurface = EGL_NO_SURFACE;
    EGLConfig mEGLConfig;
};

} // namespace filament

#endif // TNT_FILAMENT_DRIVER_OPENGL_PLATFORM_LINUX_EGL_H
