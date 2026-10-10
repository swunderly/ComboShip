package org.comboship.android;

import android.app.*;
import android.content.ComponentName;
import android.content.Intent;
import android.os.Bundle;
import java.util.List;

/** The launcher starts a shortcut by clearing its task, which would end a running game without
 *  the save warning. This activity has its own task: it hands App options to a running game,
 *  which asks to save first, and otherwise opens setup's App options as before. */
public final class OptionsShortcut extends Activity {
    @Override protected void onCreate(Bundle state) {
        super.onCreate(state);
        if(gameRunning())startActivity(new Intent(this,GameActivity.class).putExtra(MainActivity.MANAGE,true)
            .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK));
        else startActivity(new Intent(this,MainActivity.class).putExtra(MainActivity.MANAGE,true)
            .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK|Intent.FLAG_ACTIVITY_CLEAR_TASK));
        finish();
    }
    private boolean gameRunning() {
        ActivityManager manager=getSystemService(ActivityManager.class);
        if(manager==null)return false;
        boolean process=false,task=false;
        List<ActivityManager.RunningAppProcessInfo> processes=manager.getRunningAppProcesses();
        if(processes!=null)for(ActivityManager.RunningAppProcessInfo info:processes)
            process|=info.processName.equals(getPackageName()+":game");
        for(ActivityManager.AppTask appTask:manager.getAppTasks()) {
            ComponentName top=appTask.getTaskInfo().topActivity;
            task|=top!=null && top.getClassName().equals(GameActivity.class.getName());
        }
        // A task left behind by a killed game process holds no session and is cleared as before.
        return process && task;
    }
}
