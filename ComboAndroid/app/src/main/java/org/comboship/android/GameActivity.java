package org.comboship.android;
import org.libsdl.app.SDLActivity;
import android.app.AlertDialog;
import android.os.*;
import android.view.*;
import java.io.File;
/** A dedicated process gives the combined native runtime a clean lifetime on every relaunch. */
public final class GameActivity extends SDLActivity {
    private TouchOverlay overlay; private boolean librariesReady, returnToOptions;
    public static native void nativePad(int buttons,float lx,float ly,float rx,float ry,float lt,float rt);
    public static native void nativeMenu();
    public static native void nativeRelease();
    public static native void nativeQuit();
    public static native boolean nativeControlsSuppressed();
    public static native boolean nativeMenuOpen();
    public static native boolean nativeTrackerHitTest(float x,float y);
    public static native int nativeTrackerButtons();
    public static native void nativeToggleTracker(int kind);
    public static native void nativeMenuScale(float scale);
    private final Handler uiHandler=new Handler(Looper.getMainLooper());
    private final Runnable updateControls=new Runnable(){public void run(){
        if(librariesReady && overlay!=null){overlay.setSuppressed(nativeControlsSuppressed(),nativeMenuOpen());overlay.setTrackerButtons(nativeTrackerButtons());overlay.invalidate();}
        uiHandler.postDelayed(this,100);
    }};
    // Called by the native audio backend; keep synthesis at 32 kHz but avoid
    // forcing Android's mixer to resample it on a higher-latency output path.
    public int getAudioOutputSampleRate(){
        android.media.AudioManager manager=getSystemService(android.media.AudioManager.class);
        if(manager!=null){
            try{
                int rate=Integer.parseInt(manager.getProperty(android.media.AudioManager.PROPERTY_OUTPUT_SAMPLE_RATE));
                if(rate>=32000 && rate<=192000)return rate;
            }catch(NumberFormatException ignored){}
        }
        return 48000;
    }
    @Override protected String[] getLibraries(){return new String[]{"SDL2","comboship"};}
    @Override protected String getMainFunction(){return "SDL_main";}
    // SDL supplies desktop window titles; Android keeps the app's display name in Recents.
    @Override public void setTitle(CharSequence title){super.setTitle(getString(R.string.app_name));}
    @Override protected String[] getArguments(){
        try{return new String[]{DataFiles.root(this).getAbsolutePath(),getApplicationInfo().nativeLibraryDir,
                getIntent().getBooleanExtra("without-mods",false)?"safe":"normal",getCacheDir().getAbsolutePath()};}
        catch(java.io.IOException error){throw new IllegalStateException(error.getMessage(),error);}
    }
    @Override public void loadLibraries(){super.loadLibraries();librariesReady=true;}
    @Override protected void onCreate(Bundle saved){
        CrashReports.install(this);
        super.onCreate(saved);
        if(!librariesReady)return;
        immersive();
        View content=SDLActivity.getContentView();
        // SDL and the touch overlay use the same usable rectangle. Keep native HUD/menu
        // elements clear of cutouts and the mandatory gesture area on rotations/folds too.
        if(content!=null){
            content.setOnApplyWindowInsetsListener((view,insets)->{
                int left=0,top=0,right=0,bottom=0;
                if(Build.VERSION.SDK_INT>=30){
                    android.graphics.Insets safe=insets.getInsets(WindowInsets.Type.displayCutout()|
                            WindowInsets.Type.mandatorySystemGestures());
                    left=safe.left;top=safe.top;right=safe.right;bottom=safe.bottom;
                }else if(Build.VERSION.SDK_INT>=28 && insets.getDisplayCutout()!=null){
                    DisplayCutout cutout=insets.getDisplayCutout();
                    left=cutout.getSafeInsetLeft();top=cutout.getSafeInsetTop();
                    right=cutout.getSafeInsetRight();bottom=cutout.getSafeInsetBottom();
                }
                view.setPadding(left,top,right,bottom);
                return insets;
            });
            content.requestApplyInsets();
        }
        if(content instanceof ViewGroup){
            overlay=new TouchOverlay(this,GameActivity::nativeMenu,this::touchOptions);
            ((ViewGroup)content).addView(overlay,new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,ViewGroup.LayoutParams.MATCH_PARENT));
        }
    }
    private void immersive(){
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        getWindow().getDecorView().setSystemUiVisibility(View.SYSTEM_UI_FLAG_FULLSCREEN|View.SYSTEM_UI_FLAG_HIDE_NAVIGATION|
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY|View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN|View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION|View.SYSTEM_UI_FLAG_LAYOUT_STABLE);
        if(Build.VERSION.SDK_INT>=28){WindowManager.LayoutParams p=getWindow().getAttributes();p.layoutInDisplayCutoutMode=WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;getWindow().setAttributes(p);}
        if(Build.VERSION.SDK_INT>=30){getWindow().setDecorFitsSystemWindows(false);WindowInsetsController c=getWindow().getInsetsController();if(c!=null){c.hide(WindowInsets.Type.systemBars());c.setSystemBarsBehavior(WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);}}
    }
    private void touchOptions(){
        if(overlay==null)return;
        String[] options={overlay.isPadVisible()?"Hide touch controls":"Show touch controls","Move controls (pause the game first)",
                "Control sticks","Change control size","Change opacity","Reset current screen layout","Menu size","Face-button layout","Tracker button sizes","App options","Exit game"};
        new AlertDialog.Builder(this).setTitle("Touch controls").setItems(options,(dialog,which)->{
            switch(which){case 0:overlay.toggleVisible();break;case 1:overlay.editLayout();break;case 2:stickOptions();break;
                case 3:overlay.changeScale();break;case 4:overlay.changeOpacity();break;case 5:overlay.resetLayout();break;case 6:menuSize();break;
                case 7:new AlertDialog.Builder(this).setTitle("Face buttons").setItems(new String[]{"ABXY (Nintendo)","BAYX (Xbox)","GameCube"},(d,w)->overlay.setFaceLayout(w)).show();break;
                case 8:trackerButtonSizes();break;case 9:confirmExit(true);break;case 10:confirmExit(false);break;}
        }).setNegativeButton("Close",null).show();
    }
    private void trackerButtonSizes(){
        android.widget.LinearLayout layout=new android.widget.LinearLayout(this);
        layout.setOrientation(android.widget.LinearLayout.VERTICAL);
        int pad=(int)(16*getResources().getDisplayMetrics().density);layout.setPadding(pad,pad,pad,pad);
        String[] labels={"CT — Check Tracker","IT — Item Tracker","HT — Hint Tracker"};
        for(int i=0;i<3;i++){
            final int kind=i;
            android.widget.TextView label=new android.widget.TextView(this);label.setText(labels[i]);layout.addView(label);
            android.widget.SeekBar slider=new android.widget.SeekBar(this);slider.setMax(140);
            slider.setProgress(Math.round(100*overlay.getTrackerButtonScale(i))-60);
            slider.setOnSeekBarChangeListener(new android.widget.SeekBar.OnSeekBarChangeListener(){
                public void onProgressChanged(android.widget.SeekBar bar,int value,boolean user){if(user)overlay.setTrackerButtonScale(kind,(value+60)/100f);}
                public void onStartTrackingTouch(android.widget.SeekBar bar){}
                public void onStopTrackingTouch(android.widget.SeekBar bar){}
            });layout.addView(slider);
        }
        new AlertDialog.Builder(this).setTitle("Tracker button sizes").setView(layout).setPositiveButton("Done",null).show();
    }
    private void stickOptions(){
        String[] modes={"Invisible sticks (touch either side)","Visible fixed sticks","Visible floating left stick"};
        new AlertDialog.Builder(this).setTitle("Control sticks")
                .setSingleChoiceItems(modes,overlay.getStickMode(),(dialog,which)->{overlay.setStickMode(which);dialog.dismiss();})
                .setNegativeButton("Close",null).show();
    }
    private void menuSize(){
        String[] labels={"100%","125%","145% (default)","175%","200%","250%","300%"};
        float[] scales={1f,1.25f,1.45f,1.75f,2f,2.5f,3f};
        new AlertDialog.Builder(this).setTitle("Menu size").setItems(labels,(dialog,which)->nativeMenuScale(scales[which])).show();
    }
    private void confirmExit(boolean options){
        new AlertDialog.Builder(this).setTitle(options?"Open app options?":"Exit combined game?")
                .setMessage("Save in the game before exiting. Unsaved progress is not guaranteed to be preserved.")
                .setPositiveButton(options?"Open options":"Exit",(dialog,which)->{returnToOptions=options;nativeQuit();})
                .setNegativeButton("Keep playing",null).show();
    }
    // The App options launcher shortcut while playing (see OptionsShortcut).
    @Override protected void onNewIntent(android.content.Intent intent){
        super.onNewIntent(intent);
        if(librariesReady && intent.getBooleanExtra(MainActivity.MANAGE,false))confirmExit(true);
    }
    @Override public void onBackPressed(){if(librariesReady)nativeMenu();else super.onBackPressed();}
    @Override public boolean dispatchKeyEvent(KeyEvent event){
        // SDL's surface consumes keyboard Back before Activity.onBackPressed.
        // Let an open IME handle Back first; otherwise use it as the menu key.
        boolean menuKey=event.getKeyCode()==KeyEvent.KEYCODE_BUTTON_SELECT ||
                (event.getKeyCode()==KeyEvent.KEYCODE_BACK && !SDLActivity.isScreenKeyboardShown());
        if(librariesReady && menuKey){
            if(event.getAction()==KeyEvent.ACTION_DOWN && event.getRepeatCount()==0)nativeMenu();
            return true;
        }
        return super.dispatchKeyEvent(event);
    }
    @Override protected void onPause(){uiHandler.removeCallbacks(updateControls);if(overlay!=null)overlay.release();super.onPause();}
    @Override protected void onResume(){super.onResume();if(librariesReady){immersive();uiHandler.removeCallbacks(updateControls);uiHandler.post(updateControls);}}
    @Override public void onWindowFocusChanged(boolean focus){super.onWindowFocusChanged(focus);if(!focus && overlay!=null)overlay.release();if(focus && librariesReady)immersive();}
    @Override protected void onDestroy(){
        boolean finish=isFinishing() && !isChangingConfigurations();
        uiHandler.removeCallbacks(updateControls);if(overlay!=null)overlay.release(); super.onDestroy();
        // SDL has now joined its native thread and released the data lock before setup can edit files.
        if(finish && returnToOptions)startActivity(new android.content.Intent(this,MainActivity.class).putExtra(MainActivity.MANAGE,true));
        // Only this activity's :game process, never setup or the standalone 2S2H app. Also when the
        // activity is only being recreated: SDL has ended this session, and a second one must not
        // start in a process that still holds the first one's native state.
        android.os.Process.killProcess(android.os.Process.myPid());
    }
}
