package org.comboship.android;
import android.content.*;
import android.graphics.*;
import android.os.*;
import android.view.*;
import java.util.*;
/** Multi-pointer N64 controls; unclaimed touches continue to SDL's surface/ImGui. */
public final class TouchOverlay extends View {
    static final int STICKS_INVISIBLE=0, STICKS_FIXED=1, STICKS_FLOATING_LEFT=2;
    private static final int LEFT=200, RIGHT=201, MENU=202, Z=203, R=204, CU=210, CD=211, CL=212, CR=213, CT=214, IT=215, HT=216;
    interface PadOutput {
        void update(int buttons,float lx,float ly,float rx,float ry,float lt,float rt);
        void release();
        default boolean capturesTouch(float x,float y){return false;}
        default void toggleTracker(int kind){}
    }
    private static final class Control {
        final int id; final String label; float x,y,r;
        Control(int id,String label,float x,float y,float r){this.id=id;this.label=label;this.x=x;this.y=y;this.r=r;}
    }
    private static final class Finger {
        final Control control; final float originX,originY,controlX,controlY; float x,y; boolean longPress,dragging;
        Finger(Control c,float x,float y){control=c;originX=x;originY=y;controlX=c.x;controlY=c.y;this.x=x;this.y=y;}
    }
    private final List<Control> controls=new ArrayList<>();
    private final Map<Integer,Finger> fingers=new HashMap<>();
    private final Paint paint=new Paint(Paint.ANTI_ALIAS_FLAG);
    private final Handler handler=new Handler(Looper.getMainLooper());
    private final SharedPreferences preferences;
    private final Runnable menu,options;
    private final PadOutput output;
    private final int touchSlop;
    private boolean showPad,editing,suppressed,menuHidden;
    private float opacity,scale;
    private String profile="wide";
    private int faceLayout,stickMode,trackerButtons;
    public TouchOverlay(Context context,Runnable menu,Runnable options){
        this(context,menu,options,new PadOutput(){
            public void update(int buttons,float lx,float ly,float rx,float ry,float lt,float rt){
                GameActivity.nativePad(buttons,lx,ly,rx,ry,lt,rt);
            }
            public void release(){GameActivity.nativeRelease();}
            public boolean capturesTouch(float x,float y){return GameActivity.nativeTrackerHitTest(x,y);}
            public void toggleTracker(int kind){GameActivity.nativeToggleTracker(kind);}
        });
    }
    TouchOverlay(Context context,Runnable menu,Runnable options,PadOutput output){
        super(context);this.menu=menu;this.options=options;
        this.output=output;
        touchSlop=ViewConfiguration.get(context).getScaledTouchSlop();
        preferences=context.getSharedPreferences("combo-touch",Context.MODE_PRIVATE);
        showPad=preferences.getBoolean("show",true);
        stickMode=preferences.getInt("stick-mode",STICKS_INVISIBLE);
        if(stickMode<STICKS_INVISIBLE || stickMode>STICKS_FLOATING_LEFT)stickMode=STICKS_INVISIBLE;
        opacity=clamp(preferences.getFloat("opacity",.48f),0f,1f);scale=preferences.getFloat("scale",1f);
        faceLayout=preferences.getInt("face-layout",0);
        setContentDescription("Touch gamepad. Drag the left side to move and the right side for the right stick. Tap Menu for game settings; hold Menu for touch options.");setFocusable(false);
    }
    private void build(){
        if(getWidth()==0 || getHeight()==0)return;
        release();controls.clear();profile=(getWidth()>getHeight()*1.65f?"wide":"fold")+"."+faceLayout;
        float unit=Math.min(getWidth(),getHeight());
        float r=Math.max(23*getResources().getDisplayMetrics().density,unit*.052f)*scale;
        add(LEFT,"Move",.16f,.73f,r*2.0f);add(RIGHT,"Camera",.64f,.77f,r*1.5f);
        if(faceLayout==2){
            add(0,"A",.88f,.86f,r*1.2f);add(1,"B",.77f,.91f,r*.8f);
            add(2,"X",.96f,.73f,r*.8f);add(3,"Y",.84f,.66f,r*.8f);
        }else if(faceLayout==1){
            add(0,"A",.86f,.91f,r);add(1,"B",.94f,.81f,r);
            add(2,"X",.78f,.81f,r);add(3,"Y",.86f,.71f,r);
        }else{
            add(0,"A",.94f,.81f,r);add(1,"B",.86f,.91f,r);
            add(2,"X",.86f,.71f,r);add(3,"Y",.78f,.81f,r);
        }
        add(9,"L",.18f,.30f,r);add(Z,"Z",.07f,.30f,r);add(R,"R",.91f,.16f,r);
        add(6,"Start",.48f,.87f,r*.95f);
        add(11,"↑",.35f,.61f,r*.76f);add(12,"↓",.35f,.79f,r*.76f);
        add(13,"←",.30f,.70f,r*.76f);add(14,"→",.40f,.70f,r*.76f);
        add(CU,"C↑",.90f,.29f,r*.80f);add(CD,"C↓",.90f,.49f,r*.80f);
        add(CL,"C←",.835f,.39f,r*.80f);add(CR,"C→",.965f,.39f,r*.80f);
        if((trackerButtons&1)!=0)add(CT,"CT",.37f,.10f,r);
        if((trackerButtons&2)!=0)add(IT,"IT",.63f,.10f,r);
        if((trackerButtons&4)!=0)add(HT,"HT",.76f,.10f,r);
        // Draw and hit-test Menu above the other controls so it stays reachable after moving it.
        add(MENU,editing?"Done":"Menu",.5f,.10f,r*1.2f);invalidate();
    }
    private void add(int id,String label,float x,float y,float r){
        if(id>=CT && id<=HT)r*=getTrackerButtonScale(id-CT);
        r=Math.min(r,Math.min(getWidth(),getHeight())*.24f);
        Control c=new Control(id,label,x,y,r);
        c.x=preferences.getFloat(profile+"."+id+".x",x);c.y=preferences.getFloat(profile+"."+id+".y",y);
        c.x=clamp(c.x,r/getWidth(),1-r/getWidth());c.y=clamp(c.y,r/getHeight(),1-r/getHeight());controls.add(c);
    }
    @Override protected void onSizeChanged(int w,int h,int oldw,int oldh){super.onSizeChanged(w,h,oldw,oldh);build();}
    @Override protected void onDraw(Canvas canvas){
        super.onDraw(canvas);
        for(Control c:controls){
            if(!editing && !isUtility(c) && output.capturesTouch(c.x,c.y))continue;
            if(c.id==MENU && menuHidden && !editing)continue;
            if(!editing && c.id!=MENU && (suppressed || (!showPad && !isUtility(c))))continue;
            if(isStick(c) && stickMode==STICKS_INVISIBLE && !editing)continue;
            float x=c.x*getWidth(),y=c.y*getHeight();boolean pressed=false;
            for(Finger finger:fingers.values()) if(finger.control==c){
                pressed=true;if(isFloating(c) && !editing){x=finger.originX;y=finger.originY;}break;
            }
            paint.setStyle(Paint.Style.FILL);paint.setColor(Color.BLACK);paint.setAlpha((int)(opacity*170));canvas.drawCircle(x,y,c.r,paint);
            paint.setStyle(Paint.Style.STROKE);paint.setStrokeWidth(editing?4:2);paint.setColor(Color.WHITE);paint.setAlpha((int)(255*opacity));canvas.drawCircle(x,y,c.r,paint);
            if(c.id==LEFT || c.id==RIGHT){
                float dx=0,dy=0;
                for(Finger f:fingers.values())if(f.control==c){float[] a=stick(f);dx=a[0]*c.r*.55f;dy=a[1]*c.r*.55f;}
                paint.setStyle(Paint.Style.FILL);paint.setAlpha((int)(opacity*190));canvas.drawCircle(x+dx,y+dy,c.r*.36f,paint);
            }
            paint.setStyle(Paint.Style.FILL);paint.setAlpha(pressed?255:(int)(opacity*255));
            paint.setTextSize(Math.min(c.r*.66f,18*getResources().getDisplayMetrics().scaledDensity));paint.setTextAlign(Paint.Align.CENTER);
            canvas.drawText(c.label,x,y-(paint.ascent()+paint.descent())/2,paint);
        }
        if(editing){paint.setColor(Color.WHITE);paint.setAlpha(255);paint.setTextSize(16*getResources().getDisplayMetrics().scaledDensity);paint.setTextAlign(Paint.Align.CENTER);
            canvas.drawText("Drag any button, including Done. Tap Done to save.",getWidth()*.5f,getHeight()*.22f,paint);}
    }
    private Control hit(float x,float y){
        boolean tracker=!editing && output.capturesTouch(x/getWidth(),y/getHeight());
        for(int i=controls.size()-1;i>=0;i--){Control c=controls.get(i);
            if(tracker && !isUtility(c))continue;
            if(c.id==MENU && menuHidden && !editing)continue;
            if(!editing && c.id!=MENU && (suppressed || (!showPad && !isUtility(c))))continue;
            if(isStick(c) && stickMode==STICKS_INVISIBLE && !editing)continue;
            if(Math.hypot(x-c.x*getWidth(),y-c.y*getHeight())<=c.r*1.1f)
                return isStick(c) && isHeld(c)?null:c;
        }
        // Buttons above get first claim. Otherwise a finger owns its starting side until lift,
        // even if it crosses the middle or a second finger lands on the same side.
        if(showPad && !suppressed && !editing && !tracker){
            int id=-1;
            if(stickMode==STICKS_INVISIBLE)id=x<getWidth()*.5f?LEFT:RIGHT;
            else if(stickMode==STICKS_FLOATING_LEFT && x<getWidth()*.43f && y>getHeight()*.35f)id=LEFT;
            for(Control c:controls)if(c.id==id && !isHeld(c))return c;
        }
        return null;
    }
    private static boolean isUtility(Control c){return c.id==MENU || (c.id>=CT && c.id<=HT);}
    private static boolean isStick(Control c){return c.id==LEFT || c.id==RIGHT;}
    private boolean isFloating(Control c){return isStick(c) &&
            (stickMode==STICKS_INVISIBLE || (stickMode==STICKS_FLOATING_LEFT && c.id==LEFT));}
    private boolean isHeld(Control c){for(Finger f:fingers.values())if(f.control==c)return true;return false;}
    private void moveFinger(Finger f,float x,float y){
        f.x=x;f.y=y;
        if(editing && (f.dragging || Math.hypot(x-f.originX,y-f.originY)>touchSlop)){
            f.dragging=true;
            // Preserve the grab offset and keep the full button within the screen.
            float marginX=f.control.r/getWidth(),marginY=f.control.r/getHeight();
            f.control.x=clamp(f.controlX+(x-f.originX)/getWidth(),marginX,1-marginX);
            f.control.y=clamp(f.controlY+(y-f.originY)/getHeight(),marginY,1-marginY);
        }
    }
    @Override public boolean onTouchEvent(MotionEvent event){
        int action=event.getActionMasked(),index=event.getActionIndex(),id=event.getPointerId(index);
        if(action==MotionEvent.ACTION_DOWN || action==MotionEvent.ACTION_POINTER_DOWN){
            Control c=hit(event.getX(index),event.getY(index));if(c==null)return !fingers.isEmpty();
            Finger f=new Finger(c,event.getX(index),event.getY(index));fingers.put(id,f);
            if(c.id==MENU && !editing)handler.postDelayed(()->{if(fingers.get(id)==f){f.longPress=true;release();options.run();}},550);
        }else if(action==MotionEvent.ACTION_MOVE){
            for(int i=0;i<event.getPointerCount();i++){
                Finger f=fingers.get(event.getPointerId(i));if(f!=null)moveFinger(f,event.getX(i),event.getY(i));
            }
        }else if(action==MotionEvent.ACTION_UP || action==MotionEvent.ACTION_POINTER_UP){
            Finger f=fingers.remove(id);
            if(f!=null)moveFinger(f,event.getX(index),event.getY(index));
            if(f!=null && f.control.id==MENU && !f.longPress && !f.dragging){
                if(editing){saveLayout();editing=false;build();}else menu.run();performClick();
            }
            if(f!=null && f.control.id>=CT && f.control.id<=HT && !editing && !f.dragging &&
                    Math.hypot(f.x-f.originX,f.y-f.originY)<=touchSlop &&
                    Math.hypot(f.x-f.control.x*getWidth(),f.y-f.control.y*getHeight())<=f.control.r*1.1f){
                output.toggleTracker(f.control.id-CT);performClick();
            }
        }else if(action==MotionEvent.ACTION_CANCEL){release();return true;}
        publish();invalidate();return true;
    }
    @Override public boolean performClick(){super.performClick();return true;}
    private float[] stick(Finger f){
        float x=isFloating(f.control)?f.originX:f.control.x*getWidth();
        float y=isFloating(f.control)?f.originY:f.control.y*getHeight();
        // Match 2S2H's full-stick travel for the invisible touch areas.
        float radius=stickMode==STICKS_INVISIBLE?51f*getResources().getDisplayMetrics().density*scale:f.control.r;
        float dx=(f.x-x)/radius,dy=(f.y-y)/radius;float length=(float)Math.hypot(dx,dy);
        if(length<.10f)return new float[]{0,0};if(length>1){dx/=length;dy/=length;}return new float[]{dx,dy};
    }
    private static float clamp(float f,float min,float max){return Math.max(min,Math.min(max,f));}
    private void publish(){
        int buttons=0;float lx=0,ly=0,rx=0,ry=0,lt=0,rt=0;
        if(!editing)for(Finger f:fingers.values()){
            int id=f.control.id;
            if(id==LEFT){float[] a=stick(f);lx=a[0];ly=a[1];}
            else if(id==RIGHT){float[] a=stick(f);rx=a[0];ry=a[1];}
            else if(id==Z)lt=1;else if(id==R)rt=1;else if(id<21)buttons|=1<<id;
        }
        // Explicit C buttons override the camera's shared default right stick.
        if(!editing)for(Finger f:fingers.values()){
            if(f.control.id==CU)ry=-1;else if(f.control.id==CD)ry=1;else if(f.control.id==CL)rx=-1;else if(f.control.id==CR)rx=1;
        }
        output.update(buttons,lx,ly,rx,ry,lt,rt);
    }
    public void release(){fingers.clear();handler.removeCallbacksAndMessages(null);output.release();invalidate();}
    @Override protected void onDetachedFromWindow(){release();super.onDetachedFromWindow();}
    public void toggleVisible(){showPad=!showPad;preferences.edit().putBoolean("show",showPad).apply();release();invalidate();}
    public int getStickMode(){return stickMode;}
    public void setStickMode(int mode){
        stickMode=Math.max(STICKS_INVISIBLE,Math.min(STICKS_FLOATING_LEFT,mode));
        preferences.edit().putInt("stick-mode",stickMode).apply();release();
    }
    public void setFaceLayout(int layout){faceLayout=Math.max(0,Math.min(2,layout));preferences.edit().putInt("face-layout",faceLayout).apply();build();}
    public void setSuppressed(boolean value,boolean hideMenu){
        if(suppressed!=value || menuHidden!=hideMenu){suppressed=value;menuHidden=hideMenu;release();invalidate();}
    }
    public boolean isPadVisible(){return showPad;}
    public void editLayout(){showPad=true;editing=true;build();}
    public void setTrackerButtons(int mask){
        mask&=7;if(mask!=trackerButtons){trackerButtons=mask;build();}
    }
    public int getTrackerButtons(){return trackerButtons;}
    public float getTrackerButtonScale(int kind){
        if(kind<0 || kind>2)return 1f;
        return clamp(preferences.getFloat(profile+"."+(CT+kind)+".size",1f),.6f,2f);
    }
    public void setTrackerButtonScale(int kind,float value){
        if(kind<0 || kind>2)return;
        preferences.edit().putFloat(profile+"."+(CT+kind)+".size",clamp(value,.6f,2f)).apply();build();
    }
    // Fixed steps that include the defaults (1 and .48), so cycling always comes back to them.
    private static final float[] SCALES={.8f,.9f,1f,1.15f,1.3f,1.45f}, OPACITIES={.25f,.35f,.48f,.65f,.85f,1f};
    private static float nextStep(float[] steps,float value){
        int nearest=0; // Values saved by the old step-and-wrap cycle snap to the closest step.
        for(int i=1;i<steps.length;i++)if(Math.abs(steps[i]-value)<Math.abs(steps[nearest]-value))nearest=i;
        return steps[(nearest+1)%steps.length];
    }
    public void changeScale(){scale=nextStep(SCALES,scale);preferences.edit().putFloat("scale",scale).apply();build();}
    public void changeOpacity(){opacity=nextStep(OPACITIES,opacity);preferences.edit().putFloat("opacity",opacity).apply();invalidate();}
    private void saveLayout(){
        SharedPreferences.Editor editor=preferences.edit();
        for(Control c:controls){editor.putFloat(profile+"."+c.id+".x",c.x);editor.putFloat(profile+"."+c.id+".y",c.y);}editor.apply();
    }
    public void resetLayout(){
        SharedPreferences.Editor editor=preferences.edit();
        for(Control c:controls){editor.remove(profile+"."+c.id+".x");editor.remove(profile+"."+c.id+".y");editor.remove(profile+"."+c.id+".size");}editor.apply();build();
    }
}
