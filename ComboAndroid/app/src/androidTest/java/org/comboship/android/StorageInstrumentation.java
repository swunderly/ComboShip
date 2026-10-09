package org.comboship.android;

import android.app.Activity;
import android.app.Instrumentation;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.SharedPreferences;
import android.os.Bundle;
import java.io.*;
import java.nio.file.*;
import java.util.*;
import java.util.zip.ZipFile;
import org.json.*;

/** Opt-in tests of real Android permissions, shared storage and migration on a disposable emulator. */
public final class StorageInstrumentation extends Instrumentation {
    private int checks,scenario;
    private Path scratch,visible;
    private boolean ownsVisible;
    private Bundle arguments;
    private void check(boolean ok,String message){checks++;if(!ok)throw new AssertionError(message);}
    @Override public void onCreate(Bundle args){super.onCreate(args);arguments=args;start();}
    private Context isolated()throws IOException {
        final File files=Files.createDirectories(scratch.resolve("case-"+(++scenario))).toFile();
        final String prefix="storage-tests-"+scratch.getFileName()+"-"+scenario+"-";
        return new ContextWrapper(getTargetContext()) {
            @Override public File getFilesDir(){return files;}
            @Override public SharedPreferences getSharedPreferences(String name,int mode){
                return getTargetContext().getSharedPreferences(prefix+name,mode);
            }
        };
    }
    private File oldData(Context context)throws IOException {
        Path root=Files.createDirectories(context.getFilesDir().toPath().resolve("comboship"));
        write(root.resolve(".comboship-data-v1"),"");
        Files.createDirectories(root.resolve("Save"));
        write(root.resolve("Save/file1.combosav"),"{\"comboRelease\":\"1.0\",\"test\":\"preserve exact save bytes\"}");
        Files.createDirectories(root.resolve("Randomizer"));
        write(root.resolve("Randomizer/Combo-test.json"),"{\"seed\":12345}");
        JSONObject settings=new JSONObject();
        try {
            settings.put("CVars",new JSONObject().put("gGeneral",new JSONObject().put("ComboSpoiler",root+"/Randomizer/Combo-test.json")));
            settings.put("unrelated",root+"-other/file");
            settings.put("number",1234567890123L);
            write(root.resolve("comboship.json"),settings.toString());
        }catch(JSONException error){throw new IOException(error);}
        return root.toFile();
    }
    private static void write(Path path,String text)throws IOException {Files.write(path,text.getBytes(java.nio.charset.StandardCharsets.UTF_8));}
    private static String read(Path path)throws IOException {return new String(Files.readAllBytes(path),java.nio.charset.StandardCharsets.UTF_8);}
    private void removeVisible()throws IOException {if(ownsVisible)removeTree(visible);}
    private static void removeTree(Path path)throws IOException {
        if(!Files.exists(path))return;
        try(var files=Files.walk(path)) {
            for(Iterator<Path> it=files.sorted(Comparator.reverseOrder()).iterator();it.hasNext();)Files.delete(it.next());
        }
    }
    @Override public void onStart() {
        Bundle result=new Bundle();
        int resultCode=Activity.RESULT_CANCELED;
        try {
            if(arguments==null || !"true".equals(arguments.getString("disposableEmulator")))
                throw new IOException("Requires -e disposableEmulator true on a dedicated test emulator.");
            if(!android.os.Build.MODEL.contains("sdk") && !android.os.Build.FINGERPRINT.contains("generic"))
                throw new IOException("Storage tests are restricted to an emulator.");
            visible=StorageLocations.publicRoot().toPath();
            if(Files.exists(visible))throw new IOException("Test needs an absent ComboShip folder; existing data was not touched.");
            ownsVisible=true;
            scratch=Files.createTempDirectory(getTargetContext().getCacheDir().toPath(),"storage-tests-");
            check(StorageLocations.hasPublicAccess(getTargetContext()),"All-files permission must be granted for filesystem tests");

            Context fresh=isolated();
            check(StorageLocations.needsPublicAccess(fresh),"Fresh installs require visible-folder access");
            check(StorageLocations.initialize(fresh).toPath().equals(visible),"Fresh install did not choose the visible root");
            check(StorageLocations.current(fresh).toPath().equals(visible),"Default did not persist");
            check(!new File(fresh.getFilesDir(),"comboship").exists(),"Fresh install created hidden data");
            removeVisible();
            boolean failed=false;
            try{StorageLocations.current(fresh);}catch(IOException expected){failed=true;}
            check(failed && !Files.exists(visible),"Missing active folder silently created empty saves");

            Context upgrade=isolated();File original=oldData(upgrade);
            byte[] save=Files.readAllBytes(original.toPath().resolve("Save/file1.combosav"));
            byte[] originalConfig=Files.readAllBytes(original.toPath().resolve("comboship.json"));
            Files.createDirectory(visible); // A user may already have made an empty folder in Files.
            check(StorageLocations.initialize(upgrade).toPath().equals(visible),"Upgrade did not migrate to the visible root");
            check(Arrays.equals(save,Files.readAllBytes(visible.resolve("Save/file1.combosav"))),"Save bytes changed during migration");
            check(Arrays.equals(save,Files.readAllBytes(original.toPath().resolve("Save/file1.combosav"))),"Original save was not preserved");
            check(Arrays.equals(originalConfig,Files.readAllBytes(original.toPath().resolve("comboship.json"))),"Original config was modified");
            JSONObject config=new JSONObject(read(visible.resolve("comboship.json")));
            check(config.getJSONObject("CVars").getJSONObject("gGeneral").getString("ComboSpoiler")
                .equals(visible+"/Randomizer/Combo-test.json"),"Seed reference still points to hidden data: "+config);
            check(config.getLong("number")==1234567890123L && config.getString("unrelated").equals(original+"-other/file"),"Unrelated settings changed");
            check(read(visible.resolve("Randomizer/Combo-test.json")).equals("{\"seed\":12345}"),"Seed changed");
            write(original.toPath().resolve("Save/file1.combosav"),"stale original copy");
            StorageLocations.initialize(upgrade);
            check(Arrays.equals(save,Files.readAllBytes(visible.resolve("Save/file1.combosav"))),"Restart recopied stale data");
            write(visible.resolve("oot.z64"),"test placeholder");
            Files.createDirectories(visible.resolve("mods/soh"));
            write(visible.resolve("mods/soh/test.otr"),"test placeholder");
            File backup;
            try(DataLock lock=new DataLock(visible.toFile())){backup=DataFiles.createBackup(upgrade,visible.toFile());}
            check(backup.getParentFile().toPath().equals(visible.resolve("Backups")),"Backup was saved outside Backups");
            try(ZipFile zip=new ZipFile(backup)) {
                check(Arrays.equals(save,zip.getInputStream(zip.getEntry("Save/file1.combosav")).readAllBytes()),"Backup save differs");
                check(zip.getEntry("Randomizer/Combo-test.json")!=null,"Seed missing from backup");
                check(zip.getEntry("oot.z64")==null && zip.getEntry("mods/soh/test.otr")==null,"ROM or mod included in backup");
                check(zip.stream().noneMatch(e->e.getName().startsWith("Backups/")),"Backup included itself");
                check(zip.getEntry("android-touch-settings.json")!=null,"Touch settings missing from backup");
            }
            try(DataLock lock=new DataLock(visible.toFile())) {
                File second=DataFiles.createBackup(upgrade,visible.toFile());
                check(!backup.equals(second) && backup.isFile(),"Second backup replaced the first");
            }
            removeVisible();

            Context conflict=isolated();File conflictSource=oldData(conflict);
            Files.createDirectories(visible);write(visible.resolve("valuable-save"),"keep this");
            failed=false;
            try{StorageLocations.initialize(conflict);}catch(IOException expected){failed=true;}
            check(failed && read(visible.resolve("valuable-save")).equals("keep this"),"Conflict overwritten");
            check(StorageLocations.needsPublicAccess(conflict),"Failed migration changed active selection");
            removeVisible();
            try(DataLock held=new DataLock(conflictSource)) {
                failed=false;try{StorageLocations.initialize(conflict);}catch(IOException expected){failed=true;}
                check(failed && !Files.exists(visible),"Migration ran while source was in use");
            }
            write(conflictSource.toPath().resolve("comboship.json"),"broken settings");
            failed=false;try{StorageLocations.initialize(conflict);}catch(IOException expected){failed=true;}
            check(failed && !Files.exists(visible),"Invalid config published or selected a partial copy");

            Context reinstall=isolated();
            Files.createDirectory(visible);write(visible.resolve(".comboship-data-v1"),"");
            write(visible.resolve("existing-save"),"existing data");
            check(StorageLocations.initialize(reinstall).toPath().equals(visible),"Reinstall did not reuse recognized visible data");
            check(read(visible.resolve("existing-save")).equals("existing data"),"Reinstall modified existing data");
            result.putString("stream","PASS: "+checks+" Android storage, migration and backup checks.\n");
            resultCode=Activity.RESULT_OK;
        }catch(Throwable error){
            result.putString("stream","FAIL after "+checks+" checks: "+android.util.Log.getStackTraceString(error)+"\n");
        }finally {
            try{removeVisible();if(scratch!=null)removeTree(scratch);}catch(IOException ignored){}
        }
        finish(resultCode,result);
    }
}
