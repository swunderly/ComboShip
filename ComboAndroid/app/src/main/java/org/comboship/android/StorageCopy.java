package org.comboship.android;

import java.io.*;
import java.nio.file.*;
import java.nio.file.attribute.BasicFileAttributes;
import java.security.*;
import java.util.*;

/** Verified copy into a new directory. A failed copy never changes the source or destination. */
public final class StorageCopy {
    private StorageCopy() {}
    interface BeforePublish {void run(File staging)throws IOException;}
    static boolean isEmptyDirectory(Path directory)throws IOException {
        if(!Files.isDirectory(directory,LinkOption.NOFOLLOW_LINKS))return false;
        try(DirectoryStream<Path> entries=Files.newDirectoryStream(directory)){return !entries.iterator().hasNext();}
    }
    static String digest(File file) throws IOException {
        try(InputStream input=new FileInputStream(file)) {
            MessageDigest digest=MessageDigest.getInstance("SHA-256");
            byte[] bytes=new byte[65536];int count;
            while((count=input.read(bytes))!=-1)digest.update(bytes,0,count);
            return RomImporter.hex(digest.digest());
        }catch(NoSuchAlgorithmException e){throw new IOException(e);}
    }
    static void prepare(File source,File target) throws IOException {
        prepare(source,target,staging->{});
    }
    static synchronized void prepare(File source,File target,BeforePublish beforePublish) throws IOException {
        Path from=source.getCanonicalFile().toPath(),to=target.getCanonicalFile().toPath();
        if(!Files.isDirectory(from) || to.startsWith(from) || from.startsWith(to))
            throw new IOException("Storage folders must be separate.");
        boolean emptyTarget=Files.exists(to,LinkOption.NOFOLLOW_LINKS);
        if(emptyTarget && !isEmptyDirectory(to))throw new IOException("The destination already contains data. No files were overwritten.");
        // A copy killed mid-way never reaches the cleanup below. Copies run one at a time, so a staging
        // folder left here is abandoned; it was never published, so removing it cannot lose data.
        // Best effort throughout; it must not block this copy.
        try(DirectoryStream<Path> abandoned=Files.newDirectoryStream(to.getParent(),".comboship-copy-*")) {
            for(Path old:abandoned)if(Files.isDirectory(old,LinkOption.NOFOLLOW_LINKS))
                try{removeTree(old);}catch(IOException ignored){}
        }catch(IOException | DirectoryIteratorException ignored){}
        Path staging=Files.createTempDirectory(to.getParent(),".comboship-copy-");
        boolean committed=false;
        try {
            try(java.util.stream.Stream<Path> paths=Files.walk(from)) {
                for(Iterator<Path> it=paths.iterator();it.hasNext();) {
                    Path item=it.next(),relative=from.relativize(item),copy=staging.resolve(relative);
                    String name=relative.toString();
                    if(name.equals(".runtime.lock") || name.equals(".running"))continue;
                    if(Files.isSymbolicLink(item))throw new IOException("Data folders cannot contain symbolic links.");
                    if(Files.isDirectory(item))Files.createDirectories(copy);
                    else if(Files.isRegularFile(item)) {
                        Files.copy(item,copy);
                        if(!digest(item.toFile()).equals(digest(copy.toFile())))throw new IOException("Storage copy verification failed.");
                    }else throw new IOException("Unsupported data file.");
                }
            }
            beforePublish.run(staging.toFile());
            // Delete only the previously empty directory; a newly added file makes this fail safely.
            if(emptyTarget)Files.delete(to);
            // Within one filesystem, directory rename publishes the verified copy in one operation.
            Files.move(staging,to);
            committed=true;
        }finally {
            if(!committed)removeTree(staging);
            if(!committed && emptyTarget && !Files.exists(to))Files.createDirectory(to);
        }
    }
    /** Deletes a directory tree; symbolic links inside it are removed, never followed. */
    static void removeTree(Path root)throws IOException {
        Files.walkFileTree(root,new SimpleFileVisitor<Path>() {
            @Override public FileVisitResult visitFile(Path file,BasicFileAttributes attrs)throws IOException {
                Files.delete(file);return FileVisitResult.CONTINUE;
            }
            @Override public FileVisitResult postVisitDirectory(Path dir,IOException error)throws IOException {
                if(error!=null)throw error;Files.delete(dir);return FileVisitResult.CONTINUE;
            }
        });
    }
}
