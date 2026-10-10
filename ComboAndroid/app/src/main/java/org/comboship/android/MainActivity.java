package org.comboship.android;

import android.app.*;
import android.content.*;
import android.database.Cursor;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.net.Uri;
import android.os.Bundle;
import android.os.Build;
import android.provider.DocumentsContract;
import android.provider.OpenableColumns;
import android.provider.Settings;
import android.view.*;
import android.widget.*;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowCompat;
import androidx.core.view.WindowInsetsCompat;
import java.io.*;
import java.util.List;
import java.util.concurrent.*;

/** First run asks for the two games; maintenance tools live in App options. */
public final class MainActivity extends Activity {
    static final String MANAGE="app-options";
    private static final int OOT=10, MM=11, MOD_OOT=12, MOD_MM=13, BACKUP=14, FILE_ACCESS=15;
    private static final int BACKGROUND=0xFF0E1922, PANEL=0xFF192A36, BORDER=0xFF2D4352;
    private static final int TEXT=0xFFF5F4EF, MUTED=0xFFAABDC8, GOLD=0xFFE8C77A, GREEN=0xFF95D8B5;
    private final ExecutorService worker=Executors.newSingleThreadExecutor();
    private TextView step, heading, description, status, ootState, mmState, ootNumber, mmNumber, fileLocation;
    private Button primary, options;
    private LinearLayout gameList;
    private ProgressBar progress;
    private File root, pendingStorage;
    private boolean busy, supportReady, launchAfterImport, managing, launched, storageAccessRequired;
    private String notice="", workLabel="";
    private Uri pendingDocument;
    private int pendingRequest;

    @Override public void onCreate(Bundle state) {
        CrashReports.install(this);
        super.onCreate(state);
        removeLegacyGameTasks();
        managing=getIntent().getBooleanExtra(MANAGE,false);
        if(state!=null) {
            launchAfterImport=state.getBoolean("launch-after-import");
            String pending=state.getString("pending-document");
            if(pending!=null)pendingDocument=Uri.parse(pending);
            pendingRequest=state.getInt("pending-request");
            String storage=state.getString("pending-storage");
            if(storage!=null)pendingStorage=new File(storage);
        }
        buildScreen();beginStorage();
    }

    private void beginStorage() {
        if(busy || launched)return;
        try {
            storageAccessRequired=StorageLocations.needsPublicAccess(this) && !StorageLocations.hasPublicAccess(this);
            if(storageAccessRequired){root=null;supportReady=false;refresh();return;}
        }catch(IOException error){showError(error);return;}
        execute("Preparing your ComboShip folder…",()->{
            root=StorageLocations.initialize(this);
            try(DataLock ignored=new DataLock(root)) {
                // Only an older APK writes the versionCode key, so finding it means that build has installed its own files since.
                supportReady=!getPreferences(MODE_PRIVATE).contains(legacySupportKey()) &&
                    DataFiles.supportVersion(this).equals(getPreferences(MODE_PRIVATE).getString(supportKey(),null));
                if(!supportReady) {
                    DataFiles.installSupport(this,root);
                    supportInstalled();
                }
            }
        },"",true,()->{
            if(pendingStorage!=null && StorageLocations.hasPublicAccess(this)) {
                File destination=pendingStorage;pendingStorage=null;chooseLocation(destination);return;
            }
            if(!managing && hasRoms() && pendingDocument==null && !startupFailed())launch(false);
        },false);
    }
    private void requestFileAccess() {
        if(StorageLocations.hasPublicAccess(this)){beginStorage();return;}
        if(Build.VERSION.SDK_INT>=30) {
            try {
                startActivityForResult(new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION,
                    Uri.parse("package:"+getPackageName())),FILE_ACCESS);
            }catch(ActivityNotFoundException unavailable) {
                try{startActivityForResult(new Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION),FILE_ACCESS);}
                catch(ActivityNotFoundException missing){showError(new IOException("Open Android Settings → Apps → Special app access → All files access and allow Combo Ship."));}
            }
        }else requestPermissions(new String[]{android.Manifest.permission.READ_EXTERNAL_STORAGE,
            android.Manifest.permission.WRITE_EXTERNAL_STORAGE},FILE_ACCESS);
    }

    private String supportKey() {return "support-manifest:"+root.getAbsolutePath();}
    private String legacySupportKey() {return "support-version:"+root.getAbsolutePath();}
    private void supportInstalled() throws IOException {
        // Keyed on the bundled files, not versionCode: a same-version rebuild can change them. The old
        // versionCode key is dropped so a downgraded APK also reinstalls its own files.
        getPreferences(MODE_PRIVATE).edit().putString(supportKey(),DataFiles.supportVersion(this))
            .remove(legacySupportKey()).apply();
        supportReady=true;
    }
    private boolean startupFailed() {return new File(root,"last-startup-error.txt").isFile();}
    private void removeLegacyGameTasks() {
        // Before 0.1.4 the game had its own task affinity. Android retains its empty Recents card
        // across an APK update, so retire that old GameActivity-rooted task once setup opens.
        ActivityManager manager=getSystemService(ActivityManager.class);
        if(manager==null)return;
        for(ActivityManager.AppTask task:manager.getAppTasks()) {
            ActivityManager.RecentTaskInfo info=task.getTaskInfo();
            ComponentName component=info.baseIntent.getComponent();
            if(info.id!=getTaskId() && component!=null && component.getPackageName().equals(getPackageName()) &&
                    component.getClassName().equals(GameActivity.class.getName()))task.finishAndRemoveTask();
        }
    }

    private int dp(int value) {return Math.round(value*getResources().getDisplayMetrics().density);}
    private GradientDrawable surface(int color,int radius,boolean outline) {
        GradientDrawable drawable=new GradientDrawable();
        drawable.setColor(color);drawable.setCornerRadius(dp(radius));
        if(outline)drawable.setStroke(dp(1),BORDER);
        return drawable;
    }
    private TextView text(String value,int size,int color,boolean bold) {
        TextView view=new TextView(this);view.setText(value);view.setTextSize(size);view.setTextColor(color);
        if(bold)view.setTypeface(Typeface.create("sans-serif-medium",Typeface.NORMAL));
        return view;
    }
    private void space(LinearLayout parent,int height) {parent.addView(new View(this),new LinearLayout.LayoutParams(1,dp(height)));}

    private void buildScreen() {
        WindowCompat.setDecorFitsSystemWindows(getWindow(),false);
        ScrollView scroll=new ScrollView(this);scroll.setFillViewport(true);scroll.setBackgroundColor(BACKGROUND);
        ViewCompat.setOnApplyWindowInsetsListener(scroll,(view,insets)->{
            androidx.core.graphics.Insets bars=insets.getInsets(WindowInsetsCompat.Type.systemBars()|WindowInsetsCompat.Type.displayCutout());
            view.setPadding(bars.left,bars.top,bars.right,bars.bottom);return insets;
        });
        LinearLayout center=new LinearLayout(this);center.setOrientation(LinearLayout.VERTICAL);center.setGravity(Gravity.CENTER);
        center.setPadding(dp(24),dp(24),dp(24),dp(24));scroll.addView(center);
        LinearLayout body=new LinearLayout(this){
            @Override protected void onMeasure(int width,int height) {
                super.onMeasure(MeasureSpec.makeMeasureSpec(Math.min(MeasureSpec.getSize(width),dp(520)),MeasureSpec.EXACTLY),height);
            }
        };
        body.setOrientation(LinearLayout.VERTICAL);center.addView(body,new LinearLayout.LayoutParams(-1,-2));

        LinearLayout brand=new LinearLayout(this);brand.setGravity(Gravity.CENTER_VERTICAL);
        ImageView icon=new ImageView(this);icon.setImageResource(R.drawable.ic_combo);icon.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        brand.addView(icon,new LinearLayout.LayoutParams(dp(36),dp(36)));
        TextView name=text(getString(R.string.app_name),23,TEXT,true);name.setPadding(dp(12),0,0,0);
        brand.addView(name,new LinearLayout.LayoutParams(0,-2,1));
        options=new Button(this);options.setText("App options");options.setAllCaps(false);options.setTextSize(14);options.setTextColor(MUTED);
        options.setBackgroundResource(android.R.color.transparent);options.setMinWidth(0);options.setMinimumWidth(0);
        options.setPadding(dp(12),0,dp(12),0);options.setOnClickListener(v->showOptions());
        brand.addView(options,new LinearLayout.LayoutParams(-2,dp(48)));body.addView(brand);
        space(body,32);
        step=text("",12,GOLD,true);step.setLetterSpacing(0.12f);body.addView(step);
        space(body,10);
        heading=text("",30,TEXT,true);body.addView(heading);
        space(body,10);
        description=text("",16,MUTED,false);description.setLineSpacing(dp(3),1);body.addView(description);
        space(body,24);

        LinearLayout games=new LinearLayout(this);gameList=games;games.setOrientation(LinearLayout.VERTICAL);
        games.setPadding(dp(18),dp(6),dp(18),dp(6));games.setBackground(surface(PANEL,18,true));
        TextView[] oot=gameRow(games,"Ocarina of Time","1");ootNumber=oot[0];ootState=oot[1];
        View divider=new View(this);divider.setBackgroundColor(BORDER);games.addView(divider,new LinearLayout.LayoutParams(-1,dp(1)));
        TextView[] mm=gameRow(games,"Majora’s Mask","2");mmNumber=mm[0];mmState=mm[1];body.addView(games);
        space(body,24);

        primary=new Button(this);primary.setAllCaps(false);primary.setTextSize(17);primary.setTypeface(Typeface.DEFAULT,Typeface.BOLD);
        primary.setTextColor(BACKGROUND);primary.setBackgroundTintList(null);primary.setBackground(surface(GOLD,14,false));
        primary.setPadding(dp(16),dp(12),dp(16),dp(12));primary.setMinHeight(dp(56));
        primary.setOnClickListener(v->{
            if(storageAccessRequired)requestFileAccess();
            else if(root==null)beginStorage();
            else if(!supportReady)prepare();
            else if(!hasRom(OOT))pick(OOT);
            else if(!hasRom(MM))pick(MM);
            else launch(false);
        });
        body.addView(primary,new LinearLayout.LayoutParams(-1,-2));

        LinearLayout feedback=new LinearLayout(this);feedback.setGravity(Gravity.CENTER_VERTICAL);feedback.setPadding(0,dp(14),0,0);
        progress=new ProgressBar(this);LinearLayout.LayoutParams spinner=new LinearLayout.LayoutParams(dp(20),dp(20));spinner.setMarginEnd(dp(12));
        feedback.addView(progress,spinner);
        status=text("",14,MUTED,false);status.setLineSpacing(dp(2),1);feedback.addView(status,new LinearLayout.LayoutParams(0,-2,1));body.addView(feedback);
        space(body,20);
        fileLocation=text("",14,MUTED,false);fileLocation.setLineSpacing(dp(3),1);
        fileLocation.setPadding(0,dp(10),0,dp(10));fileLocation.setOnClickListener(v->showFiles());
        body.addView(fileLocation);setContentView(scroll);refresh();
    }

    private TextView[] gameRow(LinearLayout parent,String name,String number) {
        LinearLayout row=new LinearLayout(this);row.setGravity(Gravity.CENTER_VERTICAL);row.setPadding(0,dp(16),0,dp(16));
        TextView badge=text(number,17,GOLD,true);badge.setGravity(Gravity.CENTER);badge.setBackground(surface(BACKGROUND,20,false));
        row.addView(badge,new LinearLayout.LayoutParams(dp(40),dp(40)));
        LinearLayout labels=new LinearLayout(this);labels.setOrientation(LinearLayout.VERTICAL);labels.setPadding(dp(14),0,0,0);
        labels.addView(text(name,17,TEXT,true));TextView state=text("",14,MUTED,false);labels.addView(state);
        row.addView(labels,new LinearLayout.LayoutParams(0,-2,1));parent.addView(row);
        return new TextView[]{badge,state};
    }

    private boolean hasRom(int request) {return root!=null && new File(root,request==OOT?"oot.z64":"mm.z64").isFile();}
    private boolean hasRoms() {return hasRom(OOT) && hasRom(MM);}
    private void refresh() {
        if(primary==null)return;
        boolean oot=hasRom(OOT),mm=hasRom(MM);
        gameList.setVisibility(root==null?View.GONE:View.VISIBLE);
        ootNumber.setText(oot?"✓":"1");mmNumber.setText(mm?"✓":"2");
        ootNumber.setTextColor(oot?GREEN:GOLD);mmNumber.setTextColor(mm?GREEN:oot?GOLD:MUTED);
        ootState.setText(oot?"Added":"Choose first");mmState.setText(mm?"Added":oot?"Choose next":"Up next");
        ootState.setTextColor(oot?GREEN:MUTED);mmState.setTextColor(mm?GREEN:MUTED);
        if(storageAccessRequired) {
            step.setText("YOUR FILES");heading.setText("Use a visible ComboShip folder");
            description.setText("Saves, seeds, settings and mods will live in Internal storage → ComboShip.\n\n"+
                "On the next screen, allow file access and return here. Existing app data will be copied and verified; the original stays in place.");
            primary.setText("Allow file access");
        } else if(root==null && busy) {
            step.setText("YOUR FILES");heading.setText("Preparing your ComboShip folder");
            description.setText("Copying and checking your existing files. Keep Combo Ship open until this finishes. Your original data stays in place.");
            primary.setText("Preparing files…");
        } else if(root==null) {
            step.setText("STORAGE UNAVAILABLE");heading.setText("Let’s find your games");
            description.setText("Reconnect your selected storage, or choose an existing data location in App options.");primary.setText("Try again");
        } else if(!oot) {
            step.setText("SETUP · 1 OF 2");heading.setText("Add Ocarina of Time");
            description.setText("Choose your Ocarina of Time ROM. We’ll ask for Majora’s Mask next.");primary.setText("Choose OoT ROM");
        } else if(!mm) {
            step.setText("SETUP · 2 OF 2");heading.setText("Add Majora’s Mask");
            description.setText("One more ROM, then you’re ready. The game will start automatically.");primary.setText("Choose MM ROM");
        } else {
            step.setText("READY");heading.setText("Ready to play");
            description.setText("Ocarina of Time + Majora’s Mask\nYour combined randomizer is ready.");primary.setText("Play");
        }
        if(root!=null && !supportReady && !busy)primary.setText("Try setup again");
        primary.setEnabled(!busy);primary.setAlpha(busy?0.55f:1f);options.setEnabled(!busy);
        progress.setVisibility(busy?View.VISIBLE:View.GONE);
        String message=busy?workLabel:notice;
        if(message.isEmpty()) {
            if(root!=null && new File(root,"last-startup-error.txt").isFile())message="The last launch didn’t finish. App options has recovery tools.";
            else message=storageAccessRequired?"Android requires this permission to create and use the visible folder.":
                oot&&mm?"Your games and saves stay on this device.":"Choose a .z64, .n64 or .v64 file. Your original ROM stays unchanged.";
        }
        status.setText(message);
        fileLocation.setText(root==null?"Default: Internal storage / ComboShip":
            "Files: "+root.getAbsolutePath()+"\nTap for saves, seeds and backup paths");
        fileLocation.setEnabled(root!=null && !busy);
    }

    private void showOptions() {
        String[] labels={"Manage ROMs","Mods and recovery","Back up saves and settings","Data location / SD card","Repair game support files","About and help","Files and folder paths","Export backup elsewhere"};
        new AlertDialog.Builder(this).setTitle("App options").setItems(labels,(dialog,which)->{
            switch(which) {
                case 0:new AlertDialog.Builder(this).setTitle("Manage ROMs")
                    .setItems(new String[]{"Choose Ocarina of Time ROM","Choose Majora’s Mask ROM"},(d,w)->pick(w==0?OOT:MM)).setNegativeButton("Close",null).show();break;
                case 1:new AlertDialog.Builder(this).setTitle("Mods and recovery")
                    .setItems(new String[]{"Import an OoT mod archive","Import an MM mod archive","Play without mods"},(d,w)->{
                        if(w==2)launch(true);else pick(w==0?MOD_OOT:MOD_MM);
                    }).setNegativeButton("Close",null).show();break;
                case 2:backup();break;
                case 3:chooseStorage();break;
                case 4:prepare();break;
                case 5:showHelp();break;
                case 6:showFiles();break;
                case 7:exportBackupElsewhere();break;
            }
        }).setNegativeButton("Close",null).show();
    }
    private void showFiles() {
        if(root==null){notice="Allow file access or reconnect the active folder first.";refresh();return;}
        String paths=root.getAbsolutePath()+"\n\n"+
            "Save/ — linked OoT + MM save slots\nRandomizer/ — seeds and spoiler logs\nBackups/ — backup ZIPs\n"+
            "mods/soh/ — OoT mods\nmods/2ship/ — MM mods\ncomboship.json — game and tracker settings\n"+
            "Log and diagnostic files are also in this folder.\n\nSave and close the game before copying files.";
        new AlertDialog.Builder(this).setTitle("Your ComboShip files").setMessage(paths)
            .setPositiveButton("Open folder",(d,w)->openFolder())
            .setNeutralButton("Copy path",(d,w)->{
                getSystemService(ClipboardManager.class).setPrimaryClip(ClipData.newPlainText("ComboShip folder",root.getAbsolutePath()));
                notice="Folder path copied.";refresh();
            }).setNegativeButton("Close",null).show();
    }
    private void openFolder() {
        try {
            if(!root.equals(StorageLocations.publicRoot()))throw new ActivityNotFoundException();
            Uri folder=DocumentsContract.buildDocumentUri("com.android.externalstorage.documents","primary:ComboShip");
            startActivity(new Intent(Intent.ACTION_VIEW).setDataAndType(folder,DocumentsContract.Document.MIME_TYPE_DIR));
        }catch(IOException|ActivityNotFoundException|SecurityException error) {
            notice="Open your phone’s Files app and browse to "+root.getAbsolutePath();refresh();
        }
    }
    private void showHelp() {
        String message=getString(R.string.app_name)+" "+BuildConfig.VERSION_NAME+
            "\n\nChoose an unmodified Ocarina of Time ROM, then a Majora’s Mask ROM. The game prepares their data automatically on first launch."+
            "\n\nUse the in-game Menu for randomizer settings, checks, controls and enhancements. Hold Menu for touch-control options."+
            "\n\nFind your files in Internal storage → ComboShip by default. App options → Files and folder paths shows the active location. " +
            "Backups are saved in its Backups folder. This app has its own saves, separate from 2S2H. Backups contain saves, settings and logs, without ROMs or mods.";
        if(root!=null) {
            File error=new File(root,"last-startup-error.txt");
            if(error.isFile())try(InputStream in=new FileInputStream(error)) {
                message+="\n\nLast launch details:\n"+new String(DataFiles.readLimited(in,16384),java.nio.charset.StandardCharsets.UTF_8);
            }catch(IOException ignored) {message+="\n\nExport a backup to inspect the last launch log.";}
        }
        new AlertDialog.Builder(this).setTitle(R.string.about_app).setMessage(message).setPositiveButton("Close",null).show();
    }

    private interface Work {void run() throws Exception;}
    private void execute(String text,Work work,String complete) {execute(text,work,complete,false,null);}
    private void execute(String text,Work work,String complete,boolean allowMissingStorage,Runnable success) {
        execute(text,work,complete,allowMissingStorage,success,true);
    }
    private void execute(String text,Work work,String complete,boolean allowMissingStorage,Runnable success,boolean lockData) {
        if(busy || (root==null && !allowMissingStorage))return;
        busy=true;workLabel=text;notice="";refresh();
        worker.execute(()->{
            Exception failure=null;
            try(DataLock ignored=!lockData || root==null?null:new DataLock(root)){work.run();}catch(Exception error){failure=error;}
            final Exception result=failure;
            runOnUiThread(()->{
                if(isFinishing() || isDestroyed())return;
                busy=false;
                if(result!=null) {showError(result);return;}
                notice=complete;refresh();if(success!=null)success.run();
                consumePendingDocument();
            });
        });
    }
    private void prepare() {
        execute("Getting things ready…",()->{
            supportReady=false;DataFiles.installSupport(this,root);supportInstalled();
        },"",false,()->{if(!managing && hasRoms() && pendingDocument==null && !startupFailed())launch(false);});
    }
    private void pick(int request) {
        if(root==null){chooseStorage();return;}
        if(request==OOT || request==MM)launchAfterImport=!hasRoms();
        startActivityForResult(new Intent(Intent.ACTION_OPEN_DOCUMENT).setType("*/*").addCategory(Intent.CATEGORY_OPENABLE),request);
    }
    private void backup() {
        if(root==null){beginStorage();return;}
        final File[] saved=new File[1];
        execute("Backing up saves and settings…",()->saved[0]=DataFiles.createBackup(this,root),"",false,()->{
            notice="Backup saved: "+saved[0].getAbsolutePath();refresh();
        });
    }
    private void exportBackupElsewhere() {
        if(root==null){beginStorage();return;}
        Intent intent=new Intent(Intent.ACTION_CREATE_DOCUMENT).setType("application/zip").addCategory(Intent.CATEGORY_OPENABLE);
        try{if(root.equals(StorageLocations.publicRoot()))intent.putExtra(DocumentsContract.EXTRA_INITIAL_URI,
            DocumentsContract.buildDocumentUri("com.android.externalstorage.documents","primary:ComboShip/Backups"));}catch(IOException ignored){}
        intent.putExtra(Intent.EXTRA_TITLE,"ComboShip-backup-"+System.currentTimeMillis()+".zip");startActivityForResult(intent,BACKUP);
    }
    private void chooseStorage() {
        try {
            List<StorageLocations.Location> choices=StorageLocations.available(this);
            String[] labels=new String[choices.size()];
            for(int i=0;i<labels.length;i++)labels[i]=choices.get(i).label;
            new AlertDialog.Builder(this).setTitle("Data location")
                .setItems(labels,(dialog,which)->chooseLocation(choices.get(which).directory))
                .setNegativeButton("Close",null).show();
        }catch(IOException error){showError(error);}
    }
    private void chooseLocation(File destination) {
        try {
            if(destination.equals(StorageLocations.publicRoot()) && !StorageLocations.hasPublicAccess(this)) {
                pendingStorage=destination;requestFileAccess();return;
            }
        }catch(IOException error){showError(error);return;}
        // A copy whose deletion stopped part-way can only be deleted again.
        if(root!=null && !destination.equals(root) && StorageLocations.partlyDeleted(destination)){removeCopy(destination);return;}
        boolean existing=StorageLocations.hasData(destination);
        AlertDialog.Builder dialog=new AlertDialog.Builder(this).setTitle(existing?"Use existing Combo Ship data?":"Copy data to this storage?")
            .setMessage((existing?"This folder's saves will become the active copy. Other copies will be preserved.":
                "Saves, settings, ROMs, and mods will be copied and verified. The current copy will be preserved.")+"\n\n"+destination)
            .setPositiveButton(existing?"Use existing":"Copy and switch",(d,w)->execute("Preparing data location…",()->{
                // Only a copy needs the old location, so a removed SD card cannot block switching to existing data.
                StorageLocations.select(this,root!=null || existing?root:StorageLocations.previous(this),destination,existing);
                root=DataFiles.root(this);storageAccessRequired=false;supportReady=false;
                try(DataLock ignored=new DataLock(root)) {
                    DataFiles.installSupport(this,root);supportInstalled();
                }
            },"Data location changed.",true,null,false))
            .setNegativeButton("Cancel",null);
        // A copy left behind by a move can go once the game has run and closed normally from the active folder.
        if(existing && root!=null && !destination.equals(root) && activeFolderTested())
            dialog.setNeutralButton("Delete this copy",(d,w)->removeCopy(destination));
        dialog.show();
    }
    private boolean activeFolderTested() {
        long launched=getPreferences(MODE_PRIVATE).getLong("launched:"+root.getAbsolutePath(),0);
        return launched>0 && new File(root,"native.log").lastModified()>=launched &&
            !new File(root,".running").exists() && !startupFailed();
    }
    private void removeCopy(File copy) {
        final long[] size=new long[1];
        execute("Checking the old copy…",()->size[0]=StorageLocations.size(copy),"",false,()->
            new AlertDialog.Builder(this).setTitle("Delete this copy?")
                .setMessage(copy+"\n\nEverything in this folder will be deleted ("+android.text.format.Formatter.formatFileSize(this,size[0])+
                    "), including its saves. Your active data in "+root+" is not affected.")
                .setPositiveButton("Delete",(d,w)->execute("Deleting the old copy…",()->StorageLocations.removeCopy(this,copy),"Old copy deleted."))
                .setNegativeButton("Cancel",null).show());
    }
    private String displayName(Uri uri) {
        try(Cursor cursor=getContentResolver().query(uri,new String[]{OpenableColumns.DISPLAY_NAME},null,null,null)) {
            if(cursor!=null && cursor.moveToFirst())return cursor.getString(0);
        }catch(RuntimeException ignored){}return "";
    }
    @Override protected void onActivityResult(int request,int result,Intent data) {
        super.onActivityResult(request,result,data);
        if(request==FILE_ACCESS){fileAccessReturned();return;}
        if(result!=RESULT_OK || data==null || data.getData()==null)return;
        // A document picker can restore the activity while support verification is still running.
        pendingDocument=data.getData();pendingRequest=request;consumePendingDocument();
    }
    private void fileAccessReturned() {
        if(!StorageLocations.hasPublicAccess(this))notice="File access was not enabled. Existing saves are unchanged; allow file access to use the visible folder.";
        else if(pendingStorage!=null) {
            File destination=pendingStorage;pendingStorage=null;chooseLocation(destination);return;
        }
        beginStorage();
    }
    @Override public void onRequestPermissionsResult(int request,String[] permissions,int[] results) {
        super.onRequestPermissionsResult(request,permissions,results);
        if(request==FILE_ACCESS)fileAccessReturned();
    }
    private void consumePendingDocument() {
        if(busy || !supportReady || pendingDocument==null)return;
        Uri uri=pendingDocument;int request=pendingRequest;pendingDocument=null;
        if(request==OOT || request==MM) {
            boolean autoStart=launchAfterImport;launchAfterImport=false;
            execute(request==OOT?"Adding Ocarina of Time…":"Adding Majora’s Mask…",()->{
                try(InputStream in=getContentResolver().openInputStream(uri)) {
                    RomImporter.importRom(in,new File(root,request==OOT?"oot.z64":"mm.z64"));
                }
            },request==OOT?"Ocarina of Time added.":"Majora’s Mask added.",false,()->{if(autoStart && hasRoms())launch(false);});
        }else if(request==MOD_OOT || request==MOD_MM) {
            String name=displayName(uri);
            execute("Importing the selected mod…",()->{
                try(InputStream in=getContentResolver().openInputStream(uri)){DataFiles.importMod(in,root,request==MOD_OOT?"soh":"2ship",name);}
            },"Mod imported.");
        }else if(request==BACKUP) {
            execute("Backing up saves and settings…",()->{
                try(OutputStream out=getContentResolver().openOutputStream(uri,"w")){DataFiles.exportBackup(this,root,out);}
            },"Backup saved.");
        }
    }
    private void launch(boolean withoutMods) {
        if(root==null || busy || launched)return;
        if(!supportReady){prepare();return;}
        if(!hasRoms()) {notice="Choose both ROMs to start playing.";refresh();return;}
        if(withoutMods)try(DataLock ignored=new DataLock(root)){}catch(IOException error){showError(error);return;}
        Intent game=new Intent(this,GameActivity.class);game.putExtra("without-mods",withoutMods);
        game.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP|Intent.FLAG_ACTIVITY_SINGLE_TOP);
        // Lets Data location tell when this folder has run the game (see activeFolderTested).
        getPreferences(MODE_PRIVATE).edit().putLong("launched:"+root.getAbsolutePath(),System.currentTimeMillis()).apply();
        launched=true;startActivity(game);finish();
    }
    private void showError(Exception error) {
        notice=error.getMessage()==null?error.toString():error.getMessage();refresh();
        new AlertDialog.Builder(this).setTitle("Couldn’t finish").setMessage(notice).setPositiveButton("Close",null).show();
    }
    @Override protected void onSaveInstanceState(Bundle state) {
        state.putBoolean("launch-after-import",launchAfterImport);
        if(pendingDocument!=null)state.putString("pending-document",pendingDocument.toString());
        if(pendingStorage!=null)state.putString("pending-storage",pendingStorage.getPath());
        state.putInt("pending-request",pendingRequest);super.onSaveInstanceState(state);
    }
    @Override protected void onResume() {
        super.onResume();CrashReports.capturePreviousExit(this);
        try {
            if(root!=null && StorageLocations.needsPublicAccess(this) && !StorageLocations.hasPublicAccess(this)) {
                root=null;supportReady=false;storageAccessRequired=true;
            }
        }catch(IOException error){notice=error.getMessage();}
        refresh();
    }
    @Override protected void onDestroy() {worker.shutdown();super.onDestroy();}
}
