package org.comboship.android;

import android.app.Activity;
import android.app.Instrumentation;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.os.Bundle;
import android.os.SystemClock;
import android.view.InputDevice;
import android.view.MotionEvent;
import java.util.Arrays;

/** Real Android MotionEvents against the production overlay, with isolated preferences and a pad recorder. */
public final class TouchOverlayInstrumentation extends Instrumentation {
    private static final int W=2280,H=1080;
    private int checks,menuTaps;
    private Context context;
    private SharedPreferences prefs;
    private TouchOverlay view;
    private final Pad pad=new Pad();
    private long downTime;

    private static final class Pad implements TouchOverlay.PadOutput {
        int buttons,releases;float lx,ly,rx,ry;boolean tracker;
        final int[] toggles=new int[3];
        public void toggleTracker(int kind){toggles[kind]++;}
        public boolean capturesTouch(float x,float y){return tracker && x>=.45f && x<=.98f && y>=.05f && y<=.90f;}
        public void update(int buttons,float lx,float ly,float rx,float ry,float lt,float rt){
            this.buttons=buttons;this.lx=lx;this.ly=ly;this.rx=rx;this.ry=ry;
        }
        public void release(){buttons=0;lx=ly=rx=ry=0;releases++;}
    }
    private static final class Point {
        final int id;final float x,y;
        Point(int id,float x,float y){this.id=id;this.x=x;this.y=y;}
    }
    private Point p(int id,float x,float y){return new Point(id,x,y);}
    private void check(boolean result,String message){checks++;if(!result)throw new AssertionError(message);}
    private boolean zero(float f){return Math.abs(f)<.0001f;}
    private boolean neutral(){return pad.buttons==0 && zero(pad.lx) && zero(pad.ly) && zero(pad.rx) && zero(pad.ry);}
    private void create(){
        view=new TouchOverlay(context,()->menuTaps++,()->{},pad);
        view.layout(0,0,W,H);
    }
    private boolean event(int action,int index,Point... points){
        if(action==MotionEvent.ACTION_DOWN)downTime=SystemClock.uptimeMillis();
        MotionEvent.PointerProperties[] properties=new MotionEvent.PointerProperties[points.length];
        MotionEvent.PointerCoords[] coordinates=new MotionEvent.PointerCoords[points.length];
        for(int i=0;i<points.length;i++){
            properties[i]=new MotionEvent.PointerProperties();properties[i].id=points[i].id;
            properties[i].toolType=MotionEvent.TOOL_TYPE_FINGER;
            coordinates[i]=new MotionEvent.PointerCoords();coordinates[i].x=points[i].x;coordinates[i].y=points[i].y;
            coordinates[i].pressure=1;coordinates[i].size=.1f;
        }
        MotionEvent e=MotionEvent.obtain(downTime,SystemClock.uptimeMillis(),action|(index<<MotionEvent.ACTION_POINTER_INDEX_SHIFT),
                points.length,properties,coordinates,0,0,1,1,0,0,InputDevice.SOURCE_TOUCHSCREEN,0);
        try{return view.onTouchEvent(e);}finally{e.recycle();}
    }
    private int[] pixels(){
        Bitmap bitmap=Bitmap.createBitmap(W,H,Bitmap.Config.ARGB_8888);
        view.draw(new Canvas(bitmap));int[] pixels=new int[W*H];bitmap.getPixels(pixels,0,W,0,0,W,H);bitmap.recycle();return pixels;
    }

    @Override public void onCreate(Bundle arguments){super.onCreate(arguments);start();}
    @Override public void onStart(){
        Bundle result=new Bundle();
        try{
            runOnMainSync(()->{
                context=new ContextWrapper(getTargetContext()){
                    @Override public SharedPreferences getSharedPreferences(String name,int mode){
                        return getContext().getSharedPreferences("isolated-"+name,mode);
                    }
                };
                prefs=context.getSharedPreferences("combo-touch",Context.MODE_PRIVATE);
                prefs.edit().clear().putBoolean("floating",false).commit();
                create();
                testDualSticksAndButtons();testReleaseAndMenus();testModesAndPersistence();testMenuMovement();testTrackerTouches();testTrackerButtons();
                view.release();
            });
            result.putString("stream","PASS: "+checks+" touch-overlay checks (real MotionEvents, isolated test preferences).\n");
            finish(Activity.RESULT_OK,result);
        }catch(Throwable failure){
            result.putString("stream","FAIL after "+checks+" checks: "+failure+"\n");finish(Activity.RESULT_CANCELED,result);
        }
    }

    private void testDualSticksAndButtons(){
        check(view.getStickMode()==TouchOverlay.STICKS_INVISIBLE,"Updated and new installs default to invisible sticks");
        int[] idle=pixels();
        float travel=51*context.getResources().getDisplayMetrics().density;
        Point left=p(7,W*.10f,H*.12f),right=p(19,W*.72f,H*.12f);
        check(event(MotionEvent.ACTION_DOWN,0,left),"Upper left can start a stick away from the old circle");
        check(neutral(),"Initial left touch is centered at the finger");
        check(event(MotionEvent.ACTION_POINTER_DOWN,1,left,right),"Right side can be held simultaneously");
        check(neutral(),"Initial right touch is centered at the finger");
        left=p(7,left.x+travel,left.y);right=p(19,right.x,right.y+travel);
        event(MotionEvent.ACTION_MOVE,0,left,right);
        check(pad.lx>.99f && zero(pad.ly) && zero(pad.rx) && pad.ry>.99f,"Independent movement and right stick reach full travel");
        check(Arrays.equals(idle,pixels()),"Both sticks remain invisible while held and moved");
        Point ignored=p(23,W*.30f,H*.10f);
        event(MotionEvent.ACTION_POINTER_DOWN,2,left,right,ignored);
        event(MotionEvent.ACTION_MOVE,0,left,right,p(23,W*.40f,H*.20f));
        check(pad.lx>.99f && pad.ry>.99f,"Extra finger on an occupied side cannot steal or recenter the stick");
        left=p(7,W*.60f,H*.12f);
        event(MotionEvent.ACTION_MOVE,0,left,right,ignored);
        check(pad.lx>.99f && zero(pad.rx) && pad.ry>.99f,"Crossing the middle does not change stick ownership");
        event(MotionEvent.ACTION_POINTER_UP,2,left,right,ignored);
        check(pad.lx>.99f && pad.ry>.99f,"Lifting an unassigned finger preserves both sticks");
        event(MotionEvent.ACTION_POINTER_UP,0,left,right);
        check(zero(pad.lx) && zero(pad.ly) && pad.ry>.99f,"Lifting movement finger resets only movement");
        event(MotionEvent.ACTION_MOVE,0,right);
        check(pad.ry>.99f,"Right pointer still works after its event index changes");
        Point a=p(31,W*.94f,H*.81f);
        event(MotionEvent.ACTION_POINTER_DOWN,1,right,a);
        check((pad.buttons&1)!=0 && pad.ry>.99f,"Face button takes priority and can be held with the right stick");
        event(MotionEvent.ACTION_POINTER_UP,0,right,a);
        check((pad.buttons&1)!=0 && zero(pad.rx) && zero(pad.ry),"Lifting right stick leaves the face button pressed");
        event(MotionEvent.ACTION_UP,0,a);
        check(neutral(),"Final release clears all input");
        left=p(9,W*.08f,H*.52f);
        event(MotionEvent.ACTION_DOWN,0,left);
        check(neutral(),"A new gesture starts at its new location without a jump");
        event(MotionEvent.ACTION_MOVE,0,p(9,left.x,left.y-travel));
        check(pad.ly<-.99f && zero(pad.lx),"Lower left gesture moves upwards");
        event(MotionEvent.ACTION_CANCEL,0,left);
        check(neutral(),"Android cancellation releases sticks");
    }

    private void testReleaseAndMenus(){
        Point left=p(5,W*.1f,H*.12f),moved=p(5,W*.3f,H*.12f);
        event(MotionEvent.ACTION_DOWN,0,left);event(MotionEvent.ACTION_MOVE,0,moved);
        view.setSuppressed(true,true);
        check(neutral(),"Opening a native menu releases gameplay input");
        check(!event(MotionEvent.ACTION_DOWN,0,left),"Native menu receives left-side touches");
        check(!event(MotionEvent.ACTION_DOWN,0,p(6,W*.7f,H*.12f)),"Native menu receives right-side touches");
        view.setSuppressed(false,false);
        Point menu=p(2,W*.5f,H*.1f);
        event(MotionEvent.ACTION_DOWN,0,menu);event(MotionEvent.ACTION_UP,0,menu);
        check(menuTaps==1 && neutral(),"Menu button takes priority over invisible areas");
        event(MotionEvent.ACTION_DOWN,0,left);event(MotionEvent.ACTION_MOVE,0,moved);
        view.toggleVisible();
        check(neutral() && !event(MotionEvent.ACTION_DOWN,0,left),"Hiding all touch controls disables the touch areas");
        view.toggleVisible();
        event(MotionEvent.ACTION_DOWN,0,left);event(MotionEvent.ACTION_MOVE,0,moved);
        view.release();
        check(neutral(),"Pause/focus-loss release cannot leave movement held");
        event(MotionEvent.ACTION_DOWN,0,left);event(MotionEvent.ACTION_MOVE,0,moved);
        view.layout(0,0,W-100,H);
        check(neutral(),"Screen resizing clears fingers before rebuilding controls");
        view.layout(0,0,W,H);
        event(MotionEvent.ACTION_DOWN,0,left);event(MotionEvent.ACTION_MOVE,0,moved);
        view.onDetachedFromWindow();
        check(neutral(),"Removing the overlay releases input");
    }

    private void testModesAndPersistence(){
        Point blank=p(3,W*.1f,H*.12f);
        view.setStickMode(TouchOverlay.STICKS_FIXED);
        check(!event(MotionEvent.ACTION_DOWN,0,blank),"Visible fixed mode leaves empty screen space untouched");
        Point center=p(3,W*.16f,H*.73f);
        check(event(MotionEvent.ACTION_DOWN,0,center) && neutral(),"Fixed movement stick still starts at its drawn center");
        float radius=Math.max(23*context.getResources().getDisplayMetrics().density,H*.052f)*2;
        event(MotionEvent.ACTION_MOVE,0,p(3,center.x+radius,center.y));
        check(pad.lx>.99f,"Fixed stick still reaches full input");
        view.setStickMode(TouchOverlay.STICKS_FLOATING_LEFT);
        check(neutral(),"Changing modes releases held inputs");
        Point floating=p(8,W*.08f,H*.52f);
        check(event(MotionEvent.ACTION_DOWN,0,floating) && neutral(),"Previous floating-left mode remains available");
        event(MotionEvent.ACTION_UP,0,floating);
        check(!event(MotionEvent.ACTION_DOWN,0,p(4,W*.7f,H*.12f)),"Previous mode keeps its fixed right stick");
        view.setStickMode(TouchOverlay.STICKS_INVISIBLE);
        prefs.edit().putFloat("wide.0.0.x",.87f).putFloat("wide.0.0.y",.84f).commit();
        view.release();create();
        check(view.getStickMode()==TouchOverlay.STICKS_INVISIBLE,"Chosen mode survives recreating the overlay");
        Point customA=p(17,W*.87f,H*.84f);
        event(MotionEvent.ACTION_DOWN,0,customA);
        check((pad.buttons&1)!=0 && zero(pad.rx) && zero(pad.ry),"Saved button position is preserved and still takes priority");
        event(MotionEvent.ACTION_UP,0,customA);
        view.setStickMode(TouchOverlay.STICKS_FIXED);view.release();create();
        check(view.getStickMode()==TouchOverlay.STICKS_FIXED,"Opting back into fixed sticks persists");
        view.toggleVisible();view.release();create();
        check(!view.isPadVisible() && !event(MotionEvent.ACTION_DOWN,0,blank),"Existing hidden-pad preference stays respected");
        prefs.edit().clear().commit();
    }

    private void testTrackerTouches(){
        create();pad.tracker=true;
        Point row=p(1,W*.7f,H*.2f);
        check(!event(MotionEvent.ACTION_DOWN,0,row) && neutral(),"Tracker rows bypass invisible stick zones");
        Point a=p(2,W*.94f,H*.81f);
        check(event(MotionEvent.ACTION_DOWN,0,a) && (pad.buttons&1)!=0,"Buttons over the tracker keep working");
        event(MotionEvent.ACTION_UP,0,a);
        check(neutral(),"Releasing a button over the tracker releases it");
        int[] covered=pixels();
        check((covered[(int)(H*.81f)*W+(int)(W*.94f)]>>>24)!=0,"Buttons over the tracker are still drawn");
        Point cLeft=p(4,W*.835f,H*.39f);
        check(event(MotionEvent.ACTION_DOWN,0,cLeft) && pad.rx<-.99f,"C buttons over the tracker can assign items");
        event(MotionEvent.ACTION_UP,0,cLeft);
        Point menu=p(2,W*.5f,H*.1f);int taps=menuTaps;
        event(MotionEvent.ACTION_DOWN,0,menu);event(MotionEvent.ACTION_UP,0,menu);
        check(menuTaps==taps+1 && neutral(),"Menu remains usable even over the tracker");
        Point left=p(3,W*.1f,H*.12f);
        check(event(MotionEvent.ACTION_DOWN,0,left),"Uncovered left stick remains usable beside tracker");
        event(MotionEvent.ACTION_MOVE,0,p(3,W*.3f,H*.12f));
        check(pad.lx>.9f,"Uncovered stick still sends movement");
        event(MotionEvent.ACTION_UP,0,left);
        view.setStickMode(TouchOverlay.STICKS_FIXED);
        check(event(MotionEvent.ACTION_DOWN,0,a) && (pad.buttons&1)!=0,"Fixed controls also keep buttons over the tracker");
        event(MotionEvent.ACTION_UP,0,a);
        check(!event(MotionEvent.ACTION_DOWN,0,row) && neutral(),"Fixed controls leave the rest of the tracker to it");
        view.editLayout();
        check(event(MotionEvent.ACTION_DOWN,0,a) && neutral(),"Layout editor can still move buttons over tracker");
        event(MotionEvent.ACTION_UP,0,a);
        view.release();create();pad.tracker=false;
        event(MotionEvent.ACTION_DOWN,0,a);
        check((pad.buttons&1)!=0,"Closing tracker restores normal button hit testing");
        event(MotionEvent.ACTION_UP,0,a);prefs.edit().clear().commit();
    }

    private void testTrackerButtons(){
        prefs.edit().clear().commit();create();view.setTrackerButtons(7);pad.tracker=true;
        Point ct=p(1,W*.37f,H*.1f),it=p(2,W*.63f,H*.1f),ht=p(3,W*.76f,H*.1f);
        Point[] points={ct,it,ht};
        for(int kind=0;kind<3;kind++){
            int before=pad.toggles[kind];
            check(event(MotionEvent.ACTION_DOWN,0,points[kind]) && neutral(),"Tracker button claims touch without game input");
            check(pad.toggles[kind]==before,"Holding tracker button does not repeatedly toggle");
            event(MotionEvent.ACTION_UP,0,points[kind]);
            check(pad.toggles[kind]==before+1 && neutral(),"CT/IT/HT each toggle exactly their own tracker");
        }
        view.toggleVisible();
        for(int kind=0;kind<3;kind++){
            int before=pad.toggles[kind];event(MotionEvent.ACTION_DOWN,0,points[kind]);event(MotionEvent.ACTION_UP,0,points[kind]);
            check(pad.toggles[kind]==before+1 && neutral(),"Tracker button works with gamepad hidden and over a panel");
        }
        int before=pad.toggles[0];
        event(MotionEvent.ACTION_DOWN,0,ct);event(MotionEvent.ACTION_CANCEL,0,ct);
        check(pad.toggles[0]==before && neutral(),"Cancelling a CT press does not toggle");
        event(MotionEvent.ACTION_DOWN,0,ct);event(MotionEvent.ACTION_MOVE,0,p(1,ct.x+150,ct.y));
        event(MotionEvent.ACTION_UP,0,p(1,ct.x+150,ct.y));
        check(pad.toggles[0]==before,"A drag across CT is not a click");
        view.setSuppressed(true,false);
        check(!event(MotionEvent.ACTION_DOWN,0,ct) && neutral(),"Gameplay suppression hides tracker controls");
        view.setSuppressed(false,false);view.editLayout();
        Point moved=p(1,W*.30f,H*.20f);
        event(MotionEvent.ACTION_DOWN,0,ct);event(MotionEvent.ACTION_MOVE,0,moved);event(MotionEvent.ACTION_UP,0,moved);
        check(pad.toggles[0]==before && neutral(),"Layout dragging CT never opens it or sends input");
        Point done=p(2,W*.5f,H*.1f);event(MotionEvent.ACTION_DOWN,0,done);event(MotionEvent.ACTION_UP,0,done);
        check(zero(prefs.getFloat("wide.0.214.x",0)-.30f) && zero(prefs.getFloat("wide.0.214.y",0)-.20f),"Moved CT position is saved");
        view.setTrackerButtonScale(0,1.75f);view.release();create();view.setTrackerButtons(7);
        check(zero(view.getTrackerButtonScale(0)-1.75f) && zero(view.getTrackerButtonScale(1)-1f),"Independent CT size survives recreation without resizing IT");
        event(MotionEvent.ACTION_DOWN,0,moved);event(MotionEvent.ACTION_UP,0,moved);
        check(pad.toggles[0]==before+1,"Moved CT works after recreating overlay");
        event(MotionEvent.ACTION_DOWN,0,moved);view.setTrackerButtons(6);event(MotionEvent.ACTION_UP,0,moved);
        check(pad.toggles[0]==before+1 && neutral(),"Removing CT while pressed cancels it");
        view.setTrackerButtons(7);view.resetLayout();
        check(zero(view.getTrackerButtonScale(0)-1f) && !prefs.contains("wide.0.214.x"),"Reset restores tracker size and position");
        view.release();pad.tracker=false;prefs.edit().clear().commit();
    }

    private void testMenuMovement(){
        create();view.setStickMode(TouchOverlay.STICKS_FIXED);view.editLayout();
        int taps=menuTaps;
        Point oldMenu=p(2,W*.5f,H*.1f),movedMenu=p(2,W*.62f,H*.18f);
        event(MotionEvent.ACTION_DOWN,0,oldMenu);
        event(MotionEvent.ACTION_MOVE,0,movedMenu);
        event(MotionEvent.ACTION_UP,0,movedMenu);
        check(!prefs.contains("wide.0.202.x") && menuTaps==taps && neutral(),
                "Dragging Menu does not save, open a menu, or send game input");
        // The editor must remain active after releasing a dragged Done button.
        Point a=p(3,W*.94f,H*.81f),movedA=p(3,W*.80f,H*.50f);
        event(MotionEvent.ACTION_DOWN,0,a);event(MotionEvent.ACTION_MOVE,0,movedA);event(MotionEvent.ACTION_UP,0,movedA);
        check(neutral() && !prefs.contains("wide.0.0.x"),"Other buttons remain editable after moving Menu");
        // Small natural finger jitter still counts as a tap to finish editing.
        event(MotionEvent.ACTION_DOWN,0,movedMenu);
        event(MotionEvent.ACTION_MOVE,0,p(2,movedMenu.x+1,movedMenu.y+1));
        event(MotionEvent.ACTION_UP,0,movedMenu);
        check(zero(prefs.getFloat("wide.0.202.x",0)-.62f) && zero(prefs.getFloat("wide.0.202.y",0)-.18f)
                && zero(prefs.getFloat("wide.0.0.x",0)-.80f) && menuTaps==taps,
                "Tapping moved Done saves both Menu and the other button without opening settings");
        view.release();create();
        event(MotionEvent.ACTION_DOWN,0,movedMenu);event(MotionEvent.ACTION_UP,0,movedMenu);
        check(menuTaps==++taps && neutral(),"Moved Menu works after recreating the overlay");
        check(!event(MotionEvent.ACTION_DOWN,0,oldMenu),"Old Menu position is no longer a Menu hit target");
        view.toggleVisible();
        event(MotionEvent.ACTION_DOWN,0,movedMenu);event(MotionEvent.ACTION_UP,0,movedMenu);
        check(menuTaps==++taps && neutral(),"Moved Menu stays accessible when the rest of the pad is hidden");
        view.toggleVisible();view.editLayout();
        // A final up sample can be the first movement sample on some touch streams.
        Point finalOnly=p(2,W*.65f,H*.30f);
        event(MotionEvent.ACTION_DOWN,0,movedMenu);event(MotionEvent.ACTION_UP,0,finalOnly);
        check(zero(prefs.getFloat("wide.0.202.x",0)-.62f) && menuTaps==taps,
                "Movement only in the final touch sample is a drag, not an accidental Done tap");
        event(MotionEvent.ACTION_DOWN,0,finalOnly);event(MotionEvent.ACTION_UP,0,finalOnly);
        check(zero(prefs.getFloat("wide.0.202.x",0)-.65f),"Final-sample drag position saves on a subsequent tap");
        view.editLayout();
        // Move onto A: Menu must be drawn and hit-tested above it so Done remains reachable.
        event(MotionEvent.ACTION_DOWN,0,finalOnly);event(MotionEvent.ACTION_MOVE,0,p(2,movedA.x,movedA.y));
        event(MotionEvent.ACTION_UP,0,p(2,movedA.x,movedA.y));
        event(MotionEvent.ACTION_DOWN,0,p(2,movedA.x,movedA.y));event(MotionEvent.ACTION_UP,0,p(2,movedA.x,movedA.y));
        event(MotionEvent.ACTION_DOWN,0,p(2,movedA.x,movedA.y));event(MotionEvent.ACTION_UP,0,p(2,movedA.x,movedA.y));
        check(menuTaps==++taps && neutral(),"Menu remains reachable when it overlaps another button");
        view.editLayout();
        Point edge=p(2,-100,-100);
        event(MotionEvent.ACTION_DOWN,0,p(2,movedA.x,movedA.y));event(MotionEvent.ACTION_MOVE,0,edge);
        event(MotionEvent.ACTION_UP,0,edge);
        float radius=Math.max(23*context.getResources().getDisplayMetrics().density,H*.052f)*1.2f;
        Point clamped=p(2,radius,radius);
        event(MotionEvent.ACTION_DOWN,0,clamped);event(MotionEvent.ACTION_UP,0,clamped);
        check(zero(prefs.getFloat("wide.0.202.x",0)-radius/W) && zero(prefs.getFloat("wide.0.202.y",0)-radius/H),
                "Dragging beyond an edge keeps the full Menu button on screen and tappable");
        view.editLayout();
        event(MotionEvent.ACTION_DOWN,0,clamped);event(MotionEvent.ACTION_MOVE,0,movedMenu);
        event(MotionEvent.ACTION_CANCEL,0,movedMenu);
        check(menuTaps==taps && zero(prefs.getFloat("wide.0.202.x",0)-radius/W) && neutral(),
                "A canceled Menu drag does not save or activate Menu");
        view.release();create();view.setFaceLayout(1);
        event(MotionEvent.ACTION_DOWN,0,oldMenu);event(MotionEvent.ACTION_UP,0,oldMenu);
        check(menuTaps==++taps,"A different face layout retains its own Menu position");
        view.setFaceLayout(0);
        event(MotionEvent.ACTION_DOWN,0,clamped);event(MotionEvent.ACTION_UP,0,clamped);
        check(menuTaps==++taps,"Returning to the original layout restores its saved Menu position");
        view.resetLayout();
        event(MotionEvent.ACTION_DOWN,0,oldMenu);event(MotionEvent.ACTION_UP,0,oldMenu);
        check(!prefs.contains("wide.0.202.x") && menuTaps==++taps,"Reset layout returns Menu to its default position");
        prefs.edit().clear().commit();
    }
}
