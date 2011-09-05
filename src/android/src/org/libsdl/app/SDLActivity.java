package org.libsdl.app;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;
import javax.microedition.khronos.egl.*;

import java.util.zip.*;
import java.io.File;
import java.io.BufferedOutputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.StringReader;

import android.content.res.AssetManager;
import android.app.*;
import android.content.*;
import android.view.*;
import android.os.*;
import android.util.Log;
import android.graphics.*;
import android.text.method.*;
import android.text.*;
import android.media.*;
import android.hardware.*;
import android.content.*;

import android.widget.RelativeLayout;
import android.widget.ImageView;

import android.util.AttributeSet;
import android.util.Xml;
import org.xmlpull.v1.XmlPullParser;

import java.lang.*;

public class SDLActivity extends Activity {

    // Main components
    private static SDLActivity mSingleton;
    private static SDLSurface mSurface;

    // Audio
    private static Thread mAudioThread;
    private static AudioTrack mAudioTrack;

    // Load the .so
    static {
        System.loadLibrary("SDL");
        System.loadLibrary("paintown");
    }

    public static String getDataDirectory(){
        return Environment.getExternalStorageDirectory().getAbsolutePath() + "/paintown";
    }

    /* copy the data bundled in assets to the external data directory */
    private void setupData(Context context){
        File root = new File(getDataDirectory());
        if (root.exists()){
            return;
        }

        Log.v("SDL", "Data directory doesn't exist, creating it: " + getDataDirectory());
        if (!root.mkdirs()){
            Log.v("SDL", "Unable to make data directory");
            return;
        }

        File user = new File(root, "user");
        if (!user.exists()){
            user.mkdirs();
        }

        unzip(root, "data.zip", context);
    }

    /* unzips a file from assets into the given root directory */
    private void unzip(File root, String file, Context context){
        Log.v("SDL", "Writing data to " + root.getAbsolutePath());
        try{
            AssetManager assets = context.getResources().getAssets();
            ZipInputStream zip = new ZipInputStream(assets.open(file));

            ZipEntry entry = zip.getNextEntry();
            while (entry != null){
                String filename = entry.getName();
                if (entry.isDirectory()){
                    File directory = new File(root, filename);
                    directory.mkdirs();
                } else {
                    writeFile(new File(root, filename), entry.getSize(), zip);
                }

                entry = zip.getNextEntry();
            }
        } catch (IOException fail){
            Log.v("SDL", fail.toString());
        }
        Log.v("SDL", "Wrote data");
    }

    private void writeFile(File what, long size, ZipInputStream stream) throws IOException {
        byte[] buffer = new byte[1024];
        int count;
        BufferedOutputStream output = new BufferedOutputStream(new FileOutputStream(what));
        while ((count = stream.read(buffer, 0, buffer.length)) != -1){
            output.write(buffer, 0, count);
        }
        output.close();
    }

    // Setup
    protected void onCreate(Bundle savedInstanceState) {
        //Log.v("SDL", "onCreate()");
        super.onCreate(savedInstanceState);
        
        // So we can call stuff from static callbacks
        mSingleton = this;

        setupData(getApplication());

        // Set up the surface
        mSurface = new SDLSurface(getApplication());
        setContentView(createView(mSurface));
        SurfaceHolder holder = mSurface.getHolder();
        holder.setType(SurfaceHolder.SURFACE_TYPE_GPU);
    }

    class OnScreenPad extends ImageView implements View.OnTouchListener {
        OnScreenPad(Context context){
            super(context);
            setImageResource(R.drawable.pad);
            setOnTouchListener(this);   
        }

        Rect left = new Rect(0, 25, 30, 50);
        Rect right = new Rect(50, 25, 80, 50);
        Rect up = new Rect(30, 0, 50, 25);
        Rect down = new Rect(30, 50, 50, 75);

        private int getKey(int x, int y){
            if (left.contains(x, y)){
                return KeyEvent.KEYCODE_DPAD_LEFT;
            }
            if (right.contains(x, y)){
                return KeyEvent.KEYCODE_DPAD_RIGHT;
            }
            if (up.contains(x, y)){
                return KeyEvent.KEYCODE_DPAD_UP;
            }
            if (down.contains(x, y)){
                return KeyEvent.KEYCODE_DPAD_DOWN;
            }
            return -1;
        }

        public boolean onTouch(View view, MotionEvent event) {
            int action = event.getAction();
            float x = event.getX();
            float y = event.getY();
            float p = event.getPressure();

            Log.v("SDL", "pad " + x + ", " + y + " action " + action);
            int code = getKey((int) x, (int) y);
            if (code != -1){
                if (action == MotionEvent.ACTION_DOWN){
                    SDLActivity.onNativeKeyDown(code);
                } else if (action == MotionEvent.ACTION_UP){
                    SDLActivity.onNativeKeyUp(code);
                }
            }

            return true;
        }
    }

    class OnScreenButtons extends ImageView implements View.OnTouchListener {
        class Point{
            Point(int x, int y){
                this.x = x;
                this.y = y;
            }

            int x, y;

            int radius(){
                return 28;
            }

            boolean contains(int x, int y){
                return distance(this.x, this.y, x, y) <= radius();
            }

            double distance(int x1, int y1, int x2, int y2){
                int xs = x2 - x1;
                int ys = y2 - y1;
                return Math.sqrt(xs * xs + ys * ys);
            }
        }

        OnScreenButtons(Context context){
            super(context);
            setImageResource(R.drawable.buttons);
            setOnTouchListener(this);   
        }

        /* radius = 28
         * button1 = 30, 50
         * button2 = 107, 36
         * button3 = 190, 36
         * button4 = 34, 127
         * button5 = 108, 109
         * button6 = 190, 107
         */
        /* top row, starting from left */
        Point button1 = new Point(30, 50);
        Point button2 = new Point(107, 36);
        Point button3 = new Point(190, 36);
        /* bottom row starting from left */
        Point button4 = new Point(34, 127);
        Point button5 = new Point(108, 109);
        Point button6 = new Point(190, 107);

        private int getKey(int x, int y){
            if (button1.contains(x, y)){
                return KeyEvent.KEYCODE_A;
            }
            if (button2.contains(x, y)){
                return KeyEvent.KEYCODE_S;
            }
            if (button3.contains(x, y)){
                return KeyEvent.KEYCODE_D;
            }
            if (button4.contains(x, y)){
                return KeyEvent.KEYCODE_Z;
            }
            if (button5.contains(x, y)){
                return KeyEvent.KEYCODE_X;
            }
            if (button6.contains(x, y)){
                return KeyEvent.KEYCODE_C;
            }
            return -1;
        }

        public boolean onTouch(View view, MotionEvent event) {
            int action = event.getAction();
            float x = event.getX();
            float y = event.getY();
            float p = event.getPressure();

            Log.v("SDL", "buttons " + x + ", " + y + " action " + action);
            int code = getKey((int) x, (int) y);
            if (code != -1){
                // Log.v("SDL", " button " + code);
                if (action == MotionEvent.ACTION_DOWN){
                    SDLActivity.onNativeKeyDown(code);
                } else if (action == MotionEvent.ACTION_UP){
                    SDLActivity.onNativeKeyUp(code);
                }
            }

            return true;
        }
    }

    private View createView(SDLSurface main){
        Context context = getApplication();
        main.setId(100);
        Log.v("SDL", "Surface id " + main.getId());
        RelativeLayout group = new RelativeLayout(context);
        /*
        RelativeLayout.LayoutParams params0 = new RelativeLayout.LayoutParams(
                RelativeLayout.LayoutParams.WRAP_CONTENT,
                RelativeLayout.LayoutParams.WRAP_CONTENT);
                */
        RelativeLayout.LayoutParams params0 = new RelativeLayout.LayoutParams(
                640, 480);
        params0.addRule(RelativeLayout.CENTER_IN_PARENT);
        group.addView(main, params0);

        /*
        ImageView main = new ImageView(context);
        main.setId(105);
        main.setImageResource(R.drawable.pad);
        RelativeLayout.LayoutParams params9 = new RelativeLayout.LayoutParams(
                RelativeLayout.LayoutParams.WRAP_CONTENT,
                RelativeLayout.LayoutParams.WRAP_CONTENT);
        params9.addRule(RelativeLayout.CENTER_IN_PARENT);
        group.addView(main, params9);
        */

        OnScreenPad pad = new OnScreenPad(context);
        pad.setId(101);
        Log.v("SDL", "Pad id " + pad.getId());
        RelativeLayout.LayoutParams params = new RelativeLayout.LayoutParams(
                RelativeLayout.LayoutParams.WRAP_CONTENT,
                RelativeLayout.LayoutParams.WRAP_CONTENT);
        // params.addRule(RelativeLayout.LEFT_OF, main.getId());
        // params.addRule(RelativeLayout.ALIGN_BOTTOM, main.getId());
        params.addRule(RelativeLayout.ALIGN_PARENT_LEFT);
        params.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
        group.addView(pad, params);

        OnScreenButtons buttons = new OnScreenButtons(context);
        buttons.setId(102);
        RelativeLayout.LayoutParams params1 = new RelativeLayout.LayoutParams(
                RelativeLayout.LayoutParams.WRAP_CONTENT,
                RelativeLayout.LayoutParams.WRAP_CONTENT);
        // params1.addRule(RelativeLayout.RIGHT_OF, pad.getId());
        params1.addRule(RelativeLayout.ALIGN_PARENT_RIGHT);
        params1.addRule(RelativeLayout.ALIGN_PARENT_BOTTOM);
        // params1.addRule(RelativeLayout.RIGHT_OF, main.getId());
        // params1.addRule(RelativeLayout.ALIGN_BOTTOM, main.getId());
        group.addView(buttons, params1);

        group.bringChildToFront(pad);
        group.bringChildToFront(buttons);

        return group;
    }

    // Events
    protected void onPause() {
        //Log.v("SDL", "onPause()");
        super.onPause();
    }

    protected void onResume() {
        //Log.v("SDL", "onResume()");
        super.onResume();
    }

    // Messages from the SDLMain thread
    static int COMMAND_CHANGE_TITLE = 1;

    // Handler for the messages
    Handler commandHandler = new Handler() {
        public void handleMessage(Message msg) {
            if (msg.arg1 == COMMAND_CHANGE_TITLE) {
                setTitle((String)msg.obj);
            }
        }
    };

    // Send a message from the SDLMain thread
    void sendCommand(int command, Object data) {
        Message msg = commandHandler.obtainMessage();
        msg.arg1 = command;
        msg.obj = data;
        commandHandler.sendMessage(msg);
    }

    // C functions we call
    public static native void nativeInit();
    public static native void nativeQuit();
    public static native void onNativeResize(int x, int y, int format);
    public static native void onNativeKeyDown(int keycode);
    public static native void onNativeKeyUp(int keycode);
    public static native void onNativeTouch(int action, float x, 
                                            float y, float p);
    public static native void onNativeAccel(float x, float y, float z);
    public static native void nativeRunAudioThread();

    // Set the SD card path
    public static native void setExternalLocation(String path);
    
    // Java functions called from C

    public static boolean createGLContext(int majorVersion, int minorVersion) {
        return mSurface.initEGL(majorVersion, minorVersion);
    }

    public static void flipBuffers() {
        mSurface.flipEGL();
    }

    public static void setActivityTitle(String title) {
        // Called from SDLMain() thread and can't directly affect the view
        mSingleton.sendCommand(COMMAND_CHANGE_TITLE, title);
    }

    public static Context getContext() {
        return mSingleton;
    }

    public static SDLActivity getActivity(){
        return mSingleton;
    }

    // Audio
    private static Object buf;
    
    public static Object audioInit(int sampleRate, boolean is16Bit, boolean isStereo, int desiredFrames) {
        int channelConfig = isStereo ? AudioFormat.CHANNEL_CONFIGURATION_STEREO : AudioFormat.CHANNEL_CONFIGURATION_MONO;
        int audioFormat = is16Bit ? AudioFormat.ENCODING_PCM_16BIT : AudioFormat.ENCODING_PCM_8BIT;
        int frameSize = (isStereo ? 2 : 1) * (is16Bit ? 2 : 1);
        
        Log.v("SDL", "SDL audio: wanted " + (isStereo ? "stereo" : "mono") + " " + (is16Bit ? "16-bit" : "8-bit") + " " + ((float)sampleRate / 1000f) + "kHz, " + desiredFrames + " frames buffer");
        
        // Let the user pick a larger buffer if they really want -- but ye
        // gods they probably shouldn't, the minimums are horrifyingly high
        // latency already
        desiredFrames = Math.max(desiredFrames, (AudioTrack.getMinBufferSize(sampleRate, channelConfig, audioFormat) + frameSize - 1) / frameSize);
        
        mAudioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, sampleRate,
                channelConfig, audioFormat, desiredFrames * frameSize, AudioTrack.MODE_STREAM);
        
        audioStartThread();
        
        Log.v("SDL", "SDL audio: got " + ((mAudioTrack.getChannelCount() >= 2) ? "stereo" : "mono") + " " + ((mAudioTrack.getAudioFormat() == AudioFormat.ENCODING_PCM_16BIT) ? "16-bit" : "8-bit") + " " + ((float)mAudioTrack.getSampleRate() / 1000f) + "kHz, " + desiredFrames + " frames buffer");
        
        if (is16Bit) {
            buf = new short[desiredFrames * (isStereo ? 2 : 1)];
        } else {
            buf = new byte[desiredFrames * (isStereo ? 2 : 1)]; 
        }
        return buf;
    }
    
    public static void audioStartThread() {
        mAudioThread = new Thread(new Runnable() {
            public void run() {
                mAudioTrack.play();
                nativeRunAudioThread();
            }
        });
        
        // I'd take REALTIME if I could get it!
        mAudioThread.setPriority(Thread.MAX_PRIORITY);
        mAudioThread.start();
    }
    
    public static void audioWriteShortBuffer(short[] buffer) {
        for (int i = 0; i < buffer.length; ) {
            int result = mAudioTrack.write(buffer, i, buffer.length - i);
            if (result > 0) {
                i += result;
            } else if (result == 0) {
                try {
                    Thread.sleep(1);
                } catch(InterruptedException e) {
                    // Nom nom
                }
            } else {
                Log.w("SDL", "SDL audio: error return from write(short)");
                return;
            }
        }
    }
    
    public static void audioWriteByteBuffer(byte[] buffer) {
        for (int i = 0; i < buffer.length; ) {
            int result = mAudioTrack.write(buffer, i, buffer.length - i);
            if (result > 0) {
                i += result;
            } else if (result == 0) {
                try {
                    Thread.sleep(1);
                } catch(InterruptedException e) {
                    // Nom nom
                }
            } else {
                Log.w("SDL", "SDL audio: error return from write(short)");
                return;
            }
        }
    }

    public static void audioQuit() {
        if (mAudioThread != null) {
            try {
                mAudioThread.join();
            } catch(Exception e) {
                Log.v("SDL", "Problem stopping audio thread: " + e);
            }
            mAudioThread = null;

            //Log.v("SDL", "Finished waiting for audio thread");
        }

        if (mAudioTrack != null) {
            mAudioTrack.stop();
            mAudioTrack = null;
        }
    }
}

/**
    Simple nativeInit() runnable
*/
class SDLMain implements Runnable {
    public void run() {
        // Runs SDL_main()
        SDLActivity.setExternalLocation(SDLActivity.getDataDirectory());
        SDLActivity.nativeInit();
        Log.v("SDL", "SDL thread terminated");
        SDLActivity.getActivity().finish();
    }
}


/**
    SDLSurface. This is what we draw on, so we need to know when it's created
    in order to do anything useful. 

    Because of this, that's where we set up the SDL thread
*/
class SDLSurface extends SurfaceView implements SurfaceHolder.Callback, 
    View.OnKeyListener, View.OnTouchListener, SensorEventListener  {

    // This is what SDL runs in. It invokes SDL_main(), eventually
    private Thread mSDLThread;    
    
    // EGL private objects
    private EGLContext  mEGLContext;
    private EGLSurface  mEGLSurface;
    private EGLDisplay  mEGLDisplay;

    // Sensors
    private static SensorManager mSensorManager;

    // Startup    
    public SDLSurface(Context context) {
        super(context);
        getHolder().addCallback(this); 
    
        setFocusable(true);
        setFocusableInTouchMode(true);
        requestFocus();
        setOnKeyListener(this); 
        setOnTouchListener(this);   

        mSensorManager = (SensorManager)context.getSystemService("sensor");  
    }

    // Called when we have a valid drawing surface
    public void surfaceCreated(SurfaceHolder holder) {
        //Log.v("SDL", "surfaceCreated()");

        enableSensor(Sensor.TYPE_ACCELEROMETER, true);
    }

    // Called when we lose the surface
    public void surfaceDestroyed(SurfaceHolder holder) {
        //Log.v("SDL", "surfaceDestroyed()");

        // Send a quit message to the application
        SDLActivity.nativeQuit();

        // Now wait for the SDL thread to quit
        if (mSDLThread != null) {
            try {
                mSDLThread.join();
            } catch(Exception e) {
                Log.v("SDL", "Problem stopping thread: " + e);
            }
            mSDLThread = null;

            //Log.v("SDL", "Finished waiting for SDL thread");
        }

        enableSensor(Sensor.TYPE_ACCELEROMETER, false);
    }

    // Called when the surface is resized
    public void surfaceChanged(SurfaceHolder holder,
                               int format, int width, int height) {
        //Log.v("SDL", "surfaceChanged()");
        Log.v("SDL", "Surface changed: format " + format + " width " + width + " height " + height);
        
        /* Force screen parameters to be a minimum size */
        if (width < 640){
            width = 640;
        }
        if (height < 480){
            height = 480;
        }

        int sdlFormat = 0x85151002; // SDL_PIXELFORMAT_RGB565 by default
        switch (format) {
        case PixelFormat.A_8:
            Log.v("SDL", "pixel format A_8");
            break;
        case PixelFormat.LA_88:
            Log.v("SDL", "pixel format LA_88");
            break;
        case PixelFormat.L_8:
            Log.v("SDL", "pixel format L_8");
            break;
        case PixelFormat.RGBA_4444:
            Log.v("SDL", "pixel format RGBA_4444");
            sdlFormat = 0x85421002; // SDL_PIXELFORMAT_RGBA4444
            break;
        case PixelFormat.RGBA_5551:
            Log.v("SDL", "pixel format RGBA_5551");
            sdlFormat = 0x85441002; // SDL_PIXELFORMAT_RGBA5551
            break;
        case PixelFormat.RGBA_8888:
            Log.v("SDL", "pixel format RGBA_8888");
            sdlFormat = 0x86462004; // SDL_PIXELFORMAT_RGBA8888
            break;
        case PixelFormat.RGBX_8888:
            Log.v("SDL", "pixel format RGBX_8888");
            sdlFormat = 0x86262004; // SDL_PIXELFORMAT_RGBX8888
            break;
        case PixelFormat.RGB_332:
            Log.v("SDL", "pixel format RGB_332");
            sdlFormat = 0x84110801; // SDL_PIXELFORMAT_RGB332
            break;
        case PixelFormat.RGB_565:
            Log.v("SDL", "pixel format RGB_565");
            sdlFormat = 0x85151002; // SDL_PIXELFORMAT_RGB565
            break;
        case PixelFormat.RGB_888:
            Log.v("SDL", "pixel format RGB_888");
            // Not sure this is right, maybe SDL_PIXELFORMAT_RGB24 instead?
            sdlFormat = 0x86161804; // SDL_PIXELFORMAT_RGB888
            break;
        default:
            Log.v("SDL", "pixel format unknown " + format);
            break;
        }
        SDLActivity.onNativeResize(width, height, sdlFormat);

        // Now start up the C app thread
        if (mSDLThread == null) {
            mSDLThread = new Thread(new SDLMain(), "Paintown"); 
            mSDLThread.start();       
        }
    }

    public void onDraw(Canvas canvas){
        /* draw the touch screen here */
    }

    // EGL functions
    public boolean initEGL(int majorVersion, int minorVersion) {
        Log.v("SDL", "Starting up OpenGL ES " + majorVersion + "." + minorVersion);

        try {
            EGL10 egl = (EGL10)EGLContext.getEGL();

            EGLDisplay dpy = egl.eglGetDisplay(EGL10.EGL_DEFAULT_DISPLAY);

            int[] version = new int[2];
            egl.eglInitialize(dpy, version);

            int EGL_OPENGL_ES_BIT = 1;
            int EGL_OPENGL_ES2_BIT = 4;
            int renderableType = 0;
            if (majorVersion == 2) {
                renderableType = EGL_OPENGL_ES2_BIT;
            } else if (majorVersion == 1) {
                renderableType = EGL_OPENGL_ES_BIT;
            }
            int[] configSpec = {
                //EGL10.EGL_DEPTH_SIZE,   16,
                EGL10.EGL_RENDERABLE_TYPE, renderableType,
                EGL10.EGL_NONE
            };
            EGLConfig[] configs = new EGLConfig[1];
            int[] num_config = new int[1];
            if (!egl.eglChooseConfig(dpy, configSpec, configs, 1, num_config) || num_config[0] == 0) {
                Log.e("SDL", "No EGL config available");
                return false;
            }
            EGLConfig config = configs[0];

            EGLContext ctx = egl.eglCreateContext(dpy, config, EGL10.EGL_NO_CONTEXT, null);
            if (ctx == EGL10.EGL_NO_CONTEXT) {
                Log.e("SDL", "Couldn't create context");
                return false;
            }

            EGLSurface surface = egl.eglCreateWindowSurface(dpy, config, this, null);
            if (surface == EGL10.EGL_NO_SURFACE) {
                Log.e("SDL", "Couldn't create surface");
                return false;
            }

            if (!egl.eglMakeCurrent(dpy, surface, surface, ctx)) {
                Log.e("SDL", "Couldn't make context current");
                return false;
            }

            mEGLContext = ctx;
            mEGLDisplay = dpy;
            mEGLSurface = surface;

        } catch(Exception e) {
            Log.v("SDL", e + "");
            for (StackTraceElement s : e.getStackTrace()) {
                Log.v("SDL", s.toString());
            }
        }

        return true;
    }

    // EGL buffer flip
    public void flipEGL() {
        try {
            EGL10 egl = (EGL10)EGLContext.getEGL();

            egl.eglWaitNative(EGL10.EGL_NATIVE_RENDERABLE, null);

            // drawing here

            egl.eglWaitGL();

            egl.eglSwapBuffers(mEGLDisplay, mEGLSurface);

            
        } catch(Exception e) {
            Log.v("SDL", "flipEGL(): " + e);
            for (StackTraceElement s : e.getStackTrace()) {
                Log.v("SDL", s.toString());
            }
        }
    }

    // Key events
    public boolean onKey(View v, int keyCode, KeyEvent event) {

        if (event.getAction() == KeyEvent.ACTION_DOWN) {
            //Log.v("SDL", "key down: " + keyCode);
            SDLActivity.onNativeKeyDown(keyCode);
            return true;
        }
        else if (event.getAction() == KeyEvent.ACTION_UP) {
            //Log.v("SDL", "key up: " + keyCode);
            SDLActivity.onNativeKeyUp(keyCode);
            return true;
        }
        
        return false;
    }

    public boolean inScope(float x, float y){
        Rect out = new Rect();
        getGlobalVisibleRect(out);
        return out.contains((int) x, (int) y);
    }

    // Touch events
    public boolean onTouch(View view, MotionEvent event) {
        int action = event.getAction();
        float x = event.getX();
        float y = event.getY();
        float p = event.getPressure();

        Log.v("SDL", "touch " + x + ", " + y);

        if (action == MotionEvent.ACTION_DOWN){
            SDLActivity.onNativeKeyDown(KeyEvent.KEYCODE_ENTER);
        } else if (action == MotionEvent.ACTION_UP){
            SDLActivity.onNativeKeyUp(KeyEvent.KEYCODE_ENTER);
        }

        // TODO: Anything else we need to pass?        
        SDLActivity.onNativeTouch(action, x, y, p);
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
    
    public void onAccuracyChanged(Sensor sensor, int accuracy) {
        // TODO
    }

    public void onSensorChanged(SensorEvent event) {
        if (event.sensor.getType() == Sensor.TYPE_ACCELEROMETER) {
            SDLActivity.onNativeAccel(event.values[0],
                                           event.values[1],
                                           event.values[2]);
        }
    }

}
