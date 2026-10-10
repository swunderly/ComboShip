package org.comboship.android;

import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Environment;
import java.io.*;
import java.nio.file.*;
import java.util.*;

/** Visible shared storage by default; legacy app folders remain explicit recovery choices. */
public final class StorageLocations {
    private static final String PREFS="combo-storage", KEY="root", MARKER=".comboship-data-v1";
    private static final String VISIBLE_DEFAULT="visible-default-v1";
    public static final class Location {
        public final String label;
        public final File directory;
        Location(String label,File directory){this.label=label;this.directory=directory;}
    }
    private StorageLocations() {}
    public static File publicRoot() throws IOException {
        return new File(Environment.getExternalStorageDirectory(),"ComboShip").getCanonicalFile();
    }
    public static boolean hasPublicAccess(Context context) {
        if(Build.VERSION.SDK_INT>=30)return Environment.isExternalStorageManager();
        return context.checkSelfPermission(android.Manifest.permission.READ_EXTERNAL_STORAGE)==PackageManager.PERMISSION_GRANTED &&
            context.checkSelfPermission(android.Manifest.permission.WRITE_EXTERNAL_STORAGE)==PackageManager.PERMISSION_GRANTED;
    }
    public static boolean needsPublicAccess(Context context) throws IOException {
        return !configured(context) || publicRoot().getPath().equals(
            context.getSharedPreferences(PREFS,Context.MODE_PRIVATE).getString(KEY,""));
    }
    private static boolean configured(Context context) {
        return context.getSharedPreferences(PREFS,Context.MODE_PRIVATE).getBoolean(VISIBLE_DEFAULT,false);
    }
    public static List<Location> available(Context context) throws IOException {
        LinkedHashMap<String,Location> locations=new LinkedHashMap<>();
        File visible=publicRoot();
        locations.put(visible.getPath(),new Location("Internal storage / ComboShip (recommended)\n"+visible,visible));
        File internal=new File(context.getFilesDir(),"comboship").getCanonicalFile();
        locations.put(internal.getPath(),new Location("Private app storage (hidden)\n"+internal,internal));
        for(File base:context.getExternalFilesDirs(null)) {
            if(base==null || !Environment.MEDIA_MOUNTED.equals(Environment.getExternalStorageState(base)))continue;
            File path=new File(base,"comboship").getCanonicalFile();
            String label=Environment.isExternalStorageRemovable(base)?"SD card (app folder)":"Internal storage (Android/data app folder)";
            locations.put(path.getPath(),new Location(label+"\n"+path,path));
        }
        return new ArrayList<>(locations.values());
    }
    public static File current(Context context) throws IOException {
        if(!configured(context))throw new IOException("Open Combo Ship to set up Internal storage / ComboShip.");
        String saved=context.getSharedPreferences(PREFS,Context.MODE_PRIVATE).getString(KEY,"");
        File selected=allowed(context,new File(saved));
        requireAccess(context,selected);
        // A missing active folder must never look like a new game, even after access is revoked.
        if(!selected.isDirectory() || !hasData(selected))throw new IOException(
            "Your data folder is unavailable: "+selected+". Restore it or choose an existing data location in App options.");
        return selected;
    }
    public static File previous(Context context) throws IOException {
        String saved=context.getSharedPreferences(PREFS,Context.MODE_PRIVATE).getString(KEY,null);
        if(saved!=null)return allowed(context,new File(saved));
        File legacy=new File(context.getFilesDir(),"comboship").getCanonicalFile();
        return legacy.isDirectory()?legacy:null;
    }
    /** Called by the launcher worker before the game starts, never by the native game process. */
    public static File initialize(Context context) throws IOException {
        if(configured(context))return current(context);
        File target=publicRoot();requireAccess(context,target);
        File source=previous(context);
        if(source!=null && !source.equals(target)) {
            if(!source.isDirectory())throw new IOException("Reconnect your previous data folder before moving saves: "+source);
            try(DataLock ignored=new DataLock(source)) {
                if(hasData(target))throw new IOException("Both your previous folder and "+target+
                    " contain data. Nothing was overwritten. Choose the copy to use in App options → Data location.");
                StorageCopy.prepare(source,target,staging->DataFiles.rebaseSettings(staging,source,target));
            }
        } else {
            if(target.exists() && !hasData(target) && !StorageCopy.isEmptyDirectory(target.toPath()))
                throw new IOException("The ComboShip folder already contains unrecognized files. They were left unchanged. Rename that folder before continuing.");
            if(!target.isDirectory() && !target.mkdirs())throw new IOException("Cannot create "+target);
        }
        try(DataLock ignored=new DataLock(target)) {
            mark(target);saveSelection(context,target);
        }
        return target;
    }
    public static boolean hasData(File target){return new File(target,MARKER).isFile();}
    public static void select(Context context,File source,File requested,boolean useExisting) throws IOException {
        File target=allowed(context,requested);requireAccess(context,target);
        if(source!=null && source.getCanonicalFile().equals(target)) {
            try(DataLock ignored=new DataLock(target)){mark(target);saveSelection(context,target);}return;
        }
        if(useExisting) {
            if(!hasData(target))throw new IOException("The selected folder has no Combo Ship data.");
            try(DataLock ignored=new DataLock(target)){saveSelection(context,target);}
            return;
        }
        // previous() throws for an unavailable saved location, so no source means nothing was ever set up.
        if(source==null)throw new IOException("There is no Combo Ship data to copy yet. New installs start in Internal storage / ComboShip: "+
            "allow file access to set it up, then other locations can be chosen here.");
        try(DataLock ignored=new DataLock(source)) {
            StorageCopy.prepare(source,target,staging->DataFiles.rebaseSettings(staging,source,target));
            mark(target);saveSelection(context,target);
        }
        // The old copy is deliberately preserved until the user has tested the new storage.
    }
    private static File allowed(Context context,File requested)throws IOException {
        File target=requested.getCanonicalFile();
        for(Location location:available(context))if(target.equals(location.directory))return target;
        throw new IOException("The selected storage is unavailable. Reconnect it before playing.");
    }
    private static void requireAccess(Context context,File target)throws IOException {
        if(!target.equals(publicRoot()))return;
        if(!hasPublicAccess(context))throw new IOException("Allow file access to use Internal storage / ComboShip. Your existing saves remain in place.");
        if(!Environment.MEDIA_MOUNTED.equals(Environment.getExternalStorageState()))throw new IOException("Internal storage is unavailable.");
    }
    private static void mark(File root)throws IOException {
        File marker=new File(root,MARKER);
        if(!marker.exists() && !marker.createNewFile())throw new IOException("Cannot identify the data folder.");
    }
    private static void saveSelection(Context context,File target)throws IOException {
        if(!context.getSharedPreferences(PREFS,Context.MODE_PRIVATE).edit().putString(KEY,target.getPath()).putBoolean(VISIBLE_DEFAULT,true).commit())
            throw new IOException("Could not save the storage selection; the original copy remains available.");
    }
}
