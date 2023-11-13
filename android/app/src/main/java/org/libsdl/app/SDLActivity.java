package org.libsdl.app;

import android.app.Activity;
import android.app.UiModeManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.Process;
import android.service.wallpaper.WallpaperService;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.View;
import android.view.WindowManager;
import android.view.inputmethod.BaseInputConnection;
import android.view.inputmethod.InputConnection;

import java.util.Locale;
import java.util.concurrent.TimeUnit;

import io.github.bhowell2.debouncer.Debouncer;


/**
 * SDL Activity
 */
public class SDLActivity extends WallpaperService implements View.OnSystemUiVisibilityChangeListener {
    private static final String TAG = "SDL";
    private static final int SDL_MAJOR_VERSION = 2;
    private static final int SDL_MINOR_VERSION = 26;
    private static final int SDL_MICRO_VERSION = 5;

    public static boolean mIsResumedCalled, mHasFocus;
    public static final boolean mHasMultiWindow = (Build.VERSION.SDK_INT >= 24);

    protected static final int SDL_ORIENTATION_UNKNOWN = 0;
    protected static final int SDL_ORIENTATION_LANDSCAPE = 1;
    protected static final int SDL_ORIENTATION_LANDSCAPE_FLIPPED = 2;
    protected static final int SDL_ORIENTATION_PORTRAIT = 3;
    protected static final int SDL_ORIENTATION_PORTRAIT_FLIPPED = 4;

    protected static int mCurrentOrientation;
    protected static Locale mCurrentLocale;

    // Handle the state of the native layer
    public enum NativeState {
        INIT, RESUMED, PAUSED
    }

    public static NativeState mNextNativeState;
    public static NativeState mCurrentNativeState;

    /**
     * If shared libraries (e.g. SDL or the native application) could not be loaded.
     */
    public static boolean mBrokenLibraries = true;

    // Main components
    protected static SDLActivity mSingleton;
    protected static SDLSurface mSurface;
    protected static boolean mScreenKeyboardShown;
    protected static SDLGenericMotionListener_API12 mMotionListener;

    // This is what SDL runs in. It invokes SDL_main(), eventually
    protected static Thread mSDLThread;

    protected static SDLGenericMotionListener_API12 getMotionListener() {
        if (mMotionListener == null) {
            if (Build.VERSION.SDK_INT >= 26) {
                mMotionListener = new SDLGenericMotionListener_API26();
            } else if (Build.VERSION.SDK_INT >= 24) {
                mMotionListener = new SDLGenericMotionListener_API24();
            } else {
                mMotionListener = new SDLGenericMotionListener_API12();
            }
        }

        return mMotionListener;
    }

    /**
     * This method returns the name of the shared object with the application entry point
     * It can be overridden by derived classes.
     */
    protected String getMainSharedObject() {
        String library;
        String[] libraries = SDLActivity.mSingleton.getLibraries();
        if (libraries.length > 0) {
            library = "lib" + libraries[libraries.length - 1] + ".so";
        } else {
            library = "libmain.so";
        }
        return getContext().getApplicationInfo().nativeLibraryDir + "/" + library;
    }

    /**
     * This method returns the name of the application entry point
     * It can be overridden by derived classes.
     */
    protected String getMainFunction() {
        return "SDL_main";
    }

    /**
     * This method is called by SDL before loading the native shared libraries.
     * It can be overridden to provide names of shared libraries to be loaded.
     * The default implementation returns the defaults. It never returns null.
     * An array returned by a new implementation must at least contain "SDL2".
     * Also keep in mind that the order the libraries are loaded may matter.
     *
     * @return names of shared libraries to be loaded (e.g. "SDL2", "main").
     */
    protected String[] getLibraries() {
        return new String[]{
            "SDL2",
            "main"
        };
    }

    // Load the .so
    public void loadLibraries() {
        for (String lib : getLibraries()) {
            SDL.loadLibrary(lib);
        }
    }

    /**
     * This method is called by SDL before starting the native application thread.
     * It can be overridden to provide the arguments after the application name.
     * The default implementation returns an empty array. It never returns null.
     *
     * @return arguments for the native application.
     */
    protected String[] getArguments() {
        return new String[0];
    }

    public static void initialize() {
        // The static nature of the singleton and Android quirkyness force us to initialize everything here
        // Otherwise, when exiting the app and returning to it, these variables *keep* their pre exit values
        mSingleton = null;
        mSurface = null;
        mSDLThread = null;
        mIsResumedCalled = false;
        mHasFocus = true;
        mNextNativeState = NativeState.INIT;
        mCurrentNativeState = NativeState.INIT;
    }

    protected SDLSurface createSDLSurface(Context context) {
        return new SDLSurface(context);
    }

    @Override
    public void onCreate() {
        Log.v(TAG, "Device: " + Build.DEVICE);
        Log.v(TAG, "Model: " + Build.MODEL);
        Log.v(TAG, "onCreate()");
        super.onCreate();

        try {
            Thread.currentThread().setName("SDLActivity");
        } catch (Exception e) {
            Log.v(TAG, "modify thread properties failed " + e.toString());
        }

        String errorMsgBrokenLib = "";
        try {
            loadLibraries();
            mBrokenLibraries = false;
        } catch (UnsatisfiedLinkError | Exception e) {
            System.err.println(e.getMessage());
            mBrokenLibraries = true;
            errorMsgBrokenLib = e.getMessage();
        }

        if (!mBrokenLibraries) {
            String expected_version = String.valueOf(SDL_MAJOR_VERSION) + "." +
                String.valueOf(SDL_MINOR_VERSION) + "." +
                String.valueOf(SDL_MICRO_VERSION);
            String version = nativeGetVersion();
            if (!version.equals(expected_version)) {
                mBrokenLibraries = true;
                errorMsgBrokenLib = "SDL C/Java version mismatch (expected " + expected_version + ", got " + version + ")";
            }
        }

        if (mBrokenLibraries) {
            mSingleton = this;
            Log.e(TAG, "Broken library message: " + errorMsgBrokenLib);
            return;
        }

        SDL.setupJNI();
        SDL.initialize();

        // So we can call stuff from static callbacks
        mSingleton = this;
        SDL.setContext(this);

        mSurface = createSDLSurface(getApplication());

        mCurrentOrientation = SDLActivity.getCurrentOrientation();
        SDLActivity.onNativeOrientationChanged(mCurrentOrientation);

        try {
            if (Build.VERSION.SDK_INT < 24) {
                mCurrentLocale = getContext().getResources().getConfiguration().locale;
            } else {
                mCurrentLocale = getContext().getResources().getConfiguration().getLocales().get(0);
            }
        } catch (Exception ignored) {
        }

        setWindowStyle(false);
    }

    protected void pauseNativeThread() {
        Log.v("SDL", "pauseNativeThread()");
        mNextNativeState = NativeState.PAUSED;
        mIsResumedCalled = false;

        if (SDLActivity.mBrokenLibraries) {
            return;
        }

        SDLActivity.handleNativeState();
    }

    protected void resumeNativeThread() {
        Log.v("SDL", "resumeNativeThread()");
        mNextNativeState = NativeState.RESUMED;
        mIsResumedCalled = true;

        if (SDLActivity.mBrokenLibraries) {
            return;
        }

        SDLActivity.handleNativeState();
    }

    public static int getCurrentOrientation() {
        int result = SDL_ORIENTATION_UNKNOWN;

        Context context = getContext();
        if (context == null) {
            return result;
        }

        WindowManager windowManager = (WindowManager) context.getSystemService(Context.WINDOW_SERVICE);
        Display display = windowManager.getDefaultDisplay();

        switch (display.getRotation()) {
            case Surface.ROTATION_0:
                result = SDL_ORIENTATION_PORTRAIT;
                break;

            case Surface.ROTATION_90:
                result = SDL_ORIENTATION_LANDSCAPE;
                break;

            case Surface.ROTATION_180:
                result = SDL_ORIENTATION_PORTRAIT_FLIPPED;
                break;

            case Surface.ROTATION_270:
                result = SDL_ORIENTATION_LANDSCAPE_FLIPPED;
                break;
        }

        return result;
    }

    @Override
    public void onLowMemory() {
        Log.v(TAG, "onLowMemory()");
        super.onLowMemory();

        if (SDLActivity.mBrokenLibraries) {
            return;
        }

        SDLActivity.nativeLowMemory();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        Log.v(TAG, "onConfigurationChanged()");
        super.onConfigurationChanged(newConfig);

        if (SDLActivity.mBrokenLibraries) {
            return;
        }

        if (mCurrentLocale == null || !mCurrentLocale.equals(newConfig.locale)) {
            mCurrentLocale = newConfig.locale;
            SDLActivity.onNativeLocaleChanged();
        }
    }

    @Override
    public void onDestroy() {
        Log.v(TAG, "onDestroy()");

        if (SDLActivity.mBrokenLibraries) {
            super.onDestroy();
            return;
        }

        if (SDLActivity.mSDLThread != null) {
            try {
                SDLActivity.mSDLThread.join();
            } catch (Exception e) {
                Log.v(TAG, "Problem stopping SDLThread: " + e);
            }
        }

        SDLActivity.nativeQuit();

        super.onDestroy();

        SDLActivity.initialize();
    }

    public static int engineCounter = 0;
    private static SDLEngine mEngine;

    class SDLEngine extends Engine {
        public String TAG = "SDLEngine";
        public Thread mSDLThread;
        private SurfaceHolder mHolder;
        protected Display mDisplay;

        SDLEngine(String prefix) {
            super();
            TAG = prefix + ": " + TAG + " ENG: " + SDLActivity.engineCounter;
            SDLActivity.engineCounter++;
            Log.v(TAG, "SDLEngine");
            mDisplay = ((WindowManager) getContext().getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay();

            setTouchEventsEnabled(true);
        }

        @Override
        public void onTouchEvent(MotionEvent event) {
            if (event.getAction() == MotionEvent.ACTION_UP) {
//                nativeUpdateVisibleMapRegion();
            }
        }

        Debouncer debouncer = new Debouncer(2);

        @Override
        public void onVisibilityChanged(boolean visible) {
            Log.v(TAG, "onVisibilityChange " + (visible ? "true" : "false"));

            if (visible) {
                debouncer.cancel("visibility-off");
                debouncer.cancel("native-pause");
                SDLActivity.nativeResume();
                nativeUpdateConfigs();
            } else {
                debouncer.addRunLast(400, TimeUnit.MILLISECONDS, "visibility-off", k1 -> {
                    nativeUpdateVisibleMapRegion();

                    // small delay to let game engine update random map place and render it
                    debouncer.addRunLast(500, TimeUnit.MILLISECONDS, "native-pause", k2 -> {
                        SDLActivity.nativePause();
                    });
                });
            }
        }

        @Override
        public void onCreate(SurfaceHolder surfaceHolder) {
            super.onCreate(surfaceHolder);
            Log.v(TAG, "Engine onCreate");
        }

        @Override
        public void onDestroy() {
            Log.v(TAG, "onDestroy");
        }

        @Override
        public SurfaceHolder getSurfaceHolder() {
            Log.v(TAG, "Engine getSurfaceHolder");
            return mHolder;
        }

        @Override
        public void onSurfaceCreated(SurfaceHolder holder) {
            super.onSurfaceCreated(holder);
            SDLActivity.onNativeSurfaceCreated();
            Log.v(TAG, "Engine onSurfaceCreated");
            if (mHolder == null) {
                mHolder = holder;
            }
        }

        @Override
        public void onSurfaceChanged(SurfaceHolder holder, int format, int width, int height) {
            Log.v(TAG, String.format("Engine onSurfaceChanged format %d, width %d, height %d", format, width, height));

            if (mHolder != holder) {
                return;
            }

            super.onSurfaceChanged(holder, format, width, height);

            SDLActivity.nativeSetScreenResolution(width, height, width, height, mDisplay.getRefreshRate());
            SDLActivity.onNativeResize();
            SDLActivity.onNativeSurfaceChanged();

            nativeUpdateOrientation();

            if (mSDLThread == null) {
                Log.v(TAG, "Starting SDLThread");
                mSDLThread = new Thread(new SDLMain(), "SDLThread");
                mSDLThread.start();
            } else {
                Log.v(TAG, "SDLThread already exists");
                SDLActivity.nativeResume();
            }
        }

        @Override
        public void onSurfaceDestroyed(SurfaceHolder holder) {
            Log.v(TAG, "Engine onSurfaceDestroyed");

            if (holder == SDLActivity.mEngine.mHolder) {
                Log.v(TAG, "destroyed SDLActivity.mEngine.mHolder");
                SDLActivity.nativePause();
                super.onSurfaceDestroyed(holder);
            } else {
                Log.v(TAG, "Wrong destroyed");
            }
        }
    }

    @Override
    public Engine onCreateEngine() {
        Log.v(TAG, "onCreateEngine");

        if (mEngine != null) {
            Log.v(TAG, "Waiting for SDL thread");
            if (mEngine.mSDLThread != null) {
                SDLActivity.nativeQuit();

                // FIXME find way to correctly close prev thread and start new one
                // tid 19982: swapBuffers(681): error 0x300d (EGL_BAD_SURFACE)
                // egl_window_surface_t::swapBuffers called with NULL buffer

                Log.v(TAG, "kill process");

                // TODO crash can cause restart?
                Process.killProcess(Process.myPid());

                try {
                    mEngine.mSDLThread.join();
                } catch (Exception e) {
                    Log.v(TAG, "Problem stopping thread: " + e);
                }
            }
            Log.v(TAG, "SDL thread finished");
        }
        Log.v(TAG, "Creating SDL Engine");
        mEngine = new SDLEngine(TAG);
        return mEngine;
    }

    //    @Override
    public void onBackPressed() {
        Log.v("SDLActivity", "onBackPressed");
    }

    // Called by JNI from SDL.
    public static void manualBackButton() { }

    // Used to access the system back behavior.
    public void superOnBackPressed() {
        onBackPressed();
    }

    public boolean dispatchKeyEvent(KeyEvent event) {

        if (SDLActivity.mBrokenLibraries) {
            return false;
        }

        int keyCode = event.getKeyCode();
        // Ignore certain special keys so they're handled by Android
        if (keyCode == KeyEvent.KEYCODE_VOLUME_DOWN ||
            keyCode == KeyEvent.KEYCODE_VOLUME_UP ||
            keyCode == KeyEvent.KEYCODE_CAMERA ||
            keyCode == KeyEvent.KEYCODE_ZOOM_IN || /* API 11 */
            keyCode == KeyEvent.KEYCODE_ZOOM_OUT /* API 11 */
        ) {
            return false;
        }

        return true;
//        return super.dispatchKeyEvent(event);
    }

    /* Transition to next state */
    public static void handleNativeState() {
        Log.v("SDL", "handleNativeState()");

        if (mNextNativeState == mCurrentNativeState) {
            // Already in same state, discard.
            return;
        }

        // Try a transition to init state
        if (mNextNativeState == NativeState.INIT) {
            Log.v("SDL", "NativeState.INIT");

            mCurrentNativeState = mNextNativeState;
            return;
        }

        // Try a transition to paused state
        if (mNextNativeState == NativeState.PAUSED) {
            Log.v("SDL", "NativeState.PAUSED");

            if (mSDLThread != null) {
                nativePause();
            }
            if (mSurface != null) {
                mSurface.handlePause();
            }
            mCurrentNativeState = mNextNativeState;
            return;
        }

        // Try a transition to resumed state
        if (mNextNativeState == NativeState.RESUMED) {
            Log.v("SDL", "NativeState.RESUMED");
            if (mSurface.mIsSurfaceReady && mHasFocus && mIsResumedCalled) {
                Log.v("SDL", "NativeState.RESUMED 1");
                if (mSDLThread == null) {
                    Log.v("SDL", "NativeState.RESUMED 2");
                    mSDLThread = new Thread(new SDLMain(), "SDLThread");
                    mSDLThread.start();
                } else {
                    nativeResume();
                }
                mSurface.handleResume();

                mCurrentNativeState = mNextNativeState;
            }
        }
    }

    // Messages from the SDLMain thread
    static final int COMMAND_CHANGE_TITLE = 1;
    static final int COMMAND_CHANGE_WINDOW_STYLE = 2;
    static final int COMMAND_TEXTEDIT_HIDE = 3;
    static final int COMMAND_SET_KEEP_SCREEN_ON = 5;

    protected static final int COMMAND_USER = 0x8000;

    protected static boolean mFullscreenModeActive;

    /**
     * This method is called by SDL if SDL did not handle a message itself.
     * This happens if a received message contains an unsupported command.
     * Method can be overwritten to handle Messages in a different class.
     *
     * @param command the command of the message.
     * @param param   the parameter of the message. May be null.
     * @return if the message was handled in overridden method.
     */
    protected boolean onUnhandledMessage(int command, Object param) {
        return false;
    }

    /**
     * A Handler class for Messages from native SDL applications.
     * It uses current Activities as target (e.g. for the title).
     * static to prevent implicit references to enclosing object.
     */
    protected static class SDLCommandHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            Context context = SDL.getContext();
            if (context == null) {
                Log.e(TAG, "No context error handling message, getContext() returned null");
                return;
            }
            switch (msg.arg1) {
                case COMMAND_CHANGE_TITLE:
                    break;
                case COMMAND_CHANGE_WINDOW_STYLE:
                    break;
                case COMMAND_TEXTEDIT_HIDE:
                    break;
                case COMMAND_SET_KEEP_SCREEN_ON:
                    break;
                default:
                    if ((context instanceof SDLActivity) && !((SDLActivity) context).onUnhandledMessage(msg.arg1, msg.obj)) {
                        Log.e(TAG, "default error handling message, command is " + msg.arg1);
                    }
            }
        }
    }

    // Handler for the messages
    Handler commandHandler = new SDLCommandHandler();

    // Send a message from the SDLMain thread
    boolean sendCommand(int command, Object data) {
        return true;
    }

    // C functions we call
    public static native String nativeGetVersion();

    public static native void nativeUpdateOrientation();

    public static native void nativeUpdateVisibleMapRegion();

    public static native void nativeUpdateConfigs();

    public static native int nativeSetupJNI();

    public static native int nativeRunMain(String library, String function, Object arguments);

    public static native void nativeLowMemory();

    public static native void nativeSendQuit();

    public static native void nativeQuit();

    public static native void nativePause();

    public static native void nativeResume();

    public static native void nativeFocusChanged(boolean hasFocus);

    public static native void onNativeDropFile(String filename);

    public static native void nativeSetScreenResolution(int surfaceWidth, int surfaceHeight, int deviceWidth, int deviceHeight, float rate);

    public static native void onNativeResize();

    public static native void onNativeKeyDown(int keycode);

    public static native void onNativeKeyUp(int keycode);

    public static native boolean onNativeSoftReturnKey();

    public static native void onNativeKeyboardFocusLost();

    public static native void onNativeMouse(int button, int action, float x, float y, boolean relative);

    public static native void onNativeTouch(int touchDevId, int pointerFingerId,
                                            int action, float x,
                                            float y, float p);

    public static native void onNativeAccel(float x, float y, float z);

    public static native void onNativeClipboardChanged();

    public static native void onNativeSurfaceCreated();

    public static native void onNativeSurfaceChanged();

    public static native void onNativeSurfaceDestroyed();

    public static native String nativeGetHint(String name);

    public static native boolean nativeGetHintBoolean(String name, boolean default_value);

    public static native void nativeSetenv(String name, String value);

    public static native void onNativeOrientationChanged(int orientation);

    public static native void nativeAddTouch(int touchId, String name);

    public static native void nativePermissionResult(int requestCode, boolean result);

    public static native void onNativeLocaleChanged();

    /**
     * This method is called by SDL using JNI.
     */
    public static boolean setActivityTitle(String title) {
        // Called from SDLMain() thread and can't directly affect the view
        return mSingleton.sendCommand(COMMAND_CHANGE_TITLE, title);
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static void setWindowStyle(boolean fullscreen) {
        // Called from SDLMain() thread and can't directly affect the view
        mSingleton.sendCommand(COMMAND_CHANGE_WINDOW_STYLE, fullscreen ? 1 : 0);
    }

    /**
     * This method is called by SDL using JNI.
     * This is a static method for JNI convenience, it calls a non-static method
     * so that is can be overridden
     */
    public static void setOrientation(int w, int h, boolean resizable, String hint) {
        Log.v("SDL", String.format("setOrientation w: %d h: %d resizable: %s hint: %s", w, h, resizable ? "true" : "false", hint));
        if (mSingleton != null) {
            mSingleton.setOrientationBis(w, h, resizable, hint);
        }
    }

    /**
     * This can be overridden
     */
    public void setOrientationBis(int w, int h, boolean resizable, String hint) {
        int orientation_landscape = -1;
        int orientation_portrait = -1;

        /* If set, hint "explicitly controls which UI orientations are allowed". */
        if (hint.contains("LandscapeRight") && hint.contains("LandscapeLeft")) {
            orientation_landscape = ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE;
        } else if (hint.contains("LandscapeRight")) {
            orientation_landscape = ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE;
        } else if (hint.contains("LandscapeLeft")) {
            orientation_landscape = ActivityInfo.SCREEN_ORIENTATION_REVERSE_LANDSCAPE;
        }

        if (hint.contains("Portrait") && hint.contains("PortraitUpsideDown")) {
            orientation_portrait = ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT;
        } else if (hint.contains("Portrait")) {
            orientation_portrait = ActivityInfo.SCREEN_ORIENTATION_PORTRAIT;
        } else if (hint.contains("PortraitUpsideDown")) {
            orientation_portrait = ActivityInfo.SCREEN_ORIENTATION_REVERSE_PORTRAIT;
        }

        boolean is_landscape_allowed = (orientation_landscape != -1);
        boolean is_portrait_allowed = (orientation_portrait != -1);
        int req; /* Requested orientation */

        /* No valid hint, nothing is explicitly allowed */
        if (!is_portrait_allowed && !is_landscape_allowed) {
            if (resizable) {
                /* All orientations are allowed */
                req = ActivityInfo.SCREEN_ORIENTATION_FULL_SENSOR;
            } else {
                /* Fixed window and nothing specified. Get orientation from w/h of created window */
                req = (w > h ? ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE : ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT);
            }
        } else {
            /* At least one orientation is allowed */
            if (resizable) {
                if (is_portrait_allowed && is_landscape_allowed) {
                    /* hint allows both landscape and portrait, promote to full sensor */
                    req = ActivityInfo.SCREEN_ORIENTATION_FULL_SENSOR;
                } else {
                    /* Use the only one allowed "orientation" */
                    req = (is_landscape_allowed ? orientation_landscape : orientation_portrait);
                }
            } else {
                /* Fixed window and both orientations are allowed. Choose one. */
                if (is_portrait_allowed && is_landscape_allowed) {
                    req = (w > h ? orientation_landscape : orientation_portrait);
                } else {
                    /* Use the only one allowed "orientation" */
                    req = (is_landscape_allowed ? orientation_landscape : orientation_portrait);
                }
            }
        }

        Log.v(TAG, "setOrientation() requestedOrientation=" + req + " width=" + w + " height=" + h + " resizable=" + resizable + " hint=" + hint);
//        mSingleton.setRequestedOrientation(req);
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static void minimizeWindow() {
        Log.v("SDL", "minimizeWindow");
        if (mSingleton == null) {
            return;
        }

        Intent startMain = new Intent(Intent.ACTION_MAIN);
        startMain.addCategory(Intent.CATEGORY_HOME);
        startMain.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        mSingleton.startActivity(startMain);
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static boolean shouldMinimizeOnFocusLoss() {
        return false;
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static boolean isScreenKeyboardShown() {
        return false;
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static boolean supportsRelativeMouse() {
        return false;
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static boolean setRelativeMouseEnabled(boolean enabled) {
        if (enabled && !supportsRelativeMouse()) {
            return false;
        }

        return SDLActivity.getMotionListener().setRelativeMouseEnabled(enabled);
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static boolean sendMessage(int command, int param) {
        if (mSingleton == null) {
            return false;
        }
        return mSingleton.sendCommand(command, param);
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static Context getContext() {
        return SDL.getContext();
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static boolean isAndroidTV() {
        UiModeManager uiModeManager = (UiModeManager) getContext().getSystemService(UI_MODE_SERVICE);
        if (uiModeManager.getCurrentModeType() == Configuration.UI_MODE_TYPE_TELEVISION) {
            return true;
        }
        if (Build.MANUFACTURER.equals("MINIX") && Build.MODEL.equals("NEO-U1")) {
            return true;
        }
        if (Build.MANUFACTURER.equals("Amlogic") && Build.MODEL.equals("X96-W")) {
            return true;
        }
        return Build.MANUFACTURER.equals("Amlogic") && Build.MODEL.startsWith("TV");
    }

    public static double getDiagonal() {
        DisplayMetrics metrics = new DisplayMetrics();
        Activity activity = (Activity) getContext();
        if (activity == null) {
            return 0.0;
        }
        activity.getWindowManager().getDefaultDisplay().getMetrics(metrics);

        double dWidthInches = metrics.widthPixels / (double) metrics.xdpi;
        double dHeightInches = metrics.heightPixels / (double) metrics.ydpi;

        return Math.sqrt((dWidthInches * dWidthInches) + (dHeightInches * dHeightInches));
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static boolean isTablet() {
        // If our diagonal size is seven inches or greater, we consider ourselves a tablet.
        return (getDiagonal() >= 7.0);
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static boolean isChromebook() {
        if (getContext() == null) {
            return false;
        }
        return getContext().getPackageManager().hasSystemFeature("org.chromium.arc.device_management");
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static boolean isDeXMode() {
        if (Build.VERSION.SDK_INT < 24) {
            return false;
        }
        try {
            final Configuration config = getContext().getResources().getConfiguration();
            final Class<?> configClass = config.getClass();
            return configClass.getField("SEM_DESKTOP_MODE_ENABLED").getInt(configClass)
                == configClass.getField("semDesktopModeEnabled").getInt(config);
        } catch (Exception ignored) {
            return false;
        }
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static DisplayMetrics getDisplayDPI() {
        return getContext().getResources().getDisplayMetrics();
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static boolean getManifestEnvironmentVariables() {
        try {
            if (getContext() == null) {
                return false;
            }

            ApplicationInfo applicationInfo = getContext().getPackageManager().getApplicationInfo(getContext().getPackageName(), PackageManager.GET_META_DATA);
            Bundle bundle = applicationInfo.metaData;
            if (bundle == null) {
                return false;
            }
            String prefix = "SDL_ENV.";
            final int trimLength = prefix.length();
            for (String key : bundle.keySet()) {
                if (key.startsWith(prefix)) {
                    String name = key.substring(trimLength);
                    String value = bundle.get(key).toString();
                    nativeSetenv(name, value);
                }
            }
            /* environment variables set! */
            return true;
        } catch (Exception e) {
            Log.v(TAG, "exception " + e.toString());
        }
        return false;
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static boolean showTextInput(int x, int y, int w, int h) {
        return false;
    }

    public static boolean isTextInputEvent(KeyEvent event) {

        // Key pressed with Ctrl should be sent as SDL_KEYDOWN/SDL_KEYUP and not SDL_TEXTINPUT
        if (event.isCtrlPressed()) {
            return false;
        }

        return event.isPrintingKey() || event.getKeyCode() == KeyEvent.KEYCODE_SPACE;
    }

    public static boolean handleKeyEvent(View v, int keyCode, KeyEvent event, InputConnection ic) {
        return false;
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static Surface getNativeSurface() {
        Log.v(TAG, "getNativeSurface()");

        if (mEngine.mHolder == null) {
            Log.v(TAG, "getNativeSurface null");
            return null;
        }

        Log.v(TAG, "getNativeSurface. exists: " + (mEngine.mHolder.getSurface() != null));
        return mEngine.mHolder.getSurface();

//        if (SDLActivity.mSurface == null) {
//            return null;
//        }
//        return SDLActivity.mSurface.getNativeSurface();
    }

    // Input

    /**
     * This method is called by SDL using JNI.
     */
    public static void initTouch() {
        int[] ids = InputDevice.getDeviceIds();

        for (int id : ids) {
            InputDevice device = InputDevice.getDevice(id);
            /* Allow SOURCE_TOUCHSCREEN and also Virtual InputDevices because they can send TOUCHSCREEN events */
            if (device != null && ((device.getSources() & InputDevice.SOURCE_TOUCHSCREEN) == InputDevice.SOURCE_TOUCHSCREEN
                || device.isVirtual())) {

                int touchDevId = device.getId();
                /*
                 * Prevent id to be -1, since it's used in SDL internal for synthetic events
                 * Appears when using Android emulator, eg:
                 *  adb shell input mouse tap 100 100
                 *  adb shell input touchscreen tap 100 100
                 */
                if (touchDevId < 0) {
                    touchDevId -= 1;
                }
                nativeAddTouch(touchDevId, device.getName());
            }
        }
    }

    // Messagebox

    /**
     * Result of current messagebox. Also used for blocking the calling thread.
     */
    protected final int[] messageboxSelection = new int[1];

    /**
     * This method is called by SDL using JNI.
     * Shows the messagebox from UI thread and block calling thread.
     * buttonFlags, buttonIds and buttonTexts must have same length.
     *
     * @param buttonFlags array containing flags for every button.
     * @param buttonIds   array containing id for every button.
     * @param buttonTexts array containing text for every button.
     * @param colors      null for default or array of length 5 containing colors.
     * @return button id or -1.
     */
    public int messageboxShowMessageBox(
        final int flags,
        final String title,
        final String message,
        final int[] buttonFlags,
        final int[] buttonIds,
        final String[] buttonTexts,
        final int[] colors) {

        Log.v(TAG, "messageboxShowMessageBox");
        Log.v(TAG, title);
        Log.v(TAG, message);

       return 0;
    }

    private final Runnable rehideSystemUi = new Runnable() {
        @Override
        public void run() {
            if (Build.VERSION.SDK_INT >= 19) {
                int flags = View.SYSTEM_UI_FLAG_FULLSCREEN |
                    View.SYSTEM_UI_FLAG_HIDE_NAVIGATION |
                    View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY |
                    View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN |
                    View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION |
                    View.SYSTEM_UI_FLAG_LAYOUT_STABLE | View.INVISIBLE;

//                SDLActivity.this.getWindow().getDecorView().setSystemUiVisibility(flags);
            }
        }
    };

    public void onSystemUiVisibilityChange(int visibility) {
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static boolean clipboardHasText() {
        return false;
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static String clipboardGetText() {
        return "";
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static void clipboardSetText(String string) {
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static int createCustomCursor(int[] colors, int width, int height, int hotSpotX, int hotSpotY) {
        return 0;
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static void destroyCustomCursor(int cursorID) {
        return;
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static boolean setCustomCursor(int cursorID) {
        return true;
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static boolean setSystemCursor(int cursorID) {
        return true;
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static void requestPermission(String permission, int requestCode) {
        if (Build.VERSION.SDK_INT < 23) {
            nativePermissionResult(requestCode, true);
            return;
        }

        Activity activity = (Activity) getContext();
        if (activity.checkSelfPermission(permission) != PackageManager.PERMISSION_GRANTED) {
            activity.requestPermissions(new String[]{permission}, requestCode);
        } else {
            nativePermissionResult(requestCode, true);
        }
    }

    //    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        boolean result = (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED);
        nativePermissionResult(requestCode, result);
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static int openURL(String url) {
        try {
            Intent i = new Intent(Intent.ACTION_VIEW);
            i.setData(Uri.parse(url));

            int flags = Intent.FLAG_ACTIVITY_NO_HISTORY | Intent.FLAG_ACTIVITY_MULTIPLE_TASK;
            if (Build.VERSION.SDK_INT >= 21) {
                flags |= Intent.FLAG_ACTIVITY_NEW_DOCUMENT;
            } else {
                flags |= Intent.FLAG_ACTIVITY_CLEAR_WHEN_TASK_RESET;
            }
            i.addFlags(flags);

            mSingleton.startActivity(i);
        } catch (Exception ex) {
            return -1;
        }
        return 0;
    }

    /**
     * This method is called by SDL using JNI.
     */
    public static int showToast(String message, int duration, int gravity, int xOffset, int yOffset) {
        if (null == mSingleton) {
            return -1;
        }

        Log.v("SDL", String.format("Show toast: %s", message));

        return 0;
    }
}

class SDLMain implements Runnable {
    @Override
    public void run() {
        Log.v("SDL", "SDLMain run()");
        // Runs SDL_main()
        String library = SDLActivity.mSingleton.getMainSharedObject();
        String function = SDLActivity.mSingleton.getMainFunction();
        String[] arguments = SDLActivity.mSingleton.getArguments();

        try {
            android.os.Process.setThreadPriority(android.os.Process.THREAD_PRIORITY_DISPLAY);
        } catch (Exception e) {
            Log.v("SDL", "modify thread properties failed " + e.toString());
        }

        Log.v("SDL", "Running main function " + function + " from library " + library);

        SDLActivity.nativeRunMain(library, function, arguments);

        Log.v("SDL", "Finished main function");

//        if (SDLActivity.mSingleton != null && !SDLActivity.mSingleton.isFinishing()) {
//            // Let's finish the Activity
//            SDLActivity.mSDLThread = null;
//            SDLActivity.mSingleton.finish();
//        }  // else: Activity is already being destroyed

    }
}


class SDLInputConnection extends BaseInputConnection {
    public SDLInputConnection(View targetView, boolean fullEditor) {
        super(targetView, fullEditor);
    }

    public static native void nativeCommitText(String text, int newCursorPosition);

    public static native void nativeGenerateScancodeForUnichar(char c);
}