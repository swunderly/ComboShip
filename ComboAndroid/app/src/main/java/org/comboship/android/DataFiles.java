package org.comboship.android;
import android.content.Context;
import org.json.*;
import java.io.*;
import java.nio.file.*;
import java.security.*;
import java.util.*;
import java.util.zip.*;
/** Only this application's selected internal or external directory is reachable through this API. */
public final class DataFiles {
    private DataFiles() {}
    public static File root(Context context) throws IOException {
        return StorageLocations.current(context);
    }
    private static File child(File root, String relative) throws IOException {
        if (relative.isEmpty() || relative.startsWith("/") || relative.contains("\\") ||
                Arrays.asList(relative.split("/")).contains("..")) throw new IOException("Invalid relative filename.");
        File target = new File(root, relative).getCanonicalFile();
        if (!target.toPath().startsWith(root.getCanonicalFile().toPath()) || target.equals(root))
            throw new IOException("Filename escapes the data directory.");
        return target;
    }
    public static byte[] readLimited(InputStream stream, long maximum) throws IOException {
        if (stream == null) throw new IOException("Document provider returned no stream.");
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        byte[] buffer = new byte[16384]; int count;
        while ((count=stream.read(buffer)) != -1) {
            if (count == 0) continue;
            if ((long)out.size()+count > maximum) throw new IOException("File exceeds the size limit.");
            out.write(buffer,0,count);
        }
        return out.toByteArray();
    }
    static String digest(File file) throws IOException {
        try (InputStream in = new FileInputStream(file)) {
            MessageDigest digest = MessageDigest.getInstance("SHA-256");
            byte[] bytes = new byte[65536]; int count;
            while ((count=in.read(bytes))!=-1) digest.update(bytes,0,count);
            return RomImporter.hex(digest.digest());
        } catch (NoSuchAlgorithmException impossible) {throw new IOException(impossible);}
    }
    static void atomicCopy(InputStream input, File destination, String expected, long maximum) throws IOException {
        if (input == null) throw new IOException("Document provider returned no stream.");
        File parent = destination.getParentFile();
        if (!parent.isDirectory() && !parent.mkdirs()) throw new IOException("Cannot create import directory.");
        Path temporary = Files.createTempFile(parent.toPath(), ".import-", ".tmp"); boolean done=false;
        try {
            try (FileOutputStream output = new FileOutputStream(temporary.toFile())) {
                byte[] buffer = new byte[65536]; int count; long total=0;
                while ((count=input.read(buffer))!=-1) {
                    if (count==0) continue;
                    total+=count;
                    if (total>maximum) throw new IOException("File exceeds the import size limit.");
                    output.write(buffer,0,count);
                }
                if (total==0) throw new IOException("Selected file is empty.");
                output.flush();
                // Imported user data is durable before replacement. Bundled support files are
                // reconstructible and verified on every setup; syncing 9,000 XML files stalls first launch.
                if(expected==null)output.getFD().sync();
            }
            if (expected!=null && !expected.equals(digest(temporary.toFile()))) throw new IOException("Support-file checksum mismatch.");
            Files.move(temporary,destination.toPath(),StandardCopyOption.ATOMIC_MOVE,StandardCopyOption.REPLACE_EXISTING);
            done=true;
        } finally {if(!done) Files.deleteIfExists(temporary);}
    }
    /** SHA-256 of the bundled manifest, which lists every support file's own SHA-256. */
    static String supportVersion(Context context) throws IOException {
        try (InputStream in=context.getAssets().open("support-manifest.json")) {
            return RomImporter.hex(MessageDigest.getInstance("SHA-256").digest(readLimited(in,4*1024*1024)));
        } catch (NoSuchAlgorithmException impossible) {throw new IOException(impossible);}
    }
    public static void installSupport(Context context, File root) throws IOException {
        try {
            byte[] manifest;
            try (InputStream in=context.getAssets().open("support-manifest.json")) {manifest=readLimited(in,4*1024*1024);}
            JSONArray files=new JSONObject(new String(manifest,java.nio.charset.StandardCharsets.UTF_8)).getJSONArray("files");
            for (int i=0;i<files.length();i++) {
                JSONObject entry=files.getJSONObject(i);
                String relative=entry.getString("path"), sha=entry.getString("sha256");
                if (!relative.startsWith("assets/") && !relative.equals("soh.o2r") && !relative.equals("2ship.o2r") && !relative.equals("gamecontrollerdb.txt"))
                    throw new IOException("Unexpected bundled file: "+relative);
                File target=child(root,relative);
                if (!target.isFile() || !sha.equals(digest(target))) {
                    try (InputStream in=context.getAssets().open("runtime/"+relative)) {atomicCopy(in,target,sha,128L*1024*1024);}
                }
            }
            try (ZipFile archive=new ZipFile(new File(root,"soh.o2r"))) {
                if (archive.getEntry("shaders/opengl/default.shader.glsl")==null)
                    throw new IOException("Bundled renderer archive is missing its GLES shader.");
            }
            for (String directory:new String[]{"Save","Randomizer","Backups","mods/soh","mods/2ship"}) {
                File folder=child(root,directory);
                if (!folder.isDirectory() && !folder.mkdirs()) throw new IOException("Cannot create mods directory.");
            }
            String guide="Combo Ship files\n\nActive folder: "+root.getAbsolutePath()+"\n\n"+
                "Save/            Your three linked OoT + MM saves (file1.combosav to file3.combosav).\n"+
                "Randomizer/      Generated seeds, settings and spoiler logs.\n"+
                "Backups/         ZIP backups from App options.\n"+
                "mods/soh/        Ocarina of Time mods.\n"+
                "mods/2ship/      Majora's Mask mods.\n"+
                "comboship.json   Game, randomizer and tracker preferences.\n"+
                "*.log, *.txt     Runtime and startup diagnostics.\n"+
                "assets/, *.o2r, *.z64 and gamecontrollerdb.txt are used by the game.\n\n"+
                "Save in-game and close the game before copying or changing its files.\n"+
                "To get help with a seed, share a backup ZIP and a screenshot of the tracker.\n";
            atomicCopy(new ByteArrayInputStream(guide.getBytes(java.nio.charset.StandardCharsets.UTF_8)),
                child(root,"ComboShip-files.txt"),null,16384);
        } catch (JSONException error) {throw new IOException("Invalid support-file manifest.",error);}
    }
    /** Re-root remembered seed/config paths in the verified staging copy, preserving all save bytes. */
    static void rebaseSettings(File staging,File source,File target)throws IOException {
        File config=child(staging,"comboship.json");
        if(!config.isFile())return;
        try {
            JSONObject data;
            try(InputStream in=new FileInputStream(config)) {
                data=new JSONObject(new String(readLimited(in,64L*1024*1024),java.nio.charset.StandardCharsets.UTF_8));
            }
            if(rebase(data,source.getCanonicalPath(),target.getCanonicalPath())) {
                byte[] updated=data.toString(2).getBytes(java.nio.charset.StandardCharsets.UTF_8);
                atomicCopy(new ByteArrayInputStream(updated),config,null,64L*1024*1024);
            }
        }catch(JSONException error){throw new IOException("Could not read game settings for migration. The original files were preserved.",error);}
    }
    private static String rebasedPath(Object value,String oldRoot,String newRoot) {
        if(!(value instanceof String) || !((String)value).startsWith("/"))return null;
        try {
            // Android uses both /data/user/0 and /data/data for the same app files.
            String path=new File((String)value).getCanonicalPath();
            if(path.equals(oldRoot) || path.startsWith(oldRoot+"/"))return newRoot+path.substring(oldRoot.length());
        }catch(IOException ignored){} // Non-path text is left untouched.
        return null;
    }
    private static boolean rebase(Object node,String oldRoot,String newRoot)throws JSONException {
        boolean changed=false;
        if(node instanceof JSONObject) {
            JSONObject object=(JSONObject)node;
            for(Iterator<String> keys=object.keys();keys.hasNext();) {
                String key=keys.next();Object value=object.get(key);
                String path=rebasedPath(value,oldRoot,newRoot);
                if(path!=null) {
                    object.put(key,path);changed=true;
                }else changed|=rebase(value,oldRoot,newRoot);
            }
        }else if(node instanceof JSONArray) {
            JSONArray array=(JSONArray)node;
            for(int i=0;i<array.length();i++) {
                Object value=array.get(i);
                String path=rebasedPath(value,oldRoot,newRoot);
                if(path!=null) {
                    array.put(i,path);changed=true;
                }else changed|=rebase(value,oldRoot,newRoot);
            }
        }
        return changed;
    }
    public static File createBackup(Context context,File root)throws IOException {
        File folder=child(root,"Backups");
        if(!folder.isDirectory() && !folder.mkdirs())throw new IOException("Cannot create the Backups folder.");
        Path temporary=Files.createTempFile(folder.toPath(),".backup-",".tmp");
        boolean complete=false;
        try {
            try(FileOutputStream out=new FileOutputStream(temporary.toFile())){exportBackup(context,root,out);}
            // Flush the completed archive before publishing a name visible to file pickers.
            try(RandomAccessFile file=new RandomAccessFile(temporary.toFile(),"rw")){file.getFD().sync();}
            String stamp=new java.text.SimpleDateFormat("yyyy-MM-dd_HH-mm-ss",Locale.ROOT).format(new Date());
            File backup=child(root,"Backups/ComboShip-backup-"+stamp+"-"+UUID.randomUUID().toString().substring(0,8)+".zip");
            Files.move(temporary,backup.toPath());complete=true;return backup;
        }finally {if(!complete)Files.deleteIfExists(temporary);}
    }
    public static void importMod(InputStream stream, File root, String game, String displayName) throws IOException {
        if (!game.equals("soh") && !game.equals("2ship")) throw new IOException("Unknown game.");
        String name=displayName==null?"":displayName.replaceAll("[^A-Za-z0-9._ -]","_");
        if (name.length()>120) name=name.substring(name.length()-120);
        String lower=name.toLowerCase(Locale.ROOT);
        if (name.startsWith(".") || (!lower.endsWith(".o2r") && !lower.endsWith(".otr")))
            throw new IOException("Choose a .o2r or .otr mod archive.");
        File destination=child(root,"mods/"+game+"/"+name);
        if (destination.exists()) throw new IOException("A mod with that filename exists. No file was overwritten.");
        atomicCopy(stream,destination,null,1024L*1024*1024);
    }
    public static void exportBackup(Context context, File root, OutputStream stream) throws IOException {
        if (stream==null) throw new IOException("Cannot open the backup document.");
        List<Path> candidates=new ArrayList<>();
        try (java.util.stream.Stream<Path> paths=Files.walk(root.toPath())) {
            paths.filter(p->Files.isRegularFile(p,LinkOption.NOFOLLOW_LINKS)).forEach(candidates::add);
        }
        candidates.sort(Comparator.naturalOrder()); long total=0;
        try (ZipOutputStream zip=new ZipOutputStream(new BufferedOutputStream(stream))) {
            zip.putNextEntry(new ZipEntry("backup-info.txt"));
            zip.write(("ComboShip Android state backup\nSource: "+BuildConfig.VERSION_NAME+"\nROMs, support archives, and mods excluded.\n").getBytes(java.nio.charset.StandardCharsets.UTF_8)); zip.closeEntry();
            zip.putNextEntry(new ZipEntry("android-touch-settings.json"));
            zip.write(new JSONObject(context.getSharedPreferences("combo-touch",Context.MODE_PRIVATE).getAll())
                .toString().getBytes(java.nio.charset.StandardCharsets.UTF_8));zip.closeEntry();
            byte[] buffer=new byte[65536];
            for (Path path:candidates) {
                String relative=root.toPath().relativize(path).toString().replace(File.separatorChar,'/');
                String lower=relative.toLowerCase(Locale.ROOT);
                if (lower.startsWith("assets/") || lower.startsWith("mods/") || lower.startsWith("backups/") || lower.endsWith(".o2r") || lower.endsWith(".otr") ||
                    lower.endsWith(".zip") || lower.equals("gamecontrollerdb.txt") ||
                    lower.endsWith(".z64") || lower.endsWith(".n64") || lower.endsWith(".v64") || lower.endsWith(".tmp") ||
                    lower.equals(".runtime.lock") || lower.equals(".running")) continue;
                long length=Files.size(path); total+=length;
                if (length>64L*1024*1024 || total>256L*1024*1024) throw new IOException("Backup exceeds the safety size limit.");
                zip.putNextEntry(new ZipEntry(relative));
                try (InputStream in=Files.newInputStream(path)) {int count; while((count=in.read(buffer))!=-1) zip.write(buffer,0,count);}
                zip.closeEntry();
            }
        }
    }
}
