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

#include "PlatformLinuxEGL.h"

#include <cassert>
#include <cstring>
#include <cstdio>

#include <unordered_set>

#include <utils/compiler.h>
#include <utils/Log.h>

#include "OpenGLDriverFactory.h"

using namespace utils;

namespace filament {
using namespace backend;

using EGLStream = Platform::Stream;

// ---------------------------------------------------------------------------------------------
// Utilities
// ---------------------------------------------------------------------------------------------

static void logEglError(const char* name) noexcept {
    const char* err;
    switch (eglGetError()) {
        case EGL_NOT_INITIALIZED:       err = "EGL_NOT_INITIALIZED";    break;
        case EGL_BAD_ACCESS:            err = "EGL_BAD_ACCESS";         break;
        case EGL_BAD_ALLOC:             err = "EGL_BAD_ALLOC";          break;
        case EGL_BAD_ATTRIBUTE:         err = "EGL_BAD_ATTRIBUTE";      break;
        case EGL_BAD_CONTEXT:           err = "EGL_BAD_CONTEXT";        break;
        case EGL_BAD_CONFIG:            err = "EGL_BAD_CONFIG";         break;
        case EGL_BAD_CURRENT_SURFACE:   err = "EGL_BAD_CURRENT_SURFACE";break;
        case EGL_BAD_DISPLAY:           err = "EGL_BAD_DISPLAY";        break;
        case EGL_BAD_SURFACE:           err = "EGL_BAD_SURFACE";        break;
        case EGL_BAD_MATCH:             err = "EGL_BAD_MATCH";          break;
        case EGL_BAD_PARAMETER:         err = "EGL_BAD_PARAMETER";      break;
        case EGL_BAD_NATIVE_PIXMAP:     err = "EGL_BAD_NATIVE_PIXMAP";  break;
        case EGL_BAD_NATIVE_WINDOW:     err = "EGL_BAD_NATIVE_WINDOW";  break;
        case EGL_CONTEXT_LOST:          err = "EGL_CONTEXT_LOST";       break;
        default:                        err = "unknown";                break;
    }
    std::cout << name << " failed with " << err << io::endl;
}

static void clearGlError() noexcept {
    // clear GL error that may have been set by previous calls
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cout << "Ignoring pending GL error " << io::hex << error << io::endl;
    }
}

// ---------------------------------------------------------------------------------------------

PlatformLinuxEGL::PlatformLinuxEGL() noexcept {

}

Driver* PlatformLinuxEGL::createDriver(void* sharedContext) noexcept {
    mEGLDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    assert(mEGLDisplay != EGL_NO_DISPLAY);

    EGLint major, minor;
    auto initialized = eglInitialize(mEGLDisplay, &major, &minor);
    if (UTILS_UNLIKELY(!initialized)) {
        slog.e << "eglInitialize failed" << io::endl;
        return nullptr;
    }

    EGLint configsCount;
    EGLint configAttribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_NONE
    };

    EGLint contextAttribs[] = {
        EGL_CONTEXT_MAJOR_VERSION, 4,
        EGL_CONTEXT_MINOR_VERSION, 1,
        EGL_NONE
    };

    EGLint pbufferAttribs[] = {
        EGL_WIDTH, 1,
        EGL_HEIGHT, 1,
        EGL_NONE
    };

    if (!eglChooseConfig(mEGLDisplay, configAttribs, &mEGLConfig, 1, &configsCount)) {
        logEglError("eglChooseConfig");
        goto error;
    }

    eglBindAPI(EGL_OPENGL_API);

    mEGLDummySurface = eglCreatePbufferSurface(mEGLDisplay, mEGLConfig, pbufferAttribs);
    if (mEGLDummySurface == EGL_NO_SURFACE) {
        logEglError("eglCreatePbufferSurface");
        goto error;
    }

    mEGLContext = eglCreateContext(mEGLDisplay, mEGLConfig, (EGLContext)sharedContext, contextAttribs);
    if (UTILS_UNLIKELY(mEGLContext == EGL_NO_CONTEXT)) {
        // eglCreateContext failed
        logEglError("eglCreateContext");
        goto error;
    }

    if (!makeCurrent(mEGLDummySurface, mEGLDummySurface)) {
        // eglMakeCurrent failed
        logEglError("eglMakeCurrent");
        goto error;
    }

    bluegl::bind();

    return OpenGLDriverFactory::create(this, sharedContext);

error:
    // if we're here, we've failed
    if (mEGLDummySurface) {
        eglDestroySurface(mEGLDisplay, mEGLDummySurface);
    }
    if (mEGLContext) {
        eglDestroyContext(mEGLDisplay, mEGLContext);
    }

    mEGLDummySurface = EGL_NO_SURFACE;
    mEGLContext = EGL_NO_CONTEXT;

    eglTerminate(mEGLDisplay);
    eglReleaseThread();

    return nullptr;
}

EGLBoolean PlatformLinuxEGL::makeCurrent(EGLSurface drawSurface, EGLSurface readSurface) noexcept {
    if (UTILS_UNLIKELY((drawSurface != mCurrentDrawSurface || readSurface != mCurrentReadSurface))) {
        mCurrentDrawSurface = drawSurface;
        mCurrentReadSurface = readSurface;
        return eglMakeCurrent(mEGLDisplay, drawSurface, readSurface, mEGLContext);
    }
    return EGL_TRUE;
}

void PlatformLinuxEGL::terminate() noexcept {
    eglMakeCurrent(mEGLDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglDestroySurface(mEGLDisplay, mEGLDummySurface);
    eglDestroyContext(mEGLDisplay, mEGLContext);
    eglTerminate(mEGLDisplay);
    eglReleaseThread();
}

Platform::SwapChain* PlatformLinuxEGL::createSwapChain(
        void* nativeWindow, uint64_t& flags) noexcept {
    // Transparent swap chain is not supported for software renderer.
    flags &= ~backend::SWAP_CHAIN_CONFIG_TRANSPARENT;
    EGLSurface sur = eglCreateWindowSurface(mEGLDisplay, mEGLConfig,
            (EGLNativeWindowType)nativeWindow, nullptr);

    if (UTILS_UNLIKELY(sur == EGL_NO_SURFACE)) {
        logEglError("eglCreateWindowSurface");
        return nullptr;
    }
    if (!eglSurfaceAttrib(mEGLDisplay, sur, EGL_SWAP_BEHAVIOR, EGL_BUFFER_DESTROYED)) {
        logEglError("eglSurfaceAttrib(..., EGL_SWAP_BEHAVIOR, EGL_BUFFER_DESTROYED)");
        // this is not fatal
    }
    return (SwapChain*)sur;
}

Platform::SwapChain* PlatformLinuxEGL::createSwapChain(
        uint32_t width, uint32_t height, uint64_t& flags) noexcept {
    // Transparent swap chain is not supported for software renderer.
    flags &= ~backend::SWAP_CHAIN_CONFIG_TRANSPARENT;

    EGLint attribs[] = {
            EGL_WIDTH, EGLint(width),
            EGL_HEIGHT, EGLint(height),
            EGL_NONE
    };

    EGLSurface sur = eglCreatePbufferSurface(mEGLDisplay, mEGLConfig, attribs);

    if (UTILS_UNLIKELY(sur == EGL_NO_SURFACE)) {
        logEglError("eglCreatePbufferSurface");
        return nullptr;
    }
    return (SwapChain*)sur;
}

void PlatformLinuxEGL::destroySwapChain(Platform::SwapChain* swapChain) noexcept {
    EGLSurface sur = (EGLSurface) swapChain;
    if (sur != EGL_NO_SURFACE) {
        makeCurrent(mEGLDummySurface, mEGLDummySurface);
        eglDestroySurface(mEGLDisplay, sur);
    }
}

void PlatformLinuxEGL::makeCurrent(Platform::SwapChain* drawSwapChain,
                              Platform::SwapChain* readSwapChain) noexcept {
    EGLSurface drawSur = (EGLSurface) drawSwapChain;
    EGLSurface readSur = (EGLSurface) readSwapChain;
    if (drawSur != EGL_NO_SURFACE || readSur != EGL_NO_SURFACE) {
        makeCurrent(drawSur, readSur);
    }
}

void PlatformLinuxEGL::commit(Platform::SwapChain* swapChain) noexcept {
    EGLSurface sur = (EGLSurface) swapChain;
    if (sur != EGL_NO_SURFACE) {
        eglSwapBuffers(mEGLDisplay, sur);
    }
}

Platform::Fence* PlatformLinuxEGL::createFence() noexcept {
    Fence* f = nullptr;
#ifdef EGL_KHR_reusable_sync
    f = (Fence*) eglCreateSync(mEGLDisplay, EGL_SYNC_FENCE, nullptr);
#endif
    return f;
}

void PlatformLinuxEGL::destroyFence(Platform::Fence* fence) noexcept {
#ifdef EGL_KHR_reusable_sync
    EGLSync sync = (EGLSync) fence;
    if (sync != EGL_NO_SYNC) {
        eglDestroySync(mEGLDisplay, sync);
    }
#endif
}

backend::FenceStatus PlatformLinuxEGL::waitFence(
        Platform::Fence* fence, uint64_t timeout) noexcept {
#ifdef EGL_KHR_reusable_sync
    EGLSync sync = (EGLSync) fence;
    if (sync != EGL_NO_SYNC) {
        EGLint status = eglClientWaitSync(mEGLDisplay, sync, 0, (EGLTime)timeout);
        if (status == EGL_CONDITION_SATISFIED) {
            return FenceStatus::CONDITION_SATISFIED;
        }
        if (status == EGL_TIMEOUT_EXPIRED) {
            return FenceStatus::TIMEOUT_EXPIRED;
        }
    }
#endif
    return FenceStatus::ERROR;
}

} // namespace filament

// ---------------------------------------------------------------------------------------------
