package org.comboship.android;

import android.app.ActivityManager;
import android.app.ApplicationExitInfo;
import android.content.Context;
import android.os.Build;
import android.util.Log;
import java.io.*;
import java.nio.charset.StandardCharsets;

/** Local diagnostics, following the Android exit-report approach in the user's 2S2H fork. */
final class CrashReports {
    private static boolean installed;
    private CrashReports() {}
    private static File directory(Context context) {
        try {return DataFiles.root(context);}catch(IOException error){return context.getFilesDir();}
    }
    static synchronized void install(Context context) {
        if(installed)return;
        final Context app=context.getApplicationContext();
        final Thread.UncaughtExceptionHandler previous=Thread.getDefaultUncaughtExceptionHandler();
        Thread.setDefaultUncaughtExceptionHandler((thread,error)->{
            try(PrintWriter out=writer(new File(directory(app),"java-crash.txt"))){
                header(out);out.println("Thread: "+thread.getName());error.printStackTrace(out);
            }catch(IOException ignored){}
            if(previous!=null)previous.uncaughtException(thread,error);
            else android.os.Process.killProcess(android.os.Process.myPid());
        });
        installed=true;
    }
    static void capturePreviousExit(Context context) {
        if(Build.VERSION.SDK_INT<30)return;
        try {
            ActivityManager manager=context.getSystemService(ActivityManager.class);
            if(manager==null)return;
            android.content.SharedPreferences prefs=context.getSharedPreferences("combo-crashes",Context.MODE_PRIVATE);
            long timestamp=prefs.getLong("last-exit",0);
            for(ApplicationExitInfo exit:manager.getHistoricalProcessExitReasons(null,0,10)){
                int reason=exit.getReason();
                if(exit.getTimestamp()<=timestamp || !exit.getProcessName().equals(context.getPackageName()+":game"))continue;
                if(reason!=ApplicationExitInfo.REASON_CRASH && reason!=ApplicationExitInfo.REASON_CRASH_NATIVE &&
                        reason!=ApplicationExitInfo.REASON_ANR && reason!=ApplicationExitInfo.REASON_INITIALIZATION_FAILURE &&
                        reason!=ApplicationExitInfo.REASON_LOW_MEMORY)continue;
                File root=directory(context);
                try(PrintWriter out=writer(new File(root,"android-last-exit.txt"))){
                    header(out);out.println("Time: "+exit.getTimestamp());out.println("Reason: "+reason);
                    out.println("Status: "+exit.getStatus());out.println("Description: "+exit.getDescription());
                    out.println("PSS / RSS kB: "+exit.getPss()+" / "+exit.getRss());
                }
                if(Build.VERSION.SDK_INT>=31){
                    try(InputStream in=exit.getTraceInputStream()){
                        if(in!=null)try(OutputStream out=new FileOutputStream(new File(root,"android-last-exit-trace.bin"))){
                            byte[] buffer=new byte[8192];int remaining=2*1024*1024,count;
                            while(remaining>0 && (count=in.read(buffer,0,Math.min(buffer.length,remaining)))!=-1){
                                out.write(buffer,0,count);remaining-=count;
                            }
                        }
                    }
                }
                prefs.edit().putLong("last-exit",exit.getTimestamp()).apply();break;
            }
        }catch(IOException|RuntimeException error){Log.w("ComboShip","Previous-exit report unavailable",error);}
    }
    private static PrintWriter writer(File file)throws IOException{
        return new PrintWriter(new OutputStreamWriter(new FileOutputStream(file),StandardCharsets.UTF_8));
    }
    private static void header(PrintWriter out){
        out.println("ComboShip Android "+BuildConfig.VERSION_NAME);
        out.println("Android "+Build.VERSION.RELEASE+" (API "+Build.VERSION.SDK_INT+")");
        out.println("Device: "+Build.MANUFACTURER+" "+Build.MODEL);
    }
}
