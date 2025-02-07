//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hgiPresent2/testenv/testWindow.h"

#include <CoreFoundation/CoreFoundation.h>
#include <CoreGraphics/CoreGraphics.h>

#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>

@interface AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate>
{
    bool _launched;
    NSWindow* _window;
@public
    CAMetalLayer* metalLayer;
}
@end

@implementation AppDelegate
- (AppDelegate*)init
{
    _launched = false;
    _window = nil;
    metalLayer = nil;

    [self setupMenuBar];

    return self;
}

- (void)dealloc
{
    if (_window) {
        [_window close];
    }
    [super dealloc];
}

- (void)setupMenuBar
{
    NSMenu* mainMenu = [[NSMenu alloc] initWithTitle:@""];
    [NSApp setMainMenu:mainMenu];
    [mainMenu release];

    NSMenuItem* applicationMenuItem = [mainMenu addItemWithTitle:@""
                                                          action:nil
                                                   keyEquivalent:@""];

    NSMenu* applicationMenu = [[NSMenu alloc] initWithTitle:@""];

    [applicationMenu addItemWithTitle:@"Close Window"
                               action:@selector(terminate:)
                        keyEquivalent:@"w"];

    [applicationMenuItem setSubmenu:applicationMenu];
    [applicationMenu release];
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
    [NSApp stop:nil];
}

- (NSApplicationTerminateReply)applicationShouldTerminate:
    (NSApplication*)application
{
    [self closeWindow];

    return NSTerminateCancel;
}

- (void)createWindow:(const char*)title
           withWidth:(NSInteger)width
          withHeight:(NSInteger)height
{
    [self closeWindow];

    NSScreen* screen = [NSScreen mainScreen];
    const float scaleFactor = [screen backingScaleFactor];

    _window = [[NSWindow alloc]
        initWithContentRect:NSMakeRect(
                                0, 0, width / scaleFactor, height / scaleFactor)
                  styleMask:NSWindowStyleMaskTitled |
        NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                    backing:NSBackingStoreBuffered
                      defer:NO
                     screen:screen];
    _window.releasedWhenClosed = YES;
    _window.title = [NSString stringWithUTF8String:title];
    _window.delegate = self;

    NSView* view = _window.contentView;
    view.wantsLayer = YES;
    metalLayer = [CAMetalLayer layer];
    view.layer = metalLayer;

    // Set a default color space in case the rendering API doesn't set one.
    // Otherwise the color space is deduced from the pixel format, and probably
    // wrong.
    CGColorSpaceRef srgb = CGColorSpaceCreateWithName(kCGColorSpaceSRGB);
    metalLayer.colorspace = srgb;
    CGColorSpaceRelease(srgb);

    view.layer.contentsScale = scaleFactor;

    [_window center];
    [_window makeKeyAndOrderFront:NSApp];

    if (!_launched) {
        _launched = true;
        // Run will block the thread, so we'll stop immediately in
        // applicationDidFinishLaunching, and implement our own loop
        // in Update().
        [NSApp run];
    }
}

- (void)closeWindow
{
    if (_window) {
        [_window close];
        _window = nil;
        metalLayer = nil;
    }
}

- (bool)isWindowClosed
{
    return _window == nil;
}

- (BOOL)windowShouldClose:(NSWindow*)sender
{
    if (sender == _window) {
        _window = nil;
        metalLayer = nil;
    }
    return YES;
}

- (CGSize)getWindowSize
{
    CGSize size = [[_window contentView] frame].size;
    CGFloat scale = metalLayer.contentsScale;
    return {size.width * scale, size.height * scale};
}

- (void)setWindowSize:(CGFloat)width height:(CGFloat)height
{
    NSRect content = [_window contentRectForFrameRect:_window.frame];
    CGFloat scale = metalLayer.contentsScale;
    content.size = NSMakeSize(width / scale, height / scale);
    NSRect frame = [_window frameRectForContentRect:content];
    [_window setFrame:frame display:true animate:false];
}
@end

PXR_NAMESPACE_OPEN_SCOPE

class HgiPresent2TestWindowMetal final : public HgiPresent2TestWindow
{
public:
    HgiPresent2TestWindowMetal(const GfVec2i& size)
    {
        @autoreleasepool {
            static bool appSetupDone = false;
            if (!appSetupDone) {
                [NSApplication sharedApplication];
                AppDelegate* appDelegate = [AppDelegate alloc];
                [NSApp setDelegate:appDelegate];
                [NSApp
                    setActivationPolicy:NSApplicationActivationPolicyRegular];
                [appDelegate init];
                _appDelegate =
                    AppDelegateUniquePtr(appDelegate, &_DestroyAppDelegate);
                appSetupDone = true;
            }

            [_appDelegate.get() createWindow:"testHgiPresent2 Metal"
                                   withWidth:size[0]
                                  withHeight:size[1]];
        }
    }

    ~HgiPresent2TestWindowMetal() override
    {
        @autoreleasepool {
            [_appDelegate.get() closeWindow];
        }
    }

    HgiPresent2TestWindowHandle GetHandle() const override
    {
        @autoreleasepool {
            return HgiPresent2TestMetalWindowHandle{_appDelegate->metalLayer};
        }
    }

    GfVec2i GetSize() const override
    {
        @autoreleasepool {
            const CGSize size = [_appDelegate.get() getWindowSize];
            return {static_cast<int>(std::lround(size.width)),
                static_cast<int>(std::lround(size.height))};
        }
    }

    void SetSize(const GfVec2i &size) override
    {
        @autoreleasepool {
            [_appDelegate.get() setWindowSize:size[0] height:size[1]];
        }
    }

    bool Update() override
    {
        @autoreleasepool {
            while (true) {
                NSEvent* event =
                    [NSApp nextEventMatchingMask:NSEventMaskAny
                                       untilDate:[NSDate distantPast]
                                          inMode:NSDefaultRunLoopMode
                                         dequeue:YES];

                if (event == nil) {
                    break;
                }

                [NSApp sendEvent:event];
            }

            return ![_appDelegate.get() isWindowClosed];
        }
    }

private:
    static void _DestroyAppDelegate(AppDelegate* appDelegate)
    {
        @autoreleasepool {
            if (appDelegate) {
                [appDelegate release];
            }
        }
    }

    using AppDelegateUniquePtr =
        std::unique_ptr<AppDelegate, decltype(&_DestroyAppDelegate)>;
    static AppDelegateUniquePtr _appDelegate;
};

HgiPresent2TestWindowMetal::AppDelegateUniquePtr
    HgiPresent2TestWindowMetal::_appDelegate = AppDelegateUniquePtr(
        nullptr, &HgiPresent2TestWindowMetal::_DestroyAppDelegate);

std::unique_ptr<HgiPresent2TestWindow>
HgiPresent2TestCreateMetalWindow(const GfVec2i& size)
{
    return std::make_unique<HgiPresent2TestWindowMetal>(size);
}

PXR_NAMESPACE_CLOSE_SCOPE
