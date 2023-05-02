package org.libsdl.app;


import android.content.Context;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Build;
import android.util.DisplayMetrics;
import android.util.Log;
import android.view.Display;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.WindowManager;


/**
    SDLSurface. This is what we draw on, so we need to know when it's created
    in order to do anything useful.

    Because of this, that's where we set up the SDL thread
*/
public class SDLSurface extends SurfaceView implements SurfaceHolder.Callback,
    View.OnKeyListener, View.OnTouchListener, SensorEventListener  {

    // Sensors
    protected SensorManager mSensorManager;
    protected Display mDisplay;

    // Keep track of the surface size to normalize touch events
    protected float mWidth, mHeight;

    // Is SurfaceView ready for rendering
    public boolean mIsSurfaceReady;

    // Startup
    public SDLSurface(Context context) {
        super(context);
        getHolder().addCallback(this);

//        setFocusable(true);
//        setFocusableInTouchMode(true);
//        requestFocus();
//        setOnKeyListener(this);
//        setOnTouchListener(this);

        mDisplay = ((WindowManager)context.getSystemService(Context.WINDOW_SERVICE)).getDefaultDisplay();
        mSensorManager = (SensorManager)context.getSystemService(Context.SENSOR_SERVICE);

        setOnGenericMotionListener(SDLActivity.getMotionListener());

        // Some arbitrary defaults to avoid a potential division by zero
        mWidth = 1.0f;
        mHeight = 1.0f;

        mIsSurfaceReady = false;
    }

    public void handlePause() {
        Log.v("SDL", "handlePause()");
//        enableSensor(Sensor.TYPE_ACCELEROMETER, false);
    }

    public void handleResume() {
        Log.v("SDL", "handleResume()");
//        setFocusable(true);
//        setFocusableInTouchMode(true);
//        requestFocus();
//        setOnKeyListener(this);
//        setOnTouchListener(this);
//        enableSensor(Sensor.TYPE_ACCELEROMETER, true);
    }

    public Surface getNativeSurface() {
        Log.v("SDL", "getNativeSurface()");
        return getHolder().getSurface();
    }

    // Called when we have a valid drawing surface
    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        Log.v("SDL", "surfaceCreated()");
        SDLActivity.onNativeSurfaceCreated();
    }

    // Called when we lose the surface
    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        Log.v("SDL", "surfaceDestroyed()");

        // Transition to pause, if needed
        SDLActivity.mNextNativeState = SDLActivity.NativeState.PAUSED;
        SDLActivity.handleNativeState();

        mIsSurfaceReady = false;
        SDLActivity.onNativeSurfaceDestroyed();
    }

    // Called when the surface is resized
    @Override
    public void surfaceChanged(SurfaceHolder holder,
                               int format, int width, int height) {
        Log.v("SDL", "surfaceChanged()");

        if (SDLActivity.mSingleton == null) {
            return;
        }

        mWidth = width;
        mHeight = height;
        int nDeviceWidth = width;
        int nDeviceHeight = height;
        try
        {
            if (Build.VERSION.SDK_INT >= 17) {
                DisplayMetrics realMetrics = new DisplayMetrics();
                mDisplay.getRealMetrics( realMetrics );
                nDeviceWidth = realMetrics.widthPixels;
                nDeviceHeight = realMetrics.heightPixels;
            }
        } catch(Exception ignored) {
            Log.v("SDL", "Something catched");
        }

        synchronized(SDLActivity.getContext()) {
            // In case we're waiting on a size change after going fullscreen, send a notification.
            SDLActivity.getContext().notifyAll();
        }

        Log.v("SDL", "Window size: " + width + "x" + height);
        Log.v("SDL", "Device size: " + nDeviceWidth + "x" + nDeviceHeight);
        SDLActivity.nativeSetScreenResolution(width, height, nDeviceWidth, nDeviceHeight, mDisplay.getRefreshRate());
        SDLActivity.onNativeResize();

        // Prevent a screen distortion glitch,
        // for instance when the device is in Landscape and a Portrait App is resumed.
        boolean skip = false;
//        int requestedOrientation = SDLActivity.mSingleton.getRequestedOrientation();
//
//        if (requestedOrientation == ActivityInfo.SCREEN_ORIENTATION_PORTRAIT || requestedOrientation == ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT) {
//            if (mWidth > mHeight) {
//               skip = true;
//            }
//        } else if (requestedOrientation == ActivityInfo.SCREEN_ORIENTATION_LANDSCAPE || requestedOrientation == ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE) {
//            if (mWidth < mHeight) {
//               skip = true;
//            }
//        }

        // Special Patch for Square Resolution: Black Berry Passport
        if (skip) {
           double min = Math.min(mWidth, mHeight);
           double max = Math.max(mWidth, mHeight);

           if (max / min < 1.20) {
              Log.v("SDL", "Don't skip on such aspect-ratio. Could be a square resolution.");
              skip = false;
           }
        }

        // Don't skip in MultiWindow.
        if (skip) {
            if (Build.VERSION.SDK_INT >= 24) {
//                if (SDLActivity.mSingleton.isInMultiWindowMode()) {
//                    Log.v("SDL", "Don't skip in Multi-Window");
//                    skip = false;
//                }
            }
        }

        if (skip) {
           Log.v("SDL", "Skip .. Surface is not ready.");
           mIsSurfaceReady = false;
           return;
        }

        /* If the surface has been previously destroyed by onNativeSurfaceDestroyed, recreate it here */
        SDLActivity.onNativeSurfaceChanged();

        /* Surface is ready */
        mIsSurfaceReady = true;

        SDLActivity.mNextNativeState = SDLActivity.NativeState.RESUMED;
        SDLActivity.handleNativeState();
    }

    // Key events
    @Override
    public boolean onKey(View v, int keyCode, KeyEvent event) {
        return SDLActivity.handleKeyEvent(v, keyCode, event, null);
    }

    // Touch events
    @Override
    public boolean onTouch(View v, MotionEvent event) {
        return true;
   }

    // Sensor events
    public void enableSensor(int sensortype, boolean enabled) {
        // TODO: This uses getDefaultSensor - what if we have >1 accels?
        if (enabled) {
            mSensorManager.registerListener(this,
                            mSensorManager.getDefaultSensor(sensortype),
                            SensorManager.SENSOR_DELAY_GAME, null);
        } else {
            mSensorManager.unregisterListener(this,
                            mSensorManager.getDefaultSensor(sensortype));
        }
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {
        // TODO
    }

    @Override
    public void onSensorChanged(SensorEvent event) {
        if (event.sensor.getType() == Sensor.TYPE_ACCELEROMETER) {

            // Since we may have an orientation set, we won't receive onConfigurationChanged events.
            // We thus should check here.
            int newOrientation;

            float x, y;
            switch (mDisplay.getRotation()) {
                case Surface.ROTATION_90:
                    x = -event.values[1];
                    y = event.values[0];
                    newOrientation = SDLActivity.SDL_ORIENTATION_LANDSCAPE;
                    break;
                case Surface.ROTATION_270:
                    x = event.values[1];
                    y = -event.values[0];
                    newOrientation = SDLActivity.SDL_ORIENTATION_LANDSCAPE_FLIPPED;
                    break;
                case Surface.ROTATION_180:
                    x = -event.values[0];
                    y = -event.values[1];
                    newOrientation = SDLActivity.SDL_ORIENTATION_PORTRAIT_FLIPPED;
                    break;
                case Surface.ROTATION_0:
                default:
                    x = event.values[0];
                    y = event.values[1];
                    newOrientation = SDLActivity.SDL_ORIENTATION_PORTRAIT;
                    break;
            }

            if (newOrientation != SDLActivity.mCurrentOrientation) {
                SDLActivity.mCurrentOrientation = newOrientation;
                SDLActivity.onNativeOrientationChanged(newOrientation);
            }

            SDLActivity.onNativeAccel(-x / SensorManager.GRAVITY_EARTH,
                                      y / SensorManager.GRAVITY_EARTH,
                                      event.values[2] / SensorManager.GRAVITY_EARTH);


        }
    }

    // Captured pointer events for API 26.
    public boolean onCapturedPointerEvent(MotionEvent event)
    {
        return false;
    }
}
