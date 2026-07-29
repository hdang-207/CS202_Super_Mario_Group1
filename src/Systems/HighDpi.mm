// SFML itself renders with OpenGL, which Apple has deprecated in favour of Metal;
// the surface calls below carry the same deprecation, so silence the noise.
#define GL_SILENCE_DEPRECATION

#include "Systems/HighDpi.hpp"
#import <AppKit/AppKit.h>
#include <OpenGL/gl.h>

namespace {

    /**
     * @brief Checks whether the OpenGL drawable really is @p pixelHeight tall.
     *
     * Asking Cocoa is not good enough: both -backingScaleFactor and
     * -convertRectToBacking: describe the screen, and keep answering "2x" even
     * while the drawable is still half that size. So ask OpenGL instead, by
     * reading a pixel on the top row that only exists once the drawable has
     * actually grown. Out-of-range reads leave the destination alone, so two
     * different fill patterns that both survive mean the row is not there.
     */
    bool drawableIsTallEnough(int pixelHeight) {
        GLubyte first[4] = {1, 2, 3, 255};
        GLubyte second[4] = {250, 251, 252, 255};
        glReadPixels(0, pixelHeight - 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, first);
        glReadPixels(0, pixelHeight - 1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, second);

        bool untouched = first[0] == 1 && first[1] == 2 && first[2] == 3
                      && second[0] == 250 && second[1] == 251 && second[2] == 252;
        return !untouched;
    }

}

namespace Systems {

    float enableHighDpi(sf::WindowHandle handle) {
        if (handle == nullptr) {
            return 1.f;
        }

        // SFML hands out the NSWindow; its content view holds the OpenGL surface.
        NSWindow* window = (__bridge NSWindow*)handle;
        NSView* view = [window contentView];
        if (view == nil || ![view respondsToSelector:@selector(setWantsBestResolutionOpenGLSurface:)]) {
            return 1.f;
        }

        float scale = static_cast<float>([window backingScaleFactor]);
        if (scale <= 1.f) {
            return 1.f; // Ordinary screen: SFML is already drawing at full resolution.
        }

        [(NSOpenGLView*)view setWantsBestResolutionOpenGLSurface:YES];

        // The flag on its own changes nothing - the context keeps the drawable it
        // was given until it is told to catch up. Without this line the game would
        // still be drawn into a half-size buffer, at double size, and the picture
        // would show only its top-left quarter.
        [[NSOpenGLContext currentContext] update];

        int expectedHeight = static_cast<int>([view bounds].size.height * scale);
        if (!drawableIsTallEnough(expectedHeight)) {
            // The driver would not grow the drawable. Undo, and let the caller draw
            // at 1x: slightly soft, but far better than a quarter of the picture.
            [(NSOpenGLView*)view setWantsBestResolutionOpenGLSurface:NO];
            [[NSOpenGLContext currentContext] update];
            return 1.f;
        }

        return scale;
    }

}
