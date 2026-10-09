package org.comboship.android;
import java.io.*;
import java.nio.channels.*;
import java.nio.file.Files;
import java.util.HashSet;
import java.util.Set;
/** POSIX record locking interoperates with the native :game process. */
public final class DataLock implements AutoCloseable {
    private static final Set<String> HELD = new HashSet<>();
    private final String key;
    private RandomAccessFile file;
    private FileLock lock;
    private boolean closed;
    public DataLock(File root) throws IOException {
        key = new File(root.getCanonicalFile(), ".runtime.lock").getPath();
        synchronized (HELD) {
            // Opening/closing a second descriptor would release this process's first POSIX lock.
            if (!HELD.add(key)) throw new IOException("Another import or backup is running.");
            try {
                if (Files.isSymbolicLink(new File(key).toPath())) throw new IOException("Invalid session lock.");
                file = new RandomAccessFile(key, "rw");
                lock = file.getChannel().tryLock();
                if (lock == null) throw new IOException("Close the game before importing or exporting data.");
            } catch (IOException | OverlappingFileLockException error) {
                HELD.remove(key);
                if (file != null) file.close();
                throw new IOException("The data folder is in use. Close the game first.", error);
            }
        }
    }
    @Override public void close() throws IOException {
        synchronized (HELD) {
            if (closed) return;
            closed = true;
            try { lock.release(); } finally { try { file.close(); } finally { HELD.remove(key); } }
        }
    }
}
